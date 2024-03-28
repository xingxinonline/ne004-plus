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
    RISCV2ARM_IRQn              = 0,      /*!< window watchdog timer interrupt                          */
    RXOVRINT0_IRQn              = 1,      /*!< LVD through EXTI line detect interrupt                   */
    TXOVRINT0_IRQn              = 2,      /*!< tamper and timestamp through EXTI line detect            */
    RXINT0_IRQn                 = 3,      /*!< RTC wakeup through EXTI line interrupt                   */
    TXINT0_IRQn                 = 4,      /*!< FMC interrupt                                            */
    TIMER0_IRQn                 = 5,      /*!< RCU and CTC interrupt                                    */
    TIMER1_IRQn                 = 6,      /*!< EXTI line 0 interrupts                                   */
    RTCINT_IRQn                 = 7,      /*!< EXTI line 1 interrupts                                   */
    DMAC0_TRANS_DONe_IRQn       = 8,      /*!< EXTI line 2 interrupts                                   */
    DMAC0_BLOCK_DONe_IRQn       = 9,      /*!< EXTI line 3 interrupts                                   */
    DMAC0_SRC_DONE_IRQn         = 10,     /*!< EXTI line 4 interrupts                                   */
    DMAC0_DST_DONE_IRQn         = 11,     /*!< DMA0 channel0 Interrupt                                  */
    DMAC0_ERR_IRQn              = 12,     /*!< DMA0 channel1 Interrupt                                  */
    DMAC0_COMBINE_DONE_IRQn     = 13,     /*!< DMA0 channel2 interrupt                                  */
    Reserved0_IRQn              = 14,     /*!< DMA0 channel3 interrupt                                  */
    REQ_FFT_DONE_IRQn           = 15,     /*!< DMA0 channel4 interrupt                                  */
    DMAC1_TRANS_DONE_IRQn       = 16,     /*!< DMA0 channel5 interrupt                                  */
    DMAC1_BLOCK_DONE_IRQn       = 17,     /*!< DMA0 channel6 interrupt                                  */
    DMAC1_SRC_DONE_IRQn         = 18,     /*!< ADC interrupt                                            */
    DMAC1_DST_DONE_IRQn         = 19,     /*!< CAN0 TX interrupt                                        */
    DMAC1_ERR_IRQn              = 20,     /*!< CAN0 RX0 interrupt                                       */
    DMAC1_COMBINE_DONE_IRQn     = 21,     /*!< CAN0 RX1 interrupt                                       */
    Reserved1_IRQn              = 22,     /*!< CAN0 EWMC interrupt                                      */
    FLASH_SSI_INTR_IRQn         = 23,     /*!< EXTI[9:5] interrupts                                     */
    AAON_ACTIVE_IRQn            = 24,     /*!< TIMER0 break and TIMER8 interrupts                       */
    VAON_ACTIVE_IRQn            = 25,     /*!< TIMER0 update and TIMER9 interrupts                      */
    PDM_AVAIL_IRQn              = 26,     /*!< TIMER0 trigger and commutation  and TIMER10 interrupts   */
    Reserved2_IRQn              = 27,     /*!< TIMER0 channel capture compare interrupt                 */
    I2C0_IRQOUT_IRQn            = 28,     /*!< TIMER1 interrupt                                         */
    I2C1_IRQOUT_IRQn            = 29,     /*!< TIMER2 interrupt                                         */
    I2S1_IRQOUT_IRQn            = 30,     /*!< TIMER3 interrupts                                        */
    I2S0_IRQOUT_IRQn            = 31,     /*!< I2C0 event interrupt                                     */
#if defined (NE004PLUS)
    GPIO0_N0_IRQn               = 32,     /*!< I2C0 error interrupt                                     */
    GPIO0_N1_IRQn               = 33,     /*!< I2C1 event interrupt                                     */
    GPIO0_N2_IRQn               = 34,     /*!< I2C1 error interrupt                                     */
    GPIO0_N3_IRQn               = 35,     /*!< SPI0 interrupt                                           */
    GPIO0_N4_IRQn               = 36,     /*!< SPI1 interrupt                                           */
    GPIO0_N5_IRQn               = 37,     /*!< USART0 interrupt                                         */
    GPIO0_N6_IRQn               = 38,     /*!< USART1 interrupt                                         */
    GPIO0_N7_IRQn               = 39,     /*!< USART2 interrupt                                         */
    GPIO0_N8_IRQn               = 40,     /*!< EXTI[15:10] interrupts                                   */
    GPIO0_N9_IRQn               = 41,     /*!< RTC alarm interrupt                                      */
    GPIO0_N10_IRQn              = 42,     /*!< USBFS wakeup interrupt                                   */
    GPIO0_N11_IRQn              = 43,     /*!< TIMER7 break and TIMER11 interrupts                      */
    GPIO0_N12_IRQn              = 44,     /*!< TIMER7 update and TIMER12 interrupts                     */
    GPIO0_N13_IRQn              = 45,     /*!< TIMER7 trigger and commutation and TIMER13 interrupts    */
    GPIO0_N14_IRQn              = 46,     /*!< TIMER7 channel capture compare interrupt                 */
    GPIO0_N15_IRQn              = 47,     /*!< DMA0 channel7 interrupt                                  */
    GPIO0_N16_IRQn              = 48,     /*!< EXMC interrupt                                           */
    GPIO0_N17_IRQn              = 49,     /*!< SDIO interrupt                                           */
    GPIO0_N018_IRQn             = 50,     /*!< TIMER4 interrupt                                         */
    PWM0_IRQn                   = 51,     /*!< SPI2 interrupt                                           */
    PWM1_IRQn                   = 52,     /*!< UART3 interrupt                                          */
    SSI_MST_IRQn                = 53,     /*!< UART4 interrupt                                          */
    SSI_RXF_IRQn                = 54,     /*!< TIMER5 and DAC0 DAC1 underrun error interrupts           */
    SSI_RXO_IRQn                = 55,     /*!< TIMER6 interrupt                                         */
    SSI_RXU_IRQn                = 56,     /*!< DMA1 channel0 interrupt                                  */
    SSI_TXE_IRQn                = 57,     /*!< DMA1 channel1 interrupt                                  */
    SSI_TXO_IRQn                = 58,     /*!< DMA1 channel2 interrupt                                  */
    RISCV_NOTICE_IRQn           = 59,     /*!< DMA1 channel3 interrupt                                  */
    Reserved3_IRQn              = 60,     /*!< DMA1 channel4 interrupt                                  */
    Reserved4_IRQn              = 61,     /*!< ENET interrupt                                           */
    Reserved5_IRQn              = 62,     /*!< ENET wakeup through EXTI line interrupt                  */
    Reserved6_IRQn              = 63,     /*!< CAN1 TX interrupt                                        */
