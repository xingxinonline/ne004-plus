/***
 * @Author       : panxinhao
 * @Date         : 2023-07-25 16:21:07
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-07-26 10:30:14
 * @FilePath     : \\testbench_base\\cortexm4_timer\\CMSIS\\NE004\\NE004PLUS\\Include\\ne004xx.h
 * @Description  :
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */



#ifndef NE0044XX_H
#define NE0044XX_H

#ifdef __cplusplus
extern "C" {
#endif

/* define NE004xx */
#if !defined (NE004)  && !defined (NE004PLUS)
// #define NE004
#define NE004PLUS
#endif /* define NE004xx */

#if !defined (NE004)  && !defined (NE004PLUS)
#error "Please select the target NE004xx device in gd32f4xx.h file"
#endif /* undefine NE004xx tip */

// /* I2S external clock in selection */
// //#define I2S_EXTERNAL_CLOCK_IN          (uint32_t)12288000U

/* NE0044xx firmware library version number V1.0 */
#define __NE004xx_STDPERIPH_VERSION_MAIN   (0x03) /*!< [31:24] main version     */
#define __NE004xx_STDPERIPH_VERSION_SUB1   (0x00) /*!< [23:16] sub1 version     */
#define __NE004xx_STDPERIPH_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version     */
#define __NE004xx_STDPERIPH_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __NE004xx_STDPERIPH_VERSION        ((NE004xx_STDPERIPH_VERSION_MAIN << 24)\
                                            |(NE004xx_STDPERIPH_VERSION_SUB1 << 16)\
                                            |(NE004xx_STDPERIPH_VERSION_SUB2 << 8)\
                                            |(NE004xx_STDPERIPH_VERSION_RC))

