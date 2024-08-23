/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-23 17:29:12
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

int hal_init(void)
{
    uint32_t temp = 0x0U;
    /* disable pdm */
    REG32(0x4000D08CU) = 0x0U;
    REG32(0x4000D05CU) = 0x2U;
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


    /* enable riscv interrupt parameters */
    // REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM2RISCV_IRQ;
    // REG32(0x4000D0F8U) |= 0x1U << RISCV_DMA1_IRQ;
    // REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM_NOTICE_IRQ;
    /* enable arm interrupt parameters */
    REG64(0x4000D174U) |= 1ULL << RXOVRINT0_IRQn;   //允许pdm2pcm中断
    REG64(0x4000D174U) |= 1ULL << TXOVRINT0_IRQn; //允许riscv_notice中断
    REG64(0x4000D174U) |= 1ULL << RXINT0_IRQn; //允许riscv_notice中断
    REG64(0x4000D174U) |= 1ULL << TXINT0_IRQn; //允许riscv_notice中断
    /* enable arm2riscv interrupt */
    // REG64(0x4000D17CU) |= 1ULL << REQ_FFT_DONE_IRQn; //发送FFT中断到riscv处理
    /* enable arm nvic irq */
    // NVIC_EnableIRQ(RXOVRINT0_IRQn);
    // NVIC_EnableIRQ(TXOVRINT0_IRQn);
    // NVIC_EnableIRQ(RXINT0_IRQn);
    // NVIC_EnableIRQ(TXINT0_IRQn);
    UART_BAUD(UART0) = UART_BAUD_DIV & (APBClock / 115200);
    UART_CTRL(UART0) = UART_CTRL_TEN | UART_CTRL_REN;

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
    (void)temp;
    return 0;
}
uint32_t value = 0;
void RXINT0_IRQHandler(void)
{
    if (rx_start_flag == 0)
    {
        /* code */
        rx_start_flag = 1;
    }
    rx_data_last = systick_cnt;
    if ((UART_STAT(UART0) & UART_STAT_RXFL) != 0)
    {
        /* code */
        uint8_t temp = UART_DATA(UART0);
        rx_data_buf[rx_data_cnt++] = temp;
        
    }
    UART_INTSTAT(UART0) |= UART_INTSTAT_RIE;
}

void TXINT0_IRQHandler(void)
{
    tx_flag = 1;
    UART_INTSTAT(UART0) |= UART_INTSTAT_TIE;
}

void RXOVRINT0_IRQHandler(void)
{
    rxover_flag = 1;
    UART_INTSTAT(UART0) |= UART_INTSTAT_RORIE;
}

void TXOVRINT0_IRQHandler(void)
{
    txover_flag = 1;
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
