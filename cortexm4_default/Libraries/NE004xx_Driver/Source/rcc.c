
#include "rcc.h"

/**
 * The function `init_cortex_m4_pll` initializes the Cortex-M4 PLL with specific configuration
 * parameters and waits for the PLL to lock within a specified timeout period.
 * 
 * @param pro The code snippet provided is a function `init_cortex_m4_pll` that initializes the PLL
 * (Phase-Locked Loop) settings for a Cortex-M4 microcontroller. The function takes a structure
 * `stPLLPRO` as a parameter, which contains the following fields:
 * 
 * @return The function `init_cortex_m4_pll` returns either `0` if the PLL initialization is
 * successful or `-1` if there is an error during the initialization process.
 */
int init_cortex_m4_pll(stPLLPRO pro)
{
	uint32_t temp;
	uint32_t timeout;
	temp = CM4_SYS_CLK_SEL_REG;
	temp &= ~0x3;
	temp |= 1;
	CM4_SYS_CLK_SEL_REG = temp;

    for(temp=0;temp < 20;)temp++;

	temp = 0
		 | 0 << 31 								//31			0x1			PLL PD
		 | 0 << 30 								//30			0X0			PLL DACPD
		 | 1 << 29 								//29			0X0			PLL DSMPD
		 | 0 << 28 								//28			0X0			PLL FOUTPOSTDIVPD
		 | 0 << 27 								//27			0X0			PLL FOUT4PHASEPD
		 | 0 << 26 								//26			0X0			PLL FOUTVCOPD
		 | 1 << 25 								//25			0X0			PLL BYPASS
		 | (pro.POSTDIV2 & 0x7) << 15 	//17:15			0X0			PLL POSTDIV2
		 | (pro.POSTDIV1 & 0x7) << 12 	//14:12			0X0			PLL POSTDIV1
		 | (pro.FBDIV & 0xfff)  			//11:0			0X50		PLL FBDIV
		 ;
	CM4_PLL_CTL_REG2 = temp;

	temp = 0
		 | (pro.REFDIV & 0x3f) << 24 		//29:24		1			PLL REFDIV
		 | (pro.FRAC & 0xffffff) 				//23:0 		1			PLL FRAC
		 ;
	CM4_PLL_CTL_REG = temp;
	timeout = 1000;
	while ((!(CM4_PLL_LOCK_STATUS & 1)) && timeout)
	{
		timeout--;
	}
	
	if(timeout)
	{
		CM4_PLL_CTL_REG2 &= ~(1 << 25);
		temp = CM4_SYS_CLK_SEL_REG;
		temp &= ~0x3;
		temp |= 0x2;
		CM4_SYS_CLK_SEL_REG = temp;
    	for(temp=0;temp < 100;)temp++;
	}
	else
	{
		return -1;
	}
	return 0;
}

/**
 * The function `set_cortex_m4_sys_clock` sets the clock parameters for cortex-m4 system based on input
 * parameters.
 * 
 * @param aon The `aon` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select aon if 1
 * @param dma1 The `dma1` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select dma1 if 1
 * @param dma0 The `dma0` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select dma0 if 1
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain clock
 * . If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_cortex_m4_sys_clock(uint8_t aon,uint8_t dma1,uint8_t dma0,int en)
{
    uint32_t temp = ((aon & 1) << 8) | ((dma1 & 1) << 4) | ((dma0 & 1) << 3);
    if(en){
        CM4_SYS_CLK_EN_REG |= temp;
    }else{
        CM4_SYS_CLK_EN_REG &= ~temp;
    }
}

/**
 * The function `set_cortex_m4_apb0_clock` sets the clock parameters for cortex-m4 apb0 based on input
 * parameters.
 * 
 * @param apb The `apb` parameter is used to select between two cortex-m4 apb controllers,see emCM4APB0. 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain clock
 * . If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_cortex_m4_apb0_clock(emCM4APB0 apb,int en)
{
    if(en){
        CM4_APB0_CLK_EN_REG |= apb;
    }else{
        CM4_APB0_CLK_EN_REG &= ~apb;
    }
}

/**
 * The function `set_cortex_m4_apb1_clock` sets the clock parameters for cortex-m4 apb1 based on input
 * parameters.
 * 
 * @param apb The `apb` parameter is used to select between two cortex-m4 apb controllers,see emCM4APB1. 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain clock
 * . If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_cortex_m4_apb1_clock(emCM4APB1 apb,int en)
{
    if(en){
        CM4_APB1_CLK_EN_REG |= apb;
    }else{
        CM4_APB1_CLK_EN_REG &= ~apb;
    }
}

/**
 * The function `set_cortex_m4_ahb_clock` sets the clock parameters for cortex-m4 ahb based on input
 * parameters.
 * 
 * @param ahb The `ahb` parameter is used to select between two cortex-m4 ahb controllers,see emCM4AHB. 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain clock
 * . If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_cortex_m4_ahb_clock(emCM4AHB ahb,int en)
{
    if(en){
        CM4_AHB_CLK_EN_REG |= ahb;
    }else{
        CM4_AHB_CLK_EN_REG &= ~ahb;
    }
}

/**
 * The function `set_apb_clock_div` sets the clock parameters for cortex-m4 apb based on input
 * parameters.
 * 
 * @param num The `num` parameter is used to select APB0 APB1. It can have a value 
 * of either 0 or 1 to indicate which controller to configure eg: APB0:0,APB1:1.
 * @param div The parameter `div` is a 4-bit unsigned integer used to set the clock division for an APB
 * BUS Clock interface. 
 * 1 division:0
 * 2 division:1
 * 4 division:2
 * 8 division:3
 * 16 division:4
 * 
 */
