/* S300 DMA driver implementation */

#include "s300_dma.h"
#include "s300_rcc.h"

/* Assumptions: Two DMACs with 1MB spacing in old code. Provide overridable base. */
static S300_DMA_Instance s_dma[2] =
{
    { .base = 0x41000000u }, /* DMA0_M4 base per docs */
    { .base = 0x42000000u }, /* DMA1_M4 if present; can be overridden */
};

#define DMAC_REG(id, off) (*(volatile uint32_t *)((s_dma[(int)(id)].base) + (uint32_t)(off)))
#define DMAC_CH_REG(id, ch, off) (*(volatile uint32_t *)((s_dma[(int)(id)].base) + (uint32_t)(off) + ((uint32_t)(ch) * 0x58u)))

/* Channel offsets */
#define DMAC_SAR        0x0000
#define DMAC_DAR        0x0008
#define DMAC_LLP        0x0010
#define DMAC_CTLL       0x0018
#define DMAC_CTLH       0x001C
#define DMAC_CFGL       0x0040
#define DMAC_CFGH       0x0044

/* Global offsets */
#define DMAC_RawTfr     0x02C0
#define DMAC_RawBlock   0x02C8
#define DMAC_RawSrcTran 0x02D0
#define DMAC_RawDstTran 0x02D8
#define DMAC_RawErr     0x02E0
#define DMAC_StatusTfr  0x02E8
#define DMAC_StatusBlock 0x02F0
#define DMAC_StatusSrcTran 0x02F8
#define DMAC_StatusDstTran 0x0300
#define DMAC_StatusErr  0x0308
#define DMAC_MaskTfr    0x0310
#define DMAC_MaskBlock  0x0318
#define DMAC_MaskSrcTran 0x0320
#define DMAC_MaskDstTran 0x0328
#define DMAC_MaskErr    0x0330
#define DMAC_ClearTfr   0x0338
#define DMAC_ClearBlock 0x0340
#define DMAC_ClearSrcTran 0x0348
#define DMAC_ClearDstTran 0x0350
#define DMAC_ClearErr   0x0358
#define DMAC_StatusInt  0x0360
#define DMAC_ReqSrcReg  0x0368
#define DMAC_ReqDstReg  0x0370
#define DMAC_SglReqSrcReg 0x0378
#define DMAC_SglReqDstReg 0x0380
#define DMAC_LstSrcReg  0x0388
#define DMAC_LstDstReg  0x0390
#define DMAC_DmaCfgReg  0x0398
#define DMAC_ChEnReg    0x03A0

/* Helpers */
static inline void set_bits(volatile uint32_t *r, uint32_t m)
{
    *r |= m;
}
static inline void clr_bits(volatile uint32_t *r, uint32_t m)
{
    *r &= ~m;
}

void S300_DMA_SetBase(S300_DMA_ID id, uintptr_t base)
{
    s_dma[(int)id].base = base;
}
uintptr_t S300_DMA_GetBase(S300_DMA_ID id)
{
    return s_dma[(int)id].base;
}

void S300_DMA_EnableClock(S300_DMA_ID id, int enable)
{
    /* RCC sys clk en reg bits: Ahb_dma0 @ bit3, Ahb_dma1 @ bit4 per RCC doc */
    uint32_t bit = (id == S300_DMA0) ? (1u << 3) : (1u << 4);
    if (enable)
    {
        S300_RCC_REG(S300_RCC_OFS_SYS_CLK_EN) |= bit;
        S300_RCC_REG(S300_RCC_OFS_SYS_RST_CTL) |= bit; /* release reset */
    }
    else
    {
        S300_RCC_REG(S300_RCC_OFS_SYS_CLK_EN) &= ~bit;
    }
}

void S300_DMA_GlobalEnable(S300_DMA_ID id, int enable)
{
    volatile uint32_t *cfg = &DMAC_REG(id, DMAC_DmaCfgReg);
    if (enable) set_bits(cfg, 1u);
    else clr_bits(cfg, 1u);
}

void S300_DMA_ClearAllInterrupts(S300_DMA_ID id)
{
    /* 仅清除 pending 标志，不修改掩码配置，避免误屏蔽中断 */
    DMAC_REG(id, DMAC_ClearTfr) = 0xFFu;
    DMAC_REG(id, DMAC_ClearBlock) = 0xFFu;
    DMAC_REG(id, DMAC_ClearSrcTran) = 0xFFu;
    DMAC_REG(id, DMAC_ClearDstTran) = 0xFFu;
    DMAC_REG(id, DMAC_ClearErr) = 0xFFu;
}

