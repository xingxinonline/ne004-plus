/**
 * @file rbl_simple.c
 * @brief 极简RBL - 不依赖printf，直接操作硬件
 * @version 1.0
 * @date 2025-09-01
 */

#include "s300.h"

// 系统时钟频率 (简化版本，直接设为24MHz)
uint32_t SystemCoreClock = 24000000;

// 系统初始化函数 (startup.s需要的)
void SystemInit(void)
{
    // 简单初始化，什么都不做
    SystemCoreClock = 24000000;
}

// 简单的UART发送函数
static void uart_send_char(char c)
{
    volatile uint32_t *const THR = (uint32_t *)(0x40013000); // UART3 THR
    volatile uint32_t *const LSR = (uint32_t *)(0x40013014); // UART3 LSR
    // 等待发送缓冲区空
    while (((*LSR) & 0x20) == 0);
    // 发送字符
    *THR = (uint32_t)c;
}

// 简单的字符串发送函数
static void uart_send_string(const char *str)
{
    while (*str)
    {
        uart_send_char(*str++);
    }
}

// 简单的UART初始化
static void simple_uart_init(void)
{
    // 启用时钟
    volatile uint32_t *const APB1_CLK_EN = (uint32_t *)(0x4000A000u + 0x000Cu);
    *APB1_CLK_EN |= (1u << 8) | (1u << 3);
    // GPIO配置
    volatile uint32_t *const IO_MATRIX_CFG1 = (uint32_t *)(0x40008000u + 4u);
    uint32_t v = *IO_MATRIX_CFG1;
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |= ((0x3u << 20) | (0x3u << 22));
    *IO_MATRIX_CFG1 = v;
    // UART配置 115200, 8N1
    uint32_t base = 0x40013000; // UART3
    // 配置寄存器
    (*(volatile uint32_t *)(base + 0x04)) = 0x0u;   // IER
    (*(volatile uint32_t *)(base + 0x08)) = 0x07u;  // FCR
    (*(volatile uint32_t *)(base + 0x0C)) = 0x03u;  // LCR: 8N1
    (*(volatile uint32_t *)(base + 0x10)) = 0x00u;  // MCR
    // 设置波特率 115200
    (*(volatile uint32_t *)(base + 0x0C)) |= 0x80u;  // LCR DLAB=1
    (*(volatile uint32_t *)(base + 0x00)) = 13;      // DLL
    (*(volatile uint32_t *)(base + 0x04)) = 0;       // DLH
    (*(volatile uint32_t *)(base + 0x0C)) &= ~0x80u; // LCR DLAB=0
}

// 简单延时
static void simple_delay(uint32_t count)
{
    for (volatile uint32_t i = 0; i < count; i++)
    {
        __NOP();
    }
}

// 主函数
int main(void)
{
    // 基本初始化
    SCB->VTOR = 0x20000000;
    __DSB();
    // 初始化UART
    simple_uart_init();
    // 发送启动信息
    uart_send_string("\r\n==== S300 RBL Minimal v1.0 ====\r\n");
    uart_send_string("Hello from SRAM RBL!\r\n");
    uart_send_string("Build: " __DATE__ " " __TIME__ "\r\n");
    uart_send_string("================================\r\n\r\n");
    uint32_t counter = 0;
    // 主循环
    while (1)
    {
        counter++;
        // 每隔一段时间发送心跳
        if (counter % 1000000 == 0)
        {
            uart_send_string("[RBL] Heartbeat ");
            // 简单的数字转字符串 (只显示低4位)
            char num = '0' + ((counter / 1000000) % 10);
            uart_send_char(num);
            uart_send_string("\r\n");
        }
        // 简单延时
        simple_delay(100);
    }
    return 0;
}
