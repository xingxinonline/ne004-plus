#include "s300.h"
#include "s300_i2c.h"
#include "systick.h"

/* Helpers for register access; reusing cortex-m4 register layout if identical */
#ifndef I2C0_BASE
#define I2C0_BASE  (0x40014000u)
#endif
#ifndef I2C_BASE
#define I2C_BASE   I2C0_BASE
#endif
#define I2C_REG(base, off) (*(volatile uint32_t *)((base) + (off)))
#define I2C_BASE_IDX(idx)  (I2C_BASE + ((uint32_t)(idx) * 0x1000u))

#define IC_CON(b)           I2C_REG(b, 0x00)
#define IC_TAR(b)           I2C_REG(b, 0x04)
#define IC_SAR(b)           I2C_REG(b, 0x08)
#define IC_DATA_CMD(b)      I2C_REG(b, 0x10)
#define IC_SS_SCL_HCNT(b)   I2C_REG(b, 0x14)
#define IC_SS_SCL_LCNT(b)   I2C_REG(b, 0x18)
#define IC_FS_SCL_HCNT(b)   I2C_REG(b, 0x1C)
#define IC_FS_SCL_LCNT(b)   I2C_REG(b, 0x20)
#define IC_HS_SCL_HCNT(b)   I2C_REG(b, 0x24)
#define IC_HS_SCL_LCNT(b)   I2C_REG(b, 0x28)
#define IC_INTR_STAT(b)     I2C_REG(b, 0x2C)
#define IC_INTR_MASK(b)     I2C_REG(b, 0x30)
#define IC_RAW_INTR_STAT(b) I2C_REG(b, 0x34)
#define IC_RX_TL(b)         I2C_REG(b, 0x38)
#define IC_TX_TL(b)         I2C_REG(b, 0x3C)
#define IC_CLR_INTR(b)      I2C_REG(b, 0x40)
#define IC_CLR_RX_UNDER(b)  I2C_REG(b, 0x44)
#define IC_CLR_RX_OVER(b)   I2C_REG(b, 0x48)
#define IC_CLR_TX_OVER(b)   I2C_REG(b, 0x4C)
#define IC_CLR_RD_REQ(b)    I2C_REG(b, 0x50)
#define IC_CLR_TX_ABRT(b)   I2C_REG(b, 0x54)
#define IC_CLR_RX_DONE(b)   I2C_REG(b, 0x58)
#define IC_CLR_ACTIVITY(b)  I2C_REG(b, 0x5C)
#define IC_CLR_STOP_DET(b)  I2C_REG(b, 0x60)
#define IC_CLR_START_DET(b) I2C_REG(b, 0x64)
#define IC_CLR_GEN_CALL(b)  I2C_REG(b, 0x68)
#define IC_ENABLE(b)        I2C_REG(b, 0x6C)
#define IC_STATUS(b)        I2C_REG(b, 0x70)
#define IC_TXFLR(b)         I2C_REG(b, 0x74)
#define IC_RXFLR(b)         I2C_REG(b, 0x78)
#define IC_SDA_HOLD(b)      I2C_REG(b, 0x7C)
#define IC_TX_ABRT_SOURCE(b) I2C_REG(b, 0x80)
#define IC_ENABLE_STATUS(b) I2C_REG(b, 0x9C)
#define IC_FS_SPKLEN(b)     I2C_REG(b, 0xA0)
#define IC_HS_SPKLEN(b)     I2C_REG(b, 0xA4)
#define IC_CLR_RESTART_DET(b) I2C_REG(b, 0xA8)

/* IC_CON bits (subset) */
#define IC_CON_MASTER_MODE           (1u << 0)
#define IC_CON_SPEED_LSB             1u
#define IC_CON_10BITADDR_SLAVE       (1u << 3)
#define IC_CON_10BITADDR_MASTER      (1u << 4)
#define IC_CON_RESTART_EN            (1u << 5)
#define IC_CON_SLAVE_DISABLE         (1u << 6)
#define IC_CON_STOP_DET_IFADDR       (1u << 7)
#define IC_CON_TX_EMPTY_CTRL         (1u << 8)

