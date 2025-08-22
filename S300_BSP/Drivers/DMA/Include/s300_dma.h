/*
 * S300 DMA Driver (DW_ahb_dmac compatible)
 */

#ifndef S300_DMA_H
#define S300_DMA_H

#include <stdint.h>
#include <stddef.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    S300_DMA0 = 0,
    S300_DMA1 = 1,
} S300_DMA_ID;

typedef enum
{
    S300_DMA_CH0 = 0, S300_DMA_CH1, S300_DMA_CH2, S300_DMA_CH3,
    S300_DMA_CH4 = 4, S300_DMA_CH5, S300_DMA_CH6, S300_DMA_CH7,
} S300_DMA_Channel;

typedef enum
{
    S300_DMA_TR_WIDTH_8  = 0,
    S300_DMA_TR_WIDTH_16 = 1,
    S300_DMA_TR_WIDTH_32 = 2,
} S300_DMA_TrWidth;

typedef enum
{
    S300_DMA_ADDR_INC = 0,
    S300_DMA_ADDR_DEC = 1,
    S300_DMA_ADDR_FIX = 2,
} S300_DMA_AddrInc;

typedef enum
{
    S300_DMA_MSIZE_1   = 0,
    S300_DMA_MSIZE_4   = 1,
    S300_DMA_MSIZE_8   = 2,
    S300_DMA_MSIZE_16  = 3,
    S300_DMA_MSIZE_32  = 4,
    S300_DMA_MSIZE_64  = 5,
    S300_DMA_MSIZE_128 = 6,
    S300_DMA_MSIZE_256 = 7,
} S300_DMA_MSize;

typedef enum
{
    S300_DMA_TT_M2M_FD  = 0x0,
    S300_DMA_TT_M2P_FD  = 0x1,
    S300_DMA_TT_P2M_FD  = 0x2,
    S300_DMA_TT_P2P_FD  = 0x3,
    S300_DMA_TT_P2M_FP  = 0x4,
    S300_DMA_TT_P2P_FSP = 0x5,
    S300_DMA_TT_M2P_FP  = 0x6,
    S300_DMA_TT_P2P_FDP = 0x7,
} S300_DMA_TrType;

/* Peripheral handshake indices (must match SoC mapping) */
typedef enum
{
    S300_DMA_HS_UART0_RX = 0x00,
    S300_DMA_HS_UART0_TX,
    S300_DMA_HS_UART1_RX,
    S300_DMA_HS_UART1_TX,
    S300_DMA_HS_UART2_RX,
    S300_DMA_HS_UART2_TX,
    S300_DMA_HS_UART3_RX,
    S300_DMA_HS_UART3_TX,
    S300_DMA_HS_SPI0_RX,
    S300_DMA_HS_SPI0_TX,
    S300_DMA_HS_SPI1_RX,
    S300_DMA_HS_SPI1_TX,
    S300_DMA_HS_I2S0_RX,
    S300_DMA_HS_I2S0_TX,
    S300_DMA_HS_I2S1_RX,
    S300_DMA_HS_I2S1_TX,
    S300_DMA_HS_PDM      = 0x100,
    S300_DMA_HS_AON,
    S300_DMA_HS_NONE     = 0xFFFF
} S300_DMA_Handshake;

typedef struct
{
    uint32_t SAR;
    uint32_t DAR;
    uint32_t LLP;
    uint32_t CTLL;
    uint32_t CTLH;
    uint32_t DSTAT; /* optional */
} S300_DMA_LLI __attribute__((aligned(4)));

typedef struct
{
    uintptr_t base;   /* DMAC base address */
} S300_DMA_Instance;

/* Driver setup */
void S300_DMA_SetBase(S300_DMA_ID id, uintptr_t base);
uintptr_t S300_DMA_GetBase(S300_DMA_ID id);

/* RCC gating (enable DMA0/DMA1 clocks via RCC sys clk en) */
void S300_DMA_EnableClock(S300_DMA_ID id, int enable);

/* Core controls */
void S300_DMA_GlobalEnable(S300_DMA_ID id, int enable);
void S300_DMA_ClearAllInterrupts(S300_DMA_ID id);

/* Standard transfer config (memory-to-memory default) */
int S300_DMA_ConfigStd(S300_DMA_ID id, S300_DMA_Channel ch,
                       uint32_t src, uint32_t dst, uint32_t bytes,
                       S300_DMA_TrWidth width);

/* Parameter adjustments */
void S300_DMA_SetIncrements(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_AddrInc src, S300_DMA_AddrInc dst);
void S300_DMA_SetBurst(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_MSize src_msize, S300_DMA_MSize dst_msize);
void S300_DMA_SetWidths(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_TrWidth src_w, S300_DMA_TrWidth dst_w);
void S300_DMA_SetTransferType(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_TrType type);

/* LLI config */
void S300_DMA_LLI_Set(S300_DMA_LLI *lli, uint32_t src, uint32_t dst, uint32_t bytes, S300_DMA_TrWidth width);
int  S300_DMA_SetLLI(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_LLI *lli_head);

/* Handshaking */
int S300_DMA_SetHandshake(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_Handshake src, S300_DMA_Handshake dst);

/* Interrupts */
typedef enum { S300_DMA_INT_BLOCK = 0x1, S300_DMA_INT_SRCTRAN = 0x2, S300_DMA_INT_DSTTRAN = 0x4, S300_DMA_INT_TFR = 0x8 } S300_DMA_IntType;
void S300_DMA_SetInterrupts(S300_DMA_ID id, S300_DMA_Channel ch, uint32_t types, int enable);

/* Run-time */
void S300_DMA_Start(S300_DMA_ID id, S300_DMA_Channel ch);
void S300_DMA_Stop(S300_DMA_ID id, S300_DMA_Channel ch);
int  S300_DMA_IsBusy(S300_DMA_ID id, S300_DMA_Channel ch);

#ifdef __cplusplus
}
#endif

#endif /* S300_DMA_H */
