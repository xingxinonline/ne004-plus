#include "wm8978.h"
#include <string.h>

static uint8_t g_wm8978_addr = WM8978_I2C_ADDR;
void wm8978_set_addr(uint8_t saddr)
{
    g_wm8978_addr = saddr;
}

static int wr(i2c_soft_t *i2c, uint16_t reg, uint16_t val)
{
    /* WM8978: 7-bit dev addr, write payload is 9-bit register (reg<<1 | val[8]) + 8-bit val[7:0] */
    uint8_t buf[2];
    buf[0] = (uint8_t)((reg << 1) | ((val >> 8) & 0x01));
    buf[1] = (uint8_t)(val & 0xFF);
    return i2c_soft_mem_write(i2c, g_wm8978_addr, buf[0], false, &buf[1], 1);
}

int wm8978_init(i2c_soft_t *i2c)
{
    int ret;
    /* soft reset */
    ret = wr(i2c, 0, 0);
    if (ret) return ret;
    /* 基于原始配置移植的最小可用路径 */
    wr(i2c, 1, 0x001B);
    wr(i2c, 2, 0x01B0);
    wr(i2c, 3, 0x006C);
    wr(i2c, 6, 0x0000);
    wr(i2c, 7, 0x0000); /* fs 16k */
    wr(i2c, 43, 1u << 4);
    wr(i2c, 47, 1u << 8);
    wr(i2c, 48, 1u << 8);
    wr(i2c, 49, 1u << 1);
    wr(i2c, 10, 1u << 3);
    wr(i2c, 14, 1u << 3);
    wr(i2c, 5, 0x0001);
    return 0;
}

int wm8978_set_adda(i2c_soft_t *i2c, bool dac_en, bool adc_en)
{
    /* R3: DACEN[1:0], R2: ADCEN[1:0] */
    uint16_t r3 = 0, r2 = 0;
    if (dac_en) r3 |= 3u << 0;
    else r3 &= ~(3u << 0);
    if (adc_en) r2 |= 3u << 0;
    else r2 &= ~(3u << 0);
    if (wr(i2c, 3, r3)) return -1;
    if (wr(i2c, 2, r2)) return -1;
    return 0;
}

int wm8978_set_linein_gain(i2c_soft_t *i2c, uint8_t gain)
{
    gain &= 0x7;
    uint16_t r47 = (uint16_t)(gain << 4);
    uint16_t r48 = (uint16_t)(gain << 4);
    if (wr(i2c, 47, r47)) return -1;
    if (wr(i2c, 48, (uint16_t)(r48 | (1u << 8)))) return -1; /* update bit */
    return 0;
}

int wm8978_set_aux_gain(i2c_soft_t *i2c, uint8_t gain)
{
    gain &= 0x7;
    uint16_t r47 = (uint16_t)(gain << 0);
    uint16_t r48 = (uint16_t)(gain << 0);
    if (wr(i2c, 47, r47)) return -1;
    if (wr(i2c, 48, (uint16_t)(r48 | (1u << 8)))) return -1;
    return 0;
}

int wm8978_set_input(i2c_soft_t *i2c, bool mic_en, bool linein_en, bool aux_en)
{
    uint16_t r2 = 0, r44 = 0;
    if (mic_en)
    {
        r2 |= 3u << 2;
        r44 |= (3u << 4) | (3u << 0);
    }
    if (wr(i2c, 2, r2)) return -1;
    if (wr(i2c, 44, r44)) return -1;
    if (wm8978_set_linein_gain(i2c, linein_en ? 5 : 0)) return -1;
    if (wm8978_set_aux_gain(i2c, aux_en ? 7 : 0)) return -1;
    return 0;
}

int wm8978_set_output(i2c_soft_t *i2c, bool dac_out_en, bool bypass_en)
{
    uint16_t v = 0;
    if (dac_out_en) v |= 1u << 0;
    if (bypass_en)
    {
        v |= 1u << 1;
        v |= 5u << 2;
    }
    if (wr(i2c, 50, v)) return -1;
    if (wr(i2c, 51, v)) return -1;
    return 0;
}

int wm8978_set_mic_gain(i2c_soft_t *i2c, uint8_t gain)
{
    gain &= 0x3F;
    if (wr(i2c, 45, gain)) return -1;
    if (wr(i2c, 46, (uint16_t)(gain | (1u << 8)))) return -1;
    return 0;
}

int wm8978_i2s_cfg(i2c_soft_t *i2c, uint8_t fmt, uint8_t len)
{
    fmt &= 0x03;
    len &= 0x03;
    return wr(i2c, 4, (uint16_t)((fmt << 3) | (len << 5)));
}

int wm8978_set_hp_vol(i2c_soft_t *i2c, uint8_t voll, uint8_t volr)
{
    voll &= 0x3F;
    volr &= 0x3F;
    if (voll == 0) voll |= 1u << 6;
    if (volr == 0) volr |= 1u << 6;
    if (wr(i2c, 52, voll)) return -1;
    if (wr(i2c, 53, (uint16_t)(volr | (1u << 8)))) return -1;
    return 0;
}

int wm8978_set_spk_vol(i2c_soft_t *i2c, uint8_t volx)
{
    volx &= 0x3F;
    if (volx == 0) volx |= 1u << 6;
    if (wr(i2c, 54, volx)) return -1;
    if (wr(i2c, 55, (uint16_t)(volx | (1u << 8)))) return -1;
    return 0;
}
