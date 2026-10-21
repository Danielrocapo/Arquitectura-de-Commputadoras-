#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/bus_shm_opt"
#define NUM_PACKETS 10000000
#define PKT_SIZE 64
#define BATCH_SIZE 500
#define TOTAL_BATCHES (NUM_PACKETS / BATCH_SIZE)

typedef struct {
    int id;
    char payload[PKT_SIZE - sizeof(int)];
} Packet;

typedef struct {
    sem_t sem_read;
    sem_t sem_write;
    Packet batch[BATCH_SIZE];
} SharedBusBuffer;

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(shm_fd, sizeof(SharedBusBuffer)) == -1) {
        perror("ftruncate"); close(shm_fd); return 1;
    }
    SharedBusBuffer *shm = mmap(NULL, sizeof(SharedBusBuffer),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); close(shm_fd); return 1; }
    if (sem_init(&shm->sem_read, 1, 0) == -1 ||
        sem_init(&shm->sem_write, 1, 1) == -1) {
        perror("sem_init"); munmap(shm, sizeof(SharedBusBuffer)); close(shm_fd); return 1;
    }
    printf("[Prod_OPT] Transfiriendo lotes de 500 paquetes mediante memoria compartida...\n");
    fflush(stdout);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int b = 0; b < TOTAL_BATCHES; b++) {
        if (sem_wait(&shm->sem_write) == -1) { perror("sem_wait"); return 1; }
        for (int i = 0; i < BATCH_SIZE; i++) {
            shm->batch[i].id = b * BATCH_SIZE + i;
            memset(shm->batch[i].payload, 'B', sizeof(shm->batch[i].payload));
        }
        if (sem_post(&shm->sem_read) == -1) { perror("sem_post"); return 1; }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    double pps = NUM_PACKETS / elapsed;
    double mbps = (NUM_PACKETS * PKT_SIZE) / (1024.0 * 1024.0 * elapsed);

    printf("\n=== METRICAS PRODUCTOR OPTIMIZADO (Prod_OPT) ===\n");
    printf("Paquetes enviados : %d\n", NUM_PACKETS);
    printf("Tiempo total      : %.4f segundos\n", elapsed);
    printf("Velocidad (PPS)   : %.0f paquetes/seg\n", pps);
    printf("Rendimiento       : %.2f MB/s\n", mbps);

    munmap(shm, sizeof(SharedBusBuffer));
    close(shm_fd);
    return 0;
}