void set_apb_clock_div(uint8_t num /**0-1 */, uint8_t div /** 4bit */)
{
    if(num){
        CM4_APB_CLK_DIV_REG &= ~(0xf0);
        CM4_APB_CLK_DIV_REG |= (div & 0xf) << 4;
    }else{
        CM4_APB_CLK_DIV_REG &= ~(0xf);
        CM4_APB_CLK_DIV_REG |= (div & 0xf);
    }
}

/**
 * The function `set_cortex_m4_core_reset` sets the reset parameters for cortex-m4 core based on input
 * parameters.
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_cortex_m4_core_reset(int en)
{
    if(en){
        CM4_SYS_SOFT_RSTN_REG &= ~1;
    }else{
        CM4_SYS_SOFT_RSTN_REG |= 1;
    }
}

/**
 * The function `set_cortex_m4_sys_reset` sets the reset parameters for cortex-m4 system based on input
 * parameters.
 * 
 * @param aon The `aon` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select aon if 1
 * @param dma1 The `dma1` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select dma1 if 1
 * @param dma0 The `dma0` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select dma0 if 1
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_cortex_m4_sys_reset(uint8_t aon,uint8_t dma1,uint8_t dma0,int en)
{
    uint32_t temp = ((aon & 1) << 8) | ((dma1 & 1) << 4) | ((dma0 & 1) << 3);
    if(en){
        CM4_SYS_RST_CTL_REG &= ~temp;
    }else{
        CM4_SYS_RST_CTL_REG |= temp;
    }
}


/**
 * The function `set_cortex_m4_apb0_reset` sets the reset parameters for cortex-m4 apb0 based on input
 * parameters.
 * 
 * @param apb The `apb` parameter is used to select between two AUDIO controllers. see emCM4APB0
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_cortex_m4_apb0_reset(emCM4APB0 apb,int en)
{
    if(en){
        CM4_APB0_RST_CTL_REG &= ~apb;
    }else{
        CM4_APB0_RST_CTL_REG |= apb;
    }
}

/**
 * The function `set_cortex_m4_apb1_reset` sets the reset parameters for cortex-m4 apb1 based on input
 * parameters.
 * 
 * @param apb The `apb` parameter is used to select between two AUDIO controllers. see emCM4APB1
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_cortex_m4_apb1_reset(emCM4APB1 apb,int en)
{
    if(en){
        CM4_APB1_RST_CTL_REG &= ~apb;
    }else{
        CM4_APB1_RST_CTL_REG |= apb;
    }
}

/**
 * The function `set_cortex_m4_ahb_reset` sets the reset parameters for cortex-m4 ahb based on input
 * parameters.
 * 
 * @param ahb The `ahb` parameter is used to select between two cortex-m4 ahb controllers,see emCM4AHB. 
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_cortex_m4_ahb_reset(emCM4AHB ahb,int en)
{
    if(en){
        CM4_AHB_RST_CTL_REG &= ~ahb;
    }else{
        CM4_AHB_RST_CTL_REG |= ahb;
    }
}

/**
 * The function `set_audio_clock` sets the clock parameters for AUDIO communication based on input
 * parameters.
 * 
 * @param num The `num` parameter is used to select between two AUDIO controllers. It can have a value
 * of either 0 or 1 to indicate which controller to configure.
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain clock
 * . If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_audio_clock(uint8_t num /**0-1 */,int en)
{
    uint32_t temp = 1 << num;
    if(en){
        CM4_AUDIO_PERF_CLK_EN_REG |= temp;
    }else{
        CM4_AUDIO_PERF_CLK_EN_REG &= ~temp;
    }
}