/* configuration of the cortex-M4 processor and core peripherals */
#define __CM4_REV                 0x0001   /*!< core revision r0p1                                       */
#define __MPU_PRESENT             1        /*!< NE0044xx provide MPU                                     */
#define __NVIC_PRIO_BITS          4        /*!< NE0044xx uses 4 bits for the priority levels             */
#define __Vendor_SysTickConfig    0        /*!< set to 1 if different sysTick config is used             */
#define __FPU_PRESENT             1        /*!< FPU present                                              */
/* define interrupt number */
typedef enum IRQn
{
    /* cortex-M4 processor exceptions numbers */
    NonMaskableInt_IRQn         = -14,    /*!< 2 non maskable interrupt                                 */
    MemoryManagement_IRQn       = -12,    /*!< 4 cortex-M4 memory management interrupt                  */
    BusFault_IRQn               = -11,    /*!< 5 cortex-M4 bus fault interrupt                          */
    UsageFault_IRQn             = -10,    /*!< 6 cortex-M4 usage fault interrupt                        */
    SVCall_IRQn                 = -5,     /*!< 11 cortex-M4 SV call interrupt                           */
    DebugMonitor_IRQn           = -4,     /*!< 12 cortex-M4 debug monitor interrupt                     */
    PendSV_IRQn                 = -2,     /*!< 14 cortex-M4 pend SV interrupt                           */
    SysTick_IRQn                = -1,     /*!< 15 cortex-M4 system tick interrupt                       */
    /* interruput numbers */
    TIMER0_IRQn                 = 0,      /*!< window watchdog timer interrupt                          */
    TIMER1_IRQn                 = 1,      /*!< LVD through EXTI line detect interrupt                   */
    TIMER2_IRQn                 = 2,      /*!< tamper and timestamp through EXTI line detect            */
    TIMER3_IRQn                 = 3,      /*!< RTC wakeup through EXTI line interrupt                   */
    TIMER4_IRQn                 = 4,      /*!< FMC interrupt                                            */
    TIMER5_IRQn                 = 5,      /*!< RCU and CTC interrupt                                    */
    WDT0_IRQn                   = 6,      /*!< EXTI line 0 interrupts                                   */
    WDT1_IRQn                   = 7,      /*!< EXTI line 1 interrupts                                   */
    WDT2_IRQn                   = 8,      /*!< EXTI line 2 interrupts                                   */
    WDT3_IRQn                   = 9,      /*!< EXTI line 3 interrupts                                   */
    N_IRQn                      = 10,     /*!< EXTI line 4 interrupts                                   */
    S_IRQn                      = 11,     /*!< DMA0 channel0 Interrupt                                  */
    FLASH_IRQn                  = 12,     /*!< DMA0 channel1 Interrupt                                  */
    UART0_IRQn                  = 13,     /*!< DMA0 channel2 interrupt                                  */
    UART1_IRQn                  = 14,     /*!< DMA0 channel3 interrupt                                  */
    UART2_IRQn                  = 15,     /*!< DMA0 channel4 interrupt                                  */
    UART3_IRQn                  = 16,     /*!< DMA0 channel5 interrupt                                  */
    I2C0_IRQn                   = 17,     /*!< DMA0 channel6 interrupt                                  */
    I2C1_IRQn                   = 18,     /*!< ADC interrupt                                            */
    I2C2_IRQn                   = 19,     /*!< CAN0 TX interrupt                                        */
    I2C3_IRQn                   = 20,     /*!< CAN0 RX0 interrupt                                       */
    GPIO_IRQn                   = 21,     /*!< CAN0 RX1 interrupt                                       */
    MAILBOX_IRQn                = 22,     /*!< CAN0 EWMC interrupt                                      */
    I2S0_IRQn                   = 23,     /*!< EXTI[9:5] interrupts                                     */
    I2S1_IRQn                   = 24,     /*!< TIMER0 break and TIMER8 interrupts                       */
    PWM_IRQn                    = 25,     /*!< TIMER0 update and TIMER9 interrupts                      */
    ETH_IRQn                    = 26,     /*!< TIMER0 trigger and commutation  and TIMER10 interrupts   */
    DMA0_IRQn                   = 27,     /*!< TIMER0 channel capture compare interrupt                 */
    DMA1_IRQn                   = 28,     /*!< TIMER1 interrupt                                         */
    SPI0_IRQn                   = 29,     /*!< TIMER2 interrupt                                         */
    SPI1_IRQn                   = 30,     /*!< TIMER3 interrupts                                        */
    LCD_IRQn                    = 31,     /*!< I2C0 event interrupt                                     */
    OV5640_IRQn                 = 32,     /*!< I2C0 error interrupt                                     */
    SDIO0_IRQn                  = 33,     /*!< I2C1 event interrupt                                     */
    SDIO1_IRQn                  = 34,     /*!< I2C1 error interrupt                                     */
    AON_GPIO_IRQn               = 35,     /*!< SPI0 interrupt                                           */
    AON_I2S1_IRQn               = 36,     /*!< SPI1 interrupt                                           */
    AON_I2S0_IRQn               = 37,     /*!< USART0 interrupt                                         */
    WDT_IRQn                    = 38,     /*!< USART1 interrupt                                         */
    RTC_IRQn                    = 39,     /*!< USART2 interrupt                                         */                           

} IRQn_Type;

typedef enum RISCV_IRQn
{
    RISCV_ARM2RISCV_IRQ         = 0,
    RISCV_MNNU_TOP0_IRQ         = 1,
    RISCV_MNNU_TOP1_IRQ         = 2,
    RISCV_MNNU_TOP2_IRQ         = 3,
    RISCV_MNNU_TOP3_IRQ         = 4,
    RISCV_MNNU_TOP4_IRQ         = 5,
    RISCV_MNNU_TOP5_IRQ         = 6,
    RISCV_MNNU_TOP6_IRQ         = 7,
    RISCV_MNNU_TOP7_IRQ         = 8,
    RISCV_MNNU_TOP8_IRQ         = 9,
    RISCV_MNNU_TOP9_IRQ         = 10,
    RISCV_DMA1_IRQ              = 11,
    RISCV_DMA0_IRQ              = 12,
    RISCV_VCOPROC_DONE_IRQ      = 13,
    RISCV_ARM_NOTICE_IRQ        = 14,
} RISCV_IRQn_Type;  

/* includes */
#include "core_cm4.h"
#include "system_ne004xx.h"
#include <stdint.h>

/* enum definitions */
typedef enum {DISABLE = 0, ENABLE = !DISABLE} EventStatus, ControlStatus;
typedef enum {RESET = 0, SET = !RESET} FlagStatus;
typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrStatus;

