/* HelloWorld app: print "Hello, world!" with software reset support */
#include "s300.h"
#include <stdio.h>
#include <string.h>

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX 3u
#endif

static void minimal_uart3_gpio_init(void)
{
    const uint32_t APB1_CLK_EN_OFF = 0x000Cu;     /* CM4_APB1_CLK_EN_REG */
    volatile uint32_t *const APB1_CLK_EN = (uint32_t *)(RCC_BASE + APB1_CLK_EN_OFF);
    *APB1_CLK_EN |= (1u << 8) | (1u << 3);
    volatile uint32_t *const IO_MATRIX_CFG1 = (uint32_t *)(IO_MATRIX_BASE + 4u);
    uint32_t v = *IO_MATRIX_CFG1;
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |= ((0x3u << 20) | (0x3u << 22));
    *IO_MATRIX_CFG1 = v;
}

static void uart_set_baud(uint32_t base, uint32_t baud)
{
    uint32_t clk = SystemCoreClock;
    if (baud == 0u || clk == 0u) return;
    uint32_t denom = baud * 16u;
    uint32_t div = clk / denom;
    uint32_t rem = clk % denom;
    uint32_t dlf = (uint32_t)((uint64_t)rem * 16u + (denom / 2u)) / denom;
    if (dlf >= 16u)
    {
        dlf = 0u;
        div += 1u;
    }
    if (div == 0u) div = 1u;
    /* LCR |= DLAB */
    (*(volatile uint32_t *)(base + 0x0Cu)) |= 0x80u;
    /* DLL/DLH */
    (*(volatile uint32_t *)(base + 0x00u)) = (div & 0xFFu);
    (*(volatile uint32_t *)(base + 0x04u)) = ((div >> 8) & 0xFFu);
    /* LCR &= ~DLAB */
    (*(volatile uint32_t *)(base + 0x0Cu)) &= ~0x80u;
    /* DLF (1/16 step) */
    (*(volatile uint32_t *)(base + 0xC0u)) = (dlf & 0x0Fu);
}

static void uart_init_poll(uint32_t idx, uint32_t baud)
{
    uint32_t base = UART0_BASE + (idx * 0x1000u);
    /* IER, FCR, LCR, MCR */
    (*(volatile uint32_t *)(base + 0x04u)) = 0x0u;   /* IER */
    (*(volatile uint32_t *)(base + 0x08u)) = 0x07u;  /* FCR */
    (*(volatile uint32_t *)(base + 0x0Cu)) = 0x03u;  /* LCR: 8N1 */
    (*(volatile uint32_t *)(base + 0x10u)) = 0x00u;  /* MCR */
    uart_set_baud(base, baud);
}

int main(void)
{
    minimal_uart3_gpio_init();
    uart_init_poll(UART_DEBUG_IDX, 115200u);
    setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("\n");
    printf("==================================================\n");
    printf("      S300 HelloWorld with Software Reset        \n");
    printf("==================================================\n");
    printf("Hello, world!\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("Core: ARM Cortex-M4 @ %lu MHz\n", SystemCoreClock / 1000000);
    
    // 初始化软件复位功能
    if (app_software_reset_init() == 0) {
        printf("✅ Software reset functionality enabled\n");
    } else {
        printf("❌ Software reset init failed\n");
    }
    
    printf("\nAvailable commands: help, info, reset, download, status\n");
    printf("==================================================\n\n");
    
    // 简单的命令行处理
    char cmd_buffer[64];
    int cmd_index = 0;
    
    for (;;)
    {
        // 处理软件复位循环任务
        app_software_reset_loop_handler();
        
        // 简单的字符输入处理
        // 这里应该替换为实际的UART接收中断处理
        // 目前只是示例，实际项目中应该用中断方式
        
        // 模拟每秒输出一次心跳
        static uint32_t last_heartbeat = 0;
        uint32_t current_time = HAL_GetTick();
        if (current_time - last_heartbeat >= 10000) { // 10秒心跳
            last_heartbeat = current_time;
            printf("[APP] Heartbeat - uptime: %lu ms\n", current_time);
        }
        
        __WFI();
    }
}
