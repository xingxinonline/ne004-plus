/*
 * @Author       : panxinhao
 * @Date         : 2023-03-06 18:33:49
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-26 18:25:41
 * @FilePath     : \\testbench_base\\riscv64_default\\dma.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include "dma.h"
int dma_enable(void)
{
    DMA_CFG(DMA0) = DMA_CFG_INT_EN | DMA_CFG_EN;
    return 0;
}
int dma_init(void)
{
    /* DMA Channel 1 */
    DMA_CHX_BLOCK_TS(DMA0, DMA_CH1) = DMA_CHX_BLOCK_TS_MASK & 7;
    DMA_CHX_CTL(DMA0, DMA_CH1) = DMA_CHX_CTL_DST_STAT_EN
                                 | DMA_CHX_CTL_SRC_STAT_EN
                                 | (DMA_CHX_CTL_AWLEN & (0x03ULL << 48))
                                 | DMA_CHX_CTL_AWLEN_EN
                                 | (DMA_CHX_CTL_ARLEN & (0x03ULL << 39))
                                 | DMA_CHX_CTL_ARLEN_EN
                                 | (DMA_CHX_CTL_DST_MSIZE & (0x01ULL << 18))
                                 | (DMA_CHX_CTL_SRC_MSIZE & (0x01ULL << 14))
                                 | (DMA_CHX_CTL_DST_TR_WIDTH & (0x05ULL << 11))
                                 | (DMA_CHX_CTL_SRC_TR_WIDTH & (0x05ULL << 8));
    DMA_CHX_CFG(DMA0, DMA_CH1) = (DMA_CHX_CFG_CH_PRIOR & (0x04ULL << 49))
                                 | DMA_CHX_CFG_HS_SEL_DST
                                 | DMA_CHX_CFG_HS_SEL_SRC;
    DMA_CHX_INTSTATUS_ENABLE(DMA0, DMA_CH1) = 0x0ULL;
    DMA_CHX_INTSIGNAL_ENABLE(DMA0, DMA_CH1) = 0x0ULL;
    /* DMA Channel 2 */
    DMA_CHX_BLOCK_TS(DMA0, DMA_CH2) = DMA_CHX_BLOCK_TS_MASK & 7;
    DMA_CHX_CTL(DMA0, DMA_CH2) = DMA_CHX_CTL_DST_STAT_EN
                                 | DMA_CHX_CTL_SRC_STAT_EN
                                 | (DMA_CHX_CTL_AWLEN & (0x03ULL << 48))
                                 | DMA_CHX_CTL_AWLEN_EN
                                 | (DMA_CHX_CTL_ARLEN & (0x03ULL << 39))
                                 | DMA_CHX_CTL_ARLEN_EN
                                 | (DMA_CHX_CTL_DST_MSIZE & (0x01ULL << 18))
                                 | (DMA_CHX_CTL_SRC_MSIZE & (0x01ULL << 14))
                                 | (DMA_CHX_CTL_DST_TR_WIDTH & (0x05ULL << 11))
                                 | (DMA_CHX_CTL_SRC_TR_WIDTH & (0x05ULL << 8));
    DMA_CHX_CFG(DMA0, DMA_CH2) = (DMA_CHX_CFG_CH_PRIOR & (0x07ULL << 49))
                                 | DMA_CHX_CFG_HS_SEL_DST
                                 | DMA_CHX_CFG_HS_SEL_SRC;
    DMA_CHX_INTSTATUS_ENABLE(DMA0, DMA_CH2) = 0x0ULL;
    DMA_CHX_INTSIGNAL_ENABLE(DMA0, DMA_CH2) = 0x0ULL;
    return 0;
}

int dma_start_weight(uint64_t src_addr)
{
    DMA_CHX_SAR(DMA0, DMA_CH1) = (uint64_t)src_addr;
    DMA_CHX_DAR(DMA0, DMA_CH1) = (uint64_t)0x60200000ULL;
    DMA_CHEN(DMA0) = (DMA_CHEN_CH_EN_WE & ((0x01ULL << (DMA_CH1 - 1)) << 8))
                     | (DMA_CHEN_CH_EN & ((0x01ULL << (DMA_CH1 - 1)) << 0));
    return 0;
}

int dma_start_feature(uint64_t src_addr, uint64_t dst_addr)
{
    DMA_CHX_SAR(DMA0, DMA_CH1) = (uint64_t)src_addr;
    DMA_CHX_DAR(DMA0, DMA_CH1) = (uint64_t)0x60240000;
    DMA_CHX_SAR(DMA0, DMA_CH2) = (uint64_t)0x60280000;
    DMA_CHX_DAR(DMA0, DMA_CH2) = (uint64_t)dst_addr;
    DMA_CHEN(DMA0) = (DMA_CHEN_CH_EN_WE & ((0x01ULL << (DMA_CH1 - 1)) << 8))
                     | (DMA_CHEN_CH_EN & ((0x01ULL << (DMA_CH1 - 1)) << 0))
                     | (DMA_CHEN_CH_EN_WE & ((0x01ULL << (DMA_CH2 - 1)) << 8))
                     | (DMA_CHEN_CH_EN & ((0x01ULL << (DMA_CH2 - 1)) << 0));
    return 0;
}


