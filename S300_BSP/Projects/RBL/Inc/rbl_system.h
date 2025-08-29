/**
 * @file rbl_system.h
 * @brief RBL系统管理头文件(基于S300 BSP)
 */

#ifndef RBL_SYSTEM_H
#define RBL_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 系统信息结构体 */
typedef struct {
    uint32_t cpu_id;
    uint32_t system_clock;
    uint32_t sram_size;
    uint32_t flash_size;
    uint32_t reset_reason;
    uint32_t tick_count;
} rbl_system_info_t;

/* 系统初始化 */
void rbl_system_early_init(void);
int rbl_system_init(void);

/* 时间管理 */
uint32_t rbl_get_tick(void);
void rbl_delay_ms(uint32_t ms);
void rbl_delay_us(uint32_t us);

/* 中断管理 */
uint32_t rbl_disable_irq(void);
void rbl_restore_irq(uint32_t primask);

/* 复位和电源管理 */
uint32_t rbl_get_reset_reason(void);
void rbl_system_reset(void);
void rbl_system_sleep(void);

/* 系统信息 */
int rbl_get_system_info(rbl_system_info_t *info);

/* 兼容性函数 */
static inline uint32_t rbl_system_get_tick_ms(void) { return rbl_get_tick(); }
static inline void rbl_system_delay_ms(uint32_t ms) { rbl_delay_ms(ms); }
static inline uint32_t rbl_system_get_reset_reason(void) { return rbl_get_reset_reason(); }

#ifdef __cplusplus
}
#endif

#endif /* RBL_SYSTEM_H */
