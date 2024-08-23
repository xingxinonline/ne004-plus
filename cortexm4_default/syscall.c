/*
 * @Author       : xingxinonline
 * @Date         : 2024-08-22 14:50:43
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-23 11:09:48
 * @FilePath     : \\ne004-plus\\cortexm4_default\\syscall.c
 * @Description  : 
 * 
 * Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

extern char _end; // 这个符号由链接器定义，表示 bss 段结束的位置，堆的开始位置
static char *heap_end;

char* _sbrk(int incr) {
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }

    prev_heap_end = heap_end;

    // 如果需要，可以检查是否超出堆的最大限制
    // if (heap_end + incr > STACK_LIMIT) {
    //     errno = ENOMEM;
    //     return (char *)-1;
    // }

    heap_end += incr;
    return (char *) prev_heap_end;
}

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

void _exit(int status) {
    while (1) {
        // 在此处你可以执行其他清理操作或仅保持系统挂起
    }
}
