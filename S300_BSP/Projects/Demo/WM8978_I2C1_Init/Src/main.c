#include "s300.h"
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "i2c_soft.h"
#include "wm8978.h"
#include <stdio.h>

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX 3u
#endif

static void debug_uart_init(void)
{
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    gpio_set_function(GPIOA, 26, FUNCTION_3);
    gpio_set_function(GPIOA, 27, FUNCTION_3);
    init_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    setvbuf(stdout, NULL, _IONBF, 0);
}

int main(void)
{
    debug_uart_init();
    printf("[WM8978] I2C1 soft init demo\n");
    i2c_soft_t i2c1;
    int ret = i2c_soft_init_default_idx(&i2c1, 1, 100000);
    if (ret)
    {
        printf("i2c init fail %d\n", ret);
        for (;;) __WFI();
    }
    /* 恢复总线并探测 0x1A/0x1B（视 CSB 引脚而定） */
    i2c_soft_bus_recover(&i2c1);
    int p1a = i2c_soft_probe(&i2c1, 0x1A);
    int p1b = i2c_soft_probe(&i2c1, 0x1B);
    printf("Probe WM8978 0x1A=%d 0x1B=%d\n", p1a, p1b);
    if (p1b == 0 && p1a != 0) wm8978_set_addr(0x1B);
    /* 直接按移植表初始化 WM8978 */
    ret = wm8978_init(&i2c1);
    printf("wm8978_init ret=%d\n", ret);
    /* 常见基本配置：开启 DAC/ADC、设置输入/输出、音量等 */
    if (ret == 0)
    {
        wm8978_set_adda(&i2c1, true, true);
        wm8978_set_input(&i2c1, true, false, false);
        wm8978_set_output(&i2c1, true, false);
        wm8978_set_mic_gain(&i2c1, 32);
        wm8978_set_hp_vol(&i2c1, 40, 40);
        wm8978_set_spk_vol(&i2c1, 40);
        wm8978_i2s_cfg(&i2c1, 0 /* I2S fmt */, 0 /* 16-bit */);
        printf("wm8978 basic cfg done\n");
    }
    for (;;)
    {
        __WFI();
    }
}
