#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "board.h"

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX BOARD_UART_DEBUG_IDX
#endif

int _write(int fd, char *pBuffer, int size)
{
#if BOARD_UART3_DEBUG_ENABLE
    for (int i = 0; i < size; i++)
        write_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, (uint8_t)pBuffer[i]);
#else
    (void)pBuffer;
    (void)size;
#endif
    (void)fd;
    return size;
}
