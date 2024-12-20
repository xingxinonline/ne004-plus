/*** 
 * @Author       : panxinhao
 * @Date         : 2023-07-27 16:18:15
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-11-21 09:27:07
 * @FilePath     : \\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Include\\ne004xx_uart.h
 * @Description  : 
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef __NE004XX_UART_H__
#define __NE004XX_UART_H__

#include "ne004xx.h"

#define UART0                           UART0_BASE
#define UART1                           UART1_BASE
#define UART2                           UART2_BASE


/* registers definitions */
// #define UART_THR(uartx)                 REG32((uartx) + 0x00U)        /*!< UART data register */
// #define UART_DLL(uartx)                 REG32((uartx) + 0x00U)        /*!< UART data register */
// #define UART_DLH(uartx)                 REG32((uartx) + 0x04U)        /*!< UART data register */
// #define UART_BAUD(uartx)                REG32((uartx) + 0x10U)        /*!< UART baud rate register */
// #define UART_LCR(uartx)                 REG32((uartx) + 0x0CU)        /*!< UART status register 0 */
// #define UART_LSR(uartx)                 REG32((uartx) + 0x14U)        /*!< UART status register 0 */
// #define UART_CTRL(uartx)                REG32((uartx) + 0x08U)        /*!< UART control register 0 */
// #define UART_USR(uartx)                 REG32((uartx) + 0x7CU)        /*!< UART status register 1 */
// #define UART_DLF(uartx)                 REG32((uartx) + 0xC0U)        /*!< UART status register 1 */

#define UART_RBR(uartx)                 REG32((uartx) + 0x00U)                 
#define UART_THR(uartx)                 REG32((uartx) + 0x00U) 
#define UART_DLL(uartx)                 REG32((uartx) + 0x00U) 
#define UART_IER(uartx)                 REG32((uartx) + 0x04U) 
#define UART_DLH(uartx)                 REG32((uartx) + 0x04U) 
#define UART_IIR(uartx)                 REG32((uartx) + 0x08U) 
#define UART_FCR(uartx)                 REG32((uartx) + 0x08U) 
#define UART_LCR(uartx)                 REG32((uartx) + 0x0cU) 
#define UART_MCR(uartx)                 REG32((uartx) + 0x10U) 
#define UART_LSR(uartx)                 REG32((uartx) + 0x14U) 
#define UART_MSR(uartx)                 REG32((uartx) + 0x18U) 
#define UART_SCR(uartx)                 REG32((uartx) + 0x1cU) 
#define UART_LPDLL(uartx)               REG32((uartx) + 0x20U) 
#define UART_LPDLH(uartx)               REG32((uartx) + 0x24U) 
#define UART_SRBRn(uartx)               REG32((uartx) + 0x30U) 
#define UART_STHRn(uartx)               REG32((uartx) + 0x30U) 
#define UART_FAR(uartx)                 REG32((uartx) + 0x70U) 
#define UART_TFR(uartx)                 REG32((uartx) + 0x74U) 
#define UART_RFW(uartx)                 REG32((uartx) + 0x78U) 
#define UART_USR(uartx)                 REG32((uartx) + 0x7cU) 
#define UART_TFL(uartx)                 REG32((uartx) + 0x80U) 
#define UART_RFL(uartx)                 REG32((uartx) + 0x84U) 
#define UART_SRR(uartx)                 REG32((uartx) + 0x88U) 
#define UART_SRTS(uartx)                REG32((uartx) + 0x8cU) 
#define UART_SBCR(uartx)                REG32((uartx) + 0x90U) 
#define UART_SDMAM(uartx)               REG32((uartx) + 0x94U) 
#define UART_SFE(uartx)                 REG32((uartx) + 0x98U) 
#define UART_SRT(uartx)                 REG32((uartx) + 0x9cU) 
#define UART_STET(uartx)                REG32((uartx) + 0xa0U) 
#define UART_HTX(uartx)                 REG32((uartx) + 0xa4U) 
#define UART_DMASA(uartx)               REG32((uartx) + 0xa8U) 
#define UART_TCR(uartx)                 REG32((uartx) + 0xacU) 
#define UART_DE_EN(uartx)               REG32((uartx) + 0xb0U) 
#define UART_RE_EN(uartx)               REG32((uartx) + 0xb4U) 
#define UART_DET(uartx)                 REG32((uartx) + 0xb8U) 
#define UART_TAT(uartx)                 REG32((uartx) + 0xbcU) 
#define UART_DLF(uartx)                 REG32((uartx) + 0xc0U) 
#define UART_RAR(uartx)                 REG32((uartx) + 0xc4U) 
#define UART_TAR(uartx)                 REG32((uartx) + 0xc8U) 
#define UART_LCR_EXT(uartx)             REG32((uartx) + 0xccU) 
#define UART_UART_PROT_LEVEL(uartx)     REG32((uartx) + 0xd0U)             
#define UART_REG_TIMEOUT_RST(uartx)     REG32((uartx) + 0xd4U)             
#define UART_CPR(uartx)                 REG32((uartx) + 0xf4U) 
#define UART_UCV(uartx)                 REG32((uartx) + 0xf8U) 
#define UART_CTR(uartx)                 REG32((uartx) + 0xfcU) 
/* bits definitions */

#define UART_LSR_THRE                   BIT(5) 
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

#define UART_USR_BUSY                  BIT(0)                         /*!< UART busy flag */ 
#define UART_USR_TFNF                  BIT(1)                         /*!< Transmit FIFO Not Full */ 
#define UART_USR_TFE                   BIT(2)                         /*!< Transmit FIFO Empty */ 
#define UART_USR_RFNE                  BIT(3)                         /*!< Receive FIFO Not Empty. */ 
#define UART_USR_RFF                   BIT(4)                         /*!< Receive FIFO Full. */ 

#define UART_LSR_DR                    BIT(0)      
#define UART_LSR_TFNF                  BIT(1)      
#define UART_LSR_TFE                   BIT(2)      
#define UART_LSR_RFNE                  BIT(3)      
#define UART_LSR_RFF                   BIT(4)      
#define UART_LSR_THRE                  BIT(5)      

int uart_init(uint32_t uartx, uint32_t clock, uint32_t baud);
uint16_t read_uart(uint32_t uartx);
int uart_write(uint32_t uartx, uint16_t data);
void uart_send_str(uint32_t uartx, const char *str);
#endif // !__NE004XX_UART_H__