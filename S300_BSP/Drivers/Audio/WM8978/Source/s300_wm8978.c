#include "s300_wm8978.h"
#include "s300_i2c.h"
#include <string.h>

/* WM8978 使用 9-bit register addressing 合并在 16-bit 数据中：
   I2C 写入为两字节：
   byte0: [A8:A1] + D8
   byte1: [D7:D0]
   其中 A 为 7-bit/9-bit 地址位，D 为 9-bit 数据低 9 位。
   这里沿用参考驱动的构造方法。*/

#define REGNUM_MAX 58

static uint16_t s_reg_cache[REGNUM_MAX];

static int wm8978_write_reg(const S300_WM8978_Bus *bus, uint8_t reg, uint16_t val)
{
    if (!bus || reg >= REGNUM_MAX) return -1;
    uint8_t buf[2];
    buf[0] = (uint8_t)((reg << 1) | ((val >> 8) & 0x1));
    buf[1] = (uint8_t)(val & 0xFF);
    int rc = S300_I2C_WriteBytes(bus->i2c_idx, S300_WM8978_I2C_ADDR, buf, 2, 1, 50);
    if (rc == 0) s_reg_cache[reg] = val;
    return rc;
}

static uint16_t wm8978_read_cache(uint8_t reg)
{
    if (reg >= REGNUM_MAX) return 0;
    return s_reg_cache[reg];
}

int S300_WM8978_SoftReset(const S300_WM8978_Bus *bus)
{
    return wm8978_write_reg(bus, 0x00, 0x0000);
}

int S300_WM8978_SetI2S(const S300_WM8978_Bus *bus, uint8_t fmt, uint8_t len)
{
    /* R4: (fmt<<3)|(len<<5) */
    fmt &= 0x03; len &= 0x03;
    return wm8978_write_reg(bus, 4, ((uint16_t)fmt << 3) | ((uint16_t)len << 5));
}

int S300_WM8978_SetADDA(const S300_WM8978_Bus *bus, uint8_t dac_en, uint8_t adc_en)
{
    /* R3[1:0] DACEN, R2[1:0] ADCEN */
    uint16_t r3 = wm8978_read_cache(3);
    if (dac_en) r3 |= (3u << 0); else r3 &= ~(3u << 0);
    int rc = wm8978_write_reg(bus, 3, r3);
    if (rc) return rc;
    uint16_t r2 = wm8978_read_cache(2);
    if (adc_en) r2 |= (3u << 0); else r2 &= ~(3u << 0);
    return wm8978_write_reg(bus, 2, r2);
}

int S300_WM8978_SetInput(const S300_WM8978_Bus *bus, uint8_t mic_en, uint8_t linein_en, uint8_t aux_en)
{
    /* R2[3:2] MICEN, R44 L/R boost en; linein/aux 通过 R47/R48 增益设置 */
    uint16_t r2 = wm8978_read_cache(2);
    if (mic_en) r2 |= (3u << 2); else r2 &= ~(3u << 2);
    int rc = wm8978_write_reg(bus, 2, r2);
    if (rc) return rc;

    uint16_t r44 = wm8978_read_cache(44);
    if (mic_en) r44 |= ((3u << 4) | (3u << 0)); else r44 &= ~((3u << 4) | (3u << 0));
    rc = wm8978_write_reg(bus, 44, r44);
    if (rc) return rc;

    rc = S300_WM8978_SetLineInGain(bus, linein_en ? 5 : 0);
    if (rc) return rc;
    rc = S300_WM8978_SetAuxGain(bus, aux_en ? 7 : 0);
    return rc;
}

int S300_WM8978_SetOutput(const S300_WM8978_Bus *bus, uint8_t dac_en, uint8_t bps_en)
{
    /* R50/51: bit0 DAC, bit1 BPS + mix vol bits */
    uint16_t v = 0;
    if (dac_en) v |= (1u << 0);
    if (bps_en) { v |= (1u << 1); v |= (5u << 2); }
    int rc = wm8978_write_reg(bus, 50, v);
    if (rc) return rc;
    rc = wm8978_write_reg(bus, 51, v);
    return rc;
}

