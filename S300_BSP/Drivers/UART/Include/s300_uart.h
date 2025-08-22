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

/* ===== Extended features ===== */
/* FIFO 水位配置（兼容 FCR/SFE/SRT/STET） */
typedef enum {
	S300_UART_RX_TRIG_1CHAR = 0,
	S300_UART_RX_TRIG_QUARTER = 1,
	S300_UART_RX_TRIG_HALF = 2,
	S300_UART_RX_TRIG_FULL_MINUS2 = 3
} S300_UartRxTrig;

typedef enum {
	S300_UART_TX_TRIG_EMPTY = 0,
	S300_UART_TX_TRIG_2CHARS = 1,
	S300_UART_TX_TRIG_QUARTER = 2,
	S300_UART_TX_TRIG_HALF = 3
} S300_UartTxTrig;

typedef struct {
	uint8_t enable;      /* 0/1: FIFO 使能 */
	S300_UartRxTrig rx_trig; /* 接收触发 */
	S300_UartTxTrig tx_trig; /* 发送触发 */
	uint8_t dma_mode;    /* 0: mode0, 1: mode1 */
} S300_UartFifoConfig;

int S300_UART_SetFIFO(uint32_t idx, const S300_UartFifoConfig *cfg);

/* 通用中断使能/禁用与状态读取（直接操作 IER/IIR/LSR） */
int S300_UART_EnableIRQ(uint32_t idx, uint32_t ier_mask);
int S300_UART_DisableIRQ(uint32_t idx, uint32_t ier_mask);
uint32_t S300_UART_GetIIR(uint32_t idx);
uint32_t S300_UART_GetLSR(uint32_t idx);

/* RS485 半双工配置（基于 DW_apb_uart: TCR/DE_EN/RE_EN/DET/TAT） */
typedef enum {
	S300_UART_RS485_TXRX = 0, /* 正常收发 */
	S300_UART_RS485_TX_ONLY = 1, /* 仅发（依 IP 实现）*/
	S300_UART_RS485_PROPRIETARY = 2 /* 厂商扩展/保留 */
} S300_UartRS485Mode;

typedef struct {
	uint8_t enable;            /* 0/1 使能 RS485 硬件模式 */
	uint8_t de_active_high;    /* DE 极性：1 高有效 */
	uint8_t re_active_high;    /* RE 极性：1 高有效 */
	S300_UartRS485Mode mode;   /* 传输模式 */
	uint8_t de_assert_time;    /* DE 置位延时（bit 时间单位，DET[7:0]）*/
	uint8_t de_deassert_time;  /* DE 释放延时（DET[23:16]）*/
	uint8_t re2de_turnaround;  /* RE->DE 转换时间（TAT[7:0]）*/
	uint8_t de2re_turnaround;  /* DE->RE 转换时间（TAT[23:16]）*/
} S300_UartRS485Config;

int S300_UART_SetRS485(uint32_t idx, const S300_UartRS485Config *cfg);

/* 9位模式（地址/数据） */
typedef struct {
	uint8_t enable;          /* 0/1 使能 9bit */
	uint8_t addr_match_en;   /* 0/1 地址匹配使能 */
	uint8_t rx_addr;         /* 接收匹配地址（RAR）*/
	uint8_t tx_addr;         /* 发送地址（TAR）*/
} S300_Uart9bitConfig;

int S300_UART_Set9bit(uint32_t idx, const S300_Uart9bitConfig *cfg);
/* 发送一次地址帧（设置 TX-ADDR 模式，发送 TAR 或提供的 addr，然后自动退出地址模式）*/
int S300_UART_9bitSendAddress(uint32_t idx, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* S300_UART_H */
