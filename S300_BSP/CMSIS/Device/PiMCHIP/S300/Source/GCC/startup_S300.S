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
    /* Minimal vector table: core exceptions only */
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
