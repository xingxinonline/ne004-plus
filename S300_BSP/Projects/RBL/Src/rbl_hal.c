#include "rbl_hal.h"
#include "s300.h"

// 寄存器与地址：优先使用 s300_memmap.h 中的定义
#ifndef UART3_BASE
#define UART3_BASE   (0x40013000u)
#endif
#define UART_THR     (*(volatile uint32_t *)(UART3_BASE + 0x00))
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

void rbl_delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; ++i) {
        __NOP();
    }
}
