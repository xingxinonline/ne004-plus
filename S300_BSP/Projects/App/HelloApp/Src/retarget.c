#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include "s300.h"
#ifndef UART_DEBUG_IDX
#define UART_DEBUG_IDX 3u
#endif
static inline void uart_tx_blocking(uint32_t idx, char c){uint32_t base=UARTn_BASE(idx);while(!((UART_USRn(base)&(1u<<1))||(UART_LSRn(base)&(1u<<5)))){__NOP();}UART_THRn(base)=(uint32_t)c;}
int _write(int fd, const void *buf, size_t count){if(fd!=STDOUT_FILENO&&fd!=STDERR_FILENO){return -1;}const char*p=(const char*)buf;for(size_t i=0;i<count;++i){char c=p[i];if(c=='\n'){uart_tx_blocking(UART_DEBUG_IDX,'\r');}uart_tx_blocking(UART_DEBUG_IDX,c);}return (int)count;}
int _close(int fd){(void)fd;return -1;}int _fstat(int fd, struct stat *st){(void)fd;if(st){st->st_mode=S_IFCHR;}return 0;}int _isatty(int fd){(void)fd;return 1;}off_t _lseek(int fd, off_t pos, int whence){(void)fd;(void)pos;(void)whence;return -1;}ssize_t _read(int fd, void *buf, size_t count){(void)fd;(void)buf;(void)count;return 0;}extern char _ebss;extern char _estack;void*_sbrk(ptrdiff_t incr){static char*heap_end;char*prev;if(!heap_end){heap_end=&_ebss;}if(heap_end+incr>=&_estack){return (void*)-1;}prev=heap_end;heap_end+=incr;return prev;}void _exit(int status){(void)status;while(1){__NOP();}}int _kill(pid_t pid,int sig){(void)pid;(void)sig;return -1;}pid_t _getpid(void){return 1;}
