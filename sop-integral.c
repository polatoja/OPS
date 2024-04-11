#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "macros.h"

// Use those names to cooperate with $make clean-resources target.
#define SHMEM_SEMAPHORE_NAME "/sop-shmem-sem"
#define SHMEM_NAME "/sop-shmem"
#define SHMEM_SIZE sizeof(shm_t)

#define DEFAULT_A -1
#define DEFAULT_B 1
#define DEFAULT_N 1000
#define BATCHES 3

volatile sig_atomic_t lastSignal = 0;

void set_handler(void (*handlerFunction)(int), int signalNumber)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = handlerFunction;
    if (-1 == sigaction(signalNumber, &act, NULL))
    {
        ERR("sigaction");
    }
}

void sigint_handler(int signalNumber) { lastSignal = signalNumber; }

typedef struct sharedMemoryStruct
{
    pthread_mutex_t mutex;
    int processCounter;
    unsigned long int pointsHit;
    unsigned long int pointsTotal;
} shm_t;

shm_t* init()
{
    // Open or create a semaphore for synchronization
    sem_t* semaphore = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (SEM_FAILED == semaphore)
    {
        ERR("sem_open");
    }

    // Wait for the semaphore to ensure exclusive access to shared resources
    sem_wait(semaphore);

    int initialize = 1;
    // Attempt to create a new shared memory segment
    int shmFd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (-1 == shmFd)
    {
        // If the shared memory segment already exists, proceed without initialization
        if (EEXIST == errno)
        {
            initialize = 0;
            shmFd = shm_open(SHMEM_NAME, O_RDWR, 0666);
            if (-1 == shmFd)
            {
                sem_post(semaphore);
                ERR("shm_open w/o O_CREAT");
            }
        }
        else
        {
            sem_post(semaphore);
            ERR("shm_open");
        }
    }

    // Initialize the shared memory segment if it's a new creation
    if (initialize && ftruncate(shmFd, SHMEM_SIZE) == -1)
    {
        // If ftruncate fails, unlink the shared memory segment and release semaphore before exiting
        shm_unlink(SHMEM_NAME);
        sem_post(semaphore);
        ERR("ftruncate");
    }

    // Map the shared memory segment into the address space
    shm_t* shmem = mmap(NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (MAP_FAILED == shmem)
    {
        // If mmap fails, unlink the shared memory segment and release semaphore before exiting
        if (initialize)
        {
            shm_unlink(SHMEM_NAME);
            sem_post(semaphore);
            ERR("mmap by init");
        }
        else
        {
            sem_post(semaphore);
            ERR("mmap");
        }
    }

    // Close the file descriptor for shared memory segment as it's no longer needed
    close(shmFd);

    // Perform initialization if it's a new shared memory segment
    if (initialize)
    {
        // Initialize the mutex attribute for shared mutex
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        
        // Initialize the shared mutex
        pthread_mutex_init(&shmem->mutex, &attr);
        if (errno)
        {
            // If pthread_mutex_init fails, unlink the shared memory segment and release semaphore before exiting
            shm_unlink(SHMEM_NAME);
            sem_post(semaphore);
            ERR("pthread_mutex");
        }

        // Initialize other shared data
        shmem->processCounter = 0;
        shmem->pointsHit = 0;
        shmem->pointsTotal = 0;
    }

    // Increase the process counter for the shared memory segment
    shmem->processCounter++;

    // Release the semaphore to allow other processes to access shared resources
    sem_post(semaphore);

    // Close the semaphore, it's no longer needed
    sem_close(semaphore);

    // Return a pointer to the initialized shared memory segment
    return shmem;
}

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int randomize_points(int N, float a, float b)
{
    int counter = 0;
    for (int i = 0; i < N; ++i)
    {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            counter++;
    }
    return counter;
}

/**
 * This function calculates approxiamtion of integral from counters of hit and total points.
 * @param total_randomized_points Number of total randomized points.
 * @param hit_points NUmber of hit points.
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return The approximation of intergal
 */
double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b)
{
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

/**
 * This function locks mutex and can sometimes die (it has 2% chance to die).
 * It cannot die if lock would return an error.
 * It doesn't handle any errors. It's users responsibility.
 * Use it only in STAGE 4.
 *
 * @param mtx Mutex to lock
 * @return Value returned from pthread_mutex_lock.
 */
int random_death_lock(pthread_mutex_t* mtx)
{
    int ret = pthread_mutex_lock(mtx);
    if (ret)
        return ret;

    // 2% chance to die
    if (rand() % 50 == 1)
        abort();
    return ret;
}

void usage(char* argv[])
{
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before rport to shared memory (default: 1000)\n");
}

void read_arguments(int argc, char** argv, int* N, float* a, float* b)
{
    *N = DEFAULT_N;
    *a = DEFAULT_A;
    *b = DEFAULT_B;
    if (argc > 1)
    {
        *a = atoi(argv[1]);
        if (argc > 2)
        {
            *b = atoi(argv[2]);
            if (argc > 3)
            {
                *N = atoi(argv[3]);
            }
        }
    }
    if (*N <= 1 || *a > *b)
    {
        usage(argv);
    }
}

void count_processes(shm_t* shmem)
{
    int hasDied = 0;
    if (EOWNERDEAD == random_death_lock(&shmem->mutex))
    {
        hasDied = 1;
        WAR("Mutex inconsistent");
        if (0 != pthread_mutex_consistent(&shmem->mutex))
        {
            ERR("Failed to make mutex consistent");
        }
    }
    if (hasDied)
    {
        shmem->processCounter--;
    }
    if (-1 == pthread_mutex_unlock(&shmem->mutex))
    {
        ERR("pthread_mutex_unlock");
    }
}

void close_shmem(shm_t* shmem, float a, float b)
{
    int hasDied = 0;
    if (EOWNERDEAD == random_death_lock(&shmem->mutex))
    {
        hasDied = 1;
        WAR("Mutex inconsistent");
        if (0 != pthread_mutex_consistent(&shmem->mutex))
        {
            ERR("Failed to make mutex consistent");
        }
    }
    if (hasDied)
    {
        shmem->processCounter--;
    }
    int processesLeft = --shmem->processCounter;
    if (0 == processesLeft)
    {
        double finalresult = summarize_calculations(shmem->pointsTotal, shmem->pointsHit, a, b);
        printf("Final result: %lf\n", finalresult);
        pthread_mutex_destroy(&shmem->mutex);
        shm_unlink(SHMEM_NAME);
        sem_unlink(SHMEM_SEMAPHORE_NAME);
        return;
    }
    if (-1 == pthread_mutex_unlock(&shmem->mutex))
    {
        ERR("pthread_mutex_unlock");
    }
    if (-1 == munmap(shmem, SHMEM_SIZE))
    {
        if (-1 == pthread_mutex_unlock(&shmem->mutex))
        {
            ERR("pthread_mutex_unlock");
        }
        ERR("munmap");
    }
}

void commit_to_memory(shm_t* shmem, uint64_t pointsHit, uint64_t N)
{
    int hasDied = 0;
    if (EOWNERDEAD == random_death_lock(&shmem->mutex))
    {
        hasDied = 1;
        WAR("Mutex inconsistent");
        if (0 != pthread_mutex_consistent(&shmem->mutex))
        {
            ERR("Failed to make mutex consistent");
        }
    }
    if (hasDied)
    {
        shmem->processCounter--;
    }
    shmem->pointsHit += pointsHit;
    shmem->pointsTotal += N;
    printf("Points hit/Total points: %ld/%ld.\n", shmem->pointsHit, shmem->pointsTotal);
    if (-1 == pthread_mutex_unlock(&shmem->mutex))
    {
        ERR("pthread_mutex_unlock");
    }
}

void work(shm_t* shmem, float a, float b, long int N)
{
    uint64_t pointsHit;
    while (lastSignal != SIGINT)
    {
        pointsHit = randomize_points(N, a, b);
        commit_to_memory(shmem, pointsHit, N);
    }
}

int main(int argc, char* argv[])
{
    int N;
    float a, b;
    srand(getpid());
    set_handler(sigint_handler, SIGINT);
    read_arguments(argc, argv, &N, &a, &b);
    shm_t* shmem = init();
    count_processes(shmem);
    work(shmem, a, b, N);
    close_shmem(shmem, a, b);
    return EXIT_SUCCESS;
}
