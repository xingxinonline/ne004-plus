#include "s300_i2s.h"
#include <stdbool.h>

/*
 Register map assumption based on DesignWare I2S (DW_apb_i2s like):
 0x00 IER  [0]IEN, [1]REN, [2]TEN, [3]GEN, [7]DMAEN
 0x04 IRER [0]RXEN
 0x08 ITER [0]TXEN
 0x0C CER  [0]CLKEN
 0x10 CCR  [1:0]WSS, [5:3]SCLKG
 0x14 RXFFR (w1) flush
 0x18 TXFFR (w1) flush
 0x1C LRBR, 0x20 RRBR (read)
 0x24 LTHR, 0x28 RTHR (write)
 0x2C RER  rx chan enable mask
 0x30 TER  tx chan enable mask
 0x34 RCR  rx config: [3:0] wordlen
 0x38 TCR  tx config: [3:0] wordlen
 0x3C ISR  interrupt status
 0x40 IMR  interrupt mask
 0x44 ROR  rx overrun clear
 0x48 TOR  tx overrun clear
 0x4C RDR  rx data available level
 0x50 TDR  tx empty level
 0x54 I2S_CLRINT - combined clear
 Note: Exact bits may differ; adapt if register docs differ.
*/

typedef volatile struct {
    uint32_t IER;      /* 0x00 */
    uint32_t IRER;     /* 0x04 */
    uint32_t ITER;     /* 0x08 */
    uint32_t CER;      /* 0x0C */
    uint32_t CCR;      /* 0x10 */
    uint32_t RXFFR;    /* 0x14 */
    uint32_t TXFFR;    /* 0x18 */
    uint32_t LRBR;     /* 0x1C */
    uint32_t RRBR;     /* 0x20 */
    uint32_t LTHR;     /* 0x24 */
    uint32_t RTHR;     /* 0x28 */
    uint32_t RER;      /* 0x2C */
    uint32_t TER;      /* 0x30 */
    uint32_t RCR;      /* 0x34 */
    uint32_t TCR;      /* 0x38 */
    uint32_t ISR;      /* 0x3C */
    uint32_t IMR;      /* 0x40 */
    uint32_t ROR;      /* 0x44 */
    uint32_t TOR;      /* 0x48 */
    uint32_t RDR;      /* 0x4C */
    uint32_t TDR;      /* 0x50 */
    uint32_t CLR;      /* 0x54 */
} S300_I2S_Regs;

#define IER_IEN     (1u << 0)
#define IER_REN     (1u << 1)
#define IER_TEN     (1u << 2)
#define IER_GEN     (1u << 3)
#define IER_DMAEN   (1u << 7)

#define IRER_RXEN   (1u << 0)
#define ITER_TXEN   (1u << 0)

#define CER_CLKEN   (1u << 0)

#define CCR_WSS_Pos 0
#define CCR_WSS_Msk (0x3u << CCR_WSS_Pos)
#define CCR_SCLKG_Pos 3
#define CCR_SCLKG_Msk (0x7u << CCR_SCLKG_Pos)

#define RCR_WL_Pos 0
#define RCR_WL_Msk (0x7u << RCR_WL_Pos)
#define TCR_WL_Pos 0
#define TCR_WL_Msk (0x7u << TCR_WL_Pos)

#define RXFFR_FLUSH (1u << 0)
#define TXFFR_FLUSH (1u << 0)

#define I2S_INT_RXDA   (1u << 0)
#define I2S_INT_TXFE   (1u << 1)
#define I2S_INT_RXFO   (1u << 2)
#define I2S_INT_TXFO   (1u << 3)

static inline S300_I2S_Regs *i2s_base(S300_I2S_Id id) {
    switch (id) {
        case S300_I2S0_M4: return (S300_I2S_Regs *)0x4001B000u;
        case S300_I2S1_M4: return (S300_I2S_Regs *)0x4001C000u;
        case S300_I2S0_AON: return (S300_I2S_Regs *)0x43010000u;
        default: return (S300_I2S_Regs *)0x4001B000u;
    }
}

static inline uint32_t map_wordlen(S300_I2S_WordLen wl) {
    switch (wl) {
        case S300_I2S_WLEN_12: return 0x0; /* example mapping */
        case S300_I2S_WLEN_16: return 0x1;
        case S300_I2S_WLEN_20: return 0x2;
        case S300_I2S_WLEN_24: return 0x3;
        case S300_I2S_WLEN_32: return 0x4;
        default: return 0x1; /* default 16 */
    }
}