/* bit operations */
#define REG64(addr)                  (*(volatile uint64_t *)(uint32_t)(addr))
#define REG32(addr)                  (*(volatile uint32_t *)(uint32_t)(addr))
#define REG16(addr)                  (*(volatile uint16_t *)(uint32_t)(addr))
#define REG8(addr)                   (*(volatile uint8_t *)(uint32_t)(addr))
#define BIT(x)                       ((uint32_t)((uint32_t)0x01U<<(x)))
#define BITS(start, end)             ((0xFFFFFFFFUL << (start)) & (0xFFFFFFFFUL >> (31U - (uint32_t)(end))))
#define GET_BITS(regval, start, end) (((regval) & BITS((start),(end))) >> (start))

// /* main flash and SRAM memory map */
// #define FLASH_BASE            ((uint32_t)0x08000000U)        /*!< main FLASH base address          */
// #define SRAM_BASE             ((uint32_t)0x20000000U)        /*!< SRAM0 base address               */

// /* peripheral memory map */
// #define APB1_BUS_BASE         ((uint32_t)0x40000000U)        /*!< apb1 base address                */
// #define APB2_BUS_BASE         ((uint32_t)0x40010000U)        /*!< apb2 base address                */
// #define AHB1_BUS_BASE         ((uint32_t)0x40020000U)        /*!< ahb1 base address                */
// #define AHB2_BUS_BASE         ((uint32_t)0x50000000U)        /*!< ahb2 base address                */

// /* advanced peripheral bus 1 memory map */
// #define TIMER_BASE            (APB1_BUS_BASE + 0x00000000U)  /*!< TIMER base address               */
// #define RTC_BASE              (APB1_BUS_BASE + 0x00002800U)  /*!< RTC base address                 */
// #define WWDGT_BASE            (APB1_BUS_BASE + 0x00002C00U)  /*!< WWDGT base address               */
// #define FWDGT_BASE            (APB1_BUS_BASE + 0x00003000U)  /*!< FWDGT base address               */
// #define I2S_ADD_BASE          (APB1_BUS_BASE + 0x00003400U)  /*!< I2S1_add base address            */
// #define SPI_BASE              (APB1_BUS_BASE + 0x00003800U)  /*!< SPI base address                 */
// #define USART_BASE            (APB1_BUS_BASE + 0x00004400U)  /*!< USART base address               */
// #define I2C_BASE              (APB1_BUS_BASE + 0x00005400U)  /*!< I2C base address                 */

// /* advanced high performance bus 1 memory map */
// #define GPIO_BASE             (AHB1_BUS_BASE + 0x00000000U)  /*!< GPIO base address                */

/* option byte and debug memory map */
#define OB_BASE               ((uint32_t)0x1FFEC000U)        /*!< OB base address                  */
#define DBG_BASE              ((uint32_t)0xE0042000U)        /*!< DBG base address                 */

#define PRO_ENDIAN                      (1 << 2) // 0: Little-Endian; 1  Big-Endian

// APB0
#define TIME0_BASE    			(0x40000000U) // TIME0_BASE    : 0x40000000U ~ 0x40000fffU	4k	True
#define TIME1_BASE    			(0x40001000U) // TIME1_BASE    : 0x40001000U ~ 0x40001fffU	4k	True
#define TIME2_BASE    			(0x40002000U) // TIME2_BASE    : 0x40002000U ~ 0x40002fffU	4k	True
#define WDT0_BASE     			(0x40003000U) // WDT0_BASE     : 0x40003000U ~ 0x40003fffU	4k	True
#define WDT1_BASE     			(0x40004000U) // WDT1_BASE     : 0x40004000U ~ 0x40004fffU	4k	True
#define WDT2_BASE     			(0x40005000U) // WDT2_BASE     : 0x40005000U ~ 0x40005fffU	4k	True
#define WDT3_BASE     			(0x40006000U) // WDT3_BASE     : 0x40006000U ~ 0x40006fffU	4k	True
#define INT_CTRL_BASE 			(0x40007000U) // INT_CTRL_BASE : 0x40007000U ~ 0x40007fffU	4k	True
#define IO_MATRIX_BASE			(0x40008000U) // IO_MATRIX_BASE: 0x40008000U ~ 0x40008fffU	4k	True
#define IO_MUX_BASE   			(0x40009000U) // IO_MUX_BASE   : 0x40009000U ~ 0x40009fffU	4k	True
#define RCC_BASE      			(0x4000a000U) // RCC_BASE      : 0x4000a000U ~ 0x4000afffU	4k	True
#define SEC_BASE      			(0x4000b000U) // SEC_BASE      : 0x4000b000U ~ 0x4000bfffU	4k	True
#define SCTRL_BASE     			(0x4000c000U) // SCTRL         : 0x4000c000U ~ 0x4000cfffU	4k	True
#define QSPI_CFG_BASE 			(0x4000d000U) // QSPI_CFG_BASE : 0x4000d000U ~ 0x4000dfffU	4k	True
#define TIME_BASE     			(0x40000000U) // TIME_BASE     : 0x40000000U ~ 0x40000fffU	4k	True
#define WDT_BASE      			(0x40003000U) // WDT_BASE      : 0x40003000U ~ 0x40003fffU	4k	True


