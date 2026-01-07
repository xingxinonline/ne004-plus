/**
 * @file main.c
 * @brief 简单显示Demo主程序 - 不使用LVGL
 * 
 * 功能说明：
 *   - 初始化板级硬件（时钟、串口、PSRAM等）
 *   - 初始化视频子系统
 *   - 初始化显示层（全绿色，透明度为0）
 *   - 通过邮箱接收DSP发送的人脸坐标，绘制矩形框
 *   - 支持串口命令控制
 * 
 * 注意：PSRAM仅支持16bit读写，不支持8bit读写
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Platform */
#include "s300.h"

/* Board and subsystems */
#include "rcc.h"
#include "board.h"
#include "video.h"
#include "psram.h"
#include "mailbox.h"
#include "gpio.h"
#include "camera_ov5640.h"

/* Simple display modules */
#include "direct_display.h"
#include "app_console.h"
#include "app_mailbox.h"

/* 1ms节拍计时 */
static volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void)
{
    g_tick_ms++;
}

static inline uint32_t millis(void)
{
    return g_tick_ms;
}

/**
 * @brief 背光控制GPIO初始化
 */
static void background_light_init(void)
{
    set_gpio_function(GPIOA, 24, FUNCTION_2);  /* GPIO复用功能 */
    set_gpio_mode(GPIOA, 24, GPIO_UP);         /* 上拉 */
    set_gpio_direction(GPIOA, 24, 1);          /* 输出模式 */
    set_gpio_data(GPIOA, 24, 0);               /* 输出低电平（开启背光） */
}

/**
 * @brief 摄像头初始化
 */
static int camera_preinit(void)
{
    printf("[DirectDisplay] Camera pre-init (Power sequence & I2C)...\r\n");
    return camera_ov5640_preinit();
}

int main(void)
{
    /* 板级初始化：时钟 + 调试串口 */
    board_init();
    printf("\r\n");
    printf("========================================\r\n");
    printf("[S300][DirectDisplayDemo] Booting...\r\n");
    printf("========================================\r\n");

    /* 配置SysTick为1ms节拍 */
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        printf("[S300][DirectDisplayDemo][ERR] SysTick_Config failed!\r\n");
    }

    /* 初始化PSRAM */
    printf("[DirectDisplay] Init PSRAM...\r\n");
    init_psram(4, 1);

    /* 初始化PLL时钟 */
    printf("[DirectDisplay] Init PLL clocks...\r\n");
    rcc_init_mm_pll(8, 400, 0, 3, 2);   /* 100MHz */
    rcc_init_dsp_pll(6, 768, 0, 2, 2);  /* 300MHz */

    /* 初始化背光 */
    background_light_init();

    /* 摄像头上电与探测（失败则仅初始化显示链路） */
    int cam_ret = camera_preinit();
    if (cam_ret != 0) {
        printf("[DirectDisplay][WARN] OV5640 init failed (%d), continue to init video for display path only.\r\n", cam_ret);
    }
    
    /* 初始化视频子系统 */
    printf("[DirectDisplay] Init video subsystem...\r\n");
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);
    
    /* 初始化简单显示系统（先清除显存，避免视频启用时显示乱码） */
    printf("[DirectDisplay] Init simple display system...\r\n");
    direct_display_init();

    /* 初始化邮箱（M4 <-> DSP通信） */
    printf("[DirectDisplay] Init mailbox...\r\n");
    init_mailbox(MAILBOX_BASE, 4, MAILBOX_IRQ_NONE);
    set_dsp_warm_reset(true);
    write_mailbox(MAILBOX_BASE, 0x5A5A5A5A);  /* 发送启动令牌 */

    /* 初始化邮箱处理 */
    app_mailbox_init();
    app_mailbox_set_time_callback(millis);

    /* 初始化串口命令 */
    app_console_init();

    printf("\r\n");
    printf("[DirectDisplay] System ready!\r\n");
    printf("[DirectDisplay] Layer: GREEN, Alpha: 0 (transparent)\r\n");
    printf("[DirectDisplay] Waiting for face detection data...\r\n");
    printf("\r\n");

    /* 绘制一个初始测试框（可选，用于验证显示） */
    #if 1
    printf("[DirectDisplay] Drawing test rectangle...\r\n");
    direct_display_draw_rect(50, 50, 80, 100, 3, 0xC0);
    printf("[DirectDisplay] Test rectangle drawn at (50,50) 80x100\r\n");
    direct_display_refresh();
    #endif

    /* 主循环 */
    while (1)
    {
        /* 轮询邮箱数据（人脸坐标） */
        app_mailbox_poll();

        /* 轮询串口命令 */
        app_console_poll();

        /* 简单延时（避免过度占用CPU） */
        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 10u) {
            /* idle spin ~10ms */
        }
    }
}
