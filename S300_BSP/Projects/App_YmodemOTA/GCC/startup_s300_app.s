/**
 * @file startup_s300_app.s
 * @brief S300 App启动文件
 * @version 1.0
 * @date 2024-01-15
 */

    .syntax unified
    .arch armv7e-m
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

.global g_pfnVectors
.global Default_Handler

/* 栈顶地址 */
.word _estack

/* 向量表入口 */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object

g_pfnVectors:
    .word _estack                   /* 栈顶指针 */
    .word Reset_Handler             /* 复位处理程序 */
    .word NMI_Handler               /* NMI处理程序 */
    .word HardFault_Handler         /* 硬件错误处理程序 */
    .word MemManage_Handler         /* 内存管理错误处理程序 */
    .word BusFault_Handler          /* 总线错误处理程序 */
    .word UsageFault_Handler        /* 使用错误处理程序 */
    .word 0                         /* 保留 */
    .word 0                         /* 保留 */
    .word 0                         /* 保留 */
    .word 0                         /* 保留 */
    .word SVC_Handler               /* SVCall处理程序 */
    .word DebugMon_Handler          /* 调试监视器处理程序 */
    .word 0                         /* 保留 */
    .word PendSV_Handler            /* PendSV处理程序 */
    .word SysTick_Handler           /* SysTick处理程序 */

    /* 外部中断 */
    .word UART0_IRQHandler          /* UART0中断 */
    .word UART1_IRQHandler          /* UART1中断 */
    .word UART2_IRQHandler          /* UART2中断 */
    .word UART3_IRQHandler          /* UART3中断 */
    .word I2C0_IRQHandler           /* I2C0中断 */
    .word I2C1_IRQHandler           /* I2C1中断 */
    .word SPI0_IRQHandler           /* SPI0中断 */
    .word SPI1_IRQHandler           /* SPI1中断 */
    .word QSPI_IRQHandler           /* QSPI中断 */
    .word DMA_IRQHandler            /* DMA中断 */
    .word GPIO_IRQHandler           /* GPIO中断 */
    .word TIMER0_IRQHandler         /* TIMER0中断 */
    .word TIMER1_IRQHandler         /* TIMER1中断 */
    .word TIMER2_IRQHandler         /* TIMER2中断 */
    .word TIMER3_IRQHandler         /* TIMER3中断 */
    .word RTC_IRQHandler            /* RTC中断 */
    .word WDT_IRQHandler            /* 看门狗中断 */
    .word ADC_IRQHandler            /* ADC中断 */
    .word I2S0_IRQHandler           /* I2S0中断 */
    .word I2S1_IRQHandler           /* I2S1中断 */

.size g_pfnVectors, .-g_pfnVectors

.text
.thumb

/**
 * @brief 复位处理程序
 */
.thumb_func
Reset_Handler:
    /* 初始化栈指针 */
    ldr r0, =_estack
    mov sp, r0

    /* 复制数据段到RAM（对于SRAM应用，这一步可以跳过） */
    /* 由于我们直接在SRAM中运行，不需要从Flash复制数据 */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sdata  /* 源地址与目标地址相同 */
    
    /* 跳过数据复制，直接初始化BSS */
    b InitBss

InitBss:
    /* 初始化BSS段 */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    /* 启用FPU */
    ldr r0, =0xE000ED88        /* CPACR地址 */
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)   /* 设置CP10和CP11位 */
    str r1, [r0]
    dsb
    isb

    /* 调用系统初始化 */
    bl SystemInit

    /* 调用main函数 */
    bl main

    /* 无限循环 */
Infinite_Loop:
    b Infinite_Loop

.size Reset_Handler, .-Reset_Handler

/**
 * @brief 默认处理程序
 */
.section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop_Default:
    b Infinite_Loop_Default
.size Default_Handler, .-Default_Handler

/* 弱符号定义 */
.weak NMI_Handler
.thumb_set NMI_Handler,Default_Handler

.weak HardFault_Handler
.thumb_set HardFault_Handler,Default_Handler

.weak MemManage_Handler
.thumb_set MemManage_Handler,Default_Handler

.weak BusFault_Handler
.thumb_set BusFault_Handler,Default_Handler

.weak UsageFault_Handler
.thumb_set UsageFault_Handler,Default_Handler

.weak SVC_Handler
.thumb_set SVC_Handler,Default_Handler

.weak DebugMon_Handler
.thumb_set DebugMon_Handler,Default_Handler

.weak PendSV_Handler
.thumb_set PendSV_Handler,Default_Handler

.weak SysTick_Handler
.thumb_set SysTick_Handler,Default_Handler

.weak UART0_IRQHandler
.thumb_set UART0_IRQHandler,Default_Handler

.weak UART1_IRQHandler
.thumb_set UART1_IRQHandler,Default_Handler

.weak UART2_IRQHandler
.thumb_set UART2_IRQHandler,Default_Handler

.weak UART3_IRQHandler
.thumb_set UART3_IRQHandler,Default_Handler

.weak I2C0_IRQHandler
.thumb_set I2C0_IRQHandler,Default_Handler

.weak I2C1_IRQHandler
.thumb_set I2C1_IRQHandler,Default_Handler

.weak SPI0_IRQHandler
.thumb_set SPI0_IRQHandler,Default_Handler

.weak SPI1_IRQHandler
.thumb_set SPI1_IRQHandler,Default_Handler

.weak QSPI_IRQHandler
.thumb_set QSPI_IRQHandler,Default_Handler

.weak DMA_IRQHandler
.thumb_set DMA_IRQHandler,Default_Handler

.weak GPIO_IRQHandler
.thumb_set GPIO_IRQHandler,Default_Handler

.weak TIMER0_IRQHandler
.thumb_set TIMER0_IRQHandler,Default_Handler

.weak TIMER1_IRQHandler
.thumb_set TIMER1_IRQHandler,Default_Handler

.weak TIMER2_IRQHandler
.thumb_set TIMER2_IRQHandler,Default_Handler

.weak TIMER3_IRQHandler
.thumb_set TIMER3_IRQHandler,Default_Handler

.weak RTC_IRQHandler
.thumb_set RTC_IRQHandler,Default_Handler

.weak WDT_IRQHandler
.thumb_set WDT_IRQHandler,Default_Handler

.weak ADC_IRQHandler
.thumb_set ADC_IRQHandler,Default_Handler

.weak I2S0_IRQHandler
.thumb_set I2S0_IRQHandler,Default_Handler

.weak I2S1_IRQHandler
.thumb_set I2S1_IRQHandler,Default_Handler

/************************ (C) COPYRIGHT S300 BSP *****END OF FILE****/
