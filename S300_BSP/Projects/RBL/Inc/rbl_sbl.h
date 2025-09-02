// SBL validation and jump for RBL Phase 3
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// === SBL 配置参数 ===

// SBL 在 Flash 中的起始地址（可配置）
#ifndef SBL_FLASH_START_ADDR
#define SBL_FLASH_START_ADDR    0x20000     // 128KB offset
#endif

// SBL 最大尺寸（可配置）
#ifndef SBL_MAX_SIZE
#define SBL_MAX_SIZE           0x40000     // 256KB max
#endif

// SBL 加载到 SRAM 的地址
#ifndef SBL_SRAM_LOAD_ADDR  
#define SBL_SRAM_LOAD_ADDR     0x20010000  // SRAM +64KB
#endif

// === 核心功能函数 ===

// SBL 完整性检查：读取、验证栈顶/复位向量、轻量CRC
// 返回: 0=有效 -1=无效
int rbl_sbl_validate(void);

// 加载 SBL 到 SRAM 并跳转
// 返回: 此函数不返回(成功跳转) 或 -1(失败)
int rbl_sbl_load_and_jump(void);

// === 辅助功能 ===

// 计算简单的CRC32（轻量实现）
uint32_t rbl_crc32_simple(const uint8_t *data, uint32_t len);

// 检查ARM Cortex-M栈顶指针是否合理（在SRAM范围内）
bool rbl_is_valid_stack_pointer(uint32_t sp);

// 检查ARM Cortex-M复位向量是否合理（奇数地址，Thumb标志）
bool rbl_is_valid_reset_vector(uint32_t reset_vector);

#ifdef __cplusplus
}
#endif
