/*** 
 * @Author       : panxinhao
 * @Date         : 2023-07-04 15:53:02
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-08-04 19:38:04
 * @FilePath     : \\testbench_base\\cortexm4_timer\\Libraries\\NE004xx_Driver\\Include\\riscv_dma.h
 * @Description  : 
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef _RISCV_DMA_H_
#define _RISCVDMA_H_

#include "stdint.h"
#include "stdio.h"

#define RISCV_REG64(addr)     (*(volatile uint64_t *)(uint32_t)(addr))
#define RISCV_REG32(addr)     (*(volatile uint32_t *)(uint32_t)(addr))
#define RISCV_REG16(addr)     (*(volatile uint16_t *)(uint32_t)(addr))
#define RISCV_REG8(addr)      (*(volatile uint8_t  *)(uint32_t)(addr))

#define RISCV_BIT(x)          ((unsigned long long)((unsigned long long)0x01ULL << (x)))
#define RISCV_BITS(start, end) ((0xFFFFFFFFFFFFFFFFUL << (start)) & (0xFFFFFFFFFFFFFFFFUL >> (63UL - (uint64_t)(end))))
#define RISCV_GET_BITS(regval, start, end) (((regval)&BITS((start), (end))) >> (start))

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

#define DMA_ID(dmax)                            RISCV_REG64((dmax) + 0x000ULL)
#define DMA_COMPVER(dmax)                       RISCV_REG64((dmax) + 0x008ULL)
#define DMA_CFG(dmax)                           RISCV_REG64((dmax) + 0x010ULL)
#define DMA_CHEN(dmax)                          RISCV_REG64((dmax) + 0x018ULL)
#define DMA_INTSTATUS(dmax)                     RISCV_REG64((dmax) + 0x030ULL)
#define DMA_COMMON_INTCLEAR(dmax)               RISCV_REG64((dmax) + 0x038ULL)
#define DMA_COMMON_INTSTATUS_ENABLE(dmax)       RISCV_REG64((dmax) + 0x040ULL)
#define DMA_COMMON_INTSIGNAL_ENABLE(dmax)       RISCV_REG64((dmax) + 0x058ULL)
#define DMA_COMMON_INTSTATUS(dmax)              RISCV_REG64((dmax) + 0x050ULL)
#define DMA_RESET(dmax)                         RISCV_REG64((dmax) + 0x058ULL)
#define DMA_CHX_SAR(dmax, chn)                  RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x000ULL)
#define DMA_CHX_DAR(dmax, chn)                  RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x008ULL)
#define DMA_CHX_BLOCK_TS(dmax, chn)             RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x010ULL)
#define DMA_CHX_CTL(dmax, chn)                  RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x018ULL)
#define DMA_CHX_CFG(dmax, chn)                  RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x020ULL)
#define DMA_CHX_LLP(dmax, chn)                  RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x028ULL)
#define DMA_CHX_STATUS(dmax, chn)               RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x030ULL)
#define DMA_CHX_SWHSSRC(dmax, chn)              RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x038ULL)
#define DMA_CHX_SWHSDST(dmax, chn)              RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x040ULL)
#define DMA_CHX_BLK_TFR_RESUMEREQ(dmax, chn)    RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x048ULL)
#define DMA_CHX_AXI_ID(dmax, chn)               RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x050ULL)
#define DMA_CHX_AXI_QOS(dmax, chn)              RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x058ULL)
#define DMA_CHX_SSTAT(dmax, chn)                RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x060ULL)
#define DMA_CHX_DSTAT(dmax, chn)                RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x068ULL)
#define DMA_CHX_SSTATAR(dmax, chn)              RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x070ULL)
#define DMA_CHX_DSTATAR(dmax, chn)              RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x078ULL)
#define DMA_CHX_INTSTATUS_ENABLE(dmax, chn)     RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x080ULL)
#define DMA_CHX_INTSTATUSREG(dmax, chn)         RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x088ULL)
#define DMA_CHX_INTSIGNAL_ENABLE(dmax, chn)     RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x090ULL)
#define DMA_CHX_INTCLEAR(dmax, chn)             RISCV_REG64((dmax) + (0x100ULL * (chn)) + 0x098ULL)

#define DMA_ID_MASK                             RISCV_BITS(0, 63)

#define DMA_COMPVER_MASK                        RISCV_BITS(0, 31)

#define DMA_CFG_INT_EN                          RISCV_BIT(1)
#define DMA_CFG_EN                              RISCV_BIT(0)

#define DMA_CHEN_CH_EN_WE                       RISCV_BITS(8, 15)
#define DMA_CHEN_CH_EN                          RISCV_BITS(0, 7)

#define DMA_CHX_BLOCK_TS_MASK                   RISCV_BITS(0, 21)

#define DMA_CHX_CTL_LLI_VAID                    RISCV_BIT(63)
#define DMA_CHX_CTL_LLI_LAST                    RISCV_BIT(62)
#define DMA_CHX_CTL_DST_STAT_EN                 RISCV_BIT(57)
#define DMA_CHX_CTL_SRC_STAT_EN                 RISCV_BIT(56)
#define DMA_CHX_CTL_AWLEN                       RISCV_BITS(48, 55)
#define DMA_CHX_CTL_AWLEN_EN                    RISCV_BIT(47)
#define DMA_CHX_CTL_ARLEN                       RISCV_BITS(39, 46)
#define DMA_CHX_CTL_ARLEN_EN                    RISCV_BIT(38)
#define DMA_CHX_CTL_AW_CACHE                    RISCV_BITS(26, 29)
#define DMA_CHX_CTL_AR_CACHE                    RISCV_BITS(22, 25)
#define DMA_CHX_CTL_DST_MSIZE                   RISCV_BITS(18, 21)
#define DMA_CHX_CTL_SRC_MSIZE                   RISCV_BITS(14, 17)
#define DMA_CHX_CTL_DST_TR_WIDTH                RISCV_BITS(11, 13)
#define DMA_CHX_CTL_SRC_TR_WIDTH                RISCV_BITS(8, 10)

#define DMA_CHX_CFG_CH_PRIOR                    RISCV_BITS(49, 51)
#define DMA_CHX_CFG_HS_SEL_DST                  RISCV_BIT(36)
#define DMA_CHX_CFG_HS_SEL_SRC                  RISCV_BIT(35)
#define DMA_CHX_CFG_TT_FC                       RISCV_BITS(32, 34)
#define DMA_CHX_CFG_DST_MULTBLK_TYPE            RISCV_BITS(2, 3)
#define DMA_CHX_CFG_SRC_MULTBLK_TYPE            RISCV_BITS(0, 1)

#define DMA_CHX_INTSTATUS_DMA_TRF_DONE          RISCV_BIT(1)
#define DMA_CHX_INTSTATUS_BLOCK_TRF_DONE        RISCV_BIT(0)

int riscv_dma_enable(uint64_t dmax);
int riscv_dma_init(uint64_t dmax, uint64_t chx, size_t data_size, size_t src_tr_width, size_t dst_tr_width);
int riscv_dma_interrupt_enable(uint64_t dmax, uint64_t chx);
int riscv_dma_interrupt_disable(uint64_t dmax, uint64_t chx);
int riscv_dma_start(uint64_t dmax, uint64_t chx, uint64_t src_addr, uint64_t dst_addr);
#endif // !_DMA_H_
