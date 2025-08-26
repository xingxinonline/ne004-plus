#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include "s300.h"

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX 3u
#endif

static inline void uart_tx_blocking(uint32_t idx, char c)
{
    const uint32_t UART0_BASE = 0x40010000u;
    uint32_t base = UART0_BASE + (idx * 0x1000u);
    /* Wait until TX FIFO not full or THR empty */
    while (!(((*(volatile uint32_t *)(base + 0x7Cu)) & (1u << 1)) ||
             ((*(volatile uint32_t *)(base + 0x14u)) & (1u << 5))))
    {
        __NOP();
    }
    (*(volatile uint32_t *)(base + 0x00u)) = (uint32_t)c; /* THR */
}

int _write(int fd, const void *buf, size_t count)
{
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO)
    {
        return -1;
    }
    const char *p = (const char *)buf;
    for (size_t i = 0; i < count; ++i)
    {
        char c = p[i];
        if (c == '\n')
        {
            uart_tx_blocking(UART_DEBUG_IDX, '\r');
        }
        uart_tx_blocking(UART_DEBUG_IDX, c);
    }
    return (int)count;
}

int _close(int fd)
{
    (void)fd;
    return -1;
}
int _fstat(int fd, struct stat *st)
{
    (void)fd;
    if (st)
    {
        st->st_mode = S_IFCHR;
    }
    return 0;
}
int _isatty(int fd)
{
    (void)fd;
    return 1;
}
off_t _lseek(int fd, off_t pos, int whence)
{
    (void)fd;
    (void)pos;
    (void)whence;
    return -1;
}
ssize_t _read(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}
extern char _ebss;
extern char _estack;
void *_sbrk(ptrdiff_t incr)
{
    static char *heap_end;
    char *prev;
    if (!heap_end)
    {
        heap_end = &_ebss;
    }
    if (heap_end + incr >= &_estack)
    {
        return (void *) -1;
    }
    prev = heap_end;
    heap_end += incr;
    return prev;
}
void _exit(int status)
{
    (void)status;
    while (1)
    {
        __NOP();
    }
}
int _kill(pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}
pid_t _getpid(void)
{
    return 1;
}
