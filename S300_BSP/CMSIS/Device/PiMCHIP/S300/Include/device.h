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
    SysTick_IRQn          =  -1  /* 15 System Tick Interrupt */
                             /* Device-specific IRQs can be added here starting at 0 */
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
