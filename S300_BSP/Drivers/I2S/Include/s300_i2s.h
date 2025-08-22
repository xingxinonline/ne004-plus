#ifndef S300_I2S_H
#define S300_I2S_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* I2S instances
   M4 domain: I2S0@0x4001B000, I2S1@0x4001C000; AON: I2S0_AON@0x43010000 */

typedef enum {
    S300_I2S0_M4 = 0,
    S300_I2S1_M4 = 1,
    S300_I2S0_AON = 2
} S300_I2S_Id;

typedef enum {
    S300_I2S_IF_I2S = 0,
    S300_I2S_IF_TDM  = 1
} S300_I2S_Interface;

typedef enum {
    S300_I2S_WLEN_IGNORE = 0,
    S300_I2S_WLEN_12 = 1,
    S300_I2S_WLEN_16 = 2,
    S300_I2S_WLEN_20 = 3,
    S300_I2S_WLEN_24 = 4,
    S300_I2S_WLEN_32 = 5
} S300_I2S_WordLen;

typedef enum {
    S300_I2S_WSS_16 = 0,  /* 16 sclk */
    S300_I2S_WSS_24 = 1,  /* 24 sclk */
    S300_I2S_WSS_32 = 2   /* 32 sclk */
} S300_I2S_WordSelectSize;

typedef enum {
    S300_I2S_SCLKG_NONE = 0,
    S300_I2S_SCLKG_12   = 1,
    S300_I2S_SCLKG_16   = 2,
    S300_I2S_SCLKG_20   = 3,
    S300_I2S_SCLKG_24   = 4
} S300_I2S_SclkGate;

typedef struct {
    S300_I2S_Interface iface;
    S300_I2S_WordLen word_len;     /* RX/TX word length */
    S300_I2S_WordSelectSize wss;   /* clock generator WSS */
    S300_I2S_SclkGate sclk_gate;   /* SCLK gating */
    uint8_t tdm_slots;             /* 1..4 for TDM (encoded per IER[11:8]) */
    uint8_t enable_rx;             /* enable RX block */
    uint8_t enable_tx;             /* enable TX block */
    uint8_t clock_enable;          /* enable clock generator */
    uint32_t mclk_hz;              /* input mclk; driver may compute divider externally */
    uint32_t sample_rate_hz;       /* target sample rate (informational) */
} S300_I2S_Config;

/* Basic control */
int S300_I2S_Init(S300_I2S_Id id, const S300_I2S_Config *cfg);
void S300_I2S_Enable(S300_I2S_Id id, uint8_t en);
void S300_I2S_ClockEnable(S300_I2S_Id id, uint8_t en);
void S300_I2S_SetWordLen(S300_I2S_Id id, S300_I2S_WordLen len);
void S300_I2S_SetInterface(S300_I2S_Id id, S300_I2S_Interface iface, uint8_t tdm_slots);
void S300_I2S_SetClockGen(S300_I2S_Id id, S300_I2S_WordSelectSize wss, S300_I2S_SclkGate gate);
void S300_I2S_EnableRx(S300_I2S_Id id, uint8_t en);
void S300_I2S_EnableTx(S300_I2S_Id id, uint8_t en);

/* FIFO */
void S300_I2S_FlushRx(S300_I2S_Id id);
void S300_I2S_FlushTx(S300_I2S_Id id);

/* IRQ */
int  S300_I2S_IntMask(S300_I2S_Id id, uint32_t mask, uint8_t en);
uint32_t S300_I2S_IntStatus(S300_I2S_Id id);

/* Data IO (stereo LR) */
void S300_I2S_WriteLR(S300_I2S_Id id, uint16_t left, uint16_t right);
uint32_t S300_I2S_ReadLR(S300_I2S_Id id);

/* DMA helper */
void S300_I2S_DmaEnable(S300_I2S_Id id, uint8_t tx_block, uint8_t rx_block, uint8_t tx_ch_mask, uint8_t rx_ch_mask);

#ifdef __cplusplus
}
#endif

#endif /* S300_I2S_H */
