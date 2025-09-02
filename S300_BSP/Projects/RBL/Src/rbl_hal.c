#include "rbl_hal.h"
#include "s300.h"
#include <stdarg.h>
#include <stdio.h>

// 寄存器与地址：优先使用 s300_memmap.h 中的定义
#ifndef UART3_BASE
#define UART3_BASE   (0x40013000u)
#endif
#define UART_THR     (*(volatile uint32_t *)(UART3_BASE + 0x00))
#define UART_RBR     (*(volatile uint32_t *)(UART3_BASE + 0x00))  /* 接收缓冲器 */
#define UART_IER     (*(volatile uint32_t *)(UART3_BASE + 0x04))
#define UART_IIR_FCR (*(volatile uint32_t *)(UART3_BASE + 0x08))
#define UART_LCR     (*(volatile uint32_t *)(UART3_BASE + 0x0C))
#define UART_MCR     (*(volatile uint32_t *)(UART3_BASE + 0x10))
#define UART_LSR     (*(volatile uint32_t *)(UART3_BASE + 0x14))

#define APB1_BASE      (0x4000A000u)
#define APB1_CLK_EN    (*(volatile uint32_t *)(APB1_BASE + 0x000Cu))

#ifndef IO_MATRIX_BASE
#define IO_MATRIX_BASE (0x40008000u)
#endif
#define IO_MATRIX_CFG1 (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x04u))

static inline void uart_send_char(char c) {
    while ((UART_LSR & 0x20u) == 0u) {
        __NOP();
    }
    UART_THR = (uint32_t)c;
}

void rbl_uart_init(void) {
    // 开启时钟：假设 bit8:UART3, bit3:IO matrix（与现有最小实现一致）
    APB1_CLK_EN |= (1u << 8) | (1u << 3);

    // 复用：将对应引脚切到 UART3 功能（保持与现实现有寄存器位一致）
    uint32_t v = IO_MATRIX_CFG1;
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |=  ((0x3u << 20) | (0x3u << 22));
    IO_MATRIX_CFG1 = v;

    // UART 基本配置：115200, 8N1
    UART_IER = 0x00u;
    UART_IIR_FCR = 0x07u;   // 使能 FIFO 并清空
    UART_LCR = 0x03u;       // 8N1
    UART_MCR = 0x00u;

    // 设置波特率：DLAB = 1，DLL=13, DLH=0（基于 24MHz）
    UART_LCR |= 0x80u;
    *(volatile uint32_t *)(UART3_BASE + 0x00) = 13u; // DLL
    *(volatile uint32_t *)(UART3_BASE + 0x04) = 0u;  // DLH
    UART_LCR &= ~0x80u;
}

void rbl_uart_write(const char *buf, size_t len) {
    if (!buf || len == 0) return;
    for (size_t i = 0; i < len; ++i) {
        uart_send_char(buf[i]);
    }
}

size_t rbl_hal_uart_receive(uint8_t *buf, size_t max_len) {
    size_t received = 0;
    
    while (received < max_len) {
        /* 检查UART是否有数据可读 */
        if (UART_LSR & 0x01) {  /* 数据准备位 */
            buf[received] = (uint8_t)(UART_RBR & 0xFF);
            received++;
        } else {
            break;  /* 没有更多数据 */
        }
    }
    
    return received;
}

void rbl_delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; ++i) {
        __NOP();
    }
}

/* 简化的字符串长度计算 */
static size_t rbl_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/* 简化的printf实现，避免使用vsnprintf */
void rbl_log_printf(const char *format, ...) {
    /* 暂时使用简化版本，直接输出格式字符串 */
    rbl_uart_write(format, rbl_strlen(format));
}

/* 空的printf替代函数，用于替换QSPI驱动中的调试输出 */
int rbl_printf_stub(const char *format, ...) {
    (void)format;  /* 静默编译器警告 */
    return 0;
}
