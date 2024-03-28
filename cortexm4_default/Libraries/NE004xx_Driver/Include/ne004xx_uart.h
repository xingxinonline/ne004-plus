/*** 
 * @Author       : panxinhao
 * @Date         : 2023-07-27 16:18:15
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-07-29 11:37:32
 * @FilePath     : \\testbench_base\\cortexm4_timer\\Libraries\\NE004xx_Driver\\Include\\ne004xx_uart.h
 * @Description  : 
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef __NE004XX_UART_H__
#define __NE004XX_UART_H__

#include "ne004xx.h"

#define UART0                           UART_BASE

/* registers definitions */
#define UART_DATA(uartx)               REG32((uartx) + 0x00U)        /*!< UART data register */
#define UART_STAT(uartx)               REG32((uartx) + 0x04U)        /*!< UART status register 0 */
#define UART_CTRL(uartx)               REG32((uartx) + 0x08U)        /*!< UART control register 0 */
#define UART_INTSTAT(uartx)            REG32((uartx) + 0x0CU)        /*!< UART status register 1 */
#define UART_BAUD(uartx)               REG32((uartx) + 0x10U)        /*!< UART baud rate register */

/* bits definitions */
/* UARTx_DATA */
#define UART_DATA_DATA                 BITS(0,8)                      /*!< transmit or read data value */

/* UARTx_STAT */
#define UART_STAT_TXFL                 BIT(0)                         /*!< TX buffer full, read-only */
#define UART_STAT_RXFL                 BIT(1)                         /*!< RX buffer full, read-only.*/
#define UART_STAT_TXOR                 BIT(2)                         /*!< TX buffer overrun, write 1 to clear */
#define UART_STAT_RXOR                 BIT(3)                         /*!< RX buffer overrun, write 1 to clearr */

/* UARTx_CTRL */
#define UART_CTRL_TEN                  BIT(0)                         /*!< enable transmitter */
#define UART_CTRL_REN                  BIT(1)                         /*!< enable receiver */
#define UART_CTRL_TIE                  BIT(2)                         /*!< TX interrupt enable */
#define UART_CTRL_RIE                  BIT(3)                         /*!< RX interrupt enable */
#define UART_CTRL_TORIE                BIT(4)                         /*!< TX overrun interrupt enable */
#define UART_CTRL_RORIE                BIT(5)                         /*!< RX overrun interrupt enable */
#define UART_CTRL_TEST                 BIT(6)                         /*!< High-speed test mode for TX only */

/* UARTx_INTSTAT */
#define UART_INTSTAT_TIE               BIT(0)                         /*!< TX interrupt. Write 1 to clear. */
#define UART_INTSTAT_RIE               BIT(1)                         /*!< RX interrupt. Write 1 to clear. */
#define UART_INTSTAT_TORIE             BIT(2)                         /*!< TX overrun interrupt. Write 1 to clear */
#define UART_INTSTAT_RORIE             BIT(3)                         /*!< RX overrun interrupt. Write 1 to clear */

/* UARTx_BAUD */
#define UART_BAUD_DIV                  BITS(0,20)                     /*!< Baud rate divider.The minimum number is 16*/
#endif // !__NE004XX_