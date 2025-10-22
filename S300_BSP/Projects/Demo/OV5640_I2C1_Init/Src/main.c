#include "s300.h"
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "i2c_soft.h"
#include "ov5640.h"
#include <stdio.h>

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX 3u
#endif

/* 假设板卡 OV5640 的复位与电源控制分别接在以下引脚，若不一致请按实际修改： */
#define CAM_RST_PIN  15u  /* GPIOA15 */
#define CAM_PWDN_PIN 6u   /* GPIOA6  */
/* 可选：启用内置色条测试图，便于快速验证视频链路（1 开启 / 0 关闭） */
#ifndef OV5640_ENABLE_COLOR_BAR
    #define OV5640_ENABLE_COLOR_BAR 1
#endif
/* 可选：控制 OV5640 补光灯（1 开灯 / 0 关灯） */
#ifndef OV5640_ENABLE_LIGHT
    #define OV5640_ENABLE_LIGHT 1
#endif

static void gpio_init_for_camera(void)
{
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    /* RST/PWDN 推挽输出，上拉 */
    gpio_set_function(GPIOA, CAM_RST_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_RST_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_RST_PIN, 1);
    gpio_set_function(GPIOA, CAM_PWDN_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_PWDN_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_PWDN_PIN, 1);
}

static void camera_power_on_sequence(void)
{
    /* 参照原始序列：PWDN 高、RST 低 -> 延时 -> PWDN 低 -> 延时 -> RST 高 -> 延时 */
    gpio_set_data(GPIOA, CAM_RST_PIN, 0);
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 1);
    for (volatile uint32_t i = 0; i < 800000; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 0);
    for (volatile uint32_t i = 0; i < 800000; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_RST_PIN, 1);
    for (volatile uint32_t i = 0; i < 2400000; i++) __asm volatile("nop");
}

static void debug_uart_init(void)
{
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    /* UART3: GPIOA26/27 复用 */
    gpio_set_function(GPIOA, 26, FUNCTION_3);
    gpio_set_function(GPIOA, 27, FUNCTION_3);
    init_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    setvbuf(stdout, NULL, _IONBF, 0);
}

int main(void)
{
    debug_uart_init();
    printf("[OV5640] I2C1 soft init demo\n");
    /* 初始化软 I2C，索引 1 -> GPIOA0/A1，50kHz（降速便于兼容上电不稳时序） */
    i2c_soft_t i2c1;
    int ret = i2c_soft_init_default_idx(&i2c1, 1, 50000);
    if (ret)
    {
        printf("i2c init fail %d\n", ret);
        for (;;) __WFI();
    }
    (*((volatile uint32_t*)(RCC_BASE + 0x0018))) |= 1;
    gpio_init_for_camera();
    camera_power_on_sequence();
    /* 恢复总线并探测 0x3C/0x3D，选择有效地址 */
    i2c_soft_bus_recover(&i2c1);
    uint8_t saddr = 0x3C;
    int p3c = i2c_soft_probe(&i2c1, 0x3C);
    int p3d = i2c_soft_probe(&i2c1, 0x3D);
    printf("Probe 0x3C=%d 0x3D=%d\n", p3c, p3d);
    if (p3c != 0 && p3d == 0) saddr = 0x3D;
    /* 读取 Chip ID（高 0x300A，低 0x300B，期望 0x56 0x40） */
    uint8_t idh = 0, idl = 0;
    i2c_soft_mem_read(&i2c1, saddr, 0x300A, true, &idh, 1);
    i2c_soft_mem_read(&i2c1, saddr, 0x300B, true, &idl, 1);
    printf("OV5640 ID: 0x%02X 0x%02X (saddr=0x%02X)\n", idh, idl, saddr);
    /* 初始化 OV5640 为 YUYV 输出（720p 缩放参数在表内） */
    ret = ov5640_init(&i2c1, saddr, OV5640_FMT_YUV422_YUYV);
    printf("ov5640_init ret=%d\n", ret);
    if (ret == 0)
    {
        /* 再次读取 ID，部分板卡上电后首次读低字节可能为 0，初始化后再读一次更稳妥 */
        uint8_t idh2 = 0, idl2 = 0;
        i2c_soft_mem_read(&i2c1, saddr, 0x300A, true, &idh2, 1);
        i2c_soft_mem_read(&i2c1, saddr, 0x300B, true, &idl2, 1);
        printf("OV5640 ID after init: 0x%02X 0x%02X\n", idh2, idl2);
#if OV5640_ENABLE_COLOR_BAR
        int cr = ov5640_set_color_bar(&i2c1, saddr, true);
        printf("Enable color bar: %s\n", cr == 0 ? "OK" : "FAIL");
#endif
#if OV5640_ENABLE_LIGHT
        int lr = ov5640_set_light(&i2c1, saddr, true);
        printf("Enable light: %s\n", lr == 0 ? "OK" : "FAIL");
    /* 简短预览一段时间后自动关闭，避免常亮 */
    for (volatile uint32_t i = 0; i < 4800000u; ++i) __asm volatile("nop");
    int lf = ov5640_set_light(&i2c1, saddr, false);
    printf("Disable light: %s\n", lf == 0 ? "OK" : "FAIL");
#endif
    }
    /* 可选：打开色条测试图 */
    // ov5640_set_color_bar(&i2c1, OV5640_I2C_ADDR, true);
    for (;;)
    {
        __WFI();
    }
}