/* IC_STATUS bits */
#define ST_ACTIVITY  (1u << 0)
#define ST_TFNF      (1u << 1)
#define ST_TFE       (1u << 2)
#define ST_RFNE      (1u << 3)
#define ST_RFF       (1u << 4)

/* IC_DATA_CMD bits */
#define CMD_READ     (1u << 8)
#define CMD_STOP     (1u << 9)
#define CMD_RESTART  (1u << 10)

static inline void i2c_enable(uint32_t base, uint8_t en)
{
    if (!en) {
        if (IC_ENABLE(base) & 1u) {
            IC_ENABLE(base) = 0u;
            while (IC_ENABLE_STATUS(base) & 1u) {}
        }
    } else {
        IC_ENABLE(base) = 1u;
    }
}

void S300_I2C_Enable(uint32_t idx, uint8_t en)
{
    i2c_enable(I2C_BASE_IDX(idx), en);
}

uint32_t S300_I2C_Status(uint32_t idx)
{
    return IC_STATUS(I2C_BASE_IDX(idx));
}

static void i2c_clear_all(uint32_t base)
{
    (void)IC_INTR_STAT(base);
    (void)IC_CLR_INTR(base);
    (void)IC_CLR_RX_UNDER(base);
    (void)IC_CLR_RX_OVER(base);
    (void)IC_CLR_TX_OVER(base);
    (void)IC_CLR_RD_REQ(base);
    (void)IC_CLR_TX_ABRT(base);
    (void)IC_CLR_RX_DONE(base);
    (void)IC_CLR_ACTIVITY(base);
    (void)IC_CLR_STOP_DET(base);
    (void)IC_CLR_START_DET(base);
    (void)IC_CLR_GEN_CALL(base);
    (void)IC_CLR_RESTART_DET(base);
}

void S300_I2C_ClearAllInts(uint32_t idx)
{
    i2c_clear_all(I2C_BASE_IDX(idx));
}

int S300_I2C_EnableIRQ(uint32_t idx, uint32_t mask)
{
    uint32_t base = I2C_BASE_IDX(idx);
    i2c_enable(base, 0);
    IC_INTR_MASK(base) |= mask;
    i2c_enable(base, 1);
    return 0;
}

int S300_I2C_DisableIRQ(uint32_t idx, uint32_t mask)
{
    uint32_t base = I2C_BASE_IDX(idx);
    i2c_enable(base, 0);
    IC_INTR_MASK(base) &= ~mask;
    i2c_enable(base, 1);
    return 0;
}

uint32_t S300_I2C_GetIntStatus(uint32_t idx)
{
    return IC_INTR_STAT(I2C_BASE_IDX(idx));
}

uint32_t S300_I2C_GetRawIntStatus(uint32_t idx)
{
    return IC_RAW_INTR_STAT(I2C_BASE_IDX(idx));
}

int S300_I2C_SetTarget(uint32_t idx, uint16_t addr, uint8_t special, uint8_t gc_or_start)
{
    uint32_t base = I2C_BASE_IDX(idx);
    uint32_t tar = (uint32_t)(addr & 0x3FFu);
    if (special) tar |= (1u << 11);
    if (gc_or_start) tar |= (1u << 10);
    IC_TAR(base) = tar;
    return 0;
}

