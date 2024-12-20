/*** 
 * @Author       : xingxinonline
 * @Date         : 2024-10-26 17:05:40
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-11-20 15:02:08
 * @FilePath     : \\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Include\\ne004xx_gpio.h
 * @Description  : 
 * @
 * @Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef _NE004XX_GPIO_H_
#define _NE004XX_GPIO_H_

#define IO_Matrtix_CFG(n)			(*((volatile uint32_t*)(IO_MATRIX_BASE + (n) * 0x0004))) // 

#define IO_MUX_CFG_CFG(n)		    (*((volatile uint32_t*)(IO_MUX_BASE + (n) * 0x0004))) // 


#define gpio_swporta_dr			    (*((volatile uint32_t*)(GPIO_BASE + 0x0000))) // Port A data register
#define gpio_swporta_ddr		    (*((volatile uint32_t*)(GPIO_BASE + 0x0004))) // Port A data direction register
#define gpio_swporta_ctl		    (*((volatile uint32_t*)(GPIO_BASE + 0x0008))) // Port A data source register
#define gpio_swportb_dr			    (*((volatile uint32_t*)(GPIO_BASE + 0x000c))) // Port B data register
#define gpio_swportb_ddr		    (*((volatile uint32_t*)(GPIO_BASE + 0x0010))) // Port B data direction register
#define gpio_swportb_ctl		    (*((volatile uint32_t*)(GPIO_BASE + 0x0014))) // Port B data source register
#define gpio_swportc_dr			    (*((volatile uint32_t*)(GPIO_BASE + 0x0018))) // Port C data register
#define gpio_swportc_ddr		    (*((volatile uint32_t*)(GPIO_BASE + 0x001c))) // Port C data direction register
#define gpio_swportc_ctl		    (*((volatile uint32_t*)(GPIO_BASE + 0x0020))) // Port C data source register
#define gpio_swportd_dr			    (*((volatile uint32_t*)(GPIO_BASE + 0x0024))) // Port D data register
#define gpio_swportd_ddr		    (*((volatile uint32_t*)(GPIO_BASE + 0x0028))) // Port D data direction register
#define gpio_swportd_ctl		    (*((volatile uint32_t*)(GPIO_BASE + 0x002c))) // Port D data source register
#define gpio_inten				    (*((volatile uint32_t*)(GPIO_BASE + 0x0030))) // Interrupt enable register
#define gpio_intmask			    (*((volatile uint32_t*)(GPIO_BASE + 0x0034))) // Interrupt mask register
#define gpio_inttype_level		    (*((volatile uint32_t*)(GPIO_BASE + 0x0038))) // Interrupt level register
#define gpio_int_polarity		    (*((volatile uint32_t*)(GPIO_BASE + 0x003c))) // Interrupt polarity register
#define gpio_intstatus			    (*((volatile uint32_t*)(GPIO_BASE + 0x0040))) // Interrupt status of Port A
#define gpio_raw_intstatus		    (*((volatile uint32_t*)(GPIO_BASE + 0x0044))) // Raw interrupt status of Port A(premasking)
#define gpio_debounce			    (*((volatile uint32_t*)(GPIO_BASE + 0x0048))) // Debounce enable register
#define gpio_porta_eoi			    (*((volatile uint32_t*)(GPIO_BASE + 0x004c))) // Port A clear interrupt register
#define gpio_ext_porta			    (*((volatile uint32_t*)(GPIO_BASE + 0x0050))) // Port A external port register
#define gpio_ext_portb			    (*((volatile uint32_t*)(GPIO_BASE + 0x0054))) // Port B external port register
#define gpio_ext_portc			    (*((volatile uint32_t*)(GPIO_BASE + 0x0058))) // Port C external port register
#define gpio_ext_portd			    (*((volatile uint32_t*)(GPIO_BASE + 0x005c))) // Port D external port register
#define gpio_ls_sync			    (*((volatile uint32_t*)(GPIO_BASE + 0x0060))) // Level - sensitive synchronization enable register
#define gpio_id_code			    (*((volatile uint32_t*)(GPIO_BASE + 0x0064))) // ID code register
//#define reserved				    (*((volatile uint32_t*)(GPIO_BASE + 0x0068))) // reserved
#define gpio_ver_id_code		    (*((volatile uint32_t*)(GPIO_BASE + 0x006c))) // Component Version register
#define gpio_config_reg1		    (*((volatile uint32_t*)(GPIO_BASE + 0x0074))) // Configuration Register 1
#define gpio_config_reg2		    (*((volatile uint32_t*)(GPIO_BASE + 0x0070))) // Configuration Register 2

#define gpio_swport_dr(n)           (*((volatile uint32_t*)(GPIO_BASE + 0x0000 + ((n) * 12)))) // Port x data register
#define gpio_swport_ddr(n)		    (*((volatile uint32_t*)(GPIO_BASE + 0x0004 + ((n) * 12)))) // Port x data direction register
#define gpio_swport_ctl(n)		    (*((volatile uint32_t*)(GPIO_BASE + 0x0008 + ((n) * 12)))) // Port x data source register
#define gpio_ext_port(n)		    (*((volatile uint32_t*)(GPIO_BASE + 0x0050 + ((n) * 4)))) // Port x external port register

#define GPIO_PIN_16                 (16)
#define GPIO_PIN_17                 (17)
#define GPIO_PIN_23                 (23)
#define GPIO_PIN_24                 (24)

typedef enum _gpio_port_{
    GPIOA,// 32
    GPIOB,// 16
//    GPIOC,
//    GPIOD,
}emGPIO;

typedef enum _gpio_function_{
    FUNCTION_0,
    FUNCTION_1,
    FUNCTION_2,
    FUNCTION_3,
}emGPIOFUNC;

typedef enum _gpio_mode_{
    GPIO_UP,
    GPIO_DOWN,
    GPIO_INPUT_OUTPUT,
}emGPIOMODE;

int set_gpio_function(emGPIO port,uint8_t pin,emGPIOFUNC func);
int set_gpio_mode(emGPIO port,uint8_t pin,emGPIOMODE mode);
int get_gpio_port_numbers(emGPIO port);
int set_gpio_data(emGPIO port,uint8_t pin,uint32_t value);
uint32_t get_gpio_data(emGPIO port);

int set_gpio_direction(emGPIO port,uint8_t pin,uint32_t isoutput);

/**
 * This function sets the software mode of a GPIO pin.
 * 
 * @param port The GPIO port number, which is an enumerated value of type emGPIO.
 * @param pin The pin number of the GPIO port that needs to be configured.
 * @param issoft The "issoft" parameter is a boolean value (true/false) that indicates whether the GPIO
 * pin should be set to software mode or not. If "issoft" is true, the pin will be set to software
 * mode, otherwise it will be set to hardware mode.
 * 
 * @return an integer value 'res', but the value of 'res' is not being set or modified within the
 * function. Therefore, it is likely that the function is intended to return an error code or status
 * indicating the success or failure of the operation, but this is not implemented in the given code
 * snippet.
 */
int SetGPIOSoftwareMode(emGPIO port,uint8_t pin,char issoft);


#endif //_GPIO_H_
