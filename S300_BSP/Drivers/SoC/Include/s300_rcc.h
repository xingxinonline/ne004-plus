/*
 * S300 RCC Driver (Clock & Reset Control)
 *
 * Provides: clock source selection, gate enable/disable, reset control,
 * PLL programming, and common bus/peripheral clock dividers.
 *
 * Notes per spec:
 * - Clock gate bits: 1 = enable, 0 = disable.
 * - Reset control bits: 1 = deassert reset (released), 0 = in reset.
 * - APB clock dividers live in one 32-bit register with 4-bit fields.
 */

#ifndef S300_RCC_H
#define S300_RCC_H

#include <stdint.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Base addresses (DSP RCC base not in s300.h) ===== */
#ifndef DSP_RCC_BASE
#define DSP_RCC_BASE (0x44080000UL)
#endif

/* ===== CM4 RCC register offsets ===== */
enum {
    S300_RCC_OFS_SYS_CLK_SEL   = 0x00,
    S300_RCC_OFS_SYS_CLK_EN    = 0x04,
    S300_RCC_OFS_APB0_CLK_EN   = 0x08,
    S300_RCC_OFS_APB1_CLK_EN   = 0x0C,
    S300_RCC_OFS_AHB_CLK_EN    = 0x10,
    S300_RCC_OFS_APB_CLK_DIV   = 0x14,
    S300_RCC_OFS_SYS_SOFT_RSTN = 0x18,
    S300_RCC_OFS_SYS_RST_CTL   = 0x1C,
    S300_RCC_OFS_APB0_RST_CTL  = 0x20,
    S300_RCC_OFS_APB1_RST_CTL  = 0x24,
    S300_RCC_OFS_AHB_RST_CTL   = 0x28,
    S300_RCC_OFS_AUD_CLK_EN    = 0x2C,
    S300_RCC_OFS_AUD_RST_CTL   = 0x30,
    S300_RCC_OFS_MM_CLK_EN     = 0x34,
    S300_RCC_OFS_MM_RST_CTL    = 0x38,
    S300_RCC_OFS_WDT3_CTL      = 0x3C,
    S300_RCC_OFS_TW_CLK_CTL    = 0x40,
    S300_RCC_OFS_MEM_CLK_CTL   = 0x44,
    S300_RCC_OFS_MEM_RST_CTL   = 0x48,
    S300_RCC_OFS_CM4_PLL_CTL1  = 0x4C,
    S300_RCC_OFS_CM4_PLL_CTL2  = 0x50,
    S300_RCC_OFS_AUD_PLL_CTL1  = 0x54,
    S300_RCC_OFS_AUD_PLL_CTL2  = 0x58,
    /* 0x5C/0x60 were MM PLL in early revisions; moved to DSP RCC in v0.4.8+ */
    S300_RCC_OFS_MM_PLL_CTL1   = 0x5C, /* deprecated */
    S300_RCC_OFS_MM_PLL_CTL2   = 0x60, /* deprecated */
    S300_RCC_OFS_I2S_CLK_DIV   = 0x64,
    S300_RCC_OFS_ETH_PLL_CTL1  = 0x68,
    S300_RCC_OFS_ETH_PLL_CTL2  = 0x6C,
    S300_RCC_OFS_ETH_CLK_DIV   = 0x70,
    S300_RCC_OFS_PLL_LOCK_STS  = 0x74,
    S300_RCC_OFS_SDIO0_DIV     = 0x78,
    S300_RCC_OFS_SDIO1_DIV     = 0x7C,
    S300_RCC_OFS_SDIO_CLK_SEL  = 0x80,
    S300_RCC_OFS_CM4_WARMRST_STS = 0x84
};

