#ifndef __LAYER_FIX_H__
#define __LAYER_FIX_H__

#if defined(RISCV)
#include <stddef.h>
struct stat {
  char  ignore;
};
int _isatty(int fd);
int _close(int fd);
int _lseek(int fd, int ptr, int dir);
int _fstat(int fd, struct stat* st);
int _read (int file, void * ptr, size_t len);
int _getpid(void);
int _kill(int pid, int sig);
#endif
#endif