/*
 * @Author       : panxinhao
 * @Date         : 2024-04-10 18:28:36
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-11-21 09:26:40
 * @FilePath     : \\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Source\\ne004xx_uart.c
 * @Description  :
 *
 * Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */
#include <stdint.h>
#include <stddef.h>  // 定义了 NULL
#include "ne004xx_uart.h"

int uart_init(uint32_t uartx, uint32_t clock, uint32_t baud)
{
    uint32_t temp;
    UART_LCR(uartx) = 0x00;
    temp = UART_RBR(uartx);
    temp = UART_LSR(uartx);
    temp = UART_MSR(uartx);
    while (UART_USR(uartx) & 1);
    UART_FCR(uartx) = 0x0;
    UART_LCR_EXT(uartx) = 0x00;
    UART_TCR(uartx) = 0x00;
    UART_IER(uartx) = 0x00;
    UART_HTX(uartx) = 0x00;
    UART_DMASA(uartx) = 0x00;
    UART_LCR(uartx) = 0
                      | 0 << 5
                      | 0 << 4
                      | 0 << 3
                      | 0 << 2
                      | 3;
    UART_MCR(uartx) = 0x00;
    UART_FCR(uartx) = 0x00
                      | 0 << 6
                      | 0 << 4
                      | 0 << 3
                      | 6
                      | 0;
    UART_LCR(uartx) |= 0x80;
    temp = clock / (16 * baud);
    UART_DLL(uartx) = temp & 0xff;
    UART_DLH(uartx) = (temp >> 8) & 0xff;
    temp = (clock - (temp * (16 * baud))) / baud;
    UART_DLF(uartx) = temp & 0xff;
    UART_LCR(uartx) &= ~0x80;
    temp = UART_LCR(uartx);
    while (UART_USR(uartx) & 1);
    UART_IER(uartx) |= 0x15;
    temp = UART_IIR(uartx);
    return 0;
}

uint16_t read_uart(uint32_t uartx)
{
    while ((UART_USR(uartx) & 0x08) == 0);
    if (UART_FCR(uartx) & 0x1)
    {
        /* code */
        return UART_SRBRn(uartx);
    }
    else
    {
        return UART_RBR(uartx);
    }
}

int uart_write(uint32_t uartx, uint16_t data)
{
    while ((UART_LSR(uartx) & UART_LSR_THRE) == 0);
    UART_THR(uartx) = (uint8_t)data;

    return 0;
}

// 串口发送字符串函数
void uart_send_str(uint32_t uartx, const char *str)
{
    if (str == NULL)
    {
        return; // 防止传入空指针
    }
    while (*str != '\0')
    {
        uart_write(uartx, (uint8_t)(*str)); // 发送当前字符
        str++;                       // 移动到下一个字符
    }
}