/* 解除所有通道、所有类型的中断屏蔽。
     说明：根据 DW DMAC 规范，Mask 寄存器写入格式为 [15:8]=写使能、[7:0]=目标掩码；
                掩码位 1 表示取消屏蔽(UNMASK)，0 表示屏蔽(MASK)。
                因此：
                    - 写 0xFFFF => 同时写使能且将 8 个通道掩码均置 1，即“全部取消屏蔽”。
                    - 写 0xFF00 => 仅写使能且掩码写 0，即“全部屏蔽”。
                本函数使用 0xFFFF 以解除全部屏蔽；旧版 demo 的 0xFF00 含义相反（会屏蔽全部）。 */
void S300_DMA_UnmaskAllInterrupts(S300_DMA_ID id)
{
    /* 上半字(15:8)=写使能，低半字(7:0)=目标掩码；1=unmask(允许)，0=mask(屏蔽)。 */
    DMAC_REG(id, DMAC_MaskTfr)     = 0x0000FFFFu;
    DMAC_REG(id, DMAC_MaskBlock)   = 0x0000FFFFu;
    DMAC_REG(id, DMAC_MaskSrcTran) = 0x0000FFFFu;
    DMAC_REG(id, DMAC_MaskDstTran) = 0x0000FFFFu;
    DMAC_REG(id, DMAC_MaskErr)     = 0x0000FFFFu;
}

static inline uint32_t width_bytes(S300_DMA_TrWidth w)
{
    return (1u << (uint32_t)w);
}

int S300_DMA_ConfigStd(S300_DMA_ID id, S300_DMA_Channel ch,
                       uint32_t src, uint32_t dst, uint32_t bytes,
                       S300_DMA_TrWidth width)
{
    uint32_t ch_bit = 1u << (uint32_t)ch;
    /* Ensure channel disabled using write-enable semantics, then wait (bounded) */
    DMAC_REG(id, DMAC_ChEnReg) = (ch_bit << 8) | 0u; /* disable ch */
    {
        volatile uint32_t spins = 1000000u;
        while ((DMAC_REG(id, DMAC_ChEnReg) & ch_bit) && --spins) { /* wait */ }
    }
    S300_DMA_ClearAllInterrupts(id);
    DMAC_CH_REG(id, ch, DMAC_SAR) = src;
    DMAC_CH_REG(id, ch, DMAC_DAR) = dst;
    /* CTLH: block_ts = transfers-1 */
    uint32_t xfers = bytes / width_bytes(width);
    if (xfers == 0) return -1;
    DMAC_CH_REG(id, ch, DMAC_CTLH) = (xfers & 0xFFFu); /* 12-bit block size */
    /* CTLL fields build */
    uint32_t ctll = 0;
    /* int_en=1 使能该通道的中断，后续由 Mask 寄存器选择具体事件；
        其余：d/src width, inc, burst size=1, type M2M FD */
    ctll |= 1u; /* bit0 int_en */
    ctll |= ((uint32_t)width & 0x7u) << 1;   /* src_tr_width at [3:1], we'll set with same for dst below */
    ctll |= ((uint32_t)width & 0x7u) << 4;   /* dst_tr_width at [6:4] */
    /* dinc/sinc: 0=increment */
    /* dinc at [8:7], sinc at [10:9] */
    /* dest/src msize: [13:11]/[16:14], both 1-beat */
    /* tt_fc at [20:18] */
    /* sms/dms at [23:22]/[21:20] left 0 */
    ctll |= (0u << 7) | (0u << 9);
    ctll |= (S300_DMA_MSIZE_1 << 11) | (S300_DMA_MSIZE_1 << 14);
    ctll |= (S300_DMA_TT_M2M_FD << 20);
    DMAC_CH_REG(id, ch, DMAC_CTLL) = ctll;
    /* CFGH/CFGL: software handshake by default */
    uint32_t cfgh = 0;
    cfgh |= (0u << 0); /* fcmode */
    cfgh |= (0u << 1); /* fifomode */
    cfgh |= (7u << 2); /* protctl */
    DMAC_CH_REG(id, ch, DMAC_CFGH) = cfgh;
    uint32_t cfgl = 0;
    cfgl |= (7u << 5); /* ch_prior */
    cfgl |= (1u << 11); /* hs_sel_dst = 1 software */
    cfgl |= (1u << 10); /* hs_sel_src = 1 software */
    cfgl |= (1u << 19); /* dst_hs_pol = active high */
    cfgl |= (1u << 18); /* src_hs_pol = active high */
    DMAC_CH_REG(id, ch, DMAC_CFGL) = cfgl;
    DMAC_CH_REG(id, ch, DMAC_LLP) = 0u;
    return 0;
}