int S300_I2C_Init(uint32_t idx, const S300_I2C_Config *cfg)
{
    if (!cfg) return -1;
    uint32_t base = I2C_BASE_IDX(idx);

    i2c_enable(base, 0);

    /* IC_CON 组装 */
    uint32_t con = 0;
    if (cfg->role == S300_I2C_ROLE_MASTER) {
        con |= IC_CON_MASTER_MODE;
        con |= IC_CON_SLAVE_DISABLE;
    }
    if (cfg->addr_10bit) {
        con |= IC_CON_10BITADDR_MASTER;
        con |= IC_CON_10BITADDR_SLAVE;
    }
    if (cfg->restart_en) con |= IC_CON_RESTART_EN;
    if (cfg->stop_det_if_addr) con |= IC_CON_STOP_DET_IFADDR;
    /* 设置速度：01=SS 10=FS 11=HS，DW 定义特殊编码，这里按 docs 取 1,2,3 映射到位域 */
    uint32_t speed_code = 0x1;
    if (cfg->speed == S300_I2C_SPEED_STANDARD) speed_code = 0x1;
    else if (cfg->speed == S300_I2C_SPEED_FAST) speed_code = 0x2;
    else speed_code = 0x3;
    con |= (speed_code << IC_CON_SPEED_LSB);
    /* 使能 TX_EMPTY_CTRL 以获得 TX_EMPTY 中断（可选） */
    con |= IC_CON_TX_EMPTY_CTRL;
    IC_CON(base) = con;

    /* 时序参数：若用户未提供则按占空比 60/40 配置，具体数值需结合 APB/I2C 输入时钟。
       由于 SoC 文档未在此文件定义 I2C 源时钟，默认使用经验值或保留现状（用户应在板级提供）。 */
    if (cfg->ss_hcnt) { IC_SS_SCL_HCNT(base) = cfg->ss_hcnt; IC_SS_SCL_LCNT(base) = cfg->ss_lcnt; }
    if (cfg->fs_hcnt) { IC_FS_SCL_HCNT(base) = cfg->fs_hcnt; IC_FS_SCL_LCNT(base) = cfg->fs_lcnt; }
    if (cfg->hs_hcnt) { IC_HS_SCL_HCNT(base) = cfg->hs_hcnt; IC_HS_SCL_LCNT(base) = cfg->hs_lcnt; }
    if (cfg->fs_spklen) IC_FS_SPKLEN(base) = cfg->fs_spklen;
    if (cfg->hs_spklen) IC_HS_SPKLEN(base) = cfg->hs_spklen;

    /* FIFO 阈值默认 0（字节级触发）*/
    IC_RX_TL(base) = 0u;
    IC_TX_TL(base) = 0u;
    /* 屏蔽所有中断，使用轮询，按需开启 */
    IC_INTR_MASK(base) = 0u;
    i2c_clear_all(base);

    i2c_enable(base, 1);
    return 0;
}

void S300_I2C_Deinit(uint32_t idx)
{
    i2c_enable(I2C_BASE_IDX(idx), 0);
}

