#include <stdio.h>
#include "s300.h"
#include "rcc.h"
#include "gpio.h"
#include "i2c_soft.h"
#include "ov5640.h"
#include "camera_ov5640.h"

#ifndef CAM_RST_PIN
#define CAM_RST_PIN  10u  /* GPIOA15 */
#endif
#ifndef CAM_PWDN_PIN
#define CAM_PWDN_PIN 19u   /* GPIOA6  */
#endif

static void cam_gpio_init(void)
{
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    gpio_set_function(GPIOA, CAM_RST_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_RST_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_RST_PIN, 1);
    gpio_set_function(GPIOA, CAM_PWDN_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_PWDN_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_PWDN_PIN, 1);
}

static void cam_power_on_sequence(void)
{
    gpio_set_data(GPIOA, CAM_RST_PIN, 0);
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 1);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 0);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_RST_PIN, 1);
    for (volatile uint32_t i = 0; i < 2400000u; i++) __asm volatile("nop");
}

int camera_ov5640_preinit(void)
{
    i2c_soft_t i2c1;
    int ret = i2c_soft_init_default_idx(&i2c1, 1, 50000);
    if (ret) {
        printf("[S300][DisplayDemo][CAM] i2c init fail %d\r\n", ret);
        return ret;
    }
    cam_gpio_init();
    cam_power_on_sequence();
    (void)i2c_soft_bus_recover(&i2c1);

    uint8_t saddr = 0x3C;
    int p3c = i2c_soft_probe(&i2c1, 0x3C);
    int p3d = i2c_soft_probe(&i2c1, 0x3D);
    if (p3c != 0 && p3d == 0) saddr = 0x3D;

    uint8_t idh = 0, idl = 0;
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Au, true, &idh, 1);
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Bu, true, &idl, 1);
    printf("[S300][DisplayDemo][CAM] OV5640 ID: 0x%02X 0x%02X (addr=0x%02X)\r\n", idh, idl, saddr);
    int lr = ov5640_set_light(&i2c1, saddr, true);
    printf("Enable light: %s\n", lr == 0 ? "OK" : "FAIL");
    for (volatile uint32_t i = 0; i < 4800000u; ++i) __asm volatile("nop");
    int lf = ov5640_set_light(&i2c1, saddr, false);
    printf("Disable light: %s\n", lf == 0 ? "OK" : "FAIL");
    ret = ov5640_init(&i2c1, saddr, OV5640_FMT_YUV422_YUYV);
    printf("[S300][DisplayDemo][CAM] ov5640_init ret=%d\r\n", ret);
    return ret;
}