int S300_I2S_Init(S300_I2S_Id id, const S300_I2S_Config *cfg)
{
    if (!cfg) return -1;
    S300_I2S_Regs *r = i2s_base(id);

    /* Disable blocks before config */
    r->IER &= ~(IER_IEN | IER_REN | IER_TEN | IER_GEN);
    r->IRER &= ~IRER_RXEN;
    r->ITER &= ~ITER_TXEN;
    r->CER  &= ~CER_CLKEN;

    /* Flush FIFOs */
    r->RXFFR = RXFFR_FLUSH;
    r->TXFFR = TXFFR_FLUSH;

    /* Interface & channel mask: I2S uses 2 channels (L/R). For TDM, allow up to 4. */
    uint32_t ch_mask = (cfg->iface == S300_I2S_IF_TDM) ? ((1u << (cfg->tdm_slots & 0xF)) - 1u) : 0x3u; /* 2ch */
    r->RER = ch_mask;
    r->TER = ch_mask;

    /* Word length */
    uint32_t wl = map_wordlen(cfg->word_len);
    r->RCR = (r->RCR & ~RCR_WL_Msk) | (wl << RCR_WL_Pos);
    r->TCR = (r->TCR & ~TCR_WL_Msk) | (wl << TCR_WL_Pos);

    /* Clock generator */
    uint32_t ccr = r->CCR;
    ccr &= ~(CCR_WSS_Msk | CCR_SCLKG_Msk);
    ccr |= ((uint32_t)cfg->wss & 0x3u) << CCR_WSS_Pos;
    ccr |= ((uint32_t)cfg->sclk_gate & 0x7u) << CCR_SCLKG_Pos;
    r->CCR = ccr;

    /* Enable clock generator */
    if (cfg->clock_enable) r->CER |= CER_CLKEN; else r->CER &= ~CER_CLKEN;

    /* Enable RX/TX and I2S global */
    if (cfg->enable_rx) r->IRER |= IRER_RXEN; else r->IRER &= ~IRER_RXEN;
    if (cfg->enable_tx) r->ITER |= ITER_TXEN; else r->ITER &= ~ITER_TXEN;

    r->IER = (cfg->enable_rx ? IER_REN : 0) | (cfg->enable_tx ? IER_TEN : 0) | IER_IEN | (cfg->clock_enable ? IER_GEN : 0);

    return 0;
}

void S300_I2S_Enable(S300_I2S_Id id, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) r->IER |= IER_IEN; else r->IER &= ~IER_IEN;
}

void S300_I2S_ClockEnable(S300_I2S_Id id, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) r->CER |= CER_CLKEN; else r->CER &= ~CER_CLKEN;
}

void S300_I2S_SetWordLen(S300_I2S_Id id, S300_I2S_WordLen len)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t wl = map_wordlen(len);
    r->RCR = (r->RCR & ~RCR_WL_Msk) | (wl << RCR_WL_Pos);
    r->TCR = (r->TCR & ~TCR_WL_Msk) | (wl << TCR_WL_Pos);
}

void S300_I2S_SetInterface(S300_I2S_Id id, S300_I2S_Interface iface, uint8_t tdm_slots)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t ch_mask = (iface == S300_I2S_IF_TDM) ? ((1u << (tdm_slots & 0xF)) - 1u) : 0x3u;
    r->RER = ch_mask;
    r->TER = ch_mask;
}

void S300_I2S_SetClockGen(S300_I2S_Id id, S300_I2S_WordSelectSize wss, S300_I2S_SclkGate gate)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t ccr = r->CCR;
    ccr &= ~(CCR_WSS_Msk | CCR_SCLKG_Msk);
    ccr |= ((uint32_t)wss & 0x3u) << CCR_WSS_Pos;
    ccr |= ((uint32_t)gate & 0x7u) << CCR_SCLKG_Pos;
    r->CCR = ccr;
}

void S300_I2S_EnableRx(S300_I2S_Id id, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) { r->IRER |= IRER_RXEN; r->IER |= IER_REN; }
    else { r->IRER &= ~IRER_RXEN; r->IER &= ~IER_REN; }
}

void S300_I2S_EnableTx(S300_I2S_Id id, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) { r->ITER |= ITER_TXEN; r->IER |= IER_TEN; }
    else { r->ITER &= ~ITER_TXEN; r->IER &= ~IER_TEN; }
}

void S300_I2S_FlushRx(S300_I2S_Id id)
{
    i2s_base(id)->RXFFR = RXFFR_FLUSH;
}

void S300_I2S_FlushTx(S300_I2S_Id id)
{
    i2s_base(id)->TXFFR = TXFFR_FLUSH;
}

int S300_I2S_IntMask(S300_I2S_Id id, uint32_t mask, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) r->IMR |= mask; else r->IMR &= ~mask;
    return 0;
}

uint32_t S300_I2S_IntStatus(S300_I2S_Id id)
{
    return i2s_base(id)->ISR;
}

void S300_I2S_WriteLR(S300_I2S_Id id, uint16_t left, uint16_t right)
{
    S300_I2S_Regs *r = i2s_base(id);
    r->LTHR = left;
    r->RTHR = right;
}

uint32_t S300_I2S_ReadLR(S300_I2S_Id id)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t l = r->LRBR & 0xFFFFu;
    uint32_t rr = r->RRBR & 0xFFFFu;
    return (rr << 16) | l;
}

void S300_I2S_DmaEnable(S300_I2S_Id id, uint8_t tx_block, uint8_t rx_block, uint8_t tx_ch_mask, uint8_t rx_ch_mask)
{
    (void)tx_block; (void)rx_block; (void)tx_ch_mask; (void)rx_ch_mask;
    /* Minimal helper: just enable global DMA bit if present */
    S300_I2S_Regs *r = i2s_base(id);
    r->IER |= IER_DMAEN;
}
