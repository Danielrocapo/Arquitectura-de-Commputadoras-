/*
 * Laboratorio: Teclado por Puerto, Cola FIFO y VRAM
 * Simulacion educativa de una RAM de 256 bytes, puertos 0x01/0x02,
 * y cuatro senales de control mutuamente excluyentes.
 * A diferencia del hardware real, los puertos son variables de software.
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra -pedantic simulador_buses.c -o simulador_buses
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define KB_BASE 0x70u
#define KB_HEAD 0x78u
#define KB_TAIL 0x79u
#define KB_COUNT 0x7Au
#define QUEUE_CAP 8u
#define VRAM_BASE 0xF0u
#define VRAM_COLS 4u
#define VRAM_ROWS 4u
#define STATUS_PORT 0x01u
#define DATA_PORT 0x02u

typedef enum { NO_SIGNAL, MEM_READ, MEM_WRITE, IO_READ, IO_WRITE } Signal;
typedef struct {
    uint8_t ram[256];
    uint8_t port_status, port_data;
    uint8_t mar, mdr, cursor;
    Signal signal;
    size_t lost_keys, bus_transfers;
    bool trace;
} Machine;

static const char *sig_name(Signal s) {
    switch (s) {
        case MEM_READ: return "MEM_READ";
        case MEM_WRITE: return "MEM_WRITE";
        case IO_READ: return "IO_READ";
        case IO_WRITE: return "IO_WRITE";
        default: return "(idle)";
    }
}
static void trace_bus(Machine *m, Signal s, uint8_t addr, uint8_t val) {
    /* Solo una senal esta activa en cada transferencia. */
    m->signal = s;
    m->mar = addr;
    m->mdr = val;
    m->bus_transfers++;
    if (m->trace)
        printf("%-9s | AddressBus=0x%02X | DataBus=0x%02X | MAR=0x%02X | MDR=0x%02X\n",
               sig_name(s), addr, val, m->mar, m->mdr);
    m->signal = NO_SIGNAL;
}
static uint8_t mem_read(Machine *m, uint8_t addr) {
    uint8_t val = m->ram[addr];
    trace_bus(m, MEM_READ, addr, val);
    return val;
}
static void mem_write(Machine *m, uint8_t addr, uint8_t val) {
    trace_bus(m, MEM_WRITE, addr, val);
    m->ram[addr] = val;
}
static uint8_t io_read(Machine *m, uint8_t port) {
    uint8_t val = 0;
    if (port == STATUS_PORT) val = m->port_status;
    else if (port == DATA_PORT) {
        val = m->port_data;
        m->port_status &= (uint8_t)~1u;
    }
    trace_bus(m, IO_READ, port, val);
    return val;
}
static void io_write(Machine *m, uint8_t port, uint8_t val) {
    trace_bus(m, IO_WRITE, port, val);
    /* Se modela la senal, pero los puertos de este teclado son de lectura. */
}
static void init(Machine *m) {
    memset(m, 0, sizeof(*m));
    for (unsigned a = VRAM_BASE; a <= 0xFFu; a++) m->ram[a] = ' ';
}
static void press_key(Machine *m, uint8_t c) {
    if (m->port_status & 1u) {
        m->lost_keys++;
        return;
    }
    /* La llegada fisica de una tecla se modela como evento del periferico. */
    m->port_data = c;
    m->port_status |= 1u;
}
static bool poll_and_enqueue(Machine *m) {
    if (!(io_read(m, STATUS_PORT) & 1u)) return false;
    uint8_t ascii = io_read(m, DATA_PORT);
    uint8_t count = mem_read(m, KB_COUNT);
    if (count == QUEUE_CAP) {
        m->lost_keys++;   /* Overflow: se descarta sin modificar la cola. */
        return false;
    }
    uint8_t tail = mem_read(m, KB_TAIL);
    mem_write(m, (uint8_t)(KB_BASE + tail), ascii);
    mem_write(m, KB_TAIL, (uint8_t)((tail + 1u) % QUEUE_CAP));
    mem_write(m, KB_COUNT, (uint8_t)(count + 1u));
    return true;
}
static bool dequeue(Machine *m, uint8_t *out) {
    uint8_t count = mem_read(m, KB_COUNT);
    if (!count) return false;
    uint8_t head = mem_read(m, KB_HEAD);
    *out = mem_read(m, (uint8_t)(KB_BASE + head));
    mem_write(m, KB_HEAD, (uint8_t)((head + 1u) % QUEUE_CAP));
    mem_write(m, KB_COUNT, (uint8_t)(count - 1u));
    return true;
}
static uint8_t vram_addr(unsigned row, unsigned col) {
    return (uint8_t)(VRAM_BASE + row * VRAM_COLS + col);
}
static void show_screen(const Machine *m) {
    puts("+----+");
    for (unsigned row = 0; row < VRAM_ROWS; ++row) {
        putchar('|');
        for (unsigned col = 0; col < VRAM_COLS; ++col)
            putchar((char)m->ram[vram_addr(row, col)]);
        puts("|");
    }
    puts("+----+");
}
static void scroll_screen(Machine *m) {
    /* Cada celda se mueve con MEM_READ y MEM_WRITE. */
    for (unsigned row = 0; row < VRAM_ROWS - 1; ++row)
        for (unsigned col = 0; col < VRAM_COLS; ++col)
            mem_write(m, vram_addr(row, col),
                      mem_read(m, vram_addr(row + 1, col)));
    for (unsigned col = 0; col < VRAM_COLS; ++col)
        mem_write(m, vram_addr(VRAM_ROWS - 1, col), ' ');
    m->cursor = 12u;
}
static void render_key(Machine *m, uint8_t c) {
    if (c == 8u || c == 127u) {
        if (m->cursor > 0u) {
            --m->cursor;
            mem_write(m, (uint8_t)(VRAM_BASE + m->cursor), ' ');
        }
        return;
    }
    if (c == '\n' || c == '\r') {
        unsigned next_row = (unsigned)m->cursor / VRAM_COLS + 1u;
        if (next_row >= VRAM_ROWS) scroll_screen(m);
        else m->cursor = (uint8_t)(next_row * VRAM_COLS);
        return;
    }
    if (c < 32u || c > 126u) return;
    if (m->cursor == 16u) scroll_screen(m);
    mem_write(m, (uint8_t)(VRAM_BASE + m->cursor), c);
    ++m->cursor;
}
static void type_and_render(Machine *m, uint8_t c) {
    press_key(m, c);
    (void)poll_and_enqueue(m);
    uint8_t ascii;
    while (dequeue(m, &ascii)) render_key(m, ascii);
}
static bool expect(bool cond, const char *reason) {
    printf("[%s] %s\n", cond ? "OK" : "FALLO", reason);
    return cond;
}
static bool experiment1(void) {
    Machine m; init(&m); m.trace = true;
    puts("\n=== EXPERIMENTO 1: PUERTOS DE TECLADO 0x01/0x02 ===");
    m.ram[0x02] = 0xA5;  /* RAM 0x02 y puerto 0x02 son recursos distintos. */
    press_key(&m, 'K');
    uint8_t status = io_read(&m, 0x01);
    uint8_t ascii = io_read(&m, 0x02);
    uint8_t cleared = io_read(&m, 0x01);
    uint8_t memory_value = mem_read(&m, 0x02);
    io_write(&m, 0x02, 0x00); /* Solo para observar IO_WRITE, sin mutar RAM. */
    printf("ASCII recibido: '%c'; RAM[0x02]=0x%02X\n", ascii, memory_value);
    printf("Respuesta de control: IO_READ=1; MEM_READ=MEM_WRITE=IO_WRITE=0.\n");
    printf("No hay colision con RAM[0x02]: la senal de control selecciona el periferico.\n");
    return expect(status == 1 && ascii == 'K' && cleared == 0 &&
                  memory_value == 0xA5, "IO_READ lee teclado, reinicia status y no altera RAM");
}
static bool experiment2(void) {
    Machine m; init(&m);
    puts("\n=== EXPERIMENTO 2: FIFO 8 BYTES CON OVERFLOW Y WRAP-AROUND ===");
    for (int c = 'A'; c <= 'I'; c++) {
        press_key(&m, (uint8_t)c);
        (void)poll_and_enqueue(&m);
    }
    printf("Tras escribir A..I: HEAD=%u TAIL=%u COUNT=%u DESCARTADAS=%zu\n",
        m.ram[KB_HEAD], m.ram[KB_TAIL], m.ram[KB_COUNT], m.lost_keys);
    bool ok = expect(m.ram[KB_COUNT] == 8u && m.lost_keys == 1u,
                     "La novena tecla se descarta al llenarse la FIFO");
    uint8_t c;
    if (!dequeue(&m, &c) || c != 'A') ok = false;
    if (!dequeue(&m, &c) || c != 'B') ok = false;
    for (int k = 'J'; k <= 'K'; k++) {
        press_key(&m, (uint8_t)k);
        (void)poll_and_enqueue(&m);
    }
    char got[9] = {0};
    size_t n = 0;
    while (n < 8 && dequeue(&m, &c)) got[n++] = (char)c;
    printf("Orden esperado: CDEFGHJK | Orden obtenido: %s\n", got);
    ok &= expect(n == 8 && strcmp(got, "CDEFGHJK") == 0 && m.ram[KB_COUNT] == 0,
                 "La extraccion respeta FIFO, HEAD/TAIL circular y cola vacia");
    return ok;
}
static bool experiment3(void) {
    Machine m; init(&m);
    puts("\n=== EXPERIMENTO 3: VRAM 4x4, DIRECCION Y SCROLL ===");
    uint8_t address = vram_addr(2u, 1u);
    m.trace = true;
    mem_write(&m, address, 'X');
    uint8_t letter = mem_read(&m, address);
    m.trace = false;
    printf("Celda (fila 2, columna 1) -> direccion 0x%02X, caracter '%c'\n", address, letter);
    bool ok = expect(address == 0xF9 && letter == 'X',
                     "Direccion VRAM=0xF0+fila*4+columna y lectura/escritura correctas");
    for (unsigned i = 0; i < 16; i++)
        mem_write(&m, (uint8_t)(VRAM_BASE + i), (uint8_t)('A' + i));
    puts("Antes del scroll:"); show_screen(&m);
    scroll_screen(&m);
    puts("Despues del scroll:"); show_screen(&m);
    ok &= expect(m.ram[0xF0] == 'E' && m.ram[0xFB] == 'P' && m.ram[0xFC] == ' ',
                 "Scroll sube una fila y limpia la ultima");
    return ok;
}
static bool experiment4(void) {
    Machine m; init(&m);
    puts("\n=== RETO INTEGRADOR: TECLEADO -> FIFO -> VRAM -> BACKSPACE ===");
    const char *input = "ABCDEFGHIJKLMNOPQ";
    for (const char *p = input; *p; ++p) type_and_render(&m, (uint8_t)*p);
    puts("Tras 17 letras (con scroll automatico):");
    show_screen(&m);
    bool ok = expect(m.ram[0xF0] == 'E' && m.ram[0xFC] == 'Q',
                     "Terminal aplica scroll al superar 16 caracteres");
    type_and_render(&m, 8); /* Backspace */
    ok &= expect(m.ram[0xFC] == ' ' && m.cursor == 12,
                 "Backspace (0x08) retrocede el cursor y borra con 0x20");
    type_and_render(&m, 'Z');
    puts("Pantalla final tras Backspace y 'Z':");
    show_screen(&m);
    ok &= expect(m.ram[0xFC] == 'Z' && m.ram[KB_COUNT] == 0,
                 "La entrada termina con Z y la cola vacia");
    return ok;
}
int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "Uso: ./simulador_buses (ejecuta 4 experimentos)\n");
        return 2;
    }
    int passed = 0;
    passed += experiment1();
    passed += experiment2();
    passed += experiment3();
    passed += experiment4();
    printf("\nRESULTADO GENERAL: %d/4 experimentos correctos.\n", passed);
    return passed == 4 ? 0 : 1;
}
