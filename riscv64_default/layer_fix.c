#include <errno.h>
#ifndef ARM_MATH_DSP
#include <unistd.h>
#endif
#include "layer_fix.h"

#if defined(RISCV)

__attribute__((weak)) int _isatty(int fd) {
  if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
    return 1;

  errno = EBADF;
  return 0;
}

__attribute__((weak)) int _close(int fd) {
  if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
    return 0;

  errno = EBADF;
  return -1;
}

__attribute__((weak)) int _lseek(int fd, int ptr, int dir) {
  (void)fd;
  (void)ptr;
  (void)dir;

  errno = EBADF;
  return -1;
}

__attribute__((weak)) int _fstat(int fd, struct stat *st) {
  // if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
  //   st->st_mode = S_IFCHR;
  //   return 0;
  // }

  (void)fd;
  (void)st;
  errno = EBADF;
  return 0;
}

int __attribute__((weak)) _read (int file, void * ptr, size_t len) {
  (void)file;
  (void)ptr;
  (void)len;
  return 1;
}


int __attribute__((weak)) _write (int file, const void * ptr, size_t len) {
  (void)file;
  (void)ptr;
  (void)len;
  return 1;
}

int _getpid(void) {
  return 1;
}

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  return 1;
}

#endif