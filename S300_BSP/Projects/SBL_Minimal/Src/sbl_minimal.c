/**
 * @file sbl_minimal.c
 * @brief 最简SBL程序 - 用于测试RBL跳转功能
 * @version 1.0
 * @date 2025-09-02
 */

#include "s300.h"

// SysTick计时器相关变量
static volatile uint32_t systick_ms_counter = 0;

// UART3寄存器定义
// #define UART3_BASE   (0x40013000u)  // 已由头文件定义
#define UART_THR     (*(volatile uint32_t *)(UART3_BASE + 0x00))
#define UART_LSR     (*(volatile uint32_t *)(UART3_BASE + 0x14))
#define UART_LCR     (*(volatile uint32_t *)(UART3_BASE + 0x0C))
#define UART_IER     (*(volatile uint32_t *)(UART3_BASE + 0x04))
#define UART_IIR_FCR (*(volatile uint32_t *)(UART3_BASE + 0x08))
#define UART_MCR     (*(volatile uint32_t *)(UART3_BASE + 0x10))

// 时钟和IO配置寄存器
#define APB1_BASE      (0x4000A000u)
#define APB1_CLK_EN    (*(volatile uint32_t *)(APB1_BASE + 0x000Cu))
// #define IO_MATRIX_BASE (0x40008000u)  // 已由头文件定义
#define IO_MATRIX_CFG1 (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x04u))

/**
 * @brief SysTick中断处理函数
 */
void SysTick_Handler(void)
{
    systick_ms_counter++;
}

/**
 * @brief SysTick初始化，配置为1ms中断周期
 */
static void systick_init(void)
{
    // 配置SysTick定时器，1ms中断一次
    // SysTick使用处理器时钟，计算重载值：SystemCoreClock / 1000 - 1
    uint32_t reload_value = (SystemCoreClock / 1000u) - 1u;
    
    // 配置SysTick
    SysTick->LOAD = reload_value & SysTick_LOAD_RELOAD_Msk;  // 设置重载值
    SysTick->VAL = 0u;                                       // 清除当前值
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |             // 使用处理器时钟
                    SysTick_CTRL_TICKINT_Msk |               // 使能SysTick异常请求
                    SysTick_CTRL_ENABLE_Msk;                 // 使能SysTick计数器
}

/**
 * @brief 获取当前毫秒计数
 */
static uint32_t get_tick_ms(void)
{
    return systick_ms_counter;
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

/**
 * @brief 基于SysTick的精确毫秒延时
 */
static void delay_ms_systick(uint32_t ms)
{
    uint32_t start_tick = get_tick_ms();
    while ((get_tick_ms() - start_tick) < ms)
    {
        __WFI();  // 等待中断，节省功耗
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

/* 旧的近似毫秒级忙等待（已废弃，使用SysTick精确延时）
static void delay_ms(uint32_t ms) __attribute__((unused));
static void delay_ms(uint32_t ms)
{
    uint32_t loops_per_ms = SystemCoreClock / 4000u;
    while (ms--)
    {
        delay_cycles(loops_per_ms);
    }
}
*/

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
    // 延时确保系统稳定（SystemInit已设置正确的VTOR）
    delay_cycles(1000);
    
    // 初始化UART
    uart_init();
    
    // 延时确保UART初始化完成
    delay_cycles(1000);
    
    // 使能全局中断（确保SysTick中断能够触发）
    __enable_irq();
    
    // 初始化SysTick定时器（必须在系统时钟配置完成后）
    systick_init();
    
    // 测试SysTick中断是否工作
    uint32_t test_start = get_tick_ms();
    delay_ms_systick(100);  // 延时100ms测试
    uint32_t test_end = get_tick_ms();
    uart_send_string("SysTick Test: ");
    uart_send_hex(test_end - test_start);
    uart_send_string(" ms elapsed\r\n");
    
    // 发送启动信息
    uart_send_string("\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("    S300 SBL Minimal Test v1.0\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("✅ SBL started successfully!\r\n");
    uart_send_string("✅ RBL jump to SBL works!\r\n");
    uart_send_string("Build: " __DATE__ " " __TIME__ "\r\n");
    uart_send_string("Flash Address: ");
    uart_send_hex(0x08010000);
    uart_send_string("\r\n");
    uart_send_string("Vector Table: ");
    uart_send_hex(SCB->VTOR);
    uart_send_string("\r\n");
    uart_send_string("Stack Pointer: ");
    uart_send_hex(__get_MSP());
    uart_send_string("\r\n");
    uart_send_string("System Clock: ");
    uart_send_hex(SystemCoreClock);
    uart_send_string(" Hz (");
    // 以MHz为单位显示，更易读
    uint32_t clock_mhz = SystemCoreClock / 1000000u;
    uint32_t clock_remainder = (SystemCoreClock % 1000000u) / 1000u;
    if (clock_mhz >= 100) uart_send_char('0' + (clock_mhz / 100) % 10);
    if (clock_mhz >= 10)  uart_send_char('0' + (clock_mhz / 10) % 10);
    uart_send_char('0' + (clock_mhz % 10));
    uart_send_char('.');
    uart_send_char('0' + (clock_remainder / 100) % 10);
    uart_send_char('0' + (clock_remainder / 10) % 10);
    uart_send_char('0' + (clock_remainder % 10));
    uart_send_string(" MHz)\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("\r\n");
    // 心跳逻辑：每1秒打印一次（使用精确的SysTick延时）
    uint32_t beat_cnt = 0;
    // 主循环
    while (1)
    {
        delay_ms_systick(1000u);  // 精确1秒延时
        beat_cnt++;
        uart_send_string("[SBL] 1s heartbeat #");
        // 打印十进制(最多3位)
        uint32_t n = beat_cnt % 1000u;
        if (n >= 100) uart_send_char('0' + (n / 100) % 10);
        if (n >= 10)  uart_send_char('0' + (n / 10) % 10);
        uart_send_char('0' + (n % 10));
        uart_send_string("\r\n");
    }
    return 0;
}
