#ifndef S300_BOARD_H
#define S300_BOARD_H

/* Board: S300_EVB default routes */
#ifndef BOARD_UART_DEBUG_ID
    #define BOARD_UART_DEBUG_ID   3u  /* default: UART3 */
#endif

/* Optional: default debug baudrate */
#ifndef BOARD_UART_DEBUG_BAUD
    #define BOARD_UART_DEBUG_BAUD 115200u
#endif

/* Board init hooks */
void Board_Clock_Init(void);
void Board_Pinmux_Init(void);

#endif /* S300_BOARD_H */