void S300_DMA_SetIncrements(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_AddrInc src, S300_DMA_AddrInc dst)
{
    uint32_t v = DMAC_CH_REG(id, ch, DMAC_CTLL);
    v &= ~((0x3u << 7) | (0x3u << 9));
    v |= ((uint32_t)dst & 0x3u) << 7;
    v |= ((uint32_t)src & 0x3u) << 9;
    DMAC_CH_REG(id, ch, DMAC_CTLL) = v;
}

void S300_DMA_SetBurst(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_MSize src_msize, S300_DMA_MSize dst_msize)
{
    uint32_t v = DMAC_CH_REG(id, ch, DMAC_CTLL);
    v &= ~((0x7u << 11) | (0x7u << 14));
    v |= ((uint32_t)dst_msize & 0x7u) << 11;
    v |= ((uint32_t)src_msize & 0x7u) << 14;
    DMAC_CH_REG(id, ch, DMAC_CTLL) = v;
}

void S300_DMA_SetWidths(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_TrWidth src_w, S300_DMA_TrWidth dst_w)
{
    uint32_t v = DMAC_CH_REG(id, ch, DMAC_CTLL);
    v &= ~((0x7u << 1) | (0x7u << 4));
    v |= ((uint32_t)src_w & 0x7u) << 1;
    v |= ((uint32_t)dst_w & 0x7u) << 4;
    DMAC_CH_REG(id, ch, DMAC_CTLL) = v;
}

void S300_DMA_SetTransferType(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_TrType type)
{
    uint32_t v = DMAC_CH_REG(id, ch, DMAC_CTLL);
    v &= ~(0x7u << 20);
    v |= ((uint32_t)type & 0x7u) << 20;
    DMAC_CH_REG(id, ch, DMAC_CTLL) = v;
}

void S300_DMA_LLI_Set(S300_DMA_LLI *lli, uint32_t src, uint32_t dst, uint32_t bytes, S300_DMA_TrWidth width)
{
    lli->SAR = src;
    lli->DAR = dst;
    lli->LLP = 0;
    uint32_t xfers = bytes / width_bytes(width);
    lli->CTLH = (xfers & 0xFFFu);
    /* defaults similar to std config */
    uint32_t ctll = 0;
    ctll |= ((uint32_t)width & 0x7u) << 1;
    ctll |= ((uint32_t)width & 0x7u) << 4;
    ctll |= (0u << 7) | (0u << 9);
    ctll |= (S300_DMA_MSIZE_1 << 11) | (S300_DMA_MSIZE_1 << 14);
    ctll |= (S300_DMA_TT_M2M_FD << 20);
    lli->CTLL = ctll;
    lli->DSTAT = 0;
}

int S300_DMA_SetLLI(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_LLI *lli_head)
{
    if (!lli_head) return -1;
    /* Program channel LLP to head */
    DMAC_CH_REG(id, ch, DMAC_LLP) = ((uint32_t)lli_head) & 0xFFFFFFFCu;
    /* Enable src/dst LLP in CTLL */
    uint32_t v = DMAC_CH_REG(id, ch, DMAC_CTLL);
    v |= (1u << 27) | (1u << 26); /* llp_src_en, llp_dst_en */
    DMAC_CH_REG(id, ch, DMAC_CTLL) = v;
    return 0;
}

int S300_DMA_SetHandshake(S300_DMA_ID id, S300_DMA_Channel ch, S300_DMA_Handshake src, S300_DMA_Handshake dst)
{
    /* If DMA1 requires only PDM/AON, enforce policy here if needed. For now only basic checks. */
    uint32_t cfgh = DMAC_CH_REG(id, ch, DMAC_CFGH);
    uint32_t cfgl = DMAC_CH_REG(id, ch, DMAC_CFGL);
    /* external handshake => hs_sel_* = 0, else software */
    /* DW DMAC: CFGL bit11=HS_SEL_SRC, bit10=HS_SEL_DST */
    if (src != S300_DMA_HS_NONE) cfgl &= ~(1u << 11);
    else cfgl |= (1u << 11);
    if (dst != S300_DMA_HS_NONE) cfgl &= ~(1u << 10);
    else cfgl |= (1u << 10);
    /* polarity active high: CFGH bit18=SRC_HS_POL, bit19=DST_HS_POL */
    cfgh |= (1u << 18) | (1u << 19);
    /* peripheral index placement: src_per [10:7], dest_per [14:11] of CFGH */
    cfgh &= ~((0xFu << 7) | (0xFu << 11));
    if (src != S300_DMA_HS_NONE) cfgh |= ((uint32_t)src & 0xFu) << 7;
    if (dst != S300_DMA_HS_NONE) cfgh |= ((uint32_t)dst & 0xFu) << 11;
    DMAC_CH_REG(id, ch, DMAC_CFGL) = cfgl;
    DMAC_CH_REG(id, ch, DMAC_CFGH) = cfgh;
    return 0;
}