/**
 * The function `set_audio_reset` sets the reset parameters for AUDIO communication based on input
 * parameters.
 * 
 * @param num The `num` parameter is used to select between two AUDIO controllers. It can have a value
 * of either 0 or 1 to indicate which controller to configure.
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_audio_reset(uint8_t num /**0-1 */,int en)
{
    uint32_t temp = 1 << num;
    if(en){
        CM4_AUDIO_RSTN_CTL_REG &= ~temp;
    }else{
        CM4_AUDIO_RSTN_CTL_REG |= temp;
    }
}

/**
 * The function `set_wdg3_reset` sets wdg3 reset control based on input parameters and an enable flag.
 * 
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain reset
 * . If `en` is non-zero (true) enable reset gate,false disabale
 */
void set_wdg3_reset(int en)
{
    uint32_t temp =  1 << 7;
    if(en){
        CM4_WDG3_RCC_CTL_REG &= ~temp;
    }else{
        CM4_WDG3_RCC_CTL_REG |= temp;
    }
}

/**
 * The function `set_timer_wdg_32k_clock` sets timer and wdg3 clock control based on input parameters and an enable flag.
 * 
 * @param timer2 The `timer2` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select timer2 if 1
 * @param wdg3 The `wdg3` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. select wdg3 if 1
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain memory
 * clocks. If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_timer_wdg_32k_clock(uint8_t timer2,uint8_t wdg3,int en)
{
    uint32_t temp = ((timer2 & 1) << 1) | ((wdg3 & 1));
    if(en){
        CM4_WDG3_RCC_CTL_REG |= temp;
    }else{
        CM4_WDG3_RCC_CTL_REG &= ~temp;
    }
}

/**
 * The function `set_timer_clock` sets timer clock control based on input parameters and an enable flag.
 * 
 * @param num The `timer` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. 0-5
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain memory
 * clocks. If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_timer_clock(uint8_t num /**0-5 */,int en)
{
    uint32_t temp = 1 << num;
    if(en){
        CM4_TW_CLK_CTL_REG |= temp;
    }else{
        CM4_TW_CLK_CTL_REG &= ~temp;
    }
}

/**
 * The function `set_wdg_clock` sets wdg clock control based on input parameters and an enable flag.
 * 
 * @param num The `wdg` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value. 0-3
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain memory
 * clocks. If `en` is non-zero (true) enable clock gate,false disabale
 */
void set_wdg_clock(uint8_t num /**0-3 */,int en)
{
    uint32_t temp = 1 << (num + 6);
    if(en){
        CM4_TW_CLK_CTL_REG |= temp;
    }else{
        CM4_TW_CLK_CTL_REG &= ~temp;
    }
}

/**
 * The function `set_mem_clock` sets memory clock control based on input parameters and an enable flag.
 * 
 * @param bus The `bus` parameter is a uint8_t type variable used to set a specific bit in the `temp`
 * variable based on its value.
 * @param rom The `rom` parameter in the `set_mem_clock` function is used to specify whether to enable
 * or disable the memory clock for the ROM (Read-Only Memory). It is a uint8_t type parameter, where a
 * value of 1 indicates enabling the clock for ROM and a value of 0
 * @param sram0 The `sram0` parameter in the `set_mem_clock` function is a uint8_t type variable
 * representing the clock control for SRAM0.
 * @param sram1 The `sram1` parameter in the `set_mem_clock` function is a uint8_t type variable
 * representing the clock control for the SRAM1 memory.
 * @param en The `en` parameter is a flag that determines whether to enable or disable certain memory
 * clocks. If `en` is non-zero (true), the memory clocks specified by the `bus`, `rom`, `sram0`, and
 * `sram1` parameters will be disabled. false disabale
 */
