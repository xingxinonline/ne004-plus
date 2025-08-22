#include "s300_i2s.h"
#include <stdbool.h>

/*
 按 docs/i2s寄存器手册.md 对齐的寄存器映射：
 0x00 IER    : [11:8]TDM_SLOTS, [5]FRAME_OFF, [1]INTF_TYPE, [0]IEN
 0x04 IRER   : [0]RXEN
 0x08 ITER   : [0]TXEN
 0x0C CER    : [0]CLKEN
 0x10 CCR    : [4:3]WSS, [2:0]SCLKG
 0x14 RXFFR  : 接收FIFO复位 (自清除)
 0x18 TXFFR  : 发送FIFO复位 (自清除)
 0x1C SR     : 状态
 0x20 LRBR0/LTHR0 : 左声道读/写（读/写窗口复用）
 0x24 RRBR0/RTHR0 : 右声道读/写（读/写窗口复用）
 0x28 RER0   : 接收通道使能掩码
 0x2C TER0   : 发送通道使能掩码
 0x30 RCR0   : 接收配置 [2:0]WLEN
 0x34 TCR0   : 发送配置 [2:0]WLEN
 0x38 ISR0   : 中断状态 (RXDA[0], RXFO[1], TXFE[4], TXFO[5])
 0x3C IMR0   : 中断屏蔽
 0x40 ROR0   : 读清 RX 溢出
 0x44 TOR0   : 读清 TX 溢出
 0x48 RFCR0  : 接收 FIFO 配置 (低4位触发阈值)
 0x4C TFCR0  : 发送 FIFO 配置 (低4位触发阈值)
 0x50 RFF0   : 接收 FIFO 刷新
 0x54 TFF0   : 发送 FIFO 刷新
 0x200 DMACR : DMA 控制 (块/通道使能)
*/

typedef volatile struct {
    uint32_t IER;      /* 0x00 */
    uint32_t IRER;     /* 0x04 */
    uint32_t ITER;     /* 0x08 */
    uint32_t CER;      /* 0x0C */
    uint32_t CCR;      /* 0x10 */
    uint32_t RXFFR;    /* 0x14 */
    uint32_t TXFFR;    /* 0x18 */
    uint32_t SR;       /* 0x1C */
    uint32_t LRBR0_LTHR0; /* 0x20 读/写窗口：左 */
    uint32_t RRBR0_RTHR0; /* 0x24 读/写窗口：右 */
    uint32_t RER0;     /* 0x28 */
    uint32_t TER0;     /* 0x2C */
    uint32_t RCR0;     /* 0x30 */
    uint32_t TCR0;     /* 0x34 */
    uint32_t ISR0;     /* 0x38 */
    uint32_t IMR0;     /* 0x3C */
    uint32_t ROR0;     /* 0x40 */
    uint32_t TOR0;     /* 0x44 */
    uint32_t RFCR0;    /* 0x48 */
    uint32_t TFCR0;    /* 0x4C */
    uint32_t RFF0;     /* 0x50 */
    uint32_t TFF0;     /* 0x54 */
    uint32_t _rsvd[0x200/4 - 0x15];
    uint32_t DMACR;    /* 0x200 */
} S300_I2S_Regs;

#define IER_IEN       (1u << 0)
#define IER_INTF_TYPE (1u << 1)
#define IER_FRAME_OFF (1u << 5)
#define IER_TDM_Pos   8u
#define IER_TDM_Msk   (0xFu << IER_TDM_Pos)

#define IRER_RXEN   (1u << 0)
#define ITER_TXEN   (1u << 0)

#define CER_CLKEN   (1u << 0)

#define CCR_SCLKG_Pos 0
#define CCR_SCLKG_Msk (0x7u << CCR_SCLKG_Pos)
#define CCR_WSS_Pos   3
#define CCR_WSS_Msk   (0x3u << CCR_WSS_Pos)

#define RCR_WL_Pos 0
#define RCR_WL_Msk (0x7u << RCR_WL_Pos)
#define TCR_WL_Pos 0
#define TCR_WL_Msk (0x7u << TCR_WL_Pos)

#define RFCR_TRIG_Pos 0
#define RFCR_TRIG_Msk 0xFu
#define TFCR_TRIG_Pos 0
#define TFCR_TRIG_Msk 0xFu

#define RXFFR_FLUSH (1u << 0)
#define TXFFR_FLUSH (1u << 0)