void S300_DMA_SetInterrupts(S300_DMA_ID id, S300_DMA_Channel ch, uint32_t types, int enable)
{
    uint32_t ch_bit = 1u << (uint32_t)ch;
    /* DW DMAC 掩码寄存器：写 bit(i+8)=1 以更新 bit(i)；bit(i)=1 表示 Unmask(允许)，bit(i)=0 表示 Mask(屏蔽)。 */
    uint32_t val_unmask = (ch_bit << 8) | ch_bit;   /* 允许：写 1 */
    uint32_t val_mask   = (ch_bit << 8) | 0u;       /* 屏蔽：写 0 */
    uint32_t v = enable ? val_unmask : val_mask;
    /* 仅对调用者指定的类型更新掩码，避免误改其它类型导致语义变化 */
    if (types & S300_DMA_INT_TFR)      DMAC_REG(id, DMAC_MaskTfr)     = v;
    if (types & S300_DMA_INT_BLOCK)    DMAC_REG(id, DMAC_MaskBlock)   = v;
    if (types & S300_DMA_INT_SRCTRAN)  DMAC_REG(id, DMAC_MaskSrcTran) = v;
    if (types & S300_DMA_INT_DSTTRAN)  DMAC_REG(id, DMAC_MaskDstTran) = v;
    if (types & S300_DMA_INT_ERR)      DMAC_REG(id, DMAC_MaskErr)     = v;
}

void S300_DMA_Start(S300_DMA_ID id, S300_DMA_Channel ch)
{
    uint32_t ch_bit = 1u << (uint32_t)ch;
    /* Global enable should be ensured by caller once; keep idempotent here */
    S300_DMA_GlobalEnable(id, 1);
    /* Channel enable: write (ch<<8 | ch) */
    DMAC_REG(id, DMAC_ChEnReg) |= (ch_bit << 8) | ch_bit;
}

void S300_DMA_Stop(S300_DMA_ID id, S300_DMA_Channel ch)
{
    uint32_t ch_bit = 1u << (uint32_t)ch;
    /* Proper disable: write-enable bit with value 0 for the channel */
    DMAC_REG(id, DMAC_ChEnReg) = (ch_bit << 8) | 0u;
}

int S300_DMA_IsBusy(S300_DMA_ID id, S300_DMA_Channel ch)
{
    uint32_t ch_bit = 1u << (uint32_t)ch;
    return (DMAC_REG(id, DMAC_ChEnReg) & ch_bit) ? 1 : 0;
}

uint32_t S300_DMA_GetStatusTfr(S300_DMA_ID id)
{
    return DMAC_REG(id, DMAC_StatusTfr);
}
uint32_t S300_DMA_GetStatusBlock(S300_DMA_ID id)
{
    return DMAC_REG(id, DMAC_StatusBlock);
}
uint32_t S300_DMA_GetStatusInt(S300_DMA_ID id)
{
    return DMAC_REG(id, DMAC_StatusInt);
}

void S300_DMA_ClearPending(S300_DMA_ID id, uint8_t chMask, uint32_t types)
{
    uint32_t m = (uint32_t)chMask & 0xFFu;
    if (types & S300_DMA_INT_TFR)     DMAC_REG(id, DMAC_ClearTfr)     = m;
    if (types & S300_DMA_INT_BLOCK)   DMAC_REG(id, DMAC_ClearBlock)   = m;
    if (types & S300_DMA_INT_SRCTRAN) DMAC_REG(id, DMAC_ClearSrcTran) = m;
    if (types & S300_DMA_INT_DSTTRAN) DMAC_REG(id, DMAC_ClearDstTran) = m;
    if (types & S300_DMA_INT_ERR)     DMAC_REG(id, DMAC_ClearErr)     = m;
}
