#ifndef S300_BSP_I2C_H
#define S300_BSP_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "s300.h"
#include "i2c_s300.h"

/* Backend selection: default to software I2C unless overridden at compile time.
 * To use hardware I2C, pass -DBSP_I2C_SOFT=0 in your CFLAGS. */
#ifndef BSP_I2C_SOFT
#define BSP_I2C_SOFT 1
#endif

/* Indices (match legacy emI2C values 0..3) */
typedef enum { I2C_IDX0 = 0, I2C_IDX1 = 1, I2C_IDX2 = 2, I2C_IDX3 = 3 } i2c_idx_t;

/* Properties (subset preserving legacy bit values where meaningful) */
typedef enum
{
    I2C_SLAVE          = 0x00,
    I2C_MASTER         = 0x41,  /* same as legacy EM_I2C_MASTER */
    I2C_100K           = 0x02,
    I2C_400K           = 0x04,
    I2C_HIGH           = 0x06,
    I2C_SLAVE_10BIT    = 0x08,
    I2C_MASTER_10BIT   = 0x10,
    I2C_RESTART_EN     = 0x20,
    I2C_STOP_DEF       = 0x80,
    I2C_INTERRUPT      = 0x100,
    I2C_SPECIAL        = 0x800,
    I2C_GC_OR_START    = 0x400,
} i2c_prop_t;

typedef enum
{
    I2C_INT_RX_UNDER   = 0x0001,
    I2C_INT_RX_OVER    = 0x0002,
    I2C_INT_RX_FULL    = 0x0004,
    I2C_INT_TX_OVER    = 0x0008,
    I2C_INT_TX_EMPTY   = 0x0010,
    I2C_INT_RD_REQ     = 0x0020,
    I2C_INT_TX_ABRT    = 0x0040,
    I2C_INT_RX_DONE    = 0x0080,
    I2C_INT_ACTIVITY   = 0x0100,
    I2C_INT_STOP_DET   = 0x0200,
    I2C_INT_START_DET  = 0x0400,
    I2C_INT_GEN_CALL   = 0x0800,
    I2C_INT_RESTART_DET = 0x1000,
    I2C_INT_MST_ON_HOLD = 0x2000,
} i2c_int_t;

/* DATA_CMD helpers */
#define I2C_CMD_READ      (1u << 8)
#define I2C_CMD_STOP      (1u << 9)
#define I2C_CMD_RESTART   (1u << 10)

/* API */
int  i2c_init(i2c_idx_t idx, i2c_prop_t prop, uint16_t saddr, uint32_t apbclk_hz, uint32_t i2cclk_hz);
int  i2c_write(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, const uint8_t *data, uint16_t dlen);
int  i2c_read(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, uint8_t *data, uint16_t dlen);
void i2c_enable(i2c_idx_t idx, bool en);
void i2c_set_interrupt(i2c_idx_t idx, i2c_int_t mask);
void i2c_send_stop(i2c_idx_t idx);
void i2c_state_clear(i2c_idx_t idx);

/* Legacy aliases for painless porting from cortex-m4-i2s/driver/i2c.[ch] */
static inline int init_i2c(int i2c, int pro, uint16_t saddr, uint32_t apb, uint32_t i2cclk)
{
    return i2c_init((i2c_idx_t)i2c, (i2c_prop_t)pro, saddr, apb, i2cclk);
}
static inline int write_i2c(int i2c, uint16_t saddr, uint16_t addr, int is16bit, uint8_t *data, uint16_t dlen)
{
    return i2c_write((i2c_idx_t)i2c, saddr, addr, (bool)is16bit, data, dlen);
}
static inline int read_i2c(int i2c, uint16_t saddr, uint16_t addr, int is16bit, uint8_t *data, uint16_t dlen)
{
    return i2c_read((i2c_idx_t)i2c, saddr, addr, (bool)is16bit, data, dlen);
}
static inline void set_i2c_enable(int i2c, int en)
{
    i2c_enable((i2c_idx_t)i2c, (bool)en);
}
static inline void set_i2c_interrupt(int i2c, int m)
{
    i2c_set_interrupt((i2c_idx_t)i2c, (i2c_int_t)m);
}
static inline void set_i2c_stop(int i2c)
{
    i2c_send_stop((i2c_idx_t)i2c);
}
static inline void set_i2c_state_clear(int i2c)
{
    i2c_state_clear((i2c_idx_t)i2c);
}

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_I2C_H */