/* ===== DSP RCC register offsets ===== */
enum {
    S300_DSP_RCC_OFS_SYS_CLK_EN   = 0x00,
    S300_DSP_RCC_OFS_PERF_CLK_EN  = 0x04,
    S300_DSP_RCC_OFS_SYS_RST_CTL  = 0x08,
    S300_DSP_RCC_OFS_PERF_RST_CTL = 0x0C,
    S300_DSP_RCC_OFS_WARM_RSTN    = 0x10,
    S300_DSP_RCC_OFS_SYS_CLK_SEL  = 0x14,
    S300_DSP_RCC_OFS_CEVA_RST_CTL = 0x18,
    S300_DSP_RCC_OFS_PLL_CTL1     = 0x1C,
    S300_DSP_RCC_OFS_PLL_CTL2     = 0x20,
    S300_DSP_RCC_OFS_PLOCK_STS    = 0x24,
    S300_DSP_RCC_OFS_CEVA_STATUS  = 0x28,
    S300_DSP_RCC_OFS_PIM_CLK_DIV  = 0x2C,
    S300_DSP_RCC_OFS_NPU_CLK_DIV  = 0x30,
    S300_DSP_RCC_OFS_MM_PLL_CTL1  = 0x34,
    S300_DSP_RCC_OFS_MM_PLL_CTL2  = 0x38,
    S300_DSP_RCC_OFS_MM_CLK_EN    = 0x3C,
    S300_DSP_RCC_OFS_MM_RST_CTL   = 0x40,
    S300_DSP_RCC_OFS_WARMRST_STS  = 0x44,
};

/* ===== Register access helpers ===== */
#define S300_REG32(base, ofs)   (*(volatile uint32_t *)((base) + (uint32_t)(ofs)))

/* CM4 RCC registers */
#define S300_RCC_REG(ofs)       S300_REG32(RCC_BASE, (ofs))
/* DSP RCC registers */
#define S300_DSP_RCC_REG(ofs)   S300_REG32(DSP_RCC_BASE, (ofs))

/* ===== Sys_clk_sel fields ===== */
typedef enum {
    S300_RCC_SYSCLK_SRC_HSE = 1,       /* 2'b01 */
    S300_RCC_SYSCLK_SRC_PLL = 2,       /* 2'b10 */
} S300_RCC_SysClkSrc;

typedef enum {
    S300_RCC_PWMCLK_SRC_HSE = 0,       /* 2'b00/11 => HSE */
    S300_RCC_PWMCLK_SRC_CM4_PLL = 1,   /* 2'b01 */
    S300_RCC_PWMCLK_SRC_IO = 2,        /* 2'b10 */
} S300_RCC_PwmClkSrc;

/* Bit positions in Sys_clk_sel */
#define S300_RCC_SYSSEL_CM4_Pos   0
#define S300_RCC_SYSSEL_AUDIO_Pos 2
#define S300_RCC_SYSSEL_MM_Pos    4
#define S300_RCC_SYSSEL_PWM_Pos   6

/* ===== Peripheral bit masks (per reference table) ===== */
typedef enum {
    /* APB0 */
    S300_APB0_TIMER0        = (1u << 0),
    S300_APB0_TIMER1        = (1u << 1),
    S300_APB0_TIMER2        = (1u << 2),
    S300_APB0_WDT0          = (1u << 3),
    S300_APB0_WDT1          = (1u << 4),
    S300_APB0_WDT2          = (1u << 5),
    S300_APB0_WDT3          = (1u << 6),
    S300_APB0_INT_CTRL      = (1u << 7),
    S300_APB0_IO_MATRIX     = (1u << 8),
    S300_APB0_IO_MUX        = (1u << 9),
    /* 10,11 reserved */
    S300_APB0_SCTRL         = (1u << 12),
    S300_APB0_QSPI_CTRL     = (1u << 13),
    S300_APB0_DVP_REGS      = (1u << 14),
} S300_RCC_APB0Mask;

typedef enum {
    /* APB1 */
    S300_APB1_UART0         = (1u << 0),
    S300_APB1_UART1         = (1u << 1),
    S300_APB1_UART2         = (1u << 2),
    S300_APB1_UART3         = (1u << 3),
    S300_APB1_I2C0          = (1u << 4),
    S300_APB1_I2C1          = (1u << 5),
    S300_APB1_I2C2          = (1u << 6),
    S300_APB1_I2C3          = (1u << 7),
    S300_APB1_GPIO          = (1u << 8),
    S300_APB1_MBOX_PCLK0    = (1u << 9),
    /* 10 reserved */
    S300_APB1_I2S0          = (1u << 11),
    S300_APB1_I2S1          = (1u << 12),
    S300_APB1_PWM           = (1u << 13),
} S300_RCC_APB1Mask;

typedef enum {
    /* AHB */
    S300_AHB_GMAC           = (1u << 4),
    S300_AHB_SEC_OPT_RAM4K  = (1u << 5),
    S300_AHB_PSRAM          = (1u << 6),
    S300_AHB_QSPI_AHB       = (1u << 7),
    S300_AHB_SPI0           = (1u << 8),
    S300_AHB_SPI1           = (1u << 9),
    S300_AHB_DVP_LCD        = (1u << 10),
    /* 11 reserved */
    S300_AHB_SDIO0          = (1u << 12),
    S300_AHB_SDIO2          = (1u << 13),
} S300_RCC_AHBMask;