void set_mem_clock(uint8_t bus,uint8_t rom,uint8_t sram0,uint8_t sram1,int en)
{
    uint32_t temp = ((bus & 1) << 3) | ((rom & 1) << 2) | ((sram0 & 1) << 1) | ((sram1 & 1));
    if(en){
        CM4_MEM_CLK_CTL_REG |= temp;
    }else{
        CM4_MEM_CLK_CTL_REG &= ~temp;
    }
}

/**
 * The function `set_mem_reset` sets or resets memory based on the input parameters.
 * 
 * @param bus The `bus` parameter is a uint8_t type variable used to specify a bus value.
 * @param sram0 sram0 is a parameter representing the state of a specific SRAM (Static Random-Access
 * Memory) module. It is a single bit value (0 or 1) indicating whether the SRAM module is enabled or
 * disabled.
 * @param sram1 The `sram1` parameter is a uint8_t type variable used to specify a bit value (0 or 1) for
 * controlling the reset of a specific memory block.
 * @param en The `en` parameter in the `set_mem_reset` function is a flag that determines whether to
 * enable or disable a memory reset operation. If `en` is non-zero (true), the memory reset operation
 * will be enabled. If `en` is zero (false), the memory reset operation will
 */
void set_mem_reset(uint8_t bus,uint8_t sram0,uint8_t sram1,int en)
{
    uint32_t temp = ((bus & 1) << 3) | ((sram0 & 1) << 1) | ((sram1 & 1));
    if(en){
        CM4_MEM_RST_CTL_REG &= ~temp;
    }else{
        CM4_MEM_RST_CTL_REG |= temp;
    }
}

/**
 * The function `init_audio_pll` initializes the audio PLL with specific configuration parameters and
 * waits for PLL lock status within a timeout period.
 * 
 * @param pro The `pro` parameter in the `init_audio_pll` function is a structure of type `stPLLPRO`
 * which contains the following fields:
 * 
 * @return The function `init_audio_pll` returns either `0` if the PLL initialization is
 * successful, or `-1` if there is a timeout waiting for the PLL lock status.
 */
int init_audio_pll(stPLLPRO pro)
{
	uint32_t temp;
	uint32_t timeout;
	temp = CM4_SYS_CLK_SEL_REG;
	temp &= ~0xc;
	temp |= 4;
	CM4_SYS_CLK_SEL_REG = temp;

    for(temp=0;temp < 20;)temp++;

	temp = 0
		 | 0 << 31 								//31			0x1			PLL PD
		 | 0 << 30 								//30			0X0			PLL DACPD
		 | 1 << 29 								//29			0X0			PLL DSMPD
		 | 0 << 28 								//28			0X0			PLL FOUTPOSTDIVPD
		 | 0 << 27 								//27			0X0			PLL FOUT4PHASEPD
		 | 0 << 26 								//26			0X0			PLL FOUTVCOPD
		 | 1 << 25 								//25			0X0			PLL BYPASS
		 | (pro.POSTDIV2 & 0x7) << 15 	//17:15			0X0			PLL POSTDIV2
		 | (pro.POSTDIV1 & 0x7) << 12 	//14:12			0X0			PLL POSTDIV1
		 | (pro.FBDIV & 0xfff)  			//11:0			0X50		PLL FBDIV
		 ;
	CM4_AUDIO_PLL_CTL_REG2 = temp;

	temp = 0
		 | (pro.REFDIV & 0x3f) << 24 		//29:24		1			PLL REFDIV
		 | (pro.FRAC & 0xffffff) 				//23:0 		1			PLL FRAC
		 ;
	CM4_AUDIO_PLL_CTL_REG = temp;
	timeout = 1000;
	while ((!(CM4_PLL_LOCK_STATUS & 2)) && timeout)
	{
		timeout--;
	}
	
	if(timeout)
	{
		CM4_AUDIO_PLL_CTL_REG2 &= ~(1 << 25);
		temp = CM4_SYS_CLK_SEL_REG;
		temp &= ~0xc;
		temp |= 0x8;
		CM4_SYS_CLK_SEL_REG = temp;
    	for(temp=0;temp < 100;)temp++;
	}
	else
	{
		return -1;
	}
	return 0;
}

/**
 * The function `init_ether_pll` initializes the Ethernet PLL with specific configuration parameters
 * and waits for PLL lock status within a timeout period.
 * 
 * @param pro The `init_ether_pll` function initializes the Ethernet PLL based on the provided
 * configuration parameters in the `pro` structure. Here is a breakdown of the parameters in the `pro`
 * structure:
 * 
 * @return The function `init_ether_pll` returns either `0` if the PLL initialization is
 * successful or `-1` if there is a timeout waiting for the PLL lock status.
 */
