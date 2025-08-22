#ifndef S300_I2C_H
#define S300_I2C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 基于 DW_apb_i2c，控制器数量 4，基地址见 s300.h 或板级寄存器映射 */

/* 速度模式 */
typedef enum {
    S300_I2C_SPEED_STANDARD = 0, /* 100k */
    S300_I2C_SPEED_FAST     = 1, /* 400k / 1M (FMP 需外部保证参数) */
    S300_I2C_SPEED_HIGH     = 2  /* 3.4M */
} S300_I2C_Speed;

/* 主从与地址模式 */
typedef enum {
    S300_I2C_ROLE_MASTER = 0,
    S300_I2C_ROLE_SLAVE  = 1
} S300_I2C_Role;

typedef struct {
    S300_I2C_Role role;         /* 仅实现主机模式；从机保留 */
    S300_I2C_Speed speed;       /* 速度模式 */
    uint8_t addr_10bit;         /* 0:7bit 1:10bit (主/从) */
    uint16_t own_addr;          /* 作为从机的本机地址（从机功能预留） */
    uint8_t restart_en;         /* 允许 RESTART */
    uint8_t stop_det_if_addr;   /* 仅被寻址时发 STOP_DET */
    uint8_t rx_full_hold_ctrl;  /* RX FIFO 满时是否 hold 总线（视 IP 配置） */
    /* 时序参数：若为 0 则按经验值/默认分配；否则使用用户提供（单位：i2c clk 计数） */
    uint16_t ss_hcnt, ss_lcnt;
    uint16_t fs_hcnt, fs_lcnt;
    uint16_t hs_hcnt, hs_lcnt;
    uint8_t  fs_spklen;         /* 快速/快速+尖峰抑制 */
    uint8_t  hs_spklen;         /* 高速尖峰抑制 */
} S300_I2C_Config;

/* 初始化 / 反初始化 */
int S300_I2C_Init(uint32_t idx, const S300_I2C_Config *cfg);
void S300_I2C_Deinit(uint32_t idx);

/* 目标地址设置（7/10bit）*/
int S300_I2C_SetTarget(uint32_t idx, uint16_t addr, uint8_t special, uint8_t gc_or_start);

/* 主机传输（常用 EEPROM 风格：先写寄存器地址，再读/写数据）*/
int S300_I2C_MasterWrite(uint32_t idx, uint16_t dev_addr, uint16_t mem_addr,
                         uint8_t mem_addr_16bit, const uint8_t *data, size_t len, uint32_t timeout_ms);
int S300_I2C_MasterRead(uint32_t idx, uint16_t dev_addr, uint16_t mem_addr,
                        uint8_t mem_addr_16bit, uint8_t *data, size_t len, uint32_t timeout_ms);

/* 低级别 FIFO/POLL 操作（可选） */
int S300_I2C_WriteBytes(uint32_t idx, uint16_t dev_addr, const uint8_t *data, size_t len, uint8_t send_stop, uint32_t timeout_ms);
int S300_I2C_ReadBytes(uint32_t idx, uint16_t dev_addr, uint8_t *data, size_t len, uint8_t send_stop, uint32_t timeout_ms);

/* 中断相关（按需使用）*/
int S300_I2C_EnableIRQ(uint32_t idx, uint32_t mask);
int S300_I2C_DisableIRQ(uint32_t idx, uint32_t mask);
uint32_t S300_I2C_GetIntStatus(uint32_t idx);
uint32_t S300_I2C_GetRawIntStatus(uint32_t idx);
void S300_I2C_ClearAllInts(uint32_t idx);

/* 其他控制 */
void S300_I2C_Enable(uint32_t idx, uint8_t en);
uint32_t S300_I2C_Status(uint32_t idx);

#ifdef __cplusplus
}
#endif

#endif /* S300_I2C_H */