/* ISR0 / IMR0 位（与头文件导出一致） */
#define I2S_INT_RXDA  (1u << 0)
#define I2S_INT_RXFO  (1u << 1)
#define I2S_INT_TXFE  (1u << 4)
#define I2S_INT_TXFO  (1u << 5)

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
    r->IER &= ~IER_IEN;
    r->IRER &= ~IRER_RXEN;
    r->ITER &= ~ITER_TXEN;
    r->CER  &= ~CER_CLKEN;

    /* Flush FIFOs */
    r->RXFFR = RXFFR_FLUSH;
    r->TXFFR = TXFFR_FLUSH;

    /* 接口类型与 TDM 槽位 */
    uint32_t ier = r->IER & ~(IER_INTF_TYPE | IER_TDM_Msk | IER_FRAME_OFF);
    if (cfg->iface == S300_I2S_IF_TDM) ier |= IER_INTF_TYPE;
    /* tdm_slots: 1..4 => 0..3 编码 */
    uint32_t slots = (cfg->tdm_slots > 0 ? (cfg->tdm_slots - 1) : 0) & 0xFu;
    ier |= (slots << IER_TDM_Pos);
    r->IER = ier;

    /* 通道掩码：I2S 双声道用 0x3；TDM 依据 slots 开启对应路数 */
    uint32_t ch_mask = (cfg->iface == S300_I2S_IF_TDM) ? ((1u << (cfg->tdm_slots & 0xF)) - 1u) : 0x3u;
    r->RER0 = ch_mask;
    r->TER0 = ch_mask;

    /* Word length */
    uint32_t wl = map_wordlen(cfg->word_len);
    r->RCR0 = (r->RCR0 & ~RCR_WL_Msk) | (wl << RCR_WL_Pos);
    r->TCR0 = (r->TCR0 & ~TCR_WL_Msk) | (wl << TCR_WL_Pos);

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

    /* 开 IEN */
    r->IER |= IER_IEN;

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
    r->RCR0 = (r->RCR0 & ~RCR_WL_Msk) | (wl << RCR_WL_Pos);
    r->TCR0 = (r->TCR0 & ~TCR_WL_Msk) | (wl << TCR_WL_Pos);
}

void S300_I2S_SetInterface(S300_I2S_Id id, S300_I2S_Interface iface, uint8_t tdm_slots)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t ier = r->IER & ~(IER_INTF_TYPE | IER_TDM_Msk);
    if (iface == S300_I2S_IF_TDM) ier |= IER_INTF_TYPE;
    uint32_t slots = (tdm_slots > 0 ? (tdm_slots - 1) : 0) & 0xFu;
    ier |= (slots << IER_TDM_Pos);
    r->IER = ier;

    uint32_t ch_mask = (iface == S300_I2S_IF_TDM) ? ((1u << (tdm_slots & 0xF)) - 1u) : 0x3u;
    r->RER0 = ch_mask;
    r->TER0 = ch_mask;
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

void S300_I2S_SetFifoTrigger(S300_I2S_Id id, uint8_t rx_level, uint8_t tx_level)
{
    S300_I2S_Regs *r = i2s_base(id);
    r->RFCR0 = (r->RFCR0 & ~RFCR_TRIG_Msk) | ((rx_level & 0xF) << RFCR_TRIG_Pos);
    r->TFCR0 = (r->TFCR0 & ~TFCR_TRIG_Msk) | ((tx_level & 0xF) << TFCR_TRIG_Pos);
}

int S300_I2S_IntMask(S300_I2S_Id id, uint32_t mask, uint8_t en)
{
    S300_I2S_Regs *r = i2s_base(id);
    if (en) r->IMR0 &= ~mask; else r->IMR0 |= mask; /* 文档定义 0=enable,1=mask */
    return 0;
}

uint32_t S300_I2S_IntStatus(S300_I2S_Id id)
{
    return i2s_base(id)->ISR0;
}

void S300_I2S_ClearOverrun(S300_I2S_Id id)
{
    volatile uint32_t tmp;
    S300_I2S_Regs *r = i2s_base(id);
    tmp = r->ROR0; (void)tmp;
    tmp = r->TOR0; (void)tmp;
}

void S300_I2S_WriteLR(S300_I2S_Id id, uint16_t left, uint16_t right)
{
    S300_I2S_Regs *r = i2s_base(id);
    r->LRBR0_LTHR0 = left;
    r->RRBR0_RTHR0 = right;
}

uint32_t S300_I2S_ReadLR(S300_I2S_Id id)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t l = r->LRBR0_LTHR0 & 0xFFFFu;
    uint32_t rr = r->RRBR0_RTHR0 & 0xFFFFu;
    return (rr << 16) | l;
}

void S300_I2S_DmaEnable(S300_I2S_Id id, uint8_t tx_block, uint8_t rx_block, uint8_t tx_ch_mask, uint8_t rx_ch_mask)
{
    S300_I2S_Regs *r = i2s_base(id);
    uint32_t v = r->DMACR;
    if (rx_block) v |= (1u << 16); else v &= ~(1u << 16);
    if (tx_block) v |= (1u << 17); else v &= ~(1u << 17);
    /* 通道 0..3 使能位 */
    v &= ~((0xFu << 0) | (0xFu << 8));
    v |= ((uint32_t)(rx_ch_mask & 0xF) << 0);
    v |= ((uint32_t)(tx_ch_mask & 0xF) << 8);
    r->DMACR = v;
}