int S300_WM8978_SetMicGain(const S300_WM8978_Bus *bus, uint8_t gain)
{
    gain &= 0x3F;
    int rc = wm8978_write_reg(bus, 45, gain);
    if (rc) return rc;
    return wm8978_write_reg(bus, 46, ((uint16_t)gain) | (1u << 8));
}

int S300_WM8978_SetLineInGain(const S300_WM8978_Bus *bus, uint8_t gain)
{
    gain &= 0x07;
    uint16_t r47 = wm8978_read_cache(47);
    r47 &= ~(7u << 4);
    int rc = wm8978_write_reg(bus, 47, r47 | ((uint16_t)gain << 4));
    if (rc) return rc;
    uint16_t r48 = wm8978_read_cache(48);
    r48 &= ~(7u << 4);
    return wm8978_write_reg(bus, 48, r48 | ((uint16_t)gain << 4));
}

int S300_WM8978_SetAuxGain(const S300_WM8978_Bus *bus, uint8_t gain)
{
    gain &= 0x07;
    uint16_t r47 = wm8978_read_cache(47);
    r47 &= ~(7u << 0);
    int rc = wm8978_write_reg(bus, 47, r47 | ((uint16_t)gain << 0));
    if (rc) return rc;
    uint16_t r48 = wm8978_read_cache(48);
    r48 &= ~(7u << 0);
    return wm8978_write_reg(bus, 48, r48 | ((uint16_t)gain << 0));
}

int S300_WM8978_SetHPVol(const S300_WM8978_Bus *bus, uint8_t voll, uint8_t volr)
{
    voll &= 0x3F; volr &= 0x3F;
    if (voll == 0) voll |= (1u << 6);
    if (volr == 0) volr |= (1u << 6);
    int rc = wm8978_write_reg(bus, 52, voll);
    if (rc) return rc;
    return wm8978_write_reg(bus, 53, ((uint16_t)volr) | (1u << 8));
}

int S300_WM8978_SetSPKVol(const S300_WM8978_Bus *bus, uint8_t vol)
{
    vol &= 0x3F;
    if (vol == 0) vol |= (1u << 6);
    int rc = wm8978_write_reg(bus, 54, vol);
    if (rc) return rc;
    return wm8978_write_reg(bus, 55, ((uint16_t)vol) | (1u << 8));
}

int S300_WM8978_Set3D(const S300_WM8978_Bus *bus, uint8_t depth)
{
    depth &= 0x0F;
    return wm8978_write_reg(bus, 41, depth);
}

int S300_WM8978_SetEQ3DDir(const S300_WM8978_Bus *bus, uint8_t dir)
{
    uint16_t r18 = wm8978_read_cache(0x12);
    if (dir) r18 |= (1u << 8); else r18 &= ~(1u << 8);
    return wm8978_write_reg(bus, 18, r18);
}

static int set_eqn(const S300_WM8978_Bus *bus, uint8_t reg, uint8_t cfreq, uint8_t gain)
{
    cfreq &= 0x3; if (gain > 24) gain = 24; gain = 24 - gain;
    uint16_t v = ((uint16_t)cfreq << 5) | gain;
    return wm8978_write_reg(bus, reg, v);
}

int S300_WM8978_SetEQ1(const S300_WM8978_Bus *bus, uint8_t cfreq, uint8_t gain)
{   /* 保留 R18 bit8 */
    uint16_t r18 = wm8978_read_cache(18) & 0x100u;
    cfreq &= 0x3; if (gain > 24) gain = 24; gain = 24 - gain;
    uint16_t v = r18 | ((uint16_t)cfreq << 5) | gain;
    return wm8978_write_reg(bus, 18, v);
}

