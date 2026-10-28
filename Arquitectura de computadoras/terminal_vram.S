/*
 * Terminal de 4x4 en ensamblador GNU x86-64 (Ubuntu Linux).
 * Simula los puertos de teclado 0x01/0x02 y RAM 0x00..0xFF.
 * NO ejecuta IN/OUT fisicos: Linux protege esos puertos.
 * Compilacion: gcc -nostdlib -no-pie terminal_vram.S -o terminal_vram
 */
.intel_syntax noprefix
.equ KB_BASE, 0x70
.equ KB_HEAD, 0x78
.equ KB_TAIL, 0x79
.equ KB_COUNT, 0x7A
.equ VRAM_BASE, 0xF0

.section .rodata
header:
 .ascii "\033[2J\033[H=== TERMINAL SIMULADA EN ENSAMBLADOR x86-64 ===\n"
 .ascii "Puerto status: 0x01 | Puerto datos: 0x02 | FIFO: 0x70-0x77\n"
 .ascii "VRAM: 0xF0-0xFF (4x4) | Backspace: borrar | ESC: salir\n"
 .ascii "+----+\n"
header_end:
footer:
 .ascii "+----+\n"
 .ascii "Teclea caracteres; Enter avanza linea. ESC termina.\n"
footer_end:
bye: .ascii "\nPrograma finalizado.\n"
bye_end:

.section .data
row_text: .ascii "|    |\n"

.section .bss
.align 16
ram: .skip 256
port_status: .skip 1
port_data: .skip 1
cursor: .skip 1
input_char: .skip 1
dropped: .skip 1

.section .text
.global _start
_start:
    call inicializar
    call dibujar

bucle_entrada:
    xor eax, eax                /* syscall read */
    xor edi, edi                /* stdin */
    lea rsi, [rip+input_char]
    mov edx, 1
    syscall
    cmp rax, 1
    jne terminar
    movzx edi, byte ptr [rip+input_char]
    cmp dil, 27                 /* ESC = salir */
    je terminar
    call inyectar_puerto
    call consumir_cola
    call dibujar
    jmp bucle_entrada

terminar:
    mov eax, 1
    mov edi, 1
    lea rsi, [rip+bye]
    mov edx, bye_end - bye
    syscall
    mov eax, 60
    xor edi, edi
    syscall

inicializar:
    lea rdi, [rip+ram+VRAM_BASE]
    mov ecx, 16
    mov al, ' '
    rep stosb
    ret

/* Introducir pulsacion en controlador simulado; IO_READ sobre 0x01, 0x02. */
inyectar_puerto:
    cmp byte ptr [rip+port_status], 0
    jne .descartar
    mov byte ptr [rip+port_data], dil
    mov byte ptr [rip+port_status], 1
    /* IN status, [0x01] */
    mov al, byte ptr [rip+port_status]
    test al, 1
    jz .fin_puerto
    /* IN ASCII, [0x02], limpiando automaticamente el status */
    mov al, byte ptr [rip+port_data]
    mov byte ptr [rip+port_status], 0
    call insertar_fifo
.fin_puerto:
    ret
.descartar:
    inc byte ptr [rip+dropped]
    ret

/* AL = dato: FIFO circular de 8 celdas. */
insertar_fifo:
    movzx ecx, byte ptr [rip+ram+KB_COUNT]
    cmp ecx, 8
    jae .lleno
    movzx edx, byte ptr [rip+ram+KB_TAIL]
    lea r8, [rip+ram]
    mov byte ptr [r8+rdx+KB_BASE], al
    inc edx
    and edx, 7
    mov byte ptr [rip+ram+KB_TAIL], dl
    inc byte ptr [rip+ram+KB_COUNT]
    ret
.lleno:
    inc byte ptr [rip+dropped]
    ret

consumir_cola:
.sig:
    cmp byte ptr [rip+ram+KB_COUNT], 0
    je .final
    movzx edx, byte ptr [rip+ram+KB_HEAD]
    lea r8, [rip+ram]
    movzx edi, byte ptr [r8+rdx+KB_BASE]
    inc edx
    and edx, 7
    mov byte ptr [rip+ram+KB_HEAD], dl
    dec byte ptr [rip+ram+KB_COUNT]
    call procesar_caracter
    jmp .sig
.final:
    ret

procesar_caracter:
    cmp dil, 8                  /* Backspace ASCII 0x08 */
    je .retroceso
    cmp dil, 127                /* Delete (habitual en terminal Linux) */
    je .retroceso
    cmp dil, 10                 /* Enter */
    je .salto
    cmp dil, 13
    je .salto
    cmp dil, 32
    jb .fin_procesar
    cmp dil, 126
    ja .fin_procesar
    push rdi
    call verificar_scroll
    pop rdi
    movzx ecx, byte ptr [rip+cursor]
    lea r8, [rip+ram]
    mov byte ptr [r8+rcx+VRAM_BASE], dil
    inc byte ptr [rip+cursor]
    ret
.retroceso:
    cmp byte ptr [rip+cursor], 0
    je .fin_procesar
    dec byte ptr [rip+cursor]
    movzx ecx, byte ptr [rip+cursor]
    lea r8, [rip+ram]
    mov byte ptr [r8+rcx+VRAM_BASE], ' '
    ret
.salto:
    movzx eax, byte ptr [rip+cursor]
    add eax, 4
    and eax, -4
    cmp eax, 16
    jae .hacer_scroll
    mov byte ptr [rip+cursor], al
    ret
.hacer_scroll:
    call desplazar_vram
.fin_procesar:
    ret

verificar_scroll:
    cmp byte ptr [rip+cursor], 16
    jb .listo
    call desplazar_vram
.listo:
    ret

desplazar_vram:
    /* MEM_READ de fila posterior y MEM_WRITE de fila superior. */
    lea rsi, [rip+ram+VRAM_BASE+4]
    lea rdi, [rip+ram+VRAM_BASE]
    mov ecx, 12
    rep movsb
    lea rdi, [rip+ram+VRAM_BASE+12]
    mov ecx, 4
    mov al, ' '
    rep stosb
    mov byte ptr [rip+cursor], 12
    ret

dibujar:
    mov eax, 1
    mov edi, 1
    lea rsi, [rip+header]
    mov edx, header_end - header
    syscall
    xor r12d, r12d            /* fila */
.fila:
    cmp r12d, 4
    jae .pie
    xor r13d, r13d            /* columna */
.columna:
    cmp r13d, 4
    jae .emitir_fila
    mov eax, r12d
    shl eax, 2
    add eax, r13d
    lea r8, [rip+ram]
    mov al, byte ptr [r8+rax+VRAM_BASE]
    lea r9, [rip+row_text]
    mov byte ptr [r9+r13+1], al
    inc r13d
    jmp .columna
.emitir_fila:
    mov eax, 1
    mov edi, 1
    lea rsi, [rip+row_text]
    mov edx, 7
    syscall
    inc r12d
    jmp .fila
.pie:
    mov eax, 1
    mov edi, 1
    lea rsi, [rip+footer]
    mov edx, footer_end - footer
    syscall
    ret
.section .note.GNU-stack,"",@progbits