/* 轮询发送若干字节，支持可选 STOP */
int S300_I2C_WriteBytes(uint32_t idx, uint16_t dev_addr, const uint8_t *data, size_t len, uint8_t send_stop, uint32_t timeout_ms)
{
    if (!data && len) return -1;
    uint32_t base = I2C_BASE_IDX(idx);
    S300_I2C_SetTarget(idx, dev_addr, 0, 0);
    uint32_t start = S300_SysTick_Millis();
    size_t i = 0;
    while (i < len) {
        if (IC_STATUS(base) & ST_TFNF) {
            IC_DATA_CMD(base) = data[i++];
            continue;
        }
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    if (send_stop) {
        /* 等待 FIFO 空后，发 STOP（仅当 Empty HOLD 使能支持）；若 IP 不支持 STOP 位，则通过读回中断 STOP_DET 判断 */
        while (!(IC_STATUS(base) & ST_TFE)) {
            if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
        }
        IC_DATA_CMD(base) = CMD_STOP;
    }
    /* 等待总线空闲 */
    while (IC_STATUS(base) & ST_ACTIVITY) {
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    return (int)i;
}

/* 轮询接收若干字节，send_stop 控制最后一个字节后 STOP */
int S300_I2C_ReadBytes(uint32_t idx, uint16_t dev_addr, uint8_t *data, size_t len, uint8_t send_stop, uint32_t timeout_ms)
{
    if (!data && len) return -1;
    uint32_t base = I2C_BASE_IDX(idx);
    S300_I2C_SetTarget(idx, dev_addr, 0, 0);
    uint32_t start = S300_SysTick_Millis();
    size_t rx = 0;
    size_t tx_req = 0;
    while (rx < len) {
        /* 填充读请求到 TX FIFO */
        if (tx_req < len && (IC_STATUS(base) & ST_TFNF)) {
            uint32_t cmd = CMD_READ;
            if (tx_req == (len - 1) && send_stop) cmd |= CMD_STOP;
            IC_DATA_CMD(base) = cmd;
            tx_req++;
            continue;
        }
        /* 读取收到的数据 */
        if (IC_STATUS(base) & ST_RFNE) {
            data[rx++] = (uint8_t)(IC_DATA_CMD(base) & 0xFFu);
            continue;
        }
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    /* 等待总线空闲 */
    while (IC_STATUS(base) & ST_ACTIVITY) {
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    return (int)rx;
}

int S300_I2C_MasterWrite(uint32_t idx, uint16_t dev_addr, uint16_t mem_addr,
                         uint8_t mem_addr_16bit, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    /* 写入子地址（1-2 字节）+ 数据 */
    uint8_t hdr[2];
    size_t hdr_len = mem_addr_16bit ? 2u : 1u;
    if (mem_addr_16bit) { hdr[0] = (uint8_t)(mem_addr >> 8); hdr[1] = (uint8_t)(mem_addr & 0xFF); }
    else { hdr[0] = (uint8_t)(mem_addr & 0xFF); }

    int n = S300_I2C_WriteBytes(idx, dev_addr, hdr, hdr_len, (len == 0), timeout_ms);
    if (n != (int)hdr_len) return -1;
    if (len == 0) return 0;
    n = S300_I2C_WriteBytes(idx, dev_addr, data, len, 1 /* stop */, timeout_ms);
    return (n == (int)len) ? (int)len : -1;
}

int S300_I2C_MasterRead(uint32_t idx, uint16_t dev_addr, uint16_t mem_addr,
                        uint8_t mem_addr_16bit, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    /* 先写寄存器地址（不发 STOP，发 RESTART），再读 len 字节（最后发 STOP）*/
    uint8_t hdr[2];
    size_t hdr_len = mem_addr_16bit ? 2u : 1u;
    if (mem_addr_16bit) { hdr[0] = (uint8_t)(mem_addr >> 8); hdr[1] = (uint8_t)(mem_addr & 0xFF); }
    else { hdr[0] = (uint8_t)(mem_addr & 0xFF); }

    /* 发送子地址 */
    int n = S300_I2C_WriteBytes(idx, dev_addr, hdr, hdr_len, 0 /* no stop */, timeout_ms);
    if (n != (int)hdr_len) return -1;

    /* RESTART + READ len 字节 + STOP */
    uint32_t base = I2C_BASE_IDX(idx);
    uint32_t start = S300_SysTick_Millis();
    size_t rx = 0, tx_req = 0;
    S300_I2C_SetTarget(idx, dev_addr, 0, 0);
    while (rx < len) {
        if (tx_req < len && (IC_STATUS(base) & ST_TFNF)) {
            uint32_t cmd = CMD_READ | CMD_RESTART; /* 第一次带 RESTART，后续可不带 */
            if (tx_req == (len - 1)) cmd |= CMD_STOP;
            IC_DATA_CMD(base) = cmd;
            tx_req++;
            continue;
        }
        if (IC_STATUS(base) & ST_RFNE) {
            data[rx++] = (uint8_t)(IC_DATA_CMD(base) & 0xFFu);
            continue;
        }
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    while (IC_STATUS(base) & ST_ACTIVITY) {
        if (timeout_ms && (S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    return (int)rx;
}
