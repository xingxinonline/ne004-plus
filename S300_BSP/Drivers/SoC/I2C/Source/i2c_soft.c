#if (BSP_I2C_SOFT==1)
#include "i2c.h"
#include "gpio.h"

#ifndef I2C_SOFT_PORT_COUNT
    #define I2C_SOFT_PORT_COUNT 4
#endif

/* Default pins mapping (can be overridden by weak symbols) */
static const gpio_port_t s_i2c_port[I2C_SOFT_PORT_COUNT][2] =
{
    { GPIOA, GPIOA }, { GPIOA, GPIOA }, { GPIOA, GPIOA }, { GPIOA, GPIOA }
};
static const uint8_t s_i2c_pin[I2C_SOFT_PORT_COUNT][2] =
{
    { 14u, 15u }, { 0u, 1u }, { 2u, 3u }, { 4u, 5u }
};
static const gpio_func_t s_i2c_func[I2C_SOFT_PORT_COUNT][2] =
{
    { FUNCTION_2, FUNCTION_2 }, { FUNCTION_2, FUNCTION_2 }, { FUNCTION_2, FUNCTION_2 }, { FUNCTION_2, FUNCTION_2 }
};

#define I2C_SCL 0
#define I2C_SDA 1

static uint32_t s_us_per_scl = 2;  /* default ~250kHz */
static uint32_t s_tick_per_us = 1; /* calibrated from SystemCoreClock */

static inline void sda_out(i2c_idx_t i)
{
    gpio_set_direction(s_i2c_port[i][I2C_SDA], s_i2c_pin[i][I2C_SDA], 1);
    gpio_set_mode(s_i2c_port[i][I2C_SDA], s_i2c_pin[i][I2C_SDA], GPIO_UP);
}
static inline void sda_in(i2c_idx_t i)
{
    gpio_set_direction(s_i2c_port[i][I2C_SDA], s_i2c_pin[i][I2C_SDA], 0);
}
static inline void scl_set(i2c_idx_t i, uint32_t v)
{
    gpio_set_data(s_i2c_port[i][I2C_SCL], s_i2c_pin[i][I2C_SCL], v);
}
static inline void sda_set(i2c_idx_t i, uint32_t v)
{
    gpio_set_data(s_i2c_port[i][I2C_SDA], s_i2c_pin[i][I2C_SDA], v);
}
static inline uint32_t sda_get(i2c_idx_t i)
{
    uint32_t portv = gpio_get_data(s_i2c_port[i][I2C_SDA]);
    return ((portv & (1u << s_i2c_pin[i][I2C_SDA])) != 0u) ? 1u : 0u;
}

static void delay_us(uint32_t us)
{
    volatile uint32_t n = (us * s_tick_per_us) / 8u;
    while (n--)
    {
        __NOP();
    }
}

static void i2c_soft_start(i2c_idx_t i)
{
    sda_out(i);
    sda_set(i, 1);
    scl_set(i, 1);
    delay_us(s_us_per_scl);
    sda_set(i, 0);
    delay_us(s_us_per_scl);
    scl_set(i, 0);
}

static void i2c_soft_stop(i2c_idx_t i)
{
    sda_out(i);
    scl_set(i, 0);
    sda_set(i, 0);
    delay_us(s_us_per_scl);
    scl_set(i, 1);
    sda_set(i, 1);
    delay_us(s_us_per_scl);
}

static uint8_t i2c_soft_wait_ack(i2c_idx_t i)
{
    uint8_t cnt = 0;
    sda_in(i);
    sda_set(i, 1);
    delay_us(s_us_per_scl / 4);
    scl_set(i, 1);
    delay_us(s_us_per_scl / 4);
    while (sda_get(i))
    {
        if (++cnt > 250)
        {
            i2c_soft_stop(i);
            return 1;
        }
    }
    scl_set(i, 0);
    return 0;
}

