/*** 
 * @Author       : panxinhao
 * @Date         : 2023-07-04 15:53:02
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-25 16:58:51
 * @FilePath     : \\testbench_base\\riscv64_default\\dma.h
 * @Description  : 
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef _DMA_H_
#define _DMA_H_

#include "reg.h"
#include "stdio.h"

#define DMA_BASE                                0x62400000ULL
#define DMA0                                    (DMA_BASE)                  // device At 0x62400000
#define DMA1                                    (DMA_BASE + 0x100000ULL)    // device At 0x62500000

#define DMA_CH1                                 (0x0001ULL)
#define DMA_CH2                                 (0x0002ULL)
#define DMA_CH3                                 (0x0003ULL)
#define DMA_CH4                                 (0x0004ULL)
#define DMA_CH5                                 (0x0005ULL)
#define DMA_CH6                                 (0x0006ULL)
#define DMA_CH7                                 (0x0007ULL)
#define DMA_CH8                                 (0x0008ULL)


#define DMA_ID(dmax)                            REG64((dmax) + 0x000ULL)
#define DMA_COMPVER(dmax)                       REG64((dmax) + 0x008ULL)
#define DMA_CFG(dmax)                           REG64((dmax) + 0x010ULL)
#define DMA_CHEN(dmax)                          REG64((dmax) + 0x018ULL)
#define DMA_INTSTATUS(dmax)                     REG64((dmax) + 0x030ULL)
#define DMA_COMMON_INTCLEAR(dmax)               REG64((dmax) + 0x038ULL)
#define DMA_COMMON_INTSTATUS_ENABLE(dmax)       REG64((dmax) + 0x040ULL)
#define DMA_COMMON_INTSIGNAL_ENABLE(dmax)       REG64((dmax) + 0x058ULL)
#define DMA_COMMON_INTSTATUS(dmax)              REG64((dmax) + 0x050ULL)
#define DMA_RESET(dmax)                         REG64((dmax) + 0x058ULL)
#define DMA_CHX_SAR(dmax, chn)                  REG64((dmax) + (0x100ULL * (chn)) + 0x000ULL)
#define DMA_CHX_DAR(dmax, chn)                  REG64((dmax) + (0x100ULL * (chn)) + 0x008ULL)
#define DMA_CHX_BLOCK_TS(dmax, chn)             REG64((dmax) + (0x100ULL * (chn)) + 0x010ULL)
#define DMA_CHX_CTL(dmax, chn)                  REG64((dmax) + (0x100ULL * (chn)) + 0x018ULL)
#define DMA_CHX_CFG(dmax, chn)                  REG64((dmax) + (0x100ULL * (chn)) + 0x020ULL)
#define DMA_CHX_LLP(dmax, chn)                  REG64((dmax) + (0x100ULL * (chn)) + 0x028ULL)
#define DMA_CHX_STATUS(dmax, chn)               REG64((dmax) + (0x100ULL * (chn)) + 0x030ULL)
#define DMA_CHX_SWHSSRC(dmax, chn)              REG64((dmax) + (0x100ULL * (chn)) + 0x038ULL)
#define DMA_CHX_SWHSDST(dmax, chn)              REG64((dmax) + (0x100ULL * (chn)) + 0x040ULL)
#define DMA_CHX_BLK_TFR_RESUMEREQ(dmax, chn)    REG64((dmax) + (0x100ULL * (chn)) + 0x048ULL)
#define DMA_CHX_AXI_ID(dmax, chn)               REG64((dmax) + (0x100ULL * (chn)) + 0x050ULL)
#define DMA_CHX_AXI_QOS(dmax, chn)              REG64((dmax) + (0x100ULL * (chn)) + 0x058ULL)
#define DMA_CHX_SSTAT(dmax, chn)                REG64((dmax) + (0x100ULL * (chn)) + 0x060ULL)
#define DMA_CHX_DSTAT(dmax, chn)                REG64((dmax) + (0x100ULL * (chn)) + 0x068ULL)
#define DMA_CHX_SSTATAR(dmax, chn)              REG64((dmax) + (0x100ULL * (chn)) + 0x070ULL)
#define DMA_CHX_DSTATAR(dmax, chn)              REG64((dmax) + (0x100ULL * (chn)) + 0x078ULL)
#define DMA_CHX_INTSTATUS_ENABLE(dmax, chn)     REG64((dmax) + (0x100ULL * (chn)) + 0x080ULL)
#define DMA_CHX_INTSTATUSREG(dmax, chn)         REG64((dmax) + (0x100ULL * (chn)) + 0x088ULL)
#define DMA_CHX_INTSIGNAL_ENABLE(dmax, chn)     REG64((dmax) + (0x100ULL * (chn)) + 0x090ULL)
#define DMA_CHX_INTCLEAR(dmax, chn)             REG64((dmax) + (0x100ULL * (chn)) + 0x098ULL)

#define DMA_ID_MASK                             BITS(0, 63)

#define DMA_COMPVER_MASK                        BITS(0, 31)

#define DMA_CFG_INT_EN                          BIT(1)
#define DMA_CFG_EN                              BIT(0)

#define DMA_CHEN_CH_EN_WE                       BITS(8, 15)
#define DMA_CHEN_CH_EN                          BITS(0, 7)

#define DMA_CHX_BLOCK_TS_MASK                   BITS(0, 21)

#define DMA_CHX_CTL_LLI_VAID                    BIT(63)
#define DMA_CHX_CTL_LLI_LAST                    BIT(62)
#define DMA_CHX_CTL_DST_STAT_EN                 BIT(57)
#define DMA_CHX_CTL_SRC_STAT_EN                 BIT(56)
#define DMA_CHX_CTL_AWLEN                       BITS(48, 55)
#define DMA_CHX_CTL_AWLEN_EN                    BIT(47)
#define DMA_CHX_CTL_ARLEN                       BITS(39, 46)
#define DMA_CHX_CTL_ARLEN_EN                    BIT(38)
#define DMA_CHX_CTL_AW_CACHE                    BITS(26, 29)
#define DMA_CHX_CTL_AR_CACHE                    BITS(22, 25)
#define DMA_CHX_CTL_DST_MSIZE                   BITS(18, 21)
#define DMA_CHX_CTL_SRC_MSIZE                   BITS(14, 17)
#define DMA_CHX_CTL_DST_TR_WIDTH                BITS(11, 13)
#define DMA_CHX_CTL_SRC_TR_WIDTH                BITS(8, 10)

#define DMA_CHX_CFG_CH_PRIOR                    BITS(49, 51)
#define DMA_CHX_CFG_HS_SEL_DST                  BIT(36)
#define DMA_CHX_CFG_HS_SEL_SRC                  BIT(35)
#define DMA_CHX_CFG_TT_FC                       BITS(32, 34)
#define DMA_CHX_CFG_DST_MULTBLK_TYPE            BITS(2, 3)
#define DMA_CHX_CFG_SRC_MULTBLK_TYPE            BITS(0, 1)


#define DMA_CHX_INTSTATUS_DMA_TRF_DONE          BIT(1)
#define DMA_CHX_INTSTATUS_BLOCK_TRF_DONE        BIT(0)

int dma_enable(void);
int dma_init(void);
int dma_start_weight(uint64_t src_addr);
int dma_start_feature(uint64_t src_addr, uint64_t dst_addr);

#endif // !_DMA_H_
