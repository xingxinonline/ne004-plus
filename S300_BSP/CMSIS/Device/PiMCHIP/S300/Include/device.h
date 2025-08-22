#ifndef PIMCHIP_DEVICE_H
#define PIMCHIP_DEVICE_H

#include <stdint.h>

#define __CM4_REV                 0x0001U
#define __MPU_PRESENT             1U
#define __NVIC_PRIO_BITS          4U
#define __Vendor_SysTickConfig    0U
#define __FPU_PRESENT             1U

/* CMSIS requires IRQn_Type defined by the device before including core header */
typedef enum IRQn
{
    NonMaskableInt_IRQn   = -14, /* 2 Non Maskable Interrupt */
    HardFault_IRQn        = -13, /* 3 HardFault Interrupt */
    MemoryManagement_IRQn = -12, /* 4 Memory Management Interrupt */
    BusFault_IRQn         = -11, /* 5 Bus Fault Interrupt */
    UsageFault_IRQn       = -10, /* 6 Usage Fault Interrupt */
    SVCall_IRQn           =  -5, /* 11 SV Call Interrupt */
    DebugMonitor_IRQn     =  -4, /* 12 Debug Monitor Interrupt */
    PendSV_IRQn           =  -2, /* 14 Pend SV Interrupt */
    SysTick_IRQn          =  -1, /* 15 System Tick Interrupt */

    /* Device-specific IRQs */
    TIMER0_IRQn           =   0,
    TIMER1_IRQn           =   1,
    TIMER2_IRQn           =   2,
    TIMER3_IRQn           =   3,
    TIMER4_IRQn           =   4,
    TIMER5_IRQn           =   5,
    WDT0_IRQn             =   6,
    WDT1_IRQn             =   7,
    WDT2_IRQn             =   8,
    WDT3_IRQn             =   9,
    N_INTR_IRQn           =  10,
    S_INTR_IRQn           =  11,
    FLASH_IRQn            =  12,
    LCD_IRQn              =  13,
    OV5640_IRQn          =  14,
    UART0_IRQn            =  15,
    UART1_IRQn            =  16,
    UART2_IRQn            =  17,
    UART3_IRQn            =  18,
    I2C0_IRQn             =  19,
    I2C1_IRQn             =  20,
    I2C2_IRQn             =  21,
    I2C3_IRQn             =  22,
    GPIO_IRQn             =  23,
    I2S0_IRQn             =  24,
    I2S1_IRQn             =  25,
    PWM_IRQn              =  26,
    DMA0_IRQn             =  27,
    DMA1_IRQn             =  28,
    SPI0_IRQn             =  29,
    SPI1_IRQn             =  30,
    SDIO0_IRQn            =  31,
    SDIO1_IRQn            =  32,
    AON_GPIO_IRQn         =  33,
    SSI1_IRQn             =  34,
    SSI0_IRQn             =  35,
    ETH_IRQn              =  36,
    WDT_RST_IRQn          =  37,
    RTC_IRQn              =  38
} IRQn_Type;

#include "core_cm4.h"

#ifdef __cplusplus
extern "C" {
#endif

void SystemInit(void);
void SystemCoreClockUpdate(void);
extern uint32_t SystemCoreClock;

#ifdef __cplusplus
}
#endif

#endif
