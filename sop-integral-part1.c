#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

#define ERR(source)     (fprintf(stderr, "%s:%d\n", _FILE_, _LINE_), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_NAME "/monthoe_carlhoe"
#define SHM_SEM_NAME "/semawhore"
#define CHILD_SHM_SIZE 1024

typedef struct
{
    pthread_mutex_t mutex;
    int counter;
}shm_struct;

typedef struct childPack
{
    shm_struct* shmmem;
    sem_t* sem_m;
}childPack;

pthread_mutexattr_t init_shm(shm_struct* obj) {
    pthread_mutexattr_t mutexattr;
    if (0 != pthread_mutexattr_init(&mutexattr))
        ERR("pthread_mutexattr_init");
    if (0 != pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED))
        ERR("pthread_mutexattr_setpshared");
    if (0 != pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST))
        ERR("pthread_mutexattr_setrobust");
    if (0 != pthread_mutex_init(&packet.shmmem->mutex, &mutexattr))
        ERR("pthread_mutex_init");

    obj->counter = 0;

    return mutexattr;
}

void mutex_lock_handler(pthread_mutex_t* mutex) {
    int error;
    if ((error = pthread_mutex_lock(mutex)) != 0)
    {
        if (error == EOWNERDEAD)
        {
            pthread_mutex_consistent(mutex);
        }
        else
        {
            ERR("pthread_mutex_lock");
        }
    }
}

void child_work(shm_struct* obj) {
    pid_t pid = getpid();
    srand(pid);

    printf("Child PID: %d was born with assigned number %d\n", pid, 1 + rand() % 10);
}

void create_children(shm_struct* obj, int n)
{
    while (n-- > 0)
    {
        switch (fork())
        {
        case 0:
            child_work(obj);
            exit(EXIT_SUCCESS);
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
}


void usage(char* name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "3 < n <= 50 - number of rand points per process\n");
    fprintf(stderr, "0 <= a <= 50 - lower bound\n");
    fprintf(stderr, "0 <= b <= 50 - upper bound\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int n;

    n = atoi(argv[1]);


    int res_fd;
    //sem_t *semaphore;
    childPack packet;

    //create the semawhore
    if ((packet.sem_m = sem_open(SHM_SEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600, 1)) == SEM_FAILED) {
        if (errno == EEXIST) { //check if it exists if yes just reopen it
            if ((packet.sem_m = sem_open(SHM_SEM_NAME, O_RDWR)) == SEM_FAILED)
                ERR("sem_open reopen");
        }
        else {
            ERR("sem_open");
        }
    }

    if ((res_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)) == -1) {
        if (errno == EEXIST) { //check if it exists if yes just reopen it
            if ((res_fd = shm_open(SHM_NAME, O_RDWR, 0)) == -1)
                ERR("shm_open reopen");
        }
        else
            ERR("shm_open");
    }

    if (sem_wait(packet.sem_m) < 0)
    {
        ERR("sem_wait");
    }


    if (ftruncate(res_fd, sizeof(shm_struct)) == -1)
        ERR("ftruncate");

    //shm_struct* mem_obj;
    if ((packet.shmmem = (shm_struct*)mmap(NULL, sizeof(shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, res_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    close(res_fd);

    pthread_mutexattr_t mutexattr;
    printf("Counter: [%d]\n", packet.shmmem->counter);
    if (packet.shmmem->counter == 0)
    {
        //mutexattr = init_shm(packet.shmmem);
        if (0 != pthread_mutexattr_init(&mutexattr))
            ERR("pthread_mutexattr_init");
        if (0 != pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED))
            ERR("pthread_mutexattr_setpshared");
        if (0 != pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST))
            ERR("pthread_mutexattr_setrobust");
        if (0 != pthread_mutex_init(&packet.shmmem->mutex, &mutexattr))
            ERR("pthread_mutex_init");

    }

    if (sem_post(packet.sem_m) < 0) ERR("sem_post");

    //create_children(packet.shmmem, n);
    //mutex_lock_handler(&packet.shmmem->mutex);
    puts("CHECK MATE\n");
    pthread_mutex_lock(&packet.shmmem->mutex); //lock
    puts("CHECK MATE\n");
    packet.shmmem->counter++;

    puts("CHECK MATE\n");
    pthread_mutex_unlock(&packet.shmmem->mutex); //unlock
    puts("CHECK MATE\n");
    fflush(NULL);


    if (sem_close(packet.sem_m) < 0) ERR("sem_close");
    if (packet.shmmem->counter == n) {
        printf("Counter: [%d]\n", packet.shmmem->counter);
        if (0 != pthread_mutex_destroy(&packet.shmmem->mutex))
            ERR("pthread_mutex_destroy");

        if (sem_unlink(SHM_SEM_NAME) < 0)
            ERR("sem_unlink");
        if (shm_unlink(SHM_NAME) < 0)
            ERR("shm_unlink");
    }
    if (0 != pthread_mutexattr_destroy(&mutexattr))
        ERR("pthread_mutexattr_destroy");
    if (munmap(packet.shmmem, sizeof(shm_struct)) == -1)
        ERR("munmap");


    return EXIT_SUCCESS;
}