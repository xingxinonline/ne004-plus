.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.extern SystemInit
.extern main

/* Linker provides _estack */

.section .isr_vector,"a",%progbits
.global g_pfnVectors
.type g_pfnVectors, %object
g_pfnVectors:
    /* Vector table: core exceptions + a subset of device IRQs (expand as needed) */
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* Device IRQs start here (index 0 corresponds to first device IRQ) */
    .word TIMER0_IRQHandler     /* 0 */
    .word TIMER1_IRQHandler     /* 1 */
    .word TIMER2_IRQHandler     /* 2 */
    .word TIMER3_IRQHandler     /* 3 */
    .word TIMER4_IRQHandler     /* 4 */
    .word TIMER5_IRQHandler     /* 5 */
    .word WDT0_IRQHandler       /* 6 */
    .word WDT1_IRQHandler       /* 7 */
    .word WDT2_IRQHandler       /* 8 */
    .word WDT3_IRQHandler       /* 9 */
    .word N_INTR_IRQHandler     /* 10 */
    .word S_INTR_IRQHandler     /* 11 */
    .word FLASH_IRQHandler      /* 12 */
    .word LCD_IRQHandler        /* 13 */
    .word OV5640_IRQHandler     /* 14 */
    .word UART0_IRQHandler      /* 15 */
    .word UART1_IRQHandler      /* 16 */
    .word UART2_IRQHandler      /* 17 */
    .word UART3_IRQHandler      /* 18 */
    .word I2C0_IRQHandler       /* 19 */
    .word I2C1_IRQHandler       /* 20 */
    .word I2C2_IRQHandler       /* 21 */
    .word I2C3_IRQHandler       /* 22 */
    .word GPIO_IRQHandler       /* 23 */
    .word I2S0_IRQHandler       /* 24 */
    .word I2S1_IRQHandler       /* 25 */
    .word PWM_IRQHandler        /* 26 */
    .word DMA0_IRQHandler       /* 27 */
    .word DMA1_IRQHandler       /* 28 */
    .word SPI0_IRQHandler       /* 29 */
    .word SPI1_IRQHandler       /* 30 */
    .word SDIO0_IRQHandler      /* 31 */
    .word SDIO1_IRQHandler      /* 32 */
    .word AON_GPIO_IRQHandler   /* 33 */
    .word SSI1_IRQHandler       /* 34 */
    .word SSI0_IRQHandler       /* 35 */
    .word ETH_IRQHandler        /* 36 */
    .word WDT_RST_IRQHandler    /* 37 */
    .word RTC_IRQHandler        /* 38 */
.size g_pfnVectors, .-g_pfnVectors

/* Code section */
.section .text.Reset_Handler,"ax",%progbits
.global Reset_Handler
.thumb_func
Reset_Handler:
    /* Zero initialize .bss */
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
1:
    cmp r0, r1
    bcc 2f
    b 3f
2:
    str r2, [r0], #4
    b 1b
3:
    ldr r0, =SystemInit
    blx r0
    ldr r0, =main
    blx r0
    b .

.section .text.DefaultHandlers,"ax",%progbits
.weak NMI_Handler
.thumb_func
NMI_Handler:
    b .

.weak HardFault_Handler
.thumb_func
HardFault_Handler:
    b .

.weak MemManage_Handler
.thumb_func
MemManage_Handler:
    b .

.weak BusFault_Handler
.thumb_func
BusFault_Handler:
    b .

.weak UsageFault_Handler
.thumb_func
UsageFault_Handler:
    b .

.weak SVC_Handler
.thumb_func
SVC_Handler:
    b .

.weak DebugMon_Handler
.thumb_func
DebugMon_Handler:
    b .

.weak PendSV_Handler
.thumb_func
PendSV_Handler:
    b .

.weak SysTick_Handler
.thumb_func
SysTick_Handler:
    b .

/* Weak device IRQ handlers */
.weak TIMER0_IRQHandler
.thumb_func
TIMER0_IRQHandler: b .
.weak TIMER1_IRQHandler
.thumb_func
TIMER1_IRQHandler: b .
.weak TIMER2_IRQHandler
.thumb_func
TIMER2_IRQHandler: b .
.weak TIMER3_IRQHandler
.thumb_func
TIMER3_IRQHandler: b .
.weak TIMER4_IRQHandler
.thumb_func
TIMER4_IRQHandler: b .
.weak TIMER5_IRQHandler
.thumb_func
TIMER5_IRQHandler: b .
.weak WDT0_IRQHandler
.thumb_func
WDT0_IRQHandler: b .
.weak WDT1_IRQHandler
.thumb_func
WDT1_IRQHandler: b .
.weak WDT2_IRQHandler
.thumb_func
WDT2_IRQHandler: b .
.weak WDT3_IRQHandler
.thumb_func
WDT3_IRQHandler: b .
.weak N_INTR_IRQHandler
.thumb_func
N_INTR_IRQHandler: b .
.weak S_INTR_IRQHandler
.thumb_func
S_INTR_IRQHandler: b .
.weak FLASH_IRQHandler
.thumb_func
FLASH_IRQHandler: b .
.weak LCD_IRQHandler
.thumb_func
LCD_IRQHandler: b .
.weak OV5640_IRQHandler
.thumb_func
OV5640_IRQHandler: b .
.weak UART0_IRQHandler
.thumb_func
UART0_IRQHandler: b .
.weak UART1_IRQHandler
.thumb_func
UART1_IRQHandler: b .
.weak UART2_IRQHandler
.thumb_func
UART2_IRQHandler: b .
.weak UART3_IRQHandler
.thumb_func
UART3_IRQHandler: b .
.weak I2C0_IRQHandler
.thumb_func
I2C0_IRQHandler: b .
.weak I2C1_IRQHandler
.thumb_func
I2C1_IRQHandler: b .
.weak I2C2_IRQHandler
.thumb_func
I2C2_IRQHandler: b .
.weak I2C3_IRQHandler
.thumb_func
I2C3_IRQHandler: b .
.weak GPIO_IRQHandler
.thumb_func
GPIO_IRQHandler: b .
.weak I2S0_IRQHandler
.thumb_func
I2S0_IRQHandler: b .
.weak I2S1_IRQHandler
.thumb_func
I2S1_IRQHandler: b .
.weak PWM_IRQHandler
.thumb_func
PWM_IRQHandler: b .
.weak DMA0_IRQHandler
.thumb_func
DMA0_IRQHandler: b .
.weak DMA1_IRQHandler
.thumb_func
DMA1_IRQHandler: b .
.weak SPI0_IRQHandler
.thumb_func
SPI0_IRQHandler: b .
.weak SPI1_IRQHandler
.thumb_func
SPI1_IRQHandler: b .
.weak SDIO0_IRQHandler
.thumb_func
SDIO0_IRQHandler: b .
.weak SDIO1_IRQHandler
.thumb_func
SDIO1_IRQHandler: b .
.weak AON_GPIO_IRQHandler
.thumb_func
AON_GPIO_IRQHandler: b .
.weak SSI1_IRQHandler
.thumb_func
SSI1_IRQHandler: b .
.weak SSI0_IRQHandler
.thumb_func
SSI0_IRQHandler: b .
.weak ETH_IRQHandler
.thumb_func
ETH_IRQHandler: b .
.weak WDT_RST_IRQHandler
.thumb_func
WDT_RST_IRQHandler: b .
.weak RTC_IRQHandler
.thumb_func
RTC_IRQHandler: b .
