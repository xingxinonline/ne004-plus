/**
 * @file sbl_minimal.c
 * @brief 最简SBL程序 - 用于测试RBL跳转功能
 * @version 1.0
 * @date 2025-09-02
 */

#include "s300.h"

// 系统时钟频率（RBL已启用PLL=192MHz，这里直接同步）
uint32_t SystemCoreClock = 192000000u;

// 关闭SysTick方案：不使用中断心跳，改为忙等待延时

// UART3寄存器定义
#define UART3_BASE   (0x40013000u)
#define UART_THR     (*(volatile uint32_t *)(UART3_BASE + 0x00))
#define UART_LSR     (*(volatile uint32_t *)(UART3_BASE + 0x14))
#define UART_LCR     (*(volatile uint32_t *)(UART3_BASE + 0x0C))
#define UART_IER     (*(volatile uint32_t *)(UART3_BASE + 0x04))
#define UART_IIR_FCR (*(volatile uint32_t *)(UART3_BASE + 0x08))
#define UART_MCR     (*(volatile uint32_t *)(UART3_BASE + 0x10))

// 时钟和IO配置寄存器
#define APB1_BASE      (0x4000A000u)
#define APB1_CLK_EN    (*(volatile uint32_t *)(APB1_BASE + 0x000Cu))
#define IO_MATRIX_BASE (0x40008000u)
#define IO_MATRIX_CFG1 (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x04u))

/**
 * @brief 系统初始化
 */
void SystemInit(void)
{
    // RBL已完成PLL与系统时钟配置，这里仅同步变量供本地延时和波特率计算使用
    SystemCoreClock = 192000000u;  // 与RBL配置保持一致
}

/**
 * @brief UART发送单个字符
 */
static void uart_send_char(char c)
{
    while ((UART_LSR & 0x20u) == 0u)
    {
        __NOP();
    }
    UART_THR = (uint32_t)c;
}

/**
 * @brief UART发送字符串
 */
static void uart_send_string(const char *str)
{
    if (!str) return;
    while (*str)
    {
        uart_send_char(*str);
        str++;
    }
}

/**
 * @brief 简单延时
 */
static void delay_cycles(uint32_t cycles)
{
    for (volatile uint32_t i = 0; i < cycles; ++i)
    {
        __NOP();
    }
}

/* 不使用SysTick中断，改用忙等待延时 */

/**
 * @brief UART初始化
 */
static void uart_init(void)
{
    // 开启UART3和IO Matrix时钟
    APB1_CLK_EN |= (1u << 8) | (1u << 3);
    // 配置IO复用为UART3功能
    uint32_t v = IO_MATRIX_CFG1;
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |= ((0x3u << 20) | (0x3u << 22));
    IO_MATRIX_CFG1 = v;
    // UART基本配置：115200, 8N1
    UART_IER = 0x00u;
    UART_IIR_FCR = 0x07u;   // 使能FIFO并清空
    UART_LCR = 0x03u;       // 8N1
    UART_MCR = 0x00u;
    // 设置波特率：115200，依据SystemCoreClock动态计算分频（假设APB1无分频）
    const uint32_t baud = 115200u;
    uint32_t divisor = (SystemCoreClock + (16u * baud / 2u)) / (16u * baud);
    if (divisor == 0u) divisor = 1u;
    UART_LCR |= 0x80u;      // DLAB=1
    *(volatile uint32_t *)(UART3_BASE + 0x00) = (divisor & 0xFFu);        // DLL
    *(volatile uint32_t *)(UART3_BASE + 0x04) = ((divisor >> 8) & 0xFFu); // DLH
    UART_LCR &= ~0x80u;     // DLAB=0
}
/* 近似毫秒级忙等待（粗略），无需定时器 */
static void delay_ms(uint32_t ms)
{
    /* 每ms近似循环次数，经验系数：假设单次循环 ~4 指令周期，这里使用/4000 近似 */
    uint32_t loops_per_ms = SystemCoreClock / 4000u;
    while (ms--)
    {
        delay_cycles(loops_per_ms);
    }
}

/**
 * @brief 输出十六进制数字
 */
static void uart_send_hex(uint32_t value)
{
    char hex_str[9];
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--)
    {
        hex_str[7 - i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    hex_str[8] = '\0';
    uart_send_string("0x");
    uart_send_string(hex_str);
}

/**
 * @brief 主函数
 */
int main(void)
{
    // 基本初始化
    // 注意：SBL运行在QSPI Flash XIP模式，向量表在Flash开始位置
    SCB->VTOR = 0x80010000;  // 向量表在QSPI Flash SBL起始地址
    __DSB();
    __ISB();
    // 延时确保系统稳定
    delay_cycles(100000);
    // 初始化UART
    uart_init();
    // 延时确保UART初始化完成
    delay_cycles(100000);
    // 不启用SysTick，直接进入心跳打印
    // 发送启动信息
    uart_send_string("\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("    S300 SBL Minimal Test v1.0\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("✅ SBL started successfully!\r\n");
    uart_send_string("✅ RBL jump to SBL works!\r\n");
    uart_send_string("Build: " __DATE__ " " __TIME__ "\r\n");
    uart_send_string("Flash Address: ");
    uart_send_hex(0x80010000);
    uart_send_string("\r\n");
    uart_send_string("Vector Table: ");
    uart_send_hex(SCB->VTOR);
    uart_send_string("\r\n");
    uart_send_string("Stack Pointer: ");
    uart_send_hex(__get_MSP());
    uart_send_string("\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("\r\n");
    // 心跳逻辑：每约1秒打印一次（忙等待近似）
    uint32_t beat_cnt = 0;
    // 主循环
    while (1)
    {
        delay_cycles(200000u);
        beat_cnt++;
        uart_send_string("[SBL] ~1s heartbeat #");
        // 打印十进制(最多3位)
        uint32_t n = beat_cnt % 1000u;
        if (n >= 100) uart_send_char('0' + (n / 100) % 10);
        if (n >= 10)  uart_send_char('0' + (n / 10) % 10);
        uart_send_char('0' + (n % 10));
        uart_send_string("\r\n");
    }
    return 0;
}