#endif /* NE004PLUS */
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

//boot reg
#define SOFT_PRST                       REG32(0x4000D018U)
#define AAON_ENABLE                     REG32(0x4000D08CU)
#define HANG_OVER                       REG32(0x4000D0A0U)
#define ISR_ALL_EN                      REG32(0x4000D0F8U)// params_enable_arm_intr 1bit 1 enable all interrupt,0 disable all
#define PDM_WIDTH                       REG32(0x4000D100U)
#define PDM_LINE_CLR                    REG32(0x4000D104U)
#define LCD_DRV_ADDR                    REG32(0x4000D10CU)
#define PSRAM_XIP_ADDR                  REG32(0x4000D110U)
#define REG_XIP                         REG32(0x4000D118U)
#define CHIP_MODE                       REG32(0x4000D200U) // 芯片启动模式，3-5bit 见BOOTMODE
#define RISC_EN                         REG32(0x4000D00CU) // bit6 RISCV启动寄存器，ARM可访问，由ARM控制RISC启动，

#define PAD_GP0_FUNCSEL                 REG32(0x4000D140U) // [3:0]
#define PAD_GP1_FUNCSEL                 REG32(0x4000D140U) // [7:4]
#define PAD_GP2_FUNCSEL                 REG32(0x4000D140U) // [11:8]
#define PAD_GP3_FUNCSEL                 REG32(0x4000D140U) // [15:12]
#define PAD_GP4_FUNCSEL                 REG32(0x4000D140U) // [19:16]
#define PAD_GP5_FUNCSEL                 REG32(0x4000D140U) // [23:20]
#define PAD_GP6_FUNCSEL                 REG32(0x4000D140U) // [27:24]
#define PAD_GP7_FUNCSEL                 REG32(0x4000D140U) // [31:28]
#define PAD_GP8_FUNCSEL                 REG32(0x4000D144U) // [3:0]
#define PAD_GP9_FUNCSEL                 REG32(0x4000D144U) // [7:4]
#define PAD_GP10_FUNCSEL                REG32(0x4000D144U) // [11:8]
#define PAD_GP11_FUNCSEL                REG32(0x4000D144U) // [15:12]
#define PAD_GP12_FUNCSEL                REG32(0x4000D144U) // [19:16]
#define PAD_GP13_FUNCSEL                REG32(0x4000D144U) // [23:20]
#define PAD_GP14_FUNCSEL                REG32(0x4000D144U) // [27:24]
#define PAD_GP15_FUNCSEL                REG32(0x4000D144U) // [31:28]
#define PAD_GP16_FUNCSEL                REG32(0x4000D148U) // [3:0]
#define PAD_GP17_FUNCSEL                REG32(0x4000D148U) // [7:4]
#define PAD_GP18_FUNCSEL                REG32(0x4000D148U) // [11:8]
#define GPIO0_DR                        REG32(0x40006000U)
#define GPIO0_DDR                       REG32(0x40006004U)



#define IPCM_ADDR                       REG32(0x62200004U)
#define ARM_EN_RISCV_RAM                REG32(0x62200004U) // [0] , params_enable_arm_256kb ARM 可以操作RISC v的ram
#define ARM_SET_RISCV_RAM_BASE          REG32(0x62200008U) // [12:0],256KB SRAM高位地址寄存器[17:5] ARM 可以操作RISC v的ram 高端地址
#define RISCV_CODE_INFO                 REG32(0x62200010U) // 0x6220_0010~0x6220001f arm_to_riscv_key_info
#define ARM_CPY_RISCV_RAM_BASE          REG32(0x62200020U) // 0x6220_0020~0x6220_003f 32个byte的映射内存，映射到256KBSRAM【4:0】

#define SPI_BASE                        ((uint32_t)0x10000000U)
#define OSPI_BASE                       ((uint32_t)0x21000000U)
#define UART_BASE                       ((uint32_t)0x40008000U)
#define GPIO_BASE                       ((uint32_t)0x40009000U)
#define WDG_BASE                        ((uint32_t)0x4000A000U)
#define RTC_BASE                        ((uint32_t)0x4000B000U)
#define TIMER_BASE                      ((uint32_t)0x4000C000U)
#define DMAC_BASE                       ((uint32_t)0x4000E000U)


//interrupt control
#define ALL_ISR_ENABLE                  ((uint32_t)0x00000001U)
#define ALL_ISR_DISABLE                 ((uint32_t)0x00000000U)

#ifdef __cplusplus
}
#endif
#endif
