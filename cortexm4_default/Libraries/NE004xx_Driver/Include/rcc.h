#ifndef _RCC_H_
#define _RCC_H_

#include "ne004xx.h"
/***
 * init pll .
 * int init_cortex_m4_pll(stPLLPRO pro);
 * int init_audio_pll(stPLLPRO pro);
 * int init_ether_pll(stPLLPRO pro);
 * int init_mm_pll(stPLLPRO pro);
 * int init_dsp_pll(stPLLPRO pro);
 * init parameter pro see stPLLPRO
 * eg:PLL supports floating point and integer frequency division, the former is 
 * relatively high precision, the example is given is the integer calculation 
 * process, at this time FRAC=0
 * PLL floating point calculations section reference manual
 * 
 * refclock = 24MHz
 * 800MHz <= fvco <= 3200MHz
 * POSTDIV2 <= POSTDIV1
 * fvco = (refclock * FBDIV)/REFDIV
 * foutclock = fvco/(POSTDIV2 * POSTDIV1 * 2)
 * 
 */

// #define CM4_SUBSYSTEM			    (*((volatile uint32_t*)(RCC_BASE + 0x0000))) // 
#define CM4_SYS_CLK_SEL_REG			    (*((volatile uint32_t*)(RCC_BASE + 0x0000))) // RW 
#define CM4_SYS_CLK_EN_REG			    (*((volatile uint32_t*)(RCC_BASE + 0x0004))) // RW 
#define CM4_APB0_CLK_EN_REG 			(*((volatile uint32_t*)(RCC_BASE + 0x0008))) // RW 
#define CM4_APB1_CLK_EN_REG 			(*((volatile uint32_t*)(RCC_BASE + 0x000C))) // RW 
#define CM4_AHB_CLK_EN_REG 			    (*((volatile uint32_t*)(RCC_BASE + 0x0010))) // RW 
#define CM4_APB_CLK_DIV_REG 			(*((volatile uint32_t*)(RCC_BASE + 0x0014))) // RW 
#define CM4_SYS_SOFT_RSTN_REG 			(*((volatile uint32_t*)(RCC_BASE + 0x0018))) // RW 
#define CM4_SYS_RST_CTL_REG			    (*((volatile uint32_t*)(RCC_BASE + 0x001C))) // RW 
#define CM4_APB0_RST_CTL_REG		    (*((volatile uint32_t*)(RCC_BASE + 0x0020))) // RW 
#define CM4_APB1_RST_CTL_REG		    (*((volatile uint32_t*)(RCC_BASE + 0x0024))) // RW 
#define CM4_AHB_RST_CTL_REG		        (*((volatile uint32_t*)(RCC_BASE + 0x0028))) // RW 
#define CM4_AUDIO_PERF_CLK_EN_REG  		(*((volatile uint32_t*)(RCC_BASE + 0x002C))) // RW AUDIO
#define CM4_AUDIO_RSTN_CTL_REG  		(*((volatile uint32_t*)(RCC_BASE + 0x0030))) // RW AUDIO
#define CM4_MM_PERF_CLK_EN_REG  		(*((volatile uint32_t*)(RCC_BASE + 0x0034))) // RW MULTIMEDIA
#define CM4_MM_RSTN_CTL_REG  		    (*((volatile uint32_t*)(RCC_BASE + 0x0038))) // RW MULTIMEDIA
#define CM4_WDG3_RCC_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x003C))) // RW 
#define CM4_TW_CLK_CTL_REG        		(*((volatile uint32_t*)(RCC_BASE + 0x0040))) // RW 
#define CM4_MEM_CLK_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0044))) // RW 
#define CM4_MEM_RST_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0048))) // RW 
#define CM4_PLL_CTL_REG      		    (*((volatile uint32_t*)(RCC_BASE + 0x004C))) // RW 
#define CM4_PLL_CTL_REG2      		    (*((volatile uint32_t*)(RCC_BASE + 0x0050))) // RW 
#define CM4_AUDIO_PLL_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0054))) // RW 
#define CM4_AUDIO_PLL_CTL_REG2     		(*((volatile uint32_t*)(RCC_BASE + 0x0058))) // RW 
#define CM4_MM_PLL_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x005C))) // RW 
#define CM4_MM_PLL_CTL_REG2      		(*((volatile uint32_t*)(RCC_BASE + 0x0060))) // RW 
#define CM4_I2S_CLK_DIV_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0064))) // RW 
#define CM4_ETH_PLL_CTL_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0068))) // RW 
#define CM4_ETH_PLL_CTL_REG2      		(*((volatile uint32_t*)(RCC_BASE + 0x006C))) // RW 
#define CM4_ETH_CLK_DIV_REG      		(*((volatile uint32_t*)(RCC_BASE + 0x0070))) // RW 
#define CM4_PLL_LOCK_STATUS      		(*((volatile uint32_t*)(RCC_BASE + 0x0074))) // RO 
#define CM4_SDIO0_CLK_DIV_CTL      		(*((volatile uint32_t*)(RCC_BASE + 0x0078))) // RO 
#define CM4_SDIO1_CLK_DIV_CTL      		(*((volatile uint32_t*)(RCC_BASE + 0x007C))) // RO 
#define CM4_SDIO_CLK_SEL          		(*((volatile uint32_t*)(RCC_BASE + 0x0080))) // RO 