// APB1
#define UART0_BASE			(0x40010000U) // UART0_BASE: 0x40010000U ~ 0x40010fffU	4k 	True
#define UART1_BASE			(0x40011000U) // UART1_BASE: 0x40011000U ~ 0x40011fffU	4k 	True
#define UART2_BASE			(0x40012000U) // UART2_BASE: 0x40012000U ~ 0x40012fffU	4k 	True
#define UART3_BASE			(0x40013000U) // UART3_BASE: 0x40013000U ~ 0x40013fffU	4k 	True
#define I2C0_BASE 			(0x40014000U) // I2C0_BASE : 0x40014000U ~ 0x40014fffU	4k 	True
#define I2C1_BASE 			(0x40015000U) // I2C1_BASE : 0x40015000U ~ 0x40015fffU	4k 	True
#define I2C2_BASE 			(0x40016000U) // I2C2_BASE : 0x40016000U ~ 0x40016fffU	4k 	True
#define I2C3_BASE 			(0x40017000U) // I2C3_BASE : 0x40017000U ~ 0x40017fffU	4k 	True
#define GPIO_BASE 			(0x40018000U) // GPIO_BASE : 0x40018000U ~ 0x40018fffU	4k 	True
#define MAILBOX_BASE	    (0x40019000U)  // 0x4001_9000 ~ 0x4001_9FFF
//#define nan       			(nan) // nan       : nan ~ nan	nan	
//#define nan       			(nan) // nan       : nan ~ nan	nan	
#define I2S0_BASE 			(0x4001b000U) // I2S0_BASE : 0x4001b000U ~ 0x4001bfffU	4k 	True
#define I2S1_BASE 			(0x4001c000U) // I2S1_BASE : 0x4001c000U ~ 0x4001cfffU	4k 	True
#define PWM_BASE  			(0x4001d000U) // PWM_BASE  : 0x4001d000U ~ 0x4001dfffU	4k 	True
#define ETH_BASE  			(0x4001e000U) // ETH_BASE  : 0x4001e000U ~ 0x4001efffU	4k 	True
#define I2C_BASE  			(0x40014000U) // I2C_BASE  : 0x40014000U ~ 0x40014fffU	4k 	True
#define I2S_BASE  			(0x4001b000U) // I2S_BASE  : 0x4001b000U ~ 0x4001bfffU	4k 	True
#define UART_BASE 			(0x40010000U) // UART_BASE : 0x40010000U ~ 0x40010fffU	4k 	True


