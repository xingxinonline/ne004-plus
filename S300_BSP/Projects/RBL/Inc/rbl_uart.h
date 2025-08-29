/**
 * @file rbl_uart.h
 * @brief RBL UART驱动头文件
 */

#ifndef RBL_UART_H
#define RBL_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明 */
int rbl_uart_init(void);
void rbl_uart_deinit(void);

/* 基础读写 */
int rbl_uart_putchar(int ch);
int rbl_uart_getchar(void);
bool rbl_uart_readable(void);

/* 字符串操作 */
int rbl_uart_puts(const char *str);

/* 数据块操作 */
int rbl_uart_write(const uint8_t *data, size_t length);
int rbl_uart_read(uint8_t *buffer, size_t length);
int rbl_uart_read_nonblock(uint8_t *buffer, size_t length);
int rbl_uart_read_timeout(uint8_t *buffer, size_t length, uint32_t timeout_ms);

/* 配置 */
int rbl_uart_set_baudrate(uint32_t baudrate);
void rbl_uart_flush(void);

/* Printf支持 */
int rbl_printf(const char *format, ...);

/* 兼容性定义 */
static inline bool rbl_uart_data_available(void) { return rbl_uart_readable(); }

#ifdef __cplusplus
}
#endif

#endif // RBL_UART_H
