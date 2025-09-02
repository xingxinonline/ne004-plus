/**
 * @file rbl_simple.c
 * @brief 极简RBL - 不依赖printf，直接操作硬件
 * @version 1.0
 * @date 2025-09-01
 */

#include "s300.h"
#include "rbl_hal.h"

// 系统时钟频率 (简化版本，直接设为24MHz)
uint32_t SystemCoreClock = 24000000;

// 系统初始化函数 (startup.s需要的)
void SystemInit(void)
{
    // 简单初始化，什么都不做
    SystemCoreClock = 24000000;
}

// 移除内联 UART 与延时，改用 HAL 抽象

// 主函数
int main(void)
{
    // 基本初始化
    SCB->VTOR = 0x20000000;
    __DSB();
    // 初始化UART
    rbl_uart_init();
    // 发送启动信息
    rbl_uart_write_str("\r\n==== S300 RBL Minimal v1.0 ====\r\n");
    rbl_uart_write_str("Hello from SRAM RBL!\r\n");
    rbl_uart_write_str("Build: " __DATE__ " " __TIME__ "\r\n");
    rbl_uart_write_str("================================\r\n\r\n");
    uint32_t counter = 0;
    // 主循环
    while (1)
    {
        counter++;
        // 每隔一段时间发送心跳
        if (counter % 1000000 == 0)
        {
            rbl_uart_write_str("[RBL] Heartbeat ");
            // 简单的数字转字符串 (只显示低4位)
            char num = '0' + ((counter / 1000000) % 10);
            rbl_uart_write(&num, 1);
            rbl_uart_write_str("\r\n");
        }
        // 简单延时
        rbl_delay_cycles(100);
    }
    return 0;
}
