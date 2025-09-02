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
#endif

// UART 初始化（固定使用 UART3，115200 8N1），内部完成时钟与复用配置
void rbl_uart_init(void);

// 写入字节串到 UART（轮询方式）
void rbl_uart_write(const char *buf, size_t len);

// 便捷输出以空终止字符串
static inline void rbl_uart_write_str(const char *s) {
    if (!s) return;
    const char *p = s;
    while (*p) p++;
    rbl_uart_write(s, (size_t)(p - s));
}

// 简单忙等待延时（按循环计数，不依赖定时器）
void rbl_delay_cycles(uint32_t cycles);

// 轻量日志宏（仅支持常量字符串）
#if RBL_LOG_ENABLE
#define RBL_LOG(msg) rbl_uart_write_str(msg)
#else
#define RBL_LOG(msg) ((void)0)
#endif

#ifdef __cplusplus
}
#endif