int S300_WM8978_SetEQ2(const S300_WM8978_Bus *bus, uint8_t cfreq, uint8_t gain)
{ return set_eqn(bus, 19, cfreq, gain); }
int S300_WM8978_SetEQ3(const S300_WM8978_Bus *bus, uint8_t cfreq, uint8_t gain)
{ return set_eqn(bus, 20, cfreq, gain); }
int S300_WM8978_SetEQ4(const S300_WM8978_Bus *bus, uint8_t cfreq, uint8_t gain)
{ return set_eqn(bus, 21, cfreq, gain); }
int S300_WM8978_SetEQ5(const S300_WM8978_Bus *bus, uint8_t cfreq, uint8_t gain)
{ return set_eqn(bus, 22, cfreq, gain); }

int S300_WM8978_ConfigPlay(const S300_WM8978_Bus *bus, uint8_t hp_vol, uint8_t spk_vol)
{
    int rc;
    rc = S300_WM8978_SetHPVol(bus, hp_vol, hp_vol); if (rc) return rc;
    rc = S300_WM8978_SetSPKVol(bus, spk_vol); if (rc) return rc;
    rc = S300_WM8978_SetADDA(bus, 1, wm8978_read_cache(2) & 0x3); if (rc) return rc;
    return 0;
}

int S300_WM8978_ConfigRecord(const S300_WM8978_Bus *bus, uint8_t mic_gain)
{
    int rc;
    rc = S300_WM8978_SetMicGain(bus, mic_gain); if (rc) return rc;
    rc = S300_WM8978_SetADDA(bus, wm8978_read_cache(3) & 0x3, 1); if (rc) return rc;
    return 0;
}

int S300_WM8978_Init(const S300_WM8978_Bus *bus, const S300_WM8978_Config *cfg)
{
    if (!bus || !cfg) return -1;

    memset(s_reg_cache, 0, sizeof(s_reg_cache));

    /* 假设 I2C 已由上层完成 S300_I2C_Init 与目标速率配置。这里只做设备级初始化。 */
    int rc = S300_WM8978_SoftReset(bus); if (rc) return rc;

    /* 参考旧驱动初始化序列 */
    if ((rc = wm8978_write_reg(bus, 1, 0x001B))) return rc;
    if ((rc = wm8978_write_reg(bus, 2, 0x01B0))) return rc;
    if ((rc = wm8978_write_reg(bus, 3, 0x006C))) return rc;
    if ((rc = wm8978_write_reg(bus, 6, 0x0000))) return rc;
    if ((rc = wm8978_write_reg(bus, 7, 0x0000))) return rc; /* fs 16k */
    if ((rc = wm8978_write_reg(bus, 43, (1u << 4)))) return rc;
    if ((rc = wm8978_write_reg(bus, 47, (1u << 8)))) return rc;
    if ((rc = wm8978_write_reg(bus, 48, (1u << 8)))) return rc;
    if ((rc = wm8978_write_reg(bus, 49, (1u << 1)))) return rc;
    if ((rc = wm8978_write_reg(bus, 10, (1u << 3)))) return rc;
    if ((rc = wm8978_write_reg(bus, 14, (1u << 3)))) return rc;
    if ((rc = wm8978_write_reg(bus, 5, 0x0001))) return rc;

    /* 应用用户配置 */
    if ((rc = S300_WM8978_SetI2S(bus, cfg->i2s_fmt, cfg->i2s_len))) return rc;
    if ((rc = S300_WM8978_SetInput(bus, cfg->mic_en, cfg->linein_en, cfg->aux_en))) return rc;
    if ((rc = S300_WM8978_SetADDA(bus, cfg->dac_en, cfg->adc_en))) return rc;

    return 0;
}

int S300_WM8978_Deinit(const S300_WM8978_Bus *bus)
{
    (void)bus;
    return 0;
}
