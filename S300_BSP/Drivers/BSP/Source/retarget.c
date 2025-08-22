/*
 * Retarget newlib syscalls to UART for printf/scanf on S300.
 * Routes stdout/stderr to BOARD_UART_DEBUG_ID (default UART3).
 * Supports float and long long formatting when linked with full newlib
 * or with newlib-nano plus -u _printf_float / -u _scanf_float.
 */

#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "s300.h"
#include "s300_uart.h"
#include "board.h"   /* BOARD_UART_DEBUG_ID / BOARD_UART_DEBUG_BAUD */

#ifndef RETARGET_UART_IDX
#define RETARGET_UART_IDX  (BOARD_UART_DEBUG_ID)
#endif

/* ===== Low-level helpers ===== */
static inline void uart_write_blocking(uint32_t idx, const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        /* Convert \n to \r\n for better terminal compatibility */
        if (buf[i] == '\n') {
            S300_UART_PutCharI(idx, '\r');
        }
        S300_UART_PutCharI(idx, (char)buf[i]);
    }
}

static inline int uart_read_blocking(uint32_t idx)
{
    uint8_t b;
    while (S300_UART_TryRead(idx, &b) == 0) {
        __NOP();
    }
    return (int)b;
}

/* ===== newlib syscall stubs ===== */
int _write(int file, const char *ptr, int len)
{
    (void)file;
    if (!ptr || len <= 0) return 0;
    uart_write_blocking(RETARGET_UART_IDX, (const uint8_t *)ptr, (size_t)len);
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    if (!ptr || len <= 0) return 0;
    int i = 0;
    for (; i < len; ++i) {
        int c = uart_read_blocking(RETARGET_UART_IDX);
        if (c < 0) break;
        ptr[i] = (char)(uint8_t)c;
        /* Simple line-buffered behavior: stop at \n */
        if (ptr[i] == '\n') { i++; break; }
    }
    return i;
}

int _close(int file)
{
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    if (!st) { errno = EINVAL; return -1; }
    st->st_mode = 0; /* character device not strictly needed */
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1; /* stdin/out/err are ttys */
}

off_t _lseek(int file, off_t ptr, int dir)
{
    (void)file; (void)ptr; (void)dir;
    return 0;
}

/* Minimal heap implementation for newlib. */
extern char __bss_end__;    /* Provided by linker in ld script */
extern char _estack;        /* Top of RAM stack from linker script */
static char *heap_end;      /* Current end of heap */

void *_sbrk(ptrdiff_t incr)
{
    if (heap_end == 0) {
        heap_end = &__bss_end__;
    }
    char *prev = heap_end;
    char *next = prev + incr;
    /* Simple guard: don't cross into stack space */
    if (next >= (char *)&_estack) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end = next;
    return (void *)prev;
}

int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void _exit(int status)
{
    (void)status;
    for (;;) { __asm__ volatile("wfi"); }
}
