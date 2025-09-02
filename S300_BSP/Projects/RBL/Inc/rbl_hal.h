// Minimal RBL HAL for UART and delay (no printf, no heap)
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 可选日志开关：1 启用串口输出，0 关闭以减小代码尺寸
#ifndef RBL_LOG_ENABLE
#define RBL_LOG_ENABLE 1
#endif /* 可用编译器宏 -DRBL_LOG_ENABLE=0 关闭日志以减小体积 */

// UART 初始化（固定使用 UART3，115200 8N1），内部完成时钟与复用配置
void rbl_uart_init(void);

// 写入字节串到 UART（轮询方式）
void rbl_uart_write(const char *buf, size_t len);

// 从UART接收数据（非阻塞）
size_t rbl_hal_uart_receive(uint8_t *buf, size_t max_len);

// UART发送函数（兼容）
static inline void rbl_hal_uart_send(const uint8_t *buf, size_t len) {
    rbl_uart_write((const char*)buf, len);
}

// 便捷输出以空终止字符串
static inline void rbl_uart_write_str(const char *s) {
    if (!s) return;
    const char *p = s;
    while (*p) p++;
    rbl_uart_write(s, (size_t)(p - s));
}

// 简单忙等待延时（按循环计数，不依赖定时器）
void rbl_delay_cycles(uint32_t cycles);

// 格式化日志输出函数
void rbl_log_printf(const char *format, ...);

// printf替代函数声明（用于替换QSPI驱动中的printf）
int rbl_printf_stub(const char *format, ...);

// 轻量日志宏（支持格式化）
#if RBL_LOG_ENABLE
#define RBL_LOG(format, ...) rbl_log_printf(format, ##__VA_ARGS__)
#else
#define RBL_LOG(format, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif
