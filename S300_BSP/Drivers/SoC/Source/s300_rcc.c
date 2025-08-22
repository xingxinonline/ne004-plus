/*
 * S300 RCC Driver implementation
 */

#include "s300_rcc.h"

/* ===== Local helpers ===== */
static inline void set_bits(volatile uint32_t *reg, uint32_t mask) { *reg |= mask; }
static inline void clr_bits(volatile uint32_t *reg, uint32_t mask) { *reg &= ~mask; }

/* ===== System clock selection ===== */
static void set_clksel_field(uint32_t pos, uint32_t val)
{
	volatile uint32_t *r = &S300_RCC_REG(S300_RCC_OFS_SYS_CLK_SEL);
	uint32_t v = *r;
	v &= ~(0x3u << pos);
	v |= ((val & 0x3u) << pos);
	*r = v;
}

void S300_RCC_SetCM4SysClk(S300_RCC_SysClkSrc src)
{
	set_clksel_field(S300_RCC_SYSSEL_CM4_Pos, (uint32_t)src);
}

void S300_RCC_SetAudioSysClk(S300_RCC_SysClkSrc src)
{
	set_clksel_field(S300_RCC_SYSSEL_AUDIO_Pos, (uint32_t)src);
}

void S300_RCC_SetMmSysClk(S300_RCC_SysClkSrc src)
{
	/* Spec indicates mm_clk_sel at [5:4]; use same encoding as CM4 */
	set_clksel_field(S300_RCC_SYSSEL_MM_Pos, (uint32_t)src);
}

void S300_RCC_SetPwmClk(S300_RCC_PwmClkSrc src)
{
	set_clksel_field(S300_RCC_SYSSEL_PWM_Pos, (uint32_t)src);
}

/* ===== Gates ===== */
void S300_RCC_EnableAPB0Mask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_APB0_CLK_EN), mask);
}

void S300_RCC_DisableAPB0Mask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_APB0_CLK_EN), mask);
}

void S300_RCC_EnableAPB1Mask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_APB1_CLK_EN), mask);
}

void S300_RCC_DisableAPB1Mask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_APB1_CLK_EN), mask);
}

void S300_RCC_EnableAHBMask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_AHB_CLK_EN), mask);
}

void S300_RCC_DisableAHBMask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_AHB_CLK_EN), mask);
}

/* ===== Reset controls (1 = released) ===== */
void S300_RCC_ReleaseAPB0ResetBits(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_APB0_RST_CTL), mask);
}

void S300_RCC_AssertAPB0ResetBits(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_APB0_RST_CTL), mask);
}

void S300_RCC_ReleaseAPB1ResetBits(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_APB1_RST_CTL), mask);
}

void S300_RCC_AssertAPB1ResetBits(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_APB1_RST_CTL), mask);
}

void S300_RCC_ReleaseAHBResetBits(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_AHB_RST_CTL), mask);
}

void S300_RCC_AssertAHBResetBits(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_AHB_RST_CTL), mask);
}

/* ===== Dividers ===== */
void S300_RCC_SetAPBClkDiv(uint8_t num, uint8_t div)
{
	volatile uint32_t *r = &S300_RCC_REG(S300_RCC_OFS_APB_CLK_DIV);
	uint32_t v = *r;
	uint32_t shift = (num == 0u) ? 0u : 4u;
	v &= ~(0xFu << shift);
	v |= ((uint32_t)(div & 0xFu) << shift);
	*r = v;
}

void S300_RCC_SetI2SClkDiv(uint8_t div)
{
	S300_RCC_REG(S300_RCC_OFS_I2S_CLK_DIV) = (uint32_t)(div & 0xFu);
}

void S300_RCC_SetEthClkDiv(uint8_t div)
{
	S300_RCC_REG(S300_RCC_OFS_ETH_CLK_DIV) = (uint32_t)div;
}

void S300_RCC_SetSDIOClkDiv(uint8_t num, uint8_t div)
{
	uint32_t ofs = (num == 0u) ? S300_RCC_OFS_SDIO0_DIV : S300_RCC_OFS_SDIO1_DIV;
	S300_RCC_REG(ofs) = (uint32_t)div;
}

void S300_RCC_SelectSDIOClk(uint8_t sel)
{
	S300_RCC_REG(S300_RCC_OFS_SDIO_CLK_SEL) = (uint32_t)sel;
}

/* ===== Memory clock & reset ===== */
void S300_RCC_SetMemClock(uint8_t bus, uint8_t rom, uint8_t sram0, uint8_t sram1, uint8_t enable)
{
	uint32_t v = 0;
	if (bus)   v |= (1u << 3);
	if (rom)   v |= (1u << 2);
	if (sram1) v |= (1u << 1);
	if (sram0) v |= (1u << 0);
	if (!enable) v = 0; /* if overall disable requested, clear */
	S300_RCC_REG(S300_RCC_OFS_MEM_CLK_CTL) = v;
}

void S300_RCC_SetMemReset(uint8_t bus, uint8_t sram0, uint8_t sram1, uint8_t reset)
{
	uint32_t v = 0;
	if (!reset) {
		/* keep in reset: write zeros */
		S300_RCC_REG(S300_RCC_OFS_MEM_RST_CTL) = 0;
		return;
	}
	if (bus)   v |= (1u << 2);
	if (sram1) v |= (1u << 1);
	if (sram0) v |= (1u << 0);
	S300_RCC_REG(S300_RCC_OFS_MEM_RST_CTL) = v;
}

/* ===== PLL programming ===== */
static void program_pll(uint32_t ctl1_addr, uint32_t ctl2_addr, const S300_RCC_PllCfg *cfg)
{
	/* ctl1: [29:24] REFDIV, [23:0] FRAC */
	uint32_t ctl1 = ((uint32_t)(cfg->refdiv & 0x3Fu) << 24) |
					((uint32_t)(cfg->frac & 0xFFFFFFu));
	/* ctl2: [31] power_down, [17:15] POSTDIV2, [14:12] POSTDIV1, [11:0] FBDIV
	 * We set power_down=0, other PD bits=0, BYPASS=0 by writing only fields we need.
	 */
	uint32_t ctl2 = ((uint32_t)(cfg->postdiv2 & 0x7u) << 15) |
					((uint32_t)(cfg->postdiv1 & 0x7u) << 12) |
					((uint32_t)(cfg->fbdiv & 0xFFFu));
	S300_REG32(RCC_BASE, ctl1_addr) = ctl1;
	S300_REG32(RCC_BASE, ctl2_addr) = ctl2;
}

void S300_RCC_ProgramPll_CM4(uint32_t ctl1_ofs, uint32_t ctl2_ofs, const S300_RCC_PllCfg *cfg)
{
	program_pll(ctl1_ofs, ctl2_ofs, cfg);
}

void S300_DSP_RCC_ProgramPll(const S300_RCC_PllCfg *cfg)
{
	/* DSP block at DSP_RCC_BASE */
	uint32_t ctl1_addr = S300_DSP_RCC_OFS_PLL_CTL1;
	uint32_t ctl2_addr = S300_DSP_RCC_OFS_PLL_CTL2;
	/* ctl1: REFDIV+FRAC, ctl2: POSTDIVs+FBDIV */
	uint32_t ctl1 = ((uint32_t)(cfg->refdiv & 0x3Fu) << 24) |
					((uint32_t)(cfg->frac & 0xFFFFFFu));
	uint32_t ctl2 = ((uint32_t)(cfg->postdiv2 & 0x7u) << 15) |
					((uint32_t)(cfg->postdiv1 & 0x7u) << 12) |
					((uint32_t)(cfg->fbdiv & 0xFFFu));
	S300_DSP_RCC_REG(ctl1_addr) = ctl1;
	S300_DSP_RCC_REG(ctl2_addr) = ctl2;
}

void S300_DSP_RCC_ProgramPll_At(uint32_t ctl1_ofs, uint32_t ctl2_ofs, const S300_RCC_PllCfg *cfg)
{
	uint32_t ctl1 = ((uint32_t)(cfg->refdiv & 0x3Fu) << 24) |
					((uint32_t)(cfg->frac & 0xFFFFFFu));
	uint32_t ctl2 = ((uint32_t)(cfg->postdiv2 & 0x7u) << 15) |
					((uint32_t)(cfg->postdiv1 & 0x7u) << 12) |
					((uint32_t)(cfg->fbdiv & 0xFFFu));
	S300_DSP_RCC_REG(ctl1_ofs) = ctl1;
	S300_DSP_RCC_REG(ctl2_ofs) = ctl2;
}

/* ===== DSP Dividers ===== */
void S300_DSP_RCC_SetPIMClkDiv(uint8_t div)
{
	uint32_t v = (uint32_t)(div & 0x1Fu);
	S300_DSP_RCC_REG(S300_DSP_RCC_OFS_PIM_CLK_DIV) = v;
}

void S300_DSP_RCC_SetNPUClkDiv(uint8_t div)
{
	uint32_t v = (uint32_t)(div & 0x1Fu);
	S300_DSP_RCC_REG(S300_DSP_RCC_OFS_NPU_CLK_DIV) = v;
}

/* ===== DSP CLK SEL ===== */
static void dsp_set_clksel(uint32_t pos, uint32_t val)
{
	volatile uint32_t *r = &S300_DSP_RCC_REG(S300_DSP_RCC_OFS_SYS_CLK_SEL);
	uint32_t v = *r;
	v &= ~(0x3u << pos);
	v |= ((val & 0x3u) << pos);
	*r = v;
}

void S300_DSP_RCC_SetSysClk(S300_RCC_SysClkSrc src)
{
	dsp_set_clksel(0, (uint32_t)src);
}

void S300_DSP_RCC_SetMMClk(S300_RCC_SysClkSrc src)
{
	dsp_set_clksel(8, (uint32_t)src);
}

/* ===== DSP gates & resets ===== */
void S300_DSP_RCC_EnableSysMask(uint32_t mask)
{
	set_bits(&S300_DSP_RCC_REG(S300_DSP_RCC_OFS_SYS_CLK_EN), mask);
}

void S300_DSP_RCC_DisableSysMask(uint32_t mask)
{
	clr_bits(&S300_DSP_RCC_REG(S300_DSP_RCC_OFS_SYS_CLK_EN), mask);
}

void S300_DSP_RCC_ReleaseSysReset(uint32_t mask)
{
	set_bits(&S300_DSP_RCC_REG(S300_DSP_RCC_OFS_SYS_RST_CTL), mask);
}

void S300_DSP_RCC_AssertSysReset(uint32_t mask)
{
	clr_bits(&S300_DSP_RCC_REG(S300_DSP_RCC_OFS_SYS_RST_CTL), mask);
}

/* ===== Audio/MM perf gates & resets ===== */
void S300_RCC_EnableAudioPerfMask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_AUD_CLK_EN), mask);
}

void S300_RCC_DisableAudioPerfMask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_AUD_CLK_EN), mask);
}

void S300_RCC_ReleaseAudioResetMask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_AUD_RST_CTL), mask);
}

void S300_RCC_AssertAudioResetMask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_AUD_RST_CTL), mask);
}

void S300_RCC_EnableMMPerfMask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_MM_CLK_EN), mask);
}

void S300_RCC_DisableMMPerfMask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_MM_CLK_EN), mask);
}

void S300_RCC_ReleaseMMResetMask(uint32_t mask)
{
	set_bits(&S300_RCC_REG(S300_RCC_OFS_MM_RST_CTL), mask);
}

void S300_RCC_AssertMMResetMask(uint32_t mask)
{
	clr_bits(&S300_RCC_REG(S300_RCC_OFS_MM_RST_CTL), mask);
}

/* ===== Lock wait ===== */
int S300_RCC_WaitPllLock(uint32_t lock_mask, uint32_t timeout_cycles)
{
	while (((S300_RCC_REG(S300_RCC_OFS_PLL_LOCK_STS) & lock_mask) != lock_mask)) {
		if (timeout_cycles-- == 0u) return -1;
	}
	return 0;
}

int S300_DSP_RCC_WaitPllLock(uint32_t lock_mask, uint32_t timeout_cycles)
{
	while (((S300_DSP_RCC_REG(S300_DSP_RCC_OFS_PLOCK_STS) & lock_mask) != lock_mask)) {
		if (timeout_cycles-- == 0u) return -1;
	}
	return 0;
}

/* ===== PLL frequency helper ===== */
uint32_t S300_RCC_CalcPLLFreq(const S300_RCC_PllCfg *cfg, uint32_t hse_hz)
{
	/* Fvco = (HSE / REFDIV) * (FBDIV + FRAC/2^24)
	   Fout = Fvco / (POSTDIV1 * POSTDIV2)
	 */
	if ((cfg->refdiv == 0u) || (cfg->postdiv1 == 0u) || (cfg->postdiv2 == 0u)) {
		return 0u;
	}
	double ref = (double)hse_hz / (double)cfg->refdiv;
	double fb  = (double)cfg->fbdiv + ((double)cfg->frac / 16777216.0); /* 2^24 */
	double fvco = ref * fb;
	double fout = fvco / ((double)cfg->postdiv1 * (double)cfg->postdiv2);
	if (fout <= 0.0 || fout > 1e10) return 0u;
	return (uint32_t)(fout + 0.5);
}

/* ===== Presets (24MHz HSE) ===== */
const S300_RCC_PllCfg S300_PLL_PRESET_CM4_384M = { .refdiv=6, .fbdiv=768, .frac=0, .postdiv1=2, .postdiv2=2 };
const S300_RCC_PllCfg S300_PLL_PRESET_CM4_192M = { .refdiv=6, .fbdiv=768, .frac=0, .postdiv1=4, .postdiv2=2 };
const S300_RCC_PllCfg S300_PLL_PRESET_CM4_160M = { .refdiv=6, .fbdiv=640, .frac=0, .postdiv1=4, .postdiv2=2 };
const S300_RCC_PllCfg S300_PLL_PRESET_CM4_120M = { .refdiv=8, .fbdiv=640, .frac=0, .postdiv1=4, .postdiv2=2 };
const S300_RCC_PllCfg S300_PLL_PRESET_CM4_100M = { .refdiv=8, .fbdiv=400, .frac=0, .postdiv1=3, .postdiv2=2 };
const S300_RCC_PllCfg S300_PLL_PRESET_AUDIO_12M = { .refdiv=3, .fbdiv=129, .frac=500000, .postdiv1=7, .postdiv2=6 };
const S300_RCC_PllCfg S300_PLL_PRESET_MM_100M = { .refdiv=8, .fbdiv=400, .frac=0, .postdiv1=3, .postdiv2=2 };

