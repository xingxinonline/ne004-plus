#ifndef S300_UART_H
#define S300_UART_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART polling APIs */
void S300_UART_Init_115200(uint32_t idx);
void S300_UART_Init(uint32_t idx, uint32_t baud);
/* ===== Config ===== */
typedef struct {
	uint32_t baud;      /* e.g. 115200 */
	uint8_t data_bits;  /* 5..8, default 8 */
	uint8_t stop_bits;  /* 1 or 2, default 1 */
	uint8_t parity;     /* 0: none, 1: odd, 2: even, default 0 */
	uint8_t fifo_enable;/* 0/1, default 1 */
} S300_UartConfig;

/* UART basic init */
int  S300_UART_Config(uint32_t idx, const S300_UartConfig *cfg);

/* ===== Polling IO ===== */
void S300_UART_PutCharI(uint32_t idx, char c);
void S300_UART_PutStringI(uint32_t idx, const char *s);
int  S300_UART_TryWrite(uint32_t idx, uint8_t byte);            /* return 1 on sent, 0 if busy */
int  S300_UART_Write(uint32_t idx, const uint8_t *buf, size_t len, uint32_t timeout_ms);
int  S300_UART_TryRead(uint32_t idx, uint8_t *byte);            /* return 1 on read, 0 if no data */
int  S300_UART_Read(uint32_t idx, uint8_t *buf, size_t len, uint32_t timeout_ms);
size_t S300_UART_RxAvailable(uint32_t idx);                     /* bytes in RX buffer if IRQ is used */
int  S300_UART_RxGet(uint32_t idx, uint8_t *byte);              /* pop one byte from RX ring if present */

/* ===== Status & Control ===== */
int  S300_UART_TxReady(uint32_t idx);   /* THR empty-able */
int  S300_UART_TxIdle(uint32_t idx);    /* transmitter fully empty */
int  S300_UART_RxReady(uint32_t idx);   /* data available */
void S300_UART_FlushRx(uint32_t idx);
void S300_UART_FlushTx(uint32_t idx);
void S300_UART_SoftReset(uint32_t idx); /* via SRR */

/* ===== Interrupt control (optional) ===== */
int  S300_UART_EnableRxIRQ(uint32_t idx, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif /* S300_UART_H */