int init_ether_pll(stPLLPRO pro)
{
	uint32_t temp;
	uint32_t timeout;

	temp = 0
		 | 0 << 31 								//31			0x1			PLL PD
		 | 0 << 30 								//30			0X0			PLL DACPD
		 | 1 << 29 								//29			0X0			PLL DSMPD
		 | 0 << 28 								//28			0X0			PLL FOUTPOSTDIVPD
		 | 0 << 27 								//27			0X0			PLL FOUT4PHASEPD
		 | 0 << 26 								//26			0X0			PLL FOUTVCOPD
		 | 1 << 25 								//25			0X0			PLL BYPASS
		 | (pro.POSTDIV2 & 0x7) << 15 	//17:15			0X0			PLL POSTDIV2
		 | (pro.POSTDIV1 & 0x7) << 12 	//14:12			0X0			PLL POSTDIV1
		 | (pro.FBDIV & 0xfff)  			//11:0			0X50		PLL FBDIV
		 ;
	CM4_ETH_PLL_CTL_REG2 = temp;

	temp = 0
		 | (pro.REFDIV & 0x3f) << 24 		//29:24		1			PLL REFDIV
		 | (pro.FRAC & 0xffffff) 				//23:0 		1			PLL FRAC
		 ;
	CM4_ETH_PLL_CTL_REG = temp;
	timeout = 1000;
	while ((!(CM4_PLL_LOCK_STATUS & 8)) && timeout)
	{
		timeout--;
	}
	
	if(timeout)
	{
		CM4_ETH_PLL_CTL_REG2 &= ~(1 << 25);
	}
	else
	{
		return -1;
	}
	return 0;
}

/**
 * The function `set_ehter_clock` sets the clock divider for the Ethernet module based on the input
 * parameters.
 * 
 * @param div The `div` parameter is a uint8_t type variable that is used to set the clock divider value
 * for the Ethernet clock.
 * @param en The `en` parameter is a flag that indicates whether the Ethernet clock should be enabled
 * or not. If `en` is non-zero (true), the Ethernet clock will be enabled, and if `en` is zero (false),
 * the Ethernet clock will be disabled.
 */
void set_ehter_clock(uint8_t div /** 4bit */,int en)
{
    if(en){
        CM4_ETH_CLK_DIV_REG = (div & 0xf) << 28;
    }else{
        CM4_ETH_CLK_DIV_REG = 1;
    }
}

/**
 * The function set_i2s_clock sets the I2S clock divider value based on the input parameter.
 * 
 * @param div The parameter `div` is a 4-bit unsigned integer used to set the clock division for an I2S
 * (Inter-IC Sound) interface. The function `set_i2s_clock` takes this parameter and writes its lower 4
 * bits to the `CM4_I2S_CLK_DIV
 * 0: 1/2
 */
void set_i2s_clock(uint8_t div /** 4bit */)
{
    CM4_I2S_CLK_DIV_REG = div & 0xf;
}

/**
 * The function `set_sdio_clock` sets the clock parameters for SDIO communication based on input
 * parameters.
 * 
 * @param num The `num` parameter is used to select between two SDIO controllers. It can have a value
 * of either 0 or 1 to indicate which controller to configure.
 * @param delay The `delay` parameter is a uint8_t type variable used to set the delay for the SDIO
 * clock. It is a value between 0 and 7.
 * 0: 1 delay cycle
 * 1: 2 delay cycle
 * 2: 4 delay cycle
 * 3: 8 delay cycle
 * 4: 13 delay cycle
 * 
 * @param div The `div` parameter in the `set_sdio_clock` function represents the division factor for
 * the SDIO clock. It is used to divide the input clock frequency to generate the desired SDIO clock
 * frequency. The value of `div` is a uint8_t type, which means it can hold values
 * 0:1/2
 * 1:1/4
 * 2:1/8
 * 3:1/16
 * 4:1/32
 * 
 * @param diven The `diven` parameter is a flag that determines whether the clock divider is enabled or
 * not. If `diven` is set to 1, the clock divider is enabled; otherwise, it is disabled.
 * diven = 0 No need for frequency division
 * diven = 1 need for frequency division,set CM4_SDIO0_CLK_DIV_CTL or CM4_SDIO1_CLK_DIV_CTL
 * 
 * @param sample The `sample` parameter in the `set_sdio_clock` function represents the sampling clock
 * phase. It is an 8-bit value, but only the lower 2 bits (bit 0 and bit 1) are used for configuration.
 * The sampling clock phase can be set to one of four
 *  diven=0,cclk_in_sample 0 : 0 phase;2: 180 phase
 *  diven=1,cclk_in_sample Select different phases,0 : 0 phase;1:90 phase;2: 180 phase;3:270 phase
 * 
 * @param drv The `drv` parameter in the `set_sdio_clock` function represents the drive strength
 * setting for the SDIO clock. It is an 8-bit unsigned integer (`uint8_t`) that is used to configure the
 * drive strength of the SDIO clock signal. The drive strength setting determines the output strength
 *  diven=0,cclk_in_drv 0 : 0 phase;2: 180 phase
 *  diven=1,cclk_in_drv Select different phases,0 : 0 phase;1:90 phase;2: 180 phase;3:270 phase
 */