static void i2c_soft_send_byte(i2c_idx_t i, uint8_t d)
{
    sda_out(i);
    scl_set(i, 0);
    for (uint8_t b = 0; b < 8; ++b)
    {
        sda_set(i, (d & 0x80u) != 0u);
        d <<= 1;
        delay_us(s_us_per_scl / 2);
        scl_set(i, 1);
        delay_us(s_us_per_scl);
        scl_set(i, 0);
        delay_us(s_us_per_scl / 2);
    }
}

static uint8_t i2c_soft_recv_byte(i2c_idx_t i)
{
    uint8_t val = 0;
    sda_in(i);
    for (uint8_t b = 0; b < 8; ++b)
    {
        scl_set(i, 0);
        delay_us(s_us_per_scl);
        scl_set(i, 1);
        delay_us(s_us_per_scl / 2);
        val = (uint8_t)((val << 1) | (sda_get(i) & 1u));
        delay_us(s_us_per_scl / 2);
    }
    return val;
}

int i2c_init(i2c_idx_t idx, i2c_prop_t prop, uint16_t saddr, uint32_t apbclk_hz, uint32_t i2cclk_hz)
{
    (void)prop;
    (void)apbclk_hz;
    (void)saddr;
    for (int k = 0; k < 2; ++k)
    {
        gpio_set_function(s_i2c_port[idx][k], s_i2c_pin[idx][k], s_i2c_func[idx][k]);
        gpio_set_direction(s_i2c_port[idx][k], s_i2c_pin[idx][k], 1);
        gpio_set_mode(s_i2c_port[idx][k], s_i2c_pin[idx][k], GPIO_UP);
    }
    s_us_per_scl = (1000000u / i2cclk_hz) / 2u;
    if (s_us_per_scl == 0u) s_us_per_scl = 1u;
    /* Calibrate tick using SystemCoreClock */
    extern uint32_t SystemCoreClock;
    s_tick_per_us = (SystemCoreClock / 1000000u);
    if (s_tick_per_us == 0u) s_tick_per_us = 1u;
    return 0;
}

int i2c_write(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, const uint8_t *data, uint16_t dlen)
{
    int ret = 0;
    uint8_t a_hi = (uint8_t)((address >> 8) & 0xFFu), a_lo = (uint8_t)(address & 0xFFu);
    i2c_soft_start(idx);
    i2c_soft_send_byte(idx, (uint8_t)(((saddr & 0xFFu) << 1) | 0u));
    if (i2c_soft_wait_ack(idx)) ret = 1;
    if (is16bit)
    {
        i2c_soft_send_byte(idx, a_hi);
        if (i2c_soft_wait_ack(idx)) ret = 1;
    }
    i2c_soft_send_byte(idx, a_lo);
    if (i2c_soft_wait_ack(idx)) ret = 1;
    while (dlen--)
    {
        i2c_soft_send_byte(idx, *data++);
        if (i2c_soft_wait_ack(idx)) ret = 1;
    }
    i2c_soft_stop(idx);
    return ret ? -1 : 0;
}

int i2c_read(i2c_idx_t idx, uint16_t saddr, uint16_t address, bool is16bit, uint8_t *data, uint16_t dlen)
{
    (void)address;
    (void)is16bit; /* Minimal read: sequential from current addr (matches legacy soft path) */
    int ret = 0;
    i2c_soft_start(idx);
    i2c_soft_send_byte(idx, (uint8_t)(((saddr & 0xFFu) << 1) | 0u));
    if (i2c_soft_wait_ack(idx)) ret = 1;
    while (dlen--)
    {
        *data++ = i2c_soft_recv_byte(idx);
    }
    i2c_soft_stop(idx);
    return ret ? -1 : 0;
}

void i2c_enable(i2c_idx_t idx, bool en)
{
    (void)idx;
    (void)en;
}
void i2c_set_interrupt(i2c_idx_t idx, i2c_int_t mask)
{
    (void)idx;
    (void)mask;
}
void i2c_send_stop(i2c_idx_t idx)
{
    (void)idx;
}
void i2c_state_clear(i2c_idx_t idx)
{
    (void)idx;
}

#endif /* BSP_I2C_SOFT==1 */
