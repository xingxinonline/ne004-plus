/***
 * @Author       : panxinhao
 * @Date         : 2023-06-09 15:52:48
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-06-13 16:12:27
 * @FilePath     : \\ne004-plus\\riscv64_kws\\reg.h
 * @Description  :
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All rights reserved.
 */

#ifndef _REG_H_
#define _REG_H_

#include <stdint.h>

/* bit operations */
#define REG64(addr)     (*(volatile uint64_t *)(uint64_t)(addr))
#define REG32(addr)     (*(volatile uint32_t *)(uint64_t)(addr))
#define REG16(addr)     (*(volatile uint16_t *)(uint64_t)(addr))
#define REG8(addr)      (*(volatile uint8_t  *)(uint64_t)(addr))

#define BIT(x)          ((unsigned long long)((unsigned long long)0x01ULL << (x)))
#define BITS(start, end) ((0xFFFFFFFFFFFFFFFFUL << (start)) & (0xFFFFFFFFFFFFFFFFUL >> (63UL - (uint64_t)(end))))
#define GET_BITS(regval, start, end) (((regval)&BITS((start), (end))) >> (start))

// boot reg
#define PRO_ENDIAN (1 << 2) // 0: Little-Endian; 1  Big-Endian

#define IPCM_ADDR REG32(0x62200004U)
#define NNU_ADDR REG32(0x62600000U)

#define ARM_EN_RISCV_RAM (*((volatile unsigned int *)(0x62200004)))        // [0] , params_enable_arm_256kb ARM 可以操作RISC v的ram
#define ARM_SET_RISCV_RAM_BASE (*((volatile unsigned long *)(0x62200008))) // [12:0],256KB SRAM高位地址寄存器[17:5] ARM 可以操作RISC v的ram 高端地址
#define RISCV_CODE_INFO (*((volatile unsigned long *)(0x62200010)))        // 0x6220_0010~0x6220001f arm_to_riscv_key_info
#define ARM_CPY_RISCV_RAM_BASE (*((volatile unsigned long *)(0x62200020))) // 0x6220_0020~0x6220_003f 32个byte的映射内存，映射到256KBSRAM【4:0】

#endif // !_REG_H_
