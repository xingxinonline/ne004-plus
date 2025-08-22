#ifndef S300_WM8978_H
#define S300_WM8978_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备 7-bit I2C 地址 */
#define S300_WM8978_I2C_ADDR  0x1A

/* I2C 控制器索引：与板级配套传入，或在 Board 层提供默认宏 */
typedef struct {
    uint32_t i2c_idx;        /* S300 I2C 控制器索引 */
    uint8_t  i2c_memaddr_16; /* 是否使用 16 位寄存器地址：WM8978 为 9 位(组合写)，此处统一走专用格式 */
} S300_WM8978_Bus;

/* 基本配置 */
typedef struct {
    uint8_t i2s_fmt;   /* 0..3: I2S/左对齐/右对齐/PCM 等（由外设寄存器 R4 定义） */
    uint8_t i2s_len;   /* 0..3: 16/20/24/32bit */
    uint8_t mic_en;    /* 开启 MIC 路径 */
    uint8_t linein_en; /* 开启 LINEIN 路径 */
    uint8_t aux_en;    /* 开启 AUX 路径 */
    uint8_t dac_en;    /* 开启 DAC 输出 */
    uint8_t adc_en;    /* 开启 ADC 输入 */
} S300_WM8978_Config;

/* 初始化 / 去初始化 */
int S300_WM8978_Init(const S300_WM8978_Bus *bus, const S300_WM8978_Config *cfg);
int S300_WM8978_Deinit(const S300_WM8978_Bus *bus);

/* 基础控制接口（参考旧驱动能力，BSP 风格重构） */
int S300_WM8978_SoftReset(const S300_WM8978_Bus *bus);
int S300_WM8978_SetI2S(const S300_WM8978_Bus *bus, uint8_t fmt, uint8_t len);
int S300_WM8978_SetADDA(const S300_WM8978_Bus *bus, uint8_t dac_en, uint8_t adc_en);
int S300_WM8978_SetInput(const S300_WM8978_Bus *bus, uint8_t mic_en, uint8_t linein_en, uint8_t aux_en);
int S300_WM8978_SetOutput(const S300_WM8978_Bus *bus, uint8_t dac_en, uint8_t bps_en);
int S300_WM8978_SetMicGain(const S300_WM8978_Bus *bus, uint8_t gain_0_63);
int S300_WM8978_SetLineInGain(const S300_WM8978_Bus *bus, uint8_t gain_0_7);
int S300_WM8978_SetAuxGain(const S300_WM8978_Bus *bus, uint8_t gain_0_7);
int S300_WM8978_SetHPVol(const S300_WM8978_Bus *bus, uint8_t volL_0_63, uint8_t volR_0_63);
int S300_WM8978_SetSPKVol(const S300_WM8978_Bus *bus, uint8_t vol_0_63);
int S300_WM8978_Set3D(const S300_WM8978_Bus *bus, uint8_t depth_0_15);
int S300_WM8978_SetEQ3DDir(const S300_WM8978_Bus *bus, uint8_t dir);
int S300_WM8978_SetEQ1(const S300_WM8978_Bus *bus, uint8_t cfreq_0_3, uint8_t gain_0_24);
int S300_WM8978_SetEQ2(const S300_WM8978_Bus *bus, uint8_t cfreq_0_3, uint8_t gain_0_24);
int S300_WM8978_SetEQ3(const S300_WM8978_Bus *bus, uint8_t cfreq_0_3, uint8_t gain_0_24);
int S300_WM8978_SetEQ4(const S300_WM8978_Bus *bus, uint8_t cfreq_0_3, uint8_t gain_0_24);
int S300_WM8978_SetEQ5(const S300_WM8978_Bus *bus, uint8_t cfreq_0_3, uint8_t gain_0_24);

/* 高级：一体化播放/录音默认配置 */
int S300_WM8978_ConfigPlay(const S300_WM8978_Bus *bus, uint8_t hp_vol, uint8_t spk_vol);
int S300_WM8978_ConfigRecord(const S300_WM8978_Bus *bus, uint8_t mic_gain);

#ifdef __cplusplus
}
#endif

#endif /* S300_WM8978_H */