void set_sdio_clock(uint8_t num/** 0 1 */,uint8_t delay,uint8_t div,int diven,uint8_t sample,uint8_t drv)
{
    uint32_t temp = ((delay & 7) << 3) | (div & 0x7);
    uint8_t cclk = (diven?0x1:0) | ((sample & 0x3) << 1) | ((drv & 0x3) << 3);
    if(num == 1)
    {
        CM4_SDIO1_CLK_DIV_CTL = temp;
        CM4_SDIO_CLK_SEL &= ~(0xff00);
        CM4_SDIO_CLK_SEL |= cclk << 8;
    }else{
        //0
        CM4_SDIO0_CLK_DIV_CTL = temp;
        CM4_SDIO_CLK_SEL &= ~(0xff);
        CM4_SDIO_CLK_SEL |= cclk;
    }
}

/**
 * The function `init_mm_pll` initializes the memory management PLL with specific configuration
 * parameters and handles timeout conditions.
 * 
 * @param pro The `pro` parameter in the `init_mm_pll` function is of type `stPLLPRO` and contains the
 * following fields:
 * 
 * @return The function `init_mm_pll` will return `0` if the PLL initialization is
 * successful. If the PLL initialization times out, it will return `-1`.
 */
int init_mm_pll(stPLLPRO pro)
{
	uint32_t temp;
	uint32_t timeout;
	temp = DSP_SYS_CLK_SEL_REG;
	temp &= ~0x30;
	temp |= 0x10;
	DSP_SYS_CLK_SEL_REG = temp;

    for(temp=0;temp < 20;)temp++;

	temp = 0
		 | 0 << 31 								//31			0x1			PLL PD
		 | 0 << 30 								//30			0X0			PLL DACPD
		 | 1 << 29 								//29			0X0			PLL DSMPD
		 | 0 << 28 								//28			0X0			PLL FOUTPOSTDIVPD
		 | 0 << 27 								//27			0X0			PLL FOUT4PHASEPD
		 | 0 << 26 								//26			0X0			PLL FOUTVCOPD
		 | 1 << 25 								//25			0X0			PLL BYPASS
		 | (pro.POSTDIV2 & 0x7) << 15 	//17:15			0X0			PLL POSTDIV2
		 | (pro.POSTDIV1 & 0x7) << 12 	//14:12			0X0			PLL POSTDIV1
		 | (pro.FBDIV & 0xfff)  			//11:0			0X50		PLL FBDIV
		 ;
	DSP_MM_PLL_CTL_REG2 = temp;

	temp = 0
		 | (pro.REFDIV & 0x3f) << 24 		//29:24		1			PLL REFDIV
		 | (pro.FRAC & 0xffffff) 				//23:0 		1			PLL FRAC
		 ;
	DSP_MM_PLL_CTL_REG = temp;
	timeout = 1000;
	while ((!(DSP_PLOCK_STATUS & 2)) && timeout)
	{
		timeout--;
	}
	
	if(timeout)
	{
		DSP_MM_PLL_CTL_REG2 &= ~(1 << 25);
		temp = DSP_SYS_CLK_SEL_REG;
		temp &= ~0x30;
		temp |= 0x20;
		DSP_SYS_CLK_SEL_REG = temp;
    	for(temp=0;temp < 100;)temp++;
	}
	else
	{
		return -1;
	}
	return 0;
}

