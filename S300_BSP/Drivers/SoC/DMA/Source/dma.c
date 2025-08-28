#include "dma.h"

static inline S300_DMA_TypeDef *dma_get(dma_idx_t d)
{
    return (d == DMA_IDX0) ? DMAC0 : DMAC1;
}

int dma_init(dma_idx_t d)
{
    S300_DMA_TypeDef *D = dma_get(d);
    /* Enable controller */
    D->DmaCfgReg = 0u; /* keep disabled until start */
    /* Mask all interrupts for safety (legacy used 0xFF00 to mask channels) */
    D->MaskTfr = 0xFF00u;
    D->MaskBlock = 0xFF00u;
    D->MaskSrcTran = 0xFF00u;
    D->MaskDstTran = 0xFF00u;
    D->MaskErr = 0xFF00u;
    /* Clear any pending */
    D->ClearTfr = 0xFFu;
    D->ClearBlock = 0xFFu;
    D->ClearSrcTran = 0xFFu;
    D->ClearDstTran = 0xFFu;
    D->ClearErr = 0xFFu;
    return 0;
}

static inline void ch_disable(S300_DMA_TypeDef *D, uint8_t ch)
{
    /* ChEnReg bits: [7:0]=CH_EN, [15:8]=CH_EN_WE */
    uint32_t we = (1u << ch) << 8;
    D->ChEnReg = we | 0u;
}
static inline void ch_enable(S300_DMA_TypeDef *D, uint8_t ch)
{
    uint32_t we = (1u << ch) << 8;
    D->ChEnReg = we | (1u << ch);
}

int dma_set_std(dma_idx_t d, uint8_t ch, uint32_t src, uint32_t dst, uint32_t len, dma_width_t width)
{
    if (ch >= 8) return -1;
    S300_DMA_TypeDef *D = dma_get(d);
    S300_DMA_Channel_TypeDef *C = &D->CH[ch];
    ch_disable(D, ch);
    C->SAR = src;
    C->DAR = dst;
    uint32_t tr_bytes = (width == DMA_WIDTH_32) ? 4u : (width == DMA_WIDTH_16 ? 2u : 1u);
    uint32_t transfers = (tr_bytes == 0u) ? 0u : (len / tr_bytes);
    C->CTL_H = (transfers & 0xFFFu);
    uint32_t ctl = 0u;
    ctl |= (1u << DMA_CTL_INT_EN_Pos); /* enable block/transfer interrupt generation */
    ctl |= ((uint32_t)width << DMA_CTL_DST_TRW_Pos);
    ctl |= ((uint32_t)width << DMA_CTL_SRC_TRW_Pos);
    ctl |= ((uint32_t)DMA_ADDR_INC << DMA_CTL_DINC_Pos);
    ctl |= ((uint32_t)DMA_ADDR_INC << DMA_CTL_SINC_Pos);
    /* 参考 demo 使用最小突发（1x），这样 CTL_L 中不包含 0x9000 差异 */
    ctl |= ((uint32_t)DMA_MSIZE_1 << DMA_CTL_DST_MSIZE_Pos);
    ctl |= ((uint32_t)DMA_MSIZE_1 << DMA_CTL_SRC_MSIZE_Pos);
    ctl |= ((uint32_t)DMA_TR_TYPE_M2M_FD << DMA_CTL_TT_FC_Pos);
    C->CTL_L = ctl;
    uint32_t cfg_l = 0u;
    cfg_l |= (7u << DMA_CFG_CH_PRIOR_Pos);
    /* 按参考 demo，默认不设置 MAX_ABRST（保持 0）且保持 HS_SEL/SRC/DST 极性为 0 */
    cfg_l &= ~((1u << DMA_CFGL_HS_SEL_DST_Pos) | (1u << DMA_CFGL_HS_SEL_SRC_Pos));
    cfg_l &= ~((1u << DMA_CFGL_DST_HS_POL_Pos) | (1u << DMA_CFGL_SRC_HS_POL_Pos));
    C->CFG_L = cfg_l;
    uint32_t cfg_h = 0u;
    cfg_h |= (0u << DMA_CFGH_FCMODE_Pos);
    cfg_h |= (0u << DMA_CFGH_FIFOMODE_Pos);
    cfg_h |= (7u << DMA_CFGH_PROTCTL_Pos);
    C->CFG_H = cfg_h;
    return 0;
}

void dma_set_link_unit(dma_lli_t *lli, uint32_t src, uint32_t dst, uint32_t len, dma_width_t width)
{
    uint32_t tr_bytes = (width == DMA_WIDTH_32) ? 4u : (width == DMA_WIDTH_16 ? 2u : 1u);
    uint32_t transfers = (tr_bytes == 0u) ? 0u : (len / tr_bytes);
    lli->SAR = src;
    lli->DAR = dst;
    lli->LLP = 0u;
    lli->CTL_H = (transfers & 0xFFFu);
    lli->CTL_L = (1u << DMA_CTL_INT_EN_Pos)
                 | ((uint32_t)width << DMA_CTL_DST_TRW_Pos)
                 | ((uint32_t)width << DMA_CTL_SRC_TRW_Pos)
                 | ((uint32_t)DMA_ADDR_INC << DMA_CTL_DINC_Pos)
                 | ((uint32_t)DMA_ADDR_INC << DMA_CTL_SINC_Pos)
                 | ((uint32_t)DMA_MSIZE_8 << DMA_CTL_DST_MSIZE_Pos)
                 | ((uint32_t)DMA_MSIZE_8 << DMA_CTL_SRC_MSIZE_Pos)
                 | ((uint32_t)DMA_TR_TYPE_M2M_FD << DMA_CTL_TT_FC_Pos);
}

