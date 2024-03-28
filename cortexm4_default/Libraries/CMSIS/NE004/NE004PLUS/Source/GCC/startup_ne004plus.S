;/*
; * Copyright (c) 2006-2021, RT-Thread Development Team
; *
; * SPDX-License-Identifier: Apache-2.0
; *
; * Change Logs:
; * Date           Author       Notes
; * 2018-05-22     tanek        first implementation
; */

.syntax unified
.arch	armv7-m
.cpu cortex-m4
.fpu softvfp
.thumb

.global  g_pfnVectors
.global  Default_Handler

    .section  .isr_vector,"ax",%progbits
    .type  g_pfnVectors, %object

g_pfnVectors:
    .word     _estack                       // Top of Stack
    .word     Reset_Handler                     // Reset Handler
    .word     NMI_Handler                       // NMI Handler
    .word     HardFault_Handler                 // Hard Fault Handler
    .word     MemManage_Handler                 // MPU Fault Handler
    .word     BusFault_Handler                  // Bus Fault Handler
    .word     UsageFault_Handler                // Usage Fault Handler
    .word     0                                 // Reserved
    .word     0                                 // Reserved
    .word     0                                 // Reserved
    .word     0                                 // Reserved
    .word     SVC_Handler                       // SVCall Handler
    .word     DebugMon_Handler                  // Debug Monitor Handler
    .word     0                                 // Reserved
    .word     PendSV_Handler                    // PendSV Handler
    .word     SysTick_Handler                   // SysTick Handler

    // external interrupts handler
    .word     RISCV2ARM_IRQHandler                  // 16:Window Watchdog Timer
    .word     RXOVRINT0_IRQHandler                    // 17:LVD through EXTI Line detect
    .word     TXOVRINT0_IRQHandler           // 18:Tamper and TimeStamp through EXTI Line detect
    .word     RXINT0_IRQHandler               // 19:RTC Wakeup through EXTI Line
    .word     TXINT0_IRQHandler                    // 20:FMC
    .word     TIMER0_IRQHandler                // 21:RCU and CTC
    .word     TIMER1_IRQHandler                  // 22:EXTI Line 0
    .word     RTCINT_IRQHandler                  // 23:EXTI Line 1
    .word     DMAC0_TRANS_DONe_IRQHandler                  // 24:EXTI Line 2
    .word     DMAC0_BLOCK_DONe_IRQHandler                  // 25:EXTI Line 3
    .word     DMAC0_SRC_DONE_IRQHandler                  // 26:EXTI Line 4
    .word     DMAC0_DST_DONE_IRQHandler          // 27:DMA0 Channel0
    .word     DMAC0_ERR_IRQHandler                  // 28:DMA0 Channel1
    .word     DMAC0_COMBINE_DONE_IRQHandler          // 29:DMA0 Channel2
    .word     0                   // 30:DMA0 Channel3
    .word     REQ_FFT_DONE_IRQHandler          // 31:DMA0 Channel4
    .word     DMAC1_TRANS_DONE_IRQHandler          // 32:DMA0 Channel5
    .word     DMAC1_BLOCK_DONE_IRQHandler          // 33:DMA0 Channel6
    .word     DMAC1_SRC_DONE_IRQHandler                    // 34:ADC
    .word     DMAC1_DST_DONE_IRQHandler                // 35:CAN0 TX
    .word     DMAC1_ERR_IRQHandler               // 36:CAN0 RX0
    .word     DMAC1_COMBINE_DONE_IRQHandler               // 37:CAN0 RX1
    .word     0               // 38:CAN0 EWMC
    .word     FLASH_SSI_INTR_IRQHandler                // 39:EXTI5 to EXTI9
    .word     AAON_ACTIVE_IRQHandler            // 40:TIMER0 Break and TIMER8
    .word     VAON_ACTIVE_IRQHandler                // 41:TIMER0 Update and TIMER9
    .word     PDM_AVAIL_IRQHandler                  // 42:TIMER0 Trigger and Commutation and TIMER10
    .word     0              // 43:TIMER0 Capture Compare
    .word     I2C0_IRQOUT_IRQHandler                 // 44:TIMER1
    .word     I2C1_IRQOUT_IRQHandler                 // 45:TIMER2
    .word     I2S1_IRQOUT_IRQHandler                 // 46:TIMER3
    .word     I2S0_IRQOUT_IRQHandler                // 47:I2C0 Event
    .word     GPIO0_N0_IRQHandler                // 48:I2C0 Error
    .word     GPIO0_N1_IRQHandler                // 49:I2C1 Event
    .word     GPIO0_N2_IRQHandler                // 50:I2C1 Error
    .word     GPIO0_N3_IRQHandler                   // 51:SPI0
    .word     GPIO0_N4_IRQHandler                   // 52:SPI1
    .word     GPIO0_N5_IRQHandler                 // 53:USART0
    .word     GPIO0_N6_IRQHandler                 // 54:USART1
    .word     GPIO0_N7_IRQHandler                 // 55:USART2
    .word     GPIO0_N8_IRQHandler              // 56:EXTI10 to EXTI15
    .word     GPIO0_N9_IRQHandler              // 57:RTC Alarm
    .word     GPIO0_N10_IRQHandler             // 58:USBFS Wakeup
    .word     GPIO0_N11_IRQHandler                  // 59:TIMER7 Break and TIMER11
    .word     GPIO0_N12_IRQHandler                  // 60:TIMER7 Update and TIMER12
    .word     GPIO0_N13_IRQHandler                      // 61:TIMER7 Trigger and Commutation and TIMER13
    .word     GPIO0_N14_IRQHandler              // 62:TIMER7 Capture Compare
    .word     GPIO0_N15_IRQHandler          // 63:DMA0 Channel7
    .word     GPIO0_N16_IRQHandler          // 63:DMA0 Channel7
    .word     GPIO0_N17_IRQHandler          // 63:DMA0 Channel7
    .word     GPIO0_N18_IRQHandler          // 63:DMA0 Channel7
    .word     PWM0_IRQHandler                   // 63:DMA0 Channel7
    .word     PWM1_IRQHandler                   // 63:DMA0 Channel7
    .word     SSI_MST_IRQHandler          // 63:DMA0 Channel7
    .word     SSI_RXF_IRQHandler          // 63:DMA0 Channel7
    .word     SSI_RXO_IRQHandler          // 63:DMA0 Channel7
    .word     SSI_RXU_IRQHandler          // 63:DMA0 Channel7
    .word     SSI_TXE_IRQHandler          // 63:DMA0 Channel7
    .word     SSI_TXO_IRQHandler          // 63:DMA0 Channel7
    .word     RISCV_NOTICE_IRQHandler          // 63:DMA0 Channel7
    .word     0          // 63:DMA0 Channel7
    .word     0          // 63:DMA0 Channel7
    .word     0          // 63:DMA0 Channel7
    .word     0          // 63:DMA0 Channel7

    .size  g_pfnVectors, .-g_pfnVectors

    .thumb
    .section  .text.Reset_Handler
    .align   2

    .thumb_func
    .weak  Reset_Handler
    .type  Reset_Handler, %function
    .globl Reset_Handler
    .fnstart