//DSP REG
#define DSP_SYS_CLK_EN_REG     		    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0000))) // RW 
#define DSP_PERF_CLK_EN_REG     	    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0004))) // RW 
#define DSP_RSTN_CTL_REG     		    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0008))) // RW 
#define DSP_PERF_RSTN_CTL_REG     	    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x000C))) // RW 
#define DSP_WARM_RSTN_REG        		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0010))) // RW 
#define DSP_SYS_CLK_SEL_REG     	    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0014))) // RW 
#define DSP_CEVA_RST_CTRL_REG     	    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0018))) // RW 
#define DSP_PLL_CTRL_REG     		    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x001C))) // RW 
#define DSP_PLL_CTRL_REG2     		    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0020))) // RW 
#define DSP_PLOCK_STATUS     		    (*((volatile uint32_t*)(DSP_RCC_BASE + 0x0024))) // RW 
#define DSP_CEVA_STATUS          		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0028))) // RW 
#define DSP_PIM_CLK_DIV          		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0030))) // RW 
#define DSP_NPU_CLK_DIV          		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0030))) // RW 
#define DSP_MM_PLL_CTL_REG         		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0034))) // RW 
#define DSP_MM_PLL_CTL_REG2        		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0038))) // RW 
#define DSP_MM_PERF_CLKEN_REG      		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x003C))) // RW 
#define DSP_MM_RSTN_CTL_REG        		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0040))) // RW 
#define DSP_WARMRST_STS_REG        		(*((volatile uint32_t*)(DSP_RCC_BASE + 0x0044))) // RW 

typedef enum _dsp_peripheral_reset_
{
    EM_RESET_MBOX = 1 << 11,
    EM_RESET_DSP_SCTRL = 8,
    EM_RESET_SRAM1 = 4,
    EM_RESET_SRAM0 = 2,
    EM_RESET_NULL = 0,
}emPERRESET;

typedef enum _dsp_system_reset_
{
    EM_RESET_NPU = 1 << 11,
    EM_RESET_H264 = 1 << 10,
    EM_RESET_INT_CTRL = 1 << 9,
    EM_RESET_PIM = 1 << 8,
    EM_RESET_AXI_DMAC = 1 << 7,
}emSYSRESET;

typedef enum _dsp_peripheral_clock_
{
    EM_CLOCK_MBOX = 1 << 11,
    EM_CLOCK_DSP_SCTRL = 8,
    EM_CLOCK_SRAM1 = 4,
    EM_CLOCK_SRAM0 = 2,
    EM_CLOCK_NULL = 0,
}emPERCLOCK;

typedef enum _dsp_system_clock_
{
    EM_CLOCK_NPU = 1 << 11,
    EM_CLOCK_H264 = 1 << 10,
    EM_CLOCK_INT_CTRL = 1 << 9,
    EM_CLOCK_PIM = 1 << 8,
    EM_CLOCK_AXI_DMAC = 1 << 7,
    EM_CLOCK_CEVA_SYS_TIME23 = 0x40,
    EM_CLOCK_CEVA_SYS_TIME01 = 0x20,
    EM_CLOCK_CEVA_SYS_WDG = 0x10,
    EM_CLOCK_CEVA_EPP_WDOG = 8,
    EM_CLOCK_CEVA_EDP_WDOG = 4,
    EM_CLOCK_CEVA_IOP_WDOG = 2,
    EM_CLOCK_CEVA_FREE = 1,
}emSYSCLOCK;

typedef enum _cortex_m4_apb0_
{
    EM_CM4_APB0_DVP = 1 << 14,
    EM_CM4_APB0_QSPIFLASH = 1 << 13,
    EM_CM4_APB0_SCTRL = 1 << 12,
    EM_CM4_APB0_IOMUX = 1 << 9,
    EM_CM4_APB0_IOMATRIX = 1 << 8,
    EM_CM4_APB0_INTCTRL = 1 << 7,
    EM_CM4_APB0_WDG3 = 1 << 6,
    EM_CM4_APB0_WDG2 = 1 << 5,
    EM_CM4_APB0_WDG1 = 1 << 4,
    EM_CM4_APB0_WDG0 = 1 << 3,
    EM_CM4_APB0_TIMER2 = 1 << 2,
    EM_CM4_APB0_TIMER1 = 1 << 1,
    EM_CM4_APB0_TIMER0 = 1,
}emCM4APB0;