/**
 * The function `init_dsp_pll` initializes the DSP PLL with the specified parameters and returns an
 * error status if the PLL fails to lock within a specified timeout period.
 * 
 * @param pro The `pro` parameter in the `init_dsp_pll` function represents a structure of type
 * `stPLLPRO` which contains the following fields:
 * 
 * @return The function `init_dsp_pll` returns either `0` if the PLL initialization is
 * successful, or `-1` if there is a timeout waiting for the PLL to lock.
 */
int init_dsp_pll(stPLLPRO pro)
{
	uint32_t temp;
	uint32_t timeout;
	temp = DSP_SYS_CLK_SEL_REG;
	temp &= ~0x3;
	temp |= 1;
	DSP_SYS_CLK_SEL_REG = temp;

    for(temp=0;temp < 20;)temp++;

	temp = 0
		 | 0 << 31 								//31			0x1			PLL PD
		 | 0 << 30 								//30			0X0			PLL DACPD
		 | 1 << 29 								//29			0X0			PLL DSMPD
		 | 0 << 28 								//28			0X0			PLL FOUTPOSTDIVPD
		 | 0 << 27 								//27			0X0			PLL FOUT4PHASEPD
		 | 0 << 26 								//26			0X0			PLL FOUTVCOPD
		 | 1 << 25 								//25			0X0			PLL BYPASS
		 | (pro.POSTDIV2 & 0x7) << 15 	//17:15			0X0			PLL POSTDIV2
		 | (pro.POSTDIV1 & 0x7) << 12 	//14:12			0X0			PLL POSTDIV1
		 | (pro.FBDIV & 0xfff)  			//11:0			0X50		PLL FBDIV
		 ;
	DSP_PLL_CTRL_REG2 = temp;

	temp = 0
		 | (pro.REFDIV & 0x3f) << 24 		//29:24		1			PLL REFDIV
		 | (pro.FRAC & 0xffffff) 				//23:0 		1			PLL FRAC
		 ;
	DSP_PLL_CTRL_REG = temp;
	timeout = 1000;
	while ((!(DSP_PLOCK_STATUS & 1)) && timeout)
	{
		timeout--;
	}
	
	if(timeout)
	{
		DSP_PLL_CTRL_REG2 &= ~(1 << 25);
		temp = DSP_SYS_CLK_SEL_REG;
		temp &= ~0x3;
		temp |= 0x2;
		DSP_SYS_CLK_SEL_REG = temp;
    	for(temp=0;temp < 100;)temp++;
	}
	else
	{
		return -1;
	}
	return 0;
}

/**
 * The function set_npu_clock_div sets the clock divider for the NPU with a 5-bit input value.
 * 
 * @param div The parameter `div` is a 5-bit unsigned integer value that represents the clock division
 * for the NPU (Neural Processing Unit).Even frequency division, at least 2 frequency division
 */
void set_npu_clock_div(uint8_t div /** 5bit */)
{
    DSP_NPU_CLK_DIV = div;
}

/**
 * The function set_pim_clock_div sets the clock divider for the PIM clock.
 * 
 * @param div The `div` parameter is a 5-bit unsigned integer value that represents the clock division
 * setting for the PIM clock.Even frequency division, 0 means no frequency
 */
void set_pim_clock_div(uint8_t div /** 5bit */)
{
    DSP_PIM_CLK_DIV = div;
}

/**
 * The function `set_mm_clock_enable` sets a specific bit in a register based on the input parameter
 * `en`.
 * 
 * @param en The `en` parameter in the `set_mm_clock_enable` function is a flag that indicates whether
 * to enable or disable the clock. If `en` is non-zero (true), the clock will be enabled by setting the
 * corresponding bit in the `DSP_MM_PERF_CLKEN_REG` register.false disabale
 */
void set_mm_clock_enable(int en)
{
    if(en){
        DSP_MM_PERF_CLKEN_REG |= 1;
    }else{
        DSP_MM_PERF_CLKEN_REG = 0;
    }
}


/**
 * The function `set_mm_reset` sets or resets a control register based on the value of the `reset`
 * parameter.
 * 
 * @param reset The `reset` parameter is an integer value that is used to determine whether to reset a
 * specific control register. If `reset` is non-zero(true), the control register `DSP_MM_RSTN_CTL_REG` is set
 * to 0. Otherwise, the least significant bit of `DSP_MM_RSTN_CTL,false disabale
 */
void set_mm_reset(int reset)
{
    if(reset){
        DSP_MM_RSTN_CTL_REG = 0;
    }else{
        DSP_MM_RSTN_CTL_REG |= 1;
    }
}

