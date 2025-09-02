/**
 * @file rbl_simple.c
 * @brief 极简RBL - 不依赖printf，直接操作硬件
 * @version 1.0
 * @date 2025-09-01
 */

#include "s300.h"
#include "rbl_hal.h"
#include "rbl_qspi.h"

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
    RBL_LOG("\r\n==== S300 RBL Minimal v1.0 ====\r\n");
    RBL_LOG("Hello from SRAM RBL!\r\n");
    RBL_LOG("Build: " __DATE__ " " __TIME__ "\r\n");
    RBL_LOG("================================\r\n\r\n");

    // Phase 2: 初始化 QSPI 并读取 JEDEC ID (使用安全的低频率)
    RBL_LOG("[RBL] Starting Phase 2: QSPI initialization...\r\n");
    uint32_t ahb_clk = SystemCoreClock;  // 24MHz
    uint32_t safe_freq = ahb_clk / 8;    // 3MHz - 很安全的频率
    RBL_LOG("[RBL] About to call rbl_qspi_init()...\r\n");
    rbl_qspi_init(ahb_clk, safe_freq);
    RBL_LOG("[RBL] rbl_qspi_init() completed\r\n");
    uint8_t id[3] = {0};
    if (rbl_qspi_read_jedec_id(id) == 0) {
        RBL_LOG("[RBL] QSPI JEDEC read ok\r\n");
    } else {
        RBL_LOG("[RBL] QSPI JEDEC read failed\r\n");
    }
    uint32_t counter = 0;
    // 主循环
    while (1)
    {
        counter++;
        // 每隔一段时间发送心跳
        if (counter % 1000000 == 0)
        {
            RBL_LOG("[RBL] Heartbeat ");
            // 简单的数字转字符串 (只显示低4位)
            char num = '0' + ((counter / 1000000) % 10);
            rbl_uart_write(&num, 1);
            RBL_LOG("\r\n");
        }
        // 简单延时
        rbl_delay_cycles(100);
    }
    return 0;
}
