/*
 * @Author       : panxinhao
 * @Date         : 2023-03-06 18:33:49
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-04 20:20:45
 * @FilePath     : \\testbench_base\\cortexm4_timer\\Libraries\\NE004xx_Driver\\Source\\riscv_dma.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include "riscv_dma.h"
int riscv_dma_enable(uint64_t dmax)
{
    DMA_CFG(dmax) = DMA_CFG_INT_EN | DMA_CFG_EN;
    return 0;
}
int riscv_dma_init(uint64_t dmax, uint64_t chx, size_t data_size, size_t src_tr_width, size_t dst_tr_width)
{
    size_t src_result = 1, dst_result = 1, src_width = 0, dst_width = 0;
    while (src_result != src_tr_width)
    {
        src_result *= 2;
        src_width++;
    }
    while (dst_result != dst_tr_width)
    {
        dst_result *= 2;
        dst_width++;
    }
    DMA_CHX_BLOCK_TS(dmax, chx) = DMA_CHX_BLOCK_TS_MASK & (data_size / src_tr_width - 1);
    DMA_CHX_CTL(dmax, chx) = DMA_CHX_CTL_DST_STAT_EN
                             | DMA_CHX_CTL_SRC_STAT_EN
                             | (DMA_CHX_CTL_AWLEN & (0x0FULL << 48))
                             | DMA_CHX_CTL_AWLEN_EN
                             | (DMA_CHX_CTL_ARLEN & (0x0FULL << 39))
                             | DMA_CHX_CTL_ARLEN_EN
                             | (DMA_CHX_CTL_AW_CACHE & (0x02ULL << 26))
                             | (DMA_CHX_CTL_AR_CACHE & (0x02ULL << 22))
                             | (DMA_CHX_CTL_DST_MSIZE & (0x02ULL << 18))
                             | (DMA_CHX_CTL_SRC_MSIZE & (0x02ULL << 14))
                             | (DMA_CHX_CTL_DST_TR_WIDTH & (dst_width << 11))
                             | (DMA_CHX_CTL_SRC_TR_WIDTH & (src_width << 8));
    DMA_CHX_CFG(dmax, chx) = (DMA_CHX_CFG_CH_PRIOR & (0x03ULL << 49))
                             | DMA_CHX_CFG_HS_SEL_DST
                             | DMA_CHX_CFG_HS_SEL_SRC;
    DMA_CHX_INTSTATUS_ENABLE(dmax, chx) = 0x0ULL;
    DMA_CHX_INTSIGNAL_ENABLE(dmax, chx) = 0x0ULL;
    return 0;
}

int riscv_dma_interrupt_enable(uint64_t dmax, uint64_t chx)
{
    DMA_CHX_INTSTATUS_ENABLE(dmax, chx) = 0xFFFFFFFFFFFFFFFFULL;
    DMA_CHX_INTSIGNAL_ENABLE(dmax, chx) = 0xFFFFFFFFFFFFFFFFULL;
    return 0;
}

int riscv_dma_interrupt_disable(uint64_t dmax, uint64_t chx)
{
    DMA_CHX_INTSTATUS_ENABLE(dmax, chx) = 0x0ULL;
    DMA_CHX_INTSIGNAL_ENABLE(dmax, chx) = 0x0ULL;
    return 0;
}

int riscv_dma_start(uint64_t dmax, uint64_t chx, uint64_t src_addr, uint64_t dst_addr)
{
    DMA_CHX_SAR(dmax, chx) = (uint64_t)src_addr;
    DMA_CHX_DAR(dmax, chx) = (uint64_t)dst_addr;
    DMA_CHEN(dmax) = (DMA_CHEN_CH_EN_WE & ((0x01ULL << (chx - 1)) << 8))
                      | (DMA_CHEN_CH_EN & ((0x01ULL << (chx - 1)) << 0));
    return 0;
}


