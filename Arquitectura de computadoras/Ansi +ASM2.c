#include <stdio.h>

#define TAMANO 5

int main(void) {

    int arreglo[TAMANO] = {0};

    __asm__ (
        "movq $0, %%rcx\n\t"

        "ini_bucle:\n\t"

        "cmpq $5, %%rcx\n\t"
        "jge fin_bucle\n\t"

        "movl %%ecx, %%eax\n\t"
        "addl $1, %%eax\n\t"
        "imull $10, %%eax\n\t"

        "movl %%eax, (%0, %%rcx, 4)\n\t"

        "incq %%rcx\n\t"
        "jmp ini_bucle\n\t"

        "fin_bucle:\n\t"

        :
        : "r" (arreglo)
        : "eax", "rcx", "memory"
    );

    printf("Contenido del arreglo:\n");

    for (int i = 0; i < TAMANO; i++) {
        printf("arreglo[%d] = %d\n", i, arreglo[i]);
    }

    return 0;
}