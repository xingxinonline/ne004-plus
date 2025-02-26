/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-03-28 15:25:29
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

void arm_delay_us(uint32_t us);

int main(void)
{
    uint32_t temp = 0x0U;
    /* disable all interrupt */
    // REG32(0x4000D0F8U) = 0x0U;  //屏蔽riscv所有外部中断
    // REG32(0x4000D0FCU) = 0x0U;  //屏蔽riscv2arm所有中断
    // REG64(0x4000D174U) = 0x0U;  //屏蔽arm所有个中断
    // REG64(0x4000D17CU) = 0x0U;  //屏蔽arm2riscv所有中断
    /* config IO */
    // PAD_GP7_FUNCSEL  |= (0x3 << 28); // JTAG
    // PAD_GP8_FUNCSEL  |= (0x3);
    // PAD_GP9_FUNCSEL  |= (0x3 << 4);
    // PAD_GP10_FUNCSEL |= (0x03 << 8);
    // PAD_GP13_FUNCSEL |= (0x01 << 20); // riscv UART
    // PAD_GP14_FUNCSEL |= (0x1 << 24);
    // PAD_GP15_FUNCSEL |= (0x02 << 28); // ARM UART
    // PAD_GP16_FUNCSEL |= (0x2);
    // UART_BAUD(UART0) = UART_BAUD_DIV & (12500000 / 115200);
    // UART_CTRL(UART0) = UART_CTRL_TEN | UART_CTRL_REN;
    REG32(0x4000D140) = 0x1;
    REG32(0x4000D144) = 0x0;
    REG32(0x4000D148) = 0x110;
    REG32(0x40006004) = 0xFFFFFFFF;
    while (1)
    {
        /* code */
        // UART_DATA(UART0) = 0x5A;
        REG32(0x40006000) = 0xFFFFFFFF;
        arm_delay_us(10);
        REG32(0x40006000) = 0x0;
        arm_delay_us(10);
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

void arm_delay_us(uint32_t us)
{
    volatile uint32_t i = 0, j = 10;
    for (i = 0; i < us; i++)//18+5 = 23cycle
    {
        while (j)//18cycle 100/20
        {
            j--;
        }
    }
}
