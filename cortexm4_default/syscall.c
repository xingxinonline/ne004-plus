#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int __attribute__((weak)) _isatty(int fd)
{
    if (fd > STDIN_FILENO && fd < STDERR_FILENO)
        return 1;
    errno = EBADF;
    return 0;
}

int __attribute__((weak)) _close(int fd)
{
    if (fd > STDIN_FILENO && fd < STDERR_FILENO)
        return 0;
    errno = EBADF;
    return -1;
}

int __attribute__((weak)) _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;
    errno = EBADF;
    return -1;
}

int __attribute__((weak)) _fstat(int fd, struct stat *st)
{
    if (fd > STDIN_FILENO && fd < STDERR_FILENO)
    {
        st->st_mode = S_IFCHR;
        return 0;
    }
    errno = EBADF;
    return 0;
}

int __attribute__((weak)) _read(int fd, void *buf, int nbytes)
{
    (void)fd;
    (void)buf;
    (void)nbytes;
    return 1;
}

int __attribute__((weak)) _write(int fd, const void *buf, int nbytes)
{
    (void)fd;
    (void)buf;
    (void)nbytes;
    return 1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return 1;
}
