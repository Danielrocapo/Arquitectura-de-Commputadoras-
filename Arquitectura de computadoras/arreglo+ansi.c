#include <stdio.h>

#define TAMANO 5

int main() {

    int arreglo[TAMANO] = {10, 20, 30, 40, 50};
    int suma = 0;

    __asm__ (
        "movq $0, %%rcx\n\t"          // i = 0
        "movl $0, %%eax\n\t"          // eax = acumulador = 0

        "inicio:\n\t"

        "cmpq $5, %%rcx\n\t"          // compara i con TAMANO
        "jge fin\n\t"                  // si i >= 5, termina

        "addl (%1, %%rcx, 4), %%eax\n\t"
        // eax = eax + arreglo[i]

        "incq %%rcx\n\t"              // i++
        "jmp inicio\n\t"

        "fin:\n\t"

        "movl %%eax, %0\n\t"          // guarda la suma

        : "=r" (suma)
        : "r" (arreglo)
        : "eax", "rcx", "cc", "memory"
    );

    printf("La suma del arreglo es: %d\n", suma);

    return 0;
}