/**
 * The function `set_dsp_reset` sets the DSP reset control register based on the input parameter
 * `reset`.dsp system reset, except rcc
 * 
 * @param reset The `reset` parameter is an integer value that is used to determine whether to reset a
 * DSP (Digital Signal Processor) or not. If `reset` is non-zero(true), the DSP will be reset by setting
 * `DSP_CEVA_RST_CTRL_REG` to 0. If `reset` is false disabale
 */
void set_dsp_reset(int reset)
{
    if(reset){
        DSP_CEVA_RST_CTRL_REG = 0;
    }else{
        DSP_CEVA_RST_CTRL_REG |= 1;
    }    
}

/**
 * The function `set_dsp_warm_reset` sets the warm reset signal for a DSP based on the input parameter
 * `reset` and the status of a specific register.
 * 
 * @param reset The `reset` parameter is a flag that indicates whether a reset operation should be
 * performed. If `reset` is non-zero (true), the function will check the status of the DSP (Digital
 * Signal Processor) and perform a warm reset by setting the `DSP_WARM_RSTN_REG` register accordingly,otherwise false, the reset will be disabled
 */
void set_dsp_warm_reset(int reset)
{
    if(reset && (DSP_CEVA_STATUS & 1))
    {
        DSP_WARM_RSTN_REG = 0;
    }else{
        DSP_WARM_RSTN_REG = 1;
    }
}

/**
 * The function `set_dsp_peripheral_reset` toggles the reset state of a specified peripheral in a DSP
 * system.
 * 
 * @param reset The `reset` parameter is an enumeration representing different reset signals for a DSP
 * peripheral.
 * @param en The `en` parameter is a flag that indicates whether to enable or disable the reset for a
 * specific peripheral. If `en` is set to a non-zero value(true), the reset for the specified peripheral will
 * be enabled. If `en` is set to false, the reset will be disabled.
 */
void set_dsp_peripheral_reset(emPERRESET reset,int en)
{
    if(en)
    {
        DSP_PERF_RSTN_CTL_REG &= ~reset;
    }else{
        DSP_PERF_RSTN_CTL_REG |= reset;
    }
}

/**
 * The function `set_dsp_system_reset` toggles the reset control bits based on the input parameters.
 * 
 * @param reset The `reset` parameter is an enumeration type `emSYSRESET` that represents different
 * reset signals for a DSP system.
 * @param en The `en` parameter is a flag that indicates whether the reset should be enabled or
 * disabled. If `en` is non-zero (true), the reset will be enabled; otherwise false, the reset will be
 * disabled.
 */
void set_dsp_system_reset(emSYSRESET reset,int en)
{
    if(en)
    {
        DSP_RSTN_CTL_REG &= ~reset;
    }else{
        DSP_RSTN_CTL_REG |= reset;
    }
}

/**
 * The function `set_dsp_peripheral_clock` is used to enable or disable a specific peripheral clock for
 * a DSP.
 * 
 * @param clock The `clock` parameter is an enum representing different peripheral clocks for the DSP
 * (Digital Signal Processor).
 * @param en The `en` parameter is a flag that indicates whether to enable or disable a specific
 * peripheral clock. If `en` is non-zero (true), the function will set the specified peripheral clock.
 * If `en` is zero (false), the function will clear the specified peripheral clock.
 */
void set_dsp_peripheral_clock(emPERCLOCK clock,int en)
{
    if(en)
    {
        DSP_RSTN_CTL_REG |= clock;
    }else{
        DSP_RSTN_CTL_REG &= ~clock;
    }
}

/**
 * The function `set_dsp_system_clock` is used to enable or disable specific clock signals in a DSP
 * system.
 * 
 * @param clock The `clock` parameter in the `set_dsp_system_clock` function is of type `emSYSCLOCK`,
 * which is likely an enumeration representing different system clock options for a DSP (Digital Signal
 * Processor) system. This parameter is used to specify which system clock to enable or disable.
 * @param en The `en` parameter is a flag that indicates whether to enable or disable a specific system
 * clock. If `en` is non-zero (true), the system clock specified by the `clock` parameter will be
 * enabled. If `en` is zero (false), the system clock will be disabled.
 */
void set_dsp_system_clock(emSYSCLOCK clock,int en)
{
    if(en)
    {
        DSP_SYS_CLK_EN_REG |= clock;
    }else{
        DSP_SYS_CLK_EN_REG &= ~clock;
    }
}

