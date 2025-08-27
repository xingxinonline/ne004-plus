#ifndef S300_BSP_WM8978_H
#define S300_BSP_WM8978_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "i2c_soft.h"

#define WM8978_I2C_ADDR 0x1A /* 7-bit */

/* 基本控制 API：与原始 externdevice/audio_wm8978.c 对齐的最小集合 */
void wm8978_set_addr(uint8_t saddr);
int wm8978_init(i2c_soft_t *i2c);
int wm8978_set_adda(i2c_soft_t *i2c, bool dac_en, bool adc_en);
int wm8978_set_input(i2c_soft_t *i2c, bool mic_en, bool linein_en, bool aux_en);
int wm8978_set_output(i2c_soft_t *i2c, bool dac_out_en, bool bypass_en);
int wm8978_set_mic_gain(i2c_soft_t *i2c, uint8_t gain_0_63);
int wm8978_set_linein_gain(i2c_soft_t *i2c, uint8_t gain_0_7);
int wm8978_set_aux_gain(i2c_soft_t *i2c, uint8_t gain_0_7);
int wm8978_i2s_cfg(i2c_soft_t *i2c, uint8_t fmt_2b, uint8_t len_2b);
int wm8978_set_hp_vol(i2c_soft_t *i2c, uint8_t vol_l_0_63, uint8_t vol_r_0_63);
int wm8978_set_spk_vol(i2c_soft_t *i2c, uint8_t vol_0_63);

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_WM8978_H */
