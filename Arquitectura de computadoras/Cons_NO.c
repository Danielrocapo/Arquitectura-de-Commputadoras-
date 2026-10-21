#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define FIFO_PATH "/tmp/bus_fifo_no_opt"
#define NUM_PACKETS 10000000
#define PKT_SIZE 64

typedef struct {
    int id;
    char payload[PKT_SIZE - sizeof(int)];
} Packet;

int main() {
    mkfifo(FIFO_PATH, 0666);
    printf("[Cons_NO] Conectando a FIFO...\n");
    fflush(stdout);
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) { perror("Error abriendo FIFO"); return 1; }

    Packet pkt;
    size_t total_received = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (total_received < NUM_PACKETS) {
        ssize_t n = read(fd, &pkt, sizeof(Packet));
        if (n == sizeof(Packet)) {
            total_received++;
        } else if (n == 0) {
            fprintf(stderr, "El productor cerro FIFO antes de tiempo.\n");
            break;
        } else {
            fprintf(stderr, "Lectura incompleta o error: %zd bytes\n", n);
            break;
        }
    }
    close(fd);
    unlink(FIFO_PATH);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    double pps = total_received / elapsed;
    double mbps = (total_received * PKT_SIZE) / (1024.0 * 1024.0 * elapsed);

    printf("\n=== METRICAS CONSUMIDOR NO OPTIMIZADO (Cons_NO) ===\n");
    printf("Paquetes recibidos: %zu\n", total_received);
    printf("Tiempo total      : %.4f segundos\n", elapsed);
    printf("Velocidad (PPS)   : %.0f paquetes/seg\n", pps);
    printf("Rendimiento       : %.2f MB/s\n", mbps);
    return total_received == NUM_PACKETS ? 0 : 1;
}
