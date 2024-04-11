#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>        // For mode constants
#include <errno.h>

#define SHMEM_NAME "/my_shmem"
#define SHMEM_SEMAPHORE_NAME "/my_shmem_semaphore"
#define SHMEM_SIZE sizeof(SharedData)

typedef struct {
    int process_counter;
} SharedData;

int main() {
    sem_t* sem;
    int fd, init = 0;
    SharedData* data;

    // 尝试打开现有的共享内存对象
    fd = shm_open(SHMEM_NAME, O_RDWR, 0666);
    if (fd < 0) {
        // 如果不存在，则创建一个新的共享内存对象
        fd = shm_open(SHMEM_NAME, O_CREAT | O_RDWR, 0666);
        if (fd < 0) {
            perror("shm_open failed");
            exit(EXIT_FAILURE);
        }
        init = 1;  // 标记需要初始化
    }

    if (init) {
        // 只有在我们需要初始化时才设置大小
        if (ftruncate(fd, SHMEM_SIZE) == -1) {
            perror("ftruncate failed");
            exit(EXIT_FAILURE);
        }
    }

    // 映射共享内存
    data = mmap(NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    if (init) {
        // 初始化共享数据
        data->process_counter = 0;
    }

    // 打开或创建信号量
    sem = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    // 修改共享内存数据前先锁定
    if (sem_wait(sem) < 0) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }

    data->process_counter++;
    printf("Number of cooperating processes: %d\n", data->process_counter);

    if (sem_post(sem) < 0) {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
    }

    // 延迟以便观察
    sleep(10);

    // 再次锁定以减少计数器
    if (sem_wait(sem) < 0) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }

    data->process_counter--;

    if (sem_post(sem) < 0) {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
    }

    // 如果是最后一个进程，清理资源
    if (data->process_counter == 0) {
        sem_close(sem);
        sem_unlink(SHMEM_SEMAPHORE_NAME);
        shm_unlink(SHMEM_NAME);
    }
    else {
        sem_close(sem);
    }

    munmap(data, SHMEM_SIZE);
    close(fd);

    return 0;
}