typedef enum _cortex_m4_apb1_
{
    EM_CM4_APB1_PWM = 1 << 13,
    EM_CM4_APB1_I2S1 = 1 << 12,
    EM_CM4_APB1_I2S0 = 1 << 11,
    EM_CM4_APB1_MBOX = 1 << 9,
    EM_CM4_APB1_GPIO = 1 << 8,
    EM_CM4_APB1_I2C3 = 1 << 7,
    EM_CM4_APB1_I2C2 = 1 << 6,
    EM_CM4_APB1_I2C1 = 1 << 5,
    EM_CM4_APB1_I2C0 = 1 << 4,
    EM_CM4_APB1_UART3 = 1 << 3,
    EM_CM4_APB1_UART2 = 1 << 2,
    EM_CM4_APB1_UART1 = 1 << 1,
    EM_CM4_APB1_UART0 = 1,

}emCM4APB1;

typedef enum _cortex_m4_ahb_
{
    EM_CM4_AHB_SDIO2 = 1 << 13,
    EM_CM4_AHB_SDIO0 = 1 << 12,
    EM_CM4_AHB_LCD = 1 << 10,
    EM_CM4_AHB_SPI1 = 1 << 9,
    EM_CM4_AHB_SPI0 = 1 << 8,
    EM_CM4_AHB_QSPIFLASH = 1 << 7,
    EM_CM4_AHB_PSRAM = 1 << 6,
    EM_CM4_AHB_SECRAM4K = 1 << 5,
    EM_CM4_AHB_GMAC = 1 << 4,
}emCM4AHB;

typedef struct _pll_pro_
{
    uint16_t FBDIV;
    uint16_t POSTDIV1;
    uint16_t POSTDIV2;
    uint16_t REFDIV;
    uint32_t FRAC;
}stPLLPRO;

int init_cortex_m4_pll(stPLLPRO pro);
int init_audio_pll(stPLLPRO pro);
int init_ether_pll(stPLLPRO pro);
int init_mm_pll(stPLLPRO pro);
int init_dsp_pll(stPLLPRO pro);
void set_cortex_m4_sys_clock(uint8_t aon,uint8_t dma1,uint8_t dma0,int en);
void set_cortex_m4_apb0_clock(emCM4APB0 apb,int en);
void set_cortex_m4_apb1_clock(emCM4APB1 apb,int en);
void set_cortex_m4_ahb_clock(emCM4AHB ahb,int en);
void set_apb_clock_div(uint8_t num /**0-1 */, uint8_t div /** 4bit */);
void set_cortex_m4_core_reset(int en);
void set_cortex_m4_sys_reset(uint8_t aon,uint8_t dma1,uint8_t dma0,int en);
void set_cortex_m4_apb0_reset(emCM4APB0 apb,int en);
void set_cortex_m4_apb1_reset(emCM4APB1 apb,int en);
void set_cortex_m4_ahb_reset(emCM4AHB ahb,int en);
void set_audio_clock(uint8_t num /**0-1 */,int en);
void set_audio_reset(uint8_t num /**0-1 */,int en);
void set_wdg3_reset(int en);
void set_timer_wdg_32k_clock(uint8_t timer2,uint8_t wdg3,int en);
void set_timer_clock(uint8_t num /**0-5 */,int en);
void set_wdg_clock(uint8_t num /**0-3 */,int en);
void set_mem_clock(uint8_t bus,uint8_t rom,uint8_t sram0,uint8_t sram1,int en);
void set_mem_reset(uint8_t bus,uint8_t sram0,uint8_t sram1,int en);
void set_ehter_clock(uint8_t div /** 4bit */,int en);
void set_i2s_clock(uint8_t div /** 4bit */);
void set_sdio_clock(uint8_t num/** 0 1 */,uint8_t delay,uint8_t div,int diven,uint8_t sample,uint8_t drv);
void set_npu_clock_div(uint8_t div /** 5bit */);
void set_pim_clock_div(uint8_t div /** 5bit */);
void set_mm_clock_enable(int en);
void set_mm_reset(int reset);
void set_dsp_reset(int reset);
void set_dsp_warm_reset(int reset);
void set_dsp_peripheral_reset(emPERRESET reset,int en);
void set_dsp_system_reset(emSYSRESET reset,int en);
void set_dsp_peripheral_clock(emPERCLOCK clock,int en);
void set_dsp_system_clock(emSYSCLOCK clock,int en);
// #endif //CONFIG_RCC_MODULE

#endif //_RCC_H_