Reset_Handler:
    ldr sp, =_estack      /* set stack pointer */
    ldr r1, =_sidata
    ldr r2, =_sdata
    ldr r3, =_edata

    subs r3, r2
    ble fill_bss_start

loop_copy_data:
    subs r3, #4
    ldr r0, [r1,r3]
    str r0, [r2,r3]
    bgt loop_copy_data

fill_bss_start:
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    movs r0, 0
    subs r2, r1
    ble startup_enter

loop_fill_bss:
    subs r2, #4
    str r0, [r1, r2]
    bgt loop_fill_bss

startup_enter:
    bl SystemInit
    bl main

    /* Exception Handlers */
    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    b       .
    .size   NMI_Handler, . - NMI_Handler

    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    b       .
    .size   HardFault_Handler, . - HardFault_Handler

    .weak   MemManage_Handler
    .type   MemManage_Handler, %function
MemManage_Handler:
    b       .
    .size   MemManage_Handler, . - MemManage_Handler

    .weak   BusFault_Handler
    .type   BusFault_Handler, %function
BusFault_Handler:
    b       .
    .size   BusFault_Handler, . - BusFault_Handler

    .weak   UsageFault_Handler
    .type   UsageFault_Handler, %function
UsageFault_Handler:
    b       .
    .size   UsageFault_Handler, . - UsageFault_Handler

    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    b       .
    .size   SVC_Handler, . - SVC_Handler

    .weak   DebugMon_Handler
    .type   DebugMon_Handler, %function