// AHB
#define DMA0_BASE 			(0x41000000U) // DMA0_BASE : 0x41000000U ~ 0x410fffffU	1M 	True
#define DMA1_BASE 			(0x41100000U) // DMA1_BASE : 0x41100000U ~ 0x411fffffU	1M 	True
#define SPI0_BASE 			(0x41800000U) // SPI0_BASE : 0x41800000U ~ 0x418fffffU	1M 	True
#define SPI1_BASE 			(0x41900000U) // SPI1_BASE : 0x41900000U ~ 0x419fffffU	1M 	True
//#define nan       			(nan) // nan       : nan ~ nan	nan	
#define DVP_BASE  			(0x41a00000U) // DVP_BASE  : 0x41a00000U ~ 0x41afffffU	1M 	True
#define DVP1_BASE  			(0x41b00000U) // DVP_BASE  : 0x41b00000U ~ 0x41bfffffU	1M 	True
//#define nan       			(nan) // nan       : nan ~ nan	nan	
#define SDIO0_BASE			(0x41c00000U) // SDIO0_BASE: 0x41c00000U ~ 0x41cfffffU	1M 	True
#define SDIO1_BASE			(0x41d00000U) // SDIO1_BASE: 0x41d00000U ~ 0x41dfffffU	1M 	True
#define DMA_BASE  			(0x41000000U) // DMA_BASE  : 0x41000000U ~ 0x410fffffU	1M 	True
#define SDIO_BASE 			(0x41c00000U) // SDIO_BASE : 0x41c00000U ~ 0x41cfffffU	1M 	True
#define SPI_BASE  			(0x41800000U) // SPI_BASE  : 0x41800000U ~ 0x418fffffU	1M 	True

/* AON addr 0x4300_0000~0x4302_FFFF */
#define AON_DMA_BASE	    (0x43000000UL) // 0x4300_0000 ~ 0x4300_3FFF
#define AON_VPROC_BASE	    (0x43004000UL) // 0x4300_4000 ~ 0x4300_7FFF
#define AON_APROC_BASE	    (0x43008000UL) // 0x4300_8000 ~ 0x4300_8FFF
#define AON_I2S0_BASE	    (0x43010000UL) // 0x4301_0000 ~ 0x4301_0FFF
#define AON_I2S1_BASE	    (0x43011000UL) // 0x4301_1000 ~ 0x4301_1FFF
#define AON_SPI0_BASE	    (0x43012000UL) // 0x4301_2000 ~ 0x4301_2FFF
#define AON_SPI1_BASE	    (0x43013000UL) // 0x4301_3000 ~ 0x4301_3FFF
#define AON_GPIO_BASE	    (0x43014000UL) // 0x4301_4000 ~ 0x4301_4FFF
#define AON_WDOG_BASE	    (0x43015000UL) // 0x4301_5000 ~ 0x4301_5FFF
#define AON_TIMER_BASE	    (0x43016000UL) // 0x4301_6000 ~ 0x4301_6FFF
#define AON_RTC_BASE	    (0x43017000UL) // 0x4301_7000 ~ 0x4301_7FFF
#define AON_CFG_BASE	    (0x43018000UL) // 0x4301_8000 ~ 0x4301_8FFF
#define AON_SRAM1_BASE      (0x43020000UL) // 0x4302_0000 ~ 0x4302_3FFF
#define AON_SRAM0_BASE      (0x43024000UL) // 0x4302_4000 ~ 0x4302_4FFF

/* DSP addr */
#define DSP_RAM0_BASE       (0x44000000UL) // 0x4400_0000 ~ 0x4403_FFFF
#define DSP_RAM1_BASE       (0x44040000UL) // 0x4404_0000 ~ 0x4407_FFFF
#define DSP_RCC_BASE        (0x44080000UL) // 0x4408_0000 ~ 0x4408_03FF
#define DSP_MAILBOX_BASE    (0x44080400UL) // 0x4408_0400 ~ 0x4408_07FF
#define DSP_SYSCTL_BASE     (0x44080800UL) // 0x4408_0800 ~ 0x4408_0BFF
#define DSP_VIDEO_SS_BASE   (0x44080C00UL) // 0x4408_0C00 ~ 0x4408_0FFF
#define DSP_AXI_DMA_BASE    (0x44088000UL) // 0x4408_8000 ~ 0x4408_9FFF
#define DSP_EDAP_BASE       (0x44800000UL) // 0x4480_0000 ~ 0x44FF_FFFF
#define DSP_PIM_BASE        (0x60000000UL) // 0x6000_0000 ~ 0x61FF_FFFF
#define DSP_PSRAM_BASE      (0x80000000U) // DSP_PSRAM_BASE      : 0x80000000U ~ 0x81FFFFFFU	32M 	True

#ifdef __cplusplus
}
#endif
#endif