int dma_set_link(dma_idx_t d, uint8_t ch, dma_lli_t *lli)
{
    if (ch >= 8) return -1;
    S300_DMA_TypeDef *D = dma_get(d);
    S300_DMA_Channel_TypeDef *C = &D->CH[ch];
    ch_disable(D, ch);
    C->LLP = (uint32_t)lli;
    C->CTL_L |= (1u << DMA_CTL_LLP_DST_EN_Pos) | (1u << DMA_CTL_LLP_SRC_EN_Pos);
    return 0;
}

void dma_start(dma_idx_t d, uint8_t ch)
{
    S300_DMA_TypeDef *D = dma_get(d);
    D->DmaCfgReg |= 1u; /* global enable */
    ch_enable(D, ch);
}

void dma_set_transfer_type(dma_idx_t d, uint8_t ch, dma_tr_type_t type)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CTL_L;
    v &= ~(7u << DMA_CTL_TT_FC_Pos);
    v |= ((uint32_t)type << DMA_CTL_TT_FC_Pos);
    C->CTL_L = v;
}

int dma_set_handshaking(dma_idx_t d, uint8_t ch, uint16_t src_hs, uint16_t dst_hs)
{
    if (ch >= 8) return -1;
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CFG_H;
    v &= ~((0xFu << DMA_CFGH_SRC_PER_Pos) | (0xFu << DMA_CFGH_DST_PER_Pos));
    /* EM_HAND_NULL(0xFFFF) 代表不使用外设握手，需写 0 到寄存器字段 */
    uint32_t src_val = (src_hs == 0xFFFFu) ? 0u : ((uint32_t)src_hs & 0xFu);
    uint32_t dst_val = (dst_hs == 0xFFFFu) ? 0u : ((uint32_t)dst_hs & 0xFu);
    v |= (src_val << DMA_CFGH_SRC_PER_Pos);
    v |= (dst_val << DMA_CFGH_DST_PER_Pos);
    C->CFG_H = v;
    return 0;
}

void dma_set_increment(dma_idx_t d, uint8_t ch, dma_inc_t s_inc, dma_inc_t d_inc)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CTL_L;
    v &= ~((3u << DMA_CTL_SINC_Pos) | (3u << DMA_CTL_DINC_Pos));
    v |= ((uint32_t)s_inc << DMA_CTL_SINC_Pos) | ((uint32_t)d_inc << DMA_CTL_DINC_Pos);
    C->CTL_L = v;
}

void dma_set_burst(dma_idx_t d, uint8_t ch, dma_msize_t s_msize, dma_msize_t d_msize)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CTL_L;
    v &= ~((7u << DMA_CTL_SRC_MSIZE_Pos) | (7u << DMA_CTL_DST_MSIZE_Pos));
    v |= ((uint32_t)s_msize << DMA_CTL_SRC_MSIZE_Pos) | ((uint32_t)d_msize << DMA_CTL_DST_MSIZE_Pos);
    C->CTL_L = v;
}

void dma_set_width(dma_idx_t d, uint8_t ch, dma_width_t s_w, dma_width_t d_w)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CTL_L;
    v &= ~((7u << DMA_CTL_SRC_TRW_Pos) | (7u << DMA_CTL_DST_TRW_Pos));
    v |= ((uint32_t)s_w << DMA_CTL_SRC_TRW_Pos) | ((uint32_t)d_w << DMA_CTL_DST_TRW_Pos);
    C->CTL_L = v;
}

void dma_set_interrupt(dma_idx_t d, uint8_t ch, dma_int_t type, bool en)
{
    /* Mask registers use write-enable in [15:8]; write desired value in [7:0] */
    S300_DMA_TypeDef *D = dma_get(d);
    uint32_t we = (1u << ch) << 8;
    uint32_t val = en ? (1u << ch) : 0u;
    if (type & DMA_INT_TFR)     D->MaskTfr    = we | val;
    if (type & DMA_INT_BLOCK)   D->MaskBlock  = we | val;
    if (type & DMA_INT_SRCTRAN) D->MaskSrcTran = we | val;
    if (type & DMA_INT_DSTTRAN) D->MaskDstTran = we | val;
}

bool dma_is_busy(dma_idx_t d, uint8_t ch)
{
    S300_DMA_TypeDef *D = dma_get(d);
    uint32_t enabled = (D->ChEnReg & (1u << ch));
    return enabled != 0u;
}

void dma_set_reload(dma_idx_t d, uint8_t ch, bool s_reload, bool d_reload)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    uint32_t v = C->CFG_L;
    if (s_reload) v |= (1u << DMA_CFG_RELOAD_SRC_Pos);
    else v &= ~(1u << DMA_CFG_RELOAD_SRC_Pos);
    if (d_reload) v |= (1u << DMA_CFG_RELOAD_DST_Pos);
    else v &= ~(1u << DMA_CFG_RELOAD_DST_Pos);
    C->CFG_L = v;
}

void dma_stop(dma_idx_t d, uint8_t ch)
{
    ch_disable(dma_get(d), ch);
}

void dma_set_address(dma_idx_t d, uint8_t ch, uint32_t src, uint32_t dst)
{
    S300_DMA_Channel_TypeDef *C = &dma_get(d)->CH[ch];
    C->SAR = src;
    C->DAR = dst;
}