/* ===== Public API ===== */

/* System clock source selects */
void S300_RCC_SetCM4SysClk(S300_RCC_SysClkSrc src);
void S300_RCC_SetAudioSysClk(S300_RCC_SysClkSrc src);
void S300_RCC_SetMmSysClk(S300_RCC_SysClkSrc src);
void S300_RCC_SetPwmClk(S300_RCC_PwmClkSrc src);

/* Bus/peripheral gates */
void S300_RCC_EnableAPB0Mask(uint32_t mask);
void S300_RCC_DisableAPB0Mask(uint32_t mask);
void S300_RCC_EnableAPB1Mask(uint32_t mask);
void S300_RCC_DisableAPB1Mask(uint32_t mask);
void S300_RCC_EnableAHBMask(uint32_t mask);
void S300_RCC_DisableAHBMask(uint32_t mask);

/* Reset controls (1 = released, 0 = in reset) */
void S300_RCC_ReleaseAPB0ResetBits(uint32_t mask);
void S300_RCC_AssertAPB0ResetBits(uint32_t mask);
void S300_RCC_ReleaseAPB1ResetBits(uint32_t mask);
void S300_RCC_AssertAPB1ResetBits(uint32_t mask);
void S300_RCC_ReleaseAHBResetBits(uint32_t mask);
void S300_RCC_AssertAHBResetBits(uint32_t mask);

/* APB clock dividers (num: 0=APB0, 1=APB1), div: 4-bit field) */
void S300_RCC_SetAPBClkDiv(uint8_t num, uint8_t div);

/* Audio/I2S divider (4-bit) */
void S300_RCC_SetI2SClkDiv(uint8_t div);

/* ETH clock divider (width not specified; use lower 8 bits) */
void S300_RCC_SetEthClkDiv(uint8_t div);

/* SDIO clock config helpers */
void S300_RCC_SetSDIOClkDiv(uint8_t num /*0 or 1*/, uint8_t div);
void S300_RCC_SelectSDIOClk(uint8_t sel /* per design */);

/* Memory clock & resets */
void S300_RCC_SetMemClock(uint8_t bus, uint8_t rom, uint8_t sram0, uint8_t sram1, uint8_t enable);
void S300_RCC_SetMemReset(uint8_t bus, uint8_t sram0, uint8_t sram1, uint8_t reset);

/* PLL programming (generic and typed wrappers). All fields per spec: */
typedef struct {
    uint16_t refdiv;   /* 6 bits */
    uint16_t fbdiv;    /* 12 bits */
    uint32_t frac;     /* 24 bits */
    uint8_t  postdiv1; /* 3 bits */
    uint8_t  postdiv2; /* 3 bits */
} S300_RCC_PllCfg;

/* Program a CM4/AUDIO/ETH PLL in CM4 RCC block */
void S300_RCC_ProgramPll_CM4(uint32_t ctl1_ofs, uint32_t ctl2_ofs, const S300_RCC_PllCfg *cfg);
/* Convenience wrappers */
static inline void S300_RCC_ConfigCM4PLL(const S300_RCC_PllCfg *cfg) { S300_RCC_ProgramPll_CM4(S300_RCC_OFS_CM4_PLL_CTL1, S300_RCC_OFS_CM4_PLL_CTL2, cfg); }
static inline void S300_RCC_ConfigAudioPLL(const S300_RCC_PllCfg *cfg) { S300_RCC_ProgramPll_CM4(S300_RCC_OFS_AUD_PLL_CTL1, S300_RCC_OFS_AUD_PLL_CTL2, cfg); }
static inline void S300_RCC_ConfigEthPLL(const S300_RCC_PllCfg *cfg) { S300_RCC_ProgramPll_CM4(S300_RCC_OFS_ETH_PLL_CTL1, S300_RCC_OFS_ETH_PLL_CTL2, cfg); }

/* DSP block PLL */
void S300_DSP_RCC_ProgramPll(const S300_RCC_PllCfg *cfg);
void S300_DSP_RCC_ProgramPll_At(uint32_t ctl1_ofs, uint32_t ctl2_ofs, const S300_RCC_PllCfg *cfg);
/* Modern MM PLL lives under DSP RCC at 0x34/0x38 */
static inline void S300_RCC_ConfigMMPLL(const S300_RCC_PllCfg *cfg) { S300_DSP_RCC_ProgramPll_At(S300_DSP_RCC_OFS_MM_PLL_CTL1, S300_DSP_RCC_OFS_MM_PLL_CTL2, cfg); }

