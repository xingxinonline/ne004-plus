#if (BSP_I2C_SOFT==0)
#include "i2c.h"

/* Keep last configured property for TAR updates (mirrors legacy gu_config) */
static uint32_t s_i2c_prop_shadow[4] = {0};

static inline S300_I2C_TypeDef *i2c_get(i2c_idx_t idx)
{
    switch (idx)
    {
    case I2C_IDX0:
        return I2C0;
    case I2C_IDX1:
        return I2C1;
    case I2C_IDX2:
        return I2C2;
    case I2C_IDX3:
        return I2C3;
    default:
        return I2C0;
    }
}

void i2c_enable(i2c_idx_t idx, bool en)
{
    S300_I2C_TypeDef *I = i2c_get(idx);
    if ((I->ENABLE & 0x1u) && !en)
    {
        I->ENABLE &= ~1u;
        while (I->ENABLE_STATUS & 0x1u) { /* wait disabled */ }
    }
    else if (en)
    {
        I->ENABLE |= 1u;
    }
}

void i2c_set_interrupt(i2c_idx_t idx, i2c_int_t mask)
{
    S300_I2C_TypeDef *I = i2c_get(idx);
    i2c_enable(idx, false);
    I->INTR_MASK = (uint32_t)mask;
    i2c_enable(idx, true);
}

void i2c_send_stop(i2c_idx_t idx)
{
    /* For DW_apb_i2c, STOP is requested via DATA_CMD with STOP bit */
    S300_I2C_TypeDef *I = i2c_get(idx);
    I->DATA_CMD = I2C_CMD_STOP;
}

void i2c_state_clear(i2c_idx_t idx)
{
    S300_I2C_TypeDef *I = i2c_get(idx);
    (void)I->INTR_STAT;
    (void)I->CLR_INTR;
    (void)I->CLR_RX_UNDER;
    (void)I->CLR_RX_OVER;
    (void)I->CLR_TX_OVER;
    (void)I->CLR_RD_REQ;
    (void)I->CLR_TX_ABRT;
    (void)I->CLR_RX_DONE;
    (void)I->CLR_ACTIVITY;
    (void)I->CLR_STOP_DET;
    (void)I->CLR_START_DET;
    (void)I->CLR_GEN_CALL;
    (void)I->CLR_RESTART_DET;
}

int i2c_init(i2c_idx_t idx, i2c_prop_t prop, uint16_t saddr, uint32_t apbclk_hz, uint32_t i2cclk_hz)
{
    (void)apbclk_hz;
    S300_I2C_TypeDef *I = i2c_get(idx);
    s_i2c_prop_shadow[idx] = (uint32_t)prop;
    i2c_enable(idx, false);
    /* Program control register (use lower 8 bits as legacy did) */
    I->CON = ((uint32_t)prop) & 0xFFu;
    /* Mask all interrupts initially */
    I->INTR_MASK = 0u;
    i2c_state_clear(idx);
    /* Target address setup */
    uint32_t tar = (uint32_t)(saddr & 0x3FFu);
    if (prop & I2C_MASTER_10BIT) tar |= (1u << 12); /* IC_TAR bit 12 */
    tar |= (uint32_t)prop & (I2C_SPECIAL | I2C_GC_OR_START);
    I->TAR = tar;
    /* Clock counts: approximate 60/40 duty like legacy logic */
    uint32_t total = (i2cclk_hz == 0u) ? 0u : (apbclk_hz / i2cclk_hz);
    if (total < 8u) total = 8u; /* avoid too small */
    uint32_t h = (total * 6u) / 10u;
    uint32_t l = total - h;
    if (prop & I2C_100K)
    {
        I->SS_SCL_HCNT = h;
        I->SS_SCL_LCNT = l;
    }
    else if (prop & I2C_400K)
    {
        I->FS_SCL_HCNT = h;
        I->FS_SCL_LCNT = l;
    }
    else     /* HS */
    {
        I->HS_SCL_HCNT = h;
        I->HS_SCL_LCNT = l;
    }
    /* Interrupt mask per request */
    I->INTR_MASK = (prop & I2C_INTERRUPT) ? 0x3FFFu : 0u;
    I->RX_TL = 0u; /* n-1 */
    I->TX_TL = 0u; /* n   */
    I->FS_SPKLEN = 10u;
    I->HS_SPKLEN = 10u;
    i2c_enable(idx, true);
    return 0;
}

static inline void i2c_wait_flag(volatile uint32_t *reg, uint32_t mask)
{
    while (((*reg) & mask) == 0u) { /* spin */ }
}

int i2c_write(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, const uint8_t *data, uint16_t dlen)
{
    S300_I2C_TypeDef *I = i2c_get(idx);
    /* Wait not busy */
    i2c_wait_flag(&I->STATUS, 0x4u); /* TFNF: transmit FIFO not full (or 0x4? legacy used 0x4 for idle) */
    i2c_state_clear(idx);
    /* Reprogram TAR for this transaction to ensure correct addressing mode */
    uint32_t tar = (uint32_t)(saddr & 0x3FFu);
    if (I->CON & (I2C_MASTER | I2C_MASTER_10BIT)) tar |= (1u << 12);
    tar |= s_i2c_prop_shadow[idx] & (I2C_SPECIAL | I2C_GC_OR_START);
    I->TAR = tar;
    if (is16bit)
    {
        I->DATA_CMD = ((address >> 8) & 0xFFu);
        i2c_wait_flag(&I->STATUS, 0x2u); /* TFNF */
    }
    I->DATA_CMD = (address & 0xFFu);
    for (uint16_t i = 0; i < dlen; ++i)
    {
        i2c_wait_flag(&I->STATUS, 0x2u);
        I->DATA_CMD = data[i];
    }
    /* Wait stop detected and bus idle similar to legacy */
    while ((I->RAW_INTR_STAT & 0x200u) == 0u) { }
    while ((I->STATUS & 0x1u) != 0u) { }
    i2c_state_clear(idx);
    return (int)dlen;
}

int i2c_read(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, uint8_t *data, uint16_t dlen)
{
    S300_I2C_TypeDef *I = i2c_get(idx);
    i2c_wait_flag(&I->STATUS, 0x4u);
    i2c_state_clear(idx);
    uint32_t tar = (uint32_t)(saddr & 0x3FFu);
    if (I->CON & (I2C_MASTER | I2C_MASTER_10BIT)) tar |= (1u << 12);
    tar |= s_i2c_prop_shadow[idx] & (I2C_SPECIAL | I2C_GC_OR_START);
    I->TAR = tar;
    if (is16bit)
    {
        I->DATA_CMD = (((address >> 8) & 0xFFu) | I2C_CMD_RESTART);
        i2c_wait_flag(&I->STATUS, 0x2u);
    }
    I->DATA_CMD = ((address & 0xFFu) | (is16bit ? 0u : I2C_CMD_READ));
    i2c_wait_flag(&I->STATUS, 0x2u);
    for (uint16_t i = 0; i < dlen; ++i)
    {
        I->DATA_CMD = I2C_CMD_READ;
        while ((I->STATUS & 0x8u) == 0u) { /* RFNE */ }
        data[i] = (uint8_t)(I->DATA_CMD & 0xFFu);
        i2c_wait_flag(&I->STATUS, 0x4u);
    }
    while ((I->STATUS & 0x4u) == 0u) { }
    while ((I->RAW_INTR_STAT & 0x200u) == 0u) { }
    while ((I->STATUS & 0x1u) != 0u) { }
    i2c_state_clear(idx);
    return (int)dlen;
}

#endif /* BSP_I2C_SOFT==0 */
