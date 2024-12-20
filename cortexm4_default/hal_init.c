/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-12-20 09:57:11
 * @FilePath     : \\ne004-plus\\cortexm4_default\\hal_init.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ne004xx.h"

#include "ne004xx_uart.h"
#include "ne004xx_gpio.h"

#define ARM_RISCV_IPCM          0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#include <stdio.h>

#define PUTCHAR_PROTOTYPE       int __io_putchar(int ch)

PUTCHAR_PROTOTYPE
{
    while ((UART_LSR(UART1) & UART_LSR_THRE) == 0);
            // usart_data_transmit(UART1, (uint32_t )a);
    UART_THR(UART1) = (uint8_t)ch;
    return ch;
}

int _write(int fd, const char *buf, int nbytes)
{
    (void)fd;
    for (int i = 0; i < nbytes; i++)
    {
        /* code */
        __io_putchar(*buf++);
    }
    return nbytes;
}

void arm_delay_ms(uint32_t ms);
void arm_delay_us(uint32_t us);

volatile uint32_t  systick_cnt = 0; // must be volatile to prevent compiler optimisations

void disableInterrupts()
{
    __disable_irq();
}

void enableInterrupts()
{
    __enable_irq();
}

volatile uint8_t rx_start_flag = 0;
volatile uint8_t rx_timeout_flag = 0;
volatile uint32_t rx_data_last = 0;

void  SysTick_Handler(void)
{
    systick_cnt++;
    if (rx_start_flag)
    {
        /* code */
        if ((systick_cnt - rx_data_last) >= 100)
        {
            /* code */
            rx_timeout_flag = 1;
            rx_start_flag = 0;
        }
    }
    if ((systick_cnt % 500) == 0)
    {
        /* code */
        REG32(0x40006000) = ~REG32(0x40006000);
    }
}

uint8_t rxover_flag = 0;
uint8_t txover_flag = 0;

uint8_t tx_flag = 0;

uint8_t rx_data_buf[128] = {0};
volatile size_t rx_data_cnt = 0;


#define UART1_RX_PIN (GPIO_PIN_16)
#define UART1_TX_PIN (GPIO_PIN_17)
#define UART2_RX_PIN (GPIO_PIN_23)
#define UART2_TX_PIN (GPIO_PIN_24)

static int rt_uart_init(void)
{
    // /* disable pdm */
    // REG32(0x4000D08CU) = 0x0U;
    // REG32(0x4000D05CU) = 0x2U;
    // /* disable all interrupt */
    // // disableInterrupts();
    // REG32(0x4000D0F8U) = 0x0U;  //屏蔽riscv所有外部中断
    // REG32(0x4000D0FCU) = 0x0U;  //屏蔽riscv2arm所有中断
    // REG64(0x4000D174U) = 0x0U;  //屏蔽arm所有个中断
    // REG64(0x4000D17CU) = 0x0U;  //屏蔽arm2riscv所有中断
    // ARM UART
    /* //uart1 */
// rx
    set_gpio_function(GPIOA, UART1_RX_PIN, 3);
    set_gpio_direction(GPIOA, UART1_RX_PIN, 0);
    set_gpio_mode(GPIOA, UART1_RX_PIN, 1);
// tx
    set_gpio_function(GPIOA, UART1_TX_PIN, 3);
    set_gpio_direction(GPIOA, UART1_TX_PIN, 1);
    set_gpio_mode(GPIOA, UART1_TX_PIN, 1);
    /* //uart2 */
// rx
    set_gpio_function(GPIOA, UART2_RX_PIN, 3);
    set_gpio_direction(GPIOA, UART2_RX_PIN, 0);
    set_gpio_mode(GPIOA, UART2_RX_PIN, 1);
// tx
    set_gpio_function(GPIOA, UART2_TX_PIN, 3);
    set_gpio_direction(GPIOA, UART2_TX_PIN, 1);
    set_gpio_mode(GPIOA, UART2_TX_PIN, 1);
    uart_init(UART1, SystemCoreClock, 115200);
    uart_init(UART2, SystemCoreClock, 115200);
    // UART_IER(UART1) |= 1;
    // NVIC_SetPriority(UART1_IRQn, 0);
    // NVIC_EnableIRQ(UART1_IRQn);
    return 0;
}

int hal_init(void)
{
    rt_uart_init();
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello from NE004-PLUS cortex-m4 core!\n");
    // SysTick_Config(AHBClock/1000); // set tick to every 1ms
    // enableInterrupts();
    // while (1)
    // {
    //     /* code */
    //     // // REG32(0x40006000) = 0xFFFFFFFF;
    //     // arm_delay_ms(500);
    //     // // REG32(0x40006000) = ~REG32(0x40006000);
    //     // printf("Hello from NE004-PLUS cortex-m4 core!\n");
    //     // // REG32(0x40006000) = 0x0;
    //     // // UART_DATA(UART0) = 0xA5;
    //     // arm_delay_ms(500);
    //     // // REG32(0x40006000) = ~REG32(0x40006000);
    //     // printf("Hello from NE004-PLUS cortex-m4 core!\n");
    //     // while (UART_STAT(UART0) & UART_STAT_RXFL);
    //     // __io_putchar(UART_DATA(UART0));
    //     if (rx_timeout_flag)
    //     {
    //         /* code */
    //         rx_timeout_flag = 0;
    //         for (size_t i = 0; i < rx_data_cnt; i++)
    //         {
    //             /* code */
    //             __io_putchar(rx_data_buf[i]);
    //         }
    //         rx_data_cnt = 0;
    //     }
    // }
    return 0;
}

void arm_delay_ms(uint32_t ms)
{
    arm_delay_us(1000 * ms);
}

void arm_delay_us(uint32_t us)
{
    uint32_t Delay = us * (APBClock / 1000000U) / 4;
    do
    {
        __NOP();
    }
    while (Delay --);
}
