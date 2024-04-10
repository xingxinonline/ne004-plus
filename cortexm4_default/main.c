/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-10 16:35:41
 * @FilePath     : \\ne004-plus\\cortexm4_default\\main.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ne004xx.h"
#include "startriscv.h"

#include "ne004xx_uart.h"

#define ARM_RISCV_IPCM          0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#include <stdio.h>

#define PUTCHAR_PROTOTYPE       int __io_putchar(int ch)

PUTCHAR_PROTOTYPE
{
    while (UART_STAT(UART0) & UART_STAT_TXFL);
    UART_DATA(UART0) = (uint8_t)ch;
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

void disableInterrupts() {
    __disable_irq();
}

void enableInterrupts() {
    __enable_irq();
}


void  SysTick_Handler(void)
{
	systick_cnt++;
    if ((systick_cnt % 500) == 0)
    {
        /* code */
        REG32(0x40006000) = ~REG32(0x40006000);
    }
}

int main(void)
{
    uint32_t temp = 0x0U;
    /* disable all interrupt */
    disableInterrupts();
    REG32(0x4000D0F8U) = 0x0U;  //屏蔽riscv所有外部中断
    REG32(0x4000D0FCU) = 0x0U;  //屏蔽riscv2arm所有中断
    REG64(0x4000D174U) = 0x0U;  //屏蔽arm所有个中断
    REG64(0x4000D17CU) = 0x0U;  //屏蔽arm2riscv所有中断
    /* config IO */
    REG32(0x4000D140) = 0x1;
    REG32(0x4000D144) = 0x0;
    REG32(0x4000D148) = 0x0;
    REG32(0x40006004) = 0xFFFFFFFF;
    PAD_GP6_FUNCSEL  |= (0x3 << 24); // riscv JTAG
    PAD_GP7_FUNCSEL  |= (0x3 << 28); 
    PAD_GP8_FUNCSEL  |= (0x3);
    PAD_GP9_FUNCSEL  |= (0x3 << 4);
    PAD_GP10_FUNCSEL |= (0x03 << 8);
    PAD_GP13_FUNCSEL |= (0x01 << 20); // riscv UART
    PAD_GP14_FUNCSEL |= (0x1 << 24);
    PAD_GP15_FUNCSEL |= (0x02 << 28); // ARM UART
    PAD_GP16_FUNCSEL |= (0x2);
    UART_BAUD(UART0) = UART_BAUD_DIV & (APBClock / 460800);
    UART_CTRL(UART0) = UART_CTRL_TEN | UART_CTRL_REN;
    /* start riscv */
    start_riscv(1);

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Hello from NE004-PLUS cortex-m4 core!\n");

    SysTick_Config(AHBClock/1000); // set tick to every 1ms

    enableInterrupts();
    
    while (1)
    {
        /* code */
        // REG32(0x40006000) = 0xFFFFFFFF;
        arm_delay_ms(500);
        // REG32(0x40006000) = ~REG32(0x40006000);
        printf("Hello from NE004-PLUS cortex-m4 core!\n");
        // REG32(0x40006000) = 0x0;
        // UART_DATA(UART0) = 0xA5;
        arm_delay_ms(500);
        // REG32(0x40006000) = ~REG32(0x40006000);
        printf("Hello from NE004-PLUS cortex-m4 core!\n");
    }
    (void)temp;
    return 0;
}
uint32_t value = 0;
void RXINT0_IRQHandler(void)
{
    UART_INTSTAT(UART0) |= UART_INTSTAT_RIE;
}

void TXINT0_IRQHandler(void)
{
    UART_INTSTAT(UART0) |= UART_INTSTAT_TIE;
}

void RXOVRINT0_IRQHandler(void)
{
    UART_INTSTAT(UART0) |= UART_INTSTAT_RORIE;
}

void TXOVRINT0_IRQHandler(void)
{
    UART_INTSTAT(UART0) |= UART_INTSTAT_TORIE;
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