/* DSP PIM/NPU clock dividers (5-bit) */
void S300_DSP_RCC_SetPIMClkDiv(uint8_t div);
void S300_DSP_RCC_SetNPUClkDiv(uint8_t div);

/* DSP clock selection */
void S300_DSP_RCC_SetSysClk(S300_RCC_SysClkSrc src);
void S300_DSP_RCC_SetMMClk(S300_RCC_SysClkSrc src);

/* DSP gates and resets */
void S300_DSP_RCC_EnableSysMask(uint32_t mask);
void S300_DSP_RCC_DisableSysMask(uint32_t mask);
void S300_DSP_RCC_ReleaseSysReset(uint32_t mask);
void S300_DSP_RCC_AssertSysReset(uint32_t mask);

/* ===== PLL lock status ===== */
/* Wait on CM4 PLL lock status register (0x74). Caller provides bit mask.
 * Returns 0 on success (locked), non-zero on timeout. */
int S300_RCC_WaitPllLock(uint32_t lock_mask, uint32_t timeout_cycles);

/* Wait on DSP PLL lock status (DSP RCC 0x24). Same contract as above. */
int S300_DSP_RCC_WaitPllLock(uint32_t lock_mask, uint32_t timeout_cycles);

/* Lock status bits (aligned with original demo definitions): */
#define S300_RCC_PLL_LOCK_CM4    (1u << 0)
#define S300_RCC_PLL_LOCK_AUDIO  (1u << 1)
#define S300_RCC_PLL_LOCK_ETH    (1u << 2)

/* PLL control helpers (only what we need for audio to mirror demo flow) */
/* Set or clear BYPASS bit (bit 25) in AUD_PLL_CTL2 */
void S300_RCC_SetAudioPllBypass(uint8_t enable);

/* ===== Audio/MM performance clock and reset generic helpers (mask based) ===== */
void S300_RCC_EnableAudioPerfMask(uint32_t mask);
void S300_RCC_DisableAudioPerfMask(uint32_t mask);
void S300_RCC_ReleaseAudioResetMask(uint32_t mask);
void S300_RCC_AssertAudioResetMask(uint32_t mask);

void S300_RCC_EnableMMPerfMask(uint32_t mask);
void S300_RCC_DisableMMPerfMask(uint32_t mask);
void S300_RCC_ReleaseMMResetMask(uint32_t mask);
void S300_RCC_AssertMMResetMask(uint32_t mask);

/* ===== PLL frequency helper ===== */
/* Compute PLL output frequency (Hz) based on cfg and reference HSE */
uint32_t S300_RCC_CalcPLLFreq(const S300_RCC_PllCfg *cfg, uint32_t hse_hz);

/* Some typical presets from documentation (24MHz HSE assumed) */
extern const S300_RCC_PllCfg S300_PLL_PRESET_CM4_384M; /* 3072/ (2*2) = 384MHz */
extern const S300_RCC_PllCfg S300_PLL_PRESET_CM4_192M;
extern const S300_RCC_PllCfg S300_PLL_PRESET_CM4_160M;
extern const S300_RCC_PllCfg S300_PLL_PRESET_CM4_120M;
extern const S300_RCC_PllCfg S300_PLL_PRESET_CM4_100M;
extern const S300_RCC_PllCfg S300_PLL_PRESET_AUDIO_12M;
extern const S300_RCC_PllCfg S300_PLL_PRESET_MM_100M;

/* ===== Backward-compatible helpers used by existing BSP code ===== */
/* Enable APB0 clock bits */
static inline void S300_RCC_EnableAPB0(uint32_t mask) { S300_RCC_EnableAPB0Mask(mask); }
/* Enable APB1 clock bits */
static inline void S300_RCC_EnableAPB1(uint32_t mask) { S300_RCC_EnableAPB1Mask(mask); }
/* Release APB1 reset (set bit = deassert reset) */
static inline void S300_RCC_ReleaseAPB1Reset(uint32_t mask) { S300_RCC_ReleaseAPB1ResetBits(mask); }

#ifdef __cplusplus
}
#endif

#endif /* S300_RCC_H */
