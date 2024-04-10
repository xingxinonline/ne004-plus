/*
 * @Author       : panxinhao
 * @Date         : 2023-07-05 19:08:38
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-10 10:26:53
 * @FilePath     : \\ne004-plus\\riscv64_default\\main.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "reg.h"
#include "plic.h"
#include "dma.h"

#include "delay.h"

#define ARM_RISCV_IPCM          0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

void user_interrupt_handler(void);

#include <stdio.h>

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)


PUTCHAR_PROTOTYPE
{
    while (REG8(0x64000003U) & 0x80);
    REG8(0x64000000U) = ch;
    return ch;
}

int _write(int fd, char *pBuffer, int size)
{
    (void)fd;
    for (int i = 0; i < size; i++)
    {
        /* code */
        __io_putchar(*pBuffer++);
    }
    return size;
}
uint64_t volatile start_cycles;
uint64_t volatile end_cycles;
uint64_t volatile take_cycles;
int main(void)
{
    uint64_t temp = 0;
    // 13.  RV将GPIO2、GPIO3设置为输出，配置8‘h0x64002008为2’b10
    REG8(0x64002008U) = 0x8U | 0x4U;
    //clear dmac intterrupt status
    DMA_CHX_INTCLEAR(DMA1, DMA_CH1) = 0xFFFFFFFFUL;
    plic_init();
    __asm volatile("csrs mie, %0" :: "r"(0x800));
    __asm volatile("csrs mstatus, 8");
    // f.   向baud_div寄存器(addr=0x6400_0018)写入数据0x364.
    REG16(0x64000018U) = 400000000U / 2000000;
    // g.   向rx_ctrl寄存器(addr=0x6400_000C)写入数据0x1.
    REG8(0x6400000CU) = 0x1U;
    // h.   向tx_ctrl寄存器(addr=0x6400_0008)写入数据0x1.
    REG8(0x64000008U) = 0x1U;
    setvbuf(stdout, NULL, _IONBF, 0);
    while (1)
    {
        printf("Hello from NE004-PLUS riscv64 core!\n");
        delayus(1000000);
    }
    (void)temp;
    return 0;
}

void handle_trap(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    if (ulMCAUSE & 0x8000000000000000UL)
    {
        if ((ulMCAUSE & 0xffUL) == 0x0B)
        {
            user_interrupt_handler();
        }
        return;
    }
    (void)ulMCAUSE;
    (void)ulMEPC;
    while (1);
}

void risc_v_application_interrupt_handler(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    handle_trap(ulMCAUSE, ulMEPC);
}

void risc_v_application_exception_handler(int64_t ulMCAUSE, uint64_t ulMEPC)
{
    handle_trap(ulMCAUSE, ulMEPC);
}

void user_interrupt_handler(void)
{
    int source = plic_claim();
    /*do anything you need according to interrupt source number*/
    if (source == ARM2RISCV_IRQ)
    {
    }
    else if (source == ARM_NOTICE_IRQ)
    {
        REG8(0x6400200CU) |= 0x4U;
        REG8(0x6400200CU) &= ~0x4U;
    }
    else if (source == DMA1_IRQ)
    {
        if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH1) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH1) = 0xFFFFFFFFUL;
        }
        else if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH2) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH2) = 0xFFFFFFFFUL;
        }
        else if (DMA_CHX_INTSTATUSREG(DMA1, DMA_CH3) & DMA_CHX_INTSTATUS_BLOCK_TRF_DONE)
        {
            DMA_CHX_INTCLEAR(DMA1, DMA_CH3) = 0xFFFFFFFFUL;
        }
    }
    plic_complete(source);
    return;
}
