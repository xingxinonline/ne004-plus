/**
 * @file rbl_main_minimal.c
 * @brief S300 ROM Bootloader (RBL) - 最小版本用于测试
 * @version 1.0
 * @date 2025-09-01
 *
 * 最小的RBL实现，主要用于测试header生成和bin合成功能
 */

#include "rbl_main.h"
#include "rbl_system.h"
#include "rbl_uart.h"

/* RBL版本信息 */
#define RBL_VERSION_MAJOR    1
#define RBL_VERSION_MINOR    0
#define RBL_VERSION_PATCH    0

/**
 * @brief 打印最小RBL Banner
 */
static void rbl_print_minimal_banner(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("  S300 RBL - ROM Bootloader (Minimal Version)  \r\n");
    printf("================================================\r\n");
    printf("Version: %d.%d.%d\r\n", RBL_VERSION_MAJOR, RBL_VERSION_MINOR, RBL_VERSION_PATCH);
    printf("Build:   %s %s\r\n", __DATE__, __TIME__);
    printf("Chip:    S300 PiMCHIP (Cortex-M4)\r\n");
    printf("Mode:    Minimal Test Version\r\n");
    printf("================================================\r\n");
    printf("\r\n");
}

/**
 * @brief 最小RBL入口函数
 */
int main(void)
{
    /* 基本系统初始化 */
    rbl_system_early_init();
    /* 初始化调试串口 */
    rbl_uart_init();
    /* 打印启动Banner */
    rbl_print_minimal_banner();
    printf("[RBL] Hello from S300 RBL Minimal!\r\n");
    printf("[RBL] This is a test version for header/bin generation\r\n");
    printf("[RBL] System Clock: %u Hz\r\n", (unsigned int)rbl_system_get_clock());
    printf("[RBL] Reset Reason: %s\r\n", rbl_get_reset_reason_string());
    /* 简单的心跳循环 */
    uint32_t counter = 0;
    while (1)
    {
        printf("[RBL] Heartbeat: %lu\r\n", (unsigned long)counter++);
        /* 简单延时 */
        for (volatile uint32_t i = 0; i < 1000000; i++)
        {
            __asm__("nop");
        }
        /* 每10次心跳检查是否有串口输入 */
        if (counter % 10 == 0)
        {
            printf("[RBL] Waiting for input...\r\n");
            printf("[RBL] Press 'r' to restart, 'h' for help\r\n");
            /* 简单的字符处理 */
            if (rbl_uart_data_available())
            {
                char ch = rbl_uart_getchar();
                switch (ch)
                {
                case 'r':
                case 'R':
                    printf("[RBL] Restarting...\r\n");
                    rbl_system_reset();
                    break;
                case 'h':
                case 'H':
                    printf("[RBL] Help:\r\n");
                    printf("  r - Restart system\r\n");
                    printf("  h - Show this help\r\n");
                    break;
                default:
                    printf("[RBL] Unknown command: '%c'\r\n", ch);
                    break;
                }
            }
        }
    }
    return 0;
}