DebugMon_Handler:
    b       .
    .size   DebugMon_Handler, . - DebugMon_Handler

    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    b       .
    .size   PendSV_Handler, . - PendSV_Handler

    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    b       .
    .size   SysTick_Handler, . - SysTick_Handler

    /* IQR Handler */
    .section  .text.Default_Handler,"ax",%progbits
    .type  Default_Handler, %function
Default_Handler:
    b  .
    .size  Default_Handler, . - Default_Handler

    .macro  IRQ handler
    .weak   \handler
    .set    \handler, Default_Handler
    .endm

    IRQ RISCV2ARM_IRQHandler
    IRQ RXOVRINT0_IRQHandler
    IRQ TXOVRINT0_IRQHandler
    IRQ RXINT0_IRQHandler
    IRQ TXINT0_IRQHandler
    IRQ TIMER0_IRQHandler
    IRQ TIMER1_IRQHandler
    IRQ RTCINT_IRQHandler
    IRQ DMAC0_TRANS_DONe_IRQHandler
    IRQ DMAC0_BLOCK_DONe_IRQHandler
    IRQ DMAC0_SRC_DONE_IRQHandler
    IRQ DMAC0_DST_DONE_IRQHandler
    IRQ DMAC0_ERR_IRQHandler
    IRQ DMAC0_COMBINE_DONE_IRQHandler
    IRQ REQ_FFT_DONE_IRQHandler
    IRQ DMAC1_TRANS_DONE_IRQHandler
    IRQ DMAC1_BLOCK_DONE_IRQHandler
    IRQ DMAC1_SRC_DONE_IRQHandler
    IRQ DMAC1_DST_DONE_IRQHandler
    IRQ DMAC1_ERR_IRQHandler
    IRQ DMAC1_COMBINE_DONE_IRQHandler
    IRQ FLASH_SSI_INTR_IRQHandler
    IRQ AAON_ACTIVE_IRQHandler
    IRQ VAON_ACTIVE_IRQHandler
    IRQ PDM_AVAIL_IRQHandler
    IRQ I2C0_IRQOUT_IRQHandler
    IRQ I2C1_IRQOUT_IRQHandler
    IRQ I2S1_IRQOUT_IRQHandler
    IRQ I2S0_IRQOUT_IRQHandler
    IRQ GPIO0_N0_IRQHandler
    IRQ GPIO0_N1_IRQHandler
    IRQ GPIO0_N2_IRQHandler
    IRQ GPIO0_N3_IRQHandler
    IRQ GPIO0_N4_IRQHandler
    IRQ GPIO0_N5_IRQHandler
    IRQ GPIO0_N6_IRQHandler
    IRQ GPIO0_N7_IRQHandler
    IRQ GPIO0_N8_IRQHandler
    IRQ GPIO0_N9_IRQHandler
    IRQ GPIO0_N10_IRQHandler
    IRQ GPIO0_N11_IRQHandler
    IRQ GPIO0_N12_IRQHandler
    IRQ GPIO0_N13_IRQHandler
    IRQ GPIO0_N14_IRQHandler
    IRQ GPIO0_N15_IRQHandler
    IRQ GPIO0_N16_IRQHandler
    IRQ GPIO0_N17_IRQHandler
    IRQ GPIO0_N18_IRQHandler
    IRQ PWM0_IRQHandler
    IRQ PWM1_IRQHandler
    IRQ SSI_MST_IRQHandler
    IRQ SSI_RXF_IRQHandler
    IRQ SSI_RXO_IRQHandler
    IRQ SSI_RXU_IRQHandler
    IRQ SSI_TXE_IRQHandler
    IRQ SSI_TXO_IRQHandler
    IRQ RISCV_NOTICE_IRQHandler