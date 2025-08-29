/**
 * @file startup_s300.c
 * @brief S300 PiMCHIP RBL启动文件
 */

#include <stdint.h>

/* 外部符号声明 */
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sidata;

/* 主程序入口 */
extern int main(void);

/* 系统初始化函数 */
extern void SystemInit(void);

/**
 * @brief Reset处理程序
 */
void Reset_Handler(void)
{
    /* 初始化数据段 */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    
    /* 清零BSS段 */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    
    /* 系统初始化 */
    SystemInit();
    
    /* 跳转到主程序 */
    main();
    
    /* 永远不应该到达这里 */
    while (1) {
        __asm("wfi");
    }
}

/**
 * @brief 默认中断处理程序
 */
void Default_Handler(void)
{
    while (1) {
        __asm("wfi");
    }
}

/* 中断服务程序声明 (弱符号) */
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* S300 外设中断 (根据实际硬件调整) */
void UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void QSPI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief 中断向量表
 */
__attribute__((section(".isr_vector")))
const uint32_t isr_vector[] = {
    /* Cortex-M4 核心中断 */
    (uint32_t)&_estack,                    /* 初始栈指针 */
    (uint32_t)Reset_Handler,               /* Reset */
    (uint32_t)NMI_Handler,                 /* NMI */
    (uint32_t)HardFault_Handler,           /* Hard Fault */
    (uint32_t)MemManage_Handler,           /* Memory Management */
    (uint32_t)BusFault_Handler,            /* Bus Fault */
    (uint32_t)UsageFault_Handler,          /* Usage Fault */
    0,                                     /* 保留 */
    0,                                     /* 保留 */
    0,                                     /* 保留 */
    0,                                     /* 保留 */
    (uint32_t)SVC_Handler,                 /* SVCall */
    (uint32_t)DebugMon_Handler,            /* Debug Monitor */
    0,                                     /* 保留 */
    (uint32_t)PendSV_Handler,              /* PendSV */
    (uint32_t)SysTick_Handler,             /* SysTick */
    
    /* S300 外设中断 (IRQ 0-15) */
    (uint32_t)UART0_IRQHandler,            /* IRQ 0: UART0 */
    (uint32_t)UART1_IRQHandler,            /* IRQ 1: UART1 */
    (uint32_t)UART2_IRQHandler,            /* IRQ 2: UART2 */
    (uint32_t)UART3_IRQHandler,            /* IRQ 3: UART3 */
    (uint32_t)QSPI_IRQHandler,             /* IRQ 4: QSPI */
    (uint32_t)GPIO_IRQHandler,             /* IRQ 5: GPIO */
    (uint32_t)DMA_IRQHandler,              /* IRQ 6: DMA */
    (uint32_t)I2C_IRQHandler,              /* IRQ 7: I2C */
    (uint32_t)I2S_IRQHandler,              /* IRQ 8: I2S */
    (uint32_t)RTC_IRQHandler,              /* IRQ 9: RTC */
    (uint32_t)WDT_IRQHandler,              /* IRQ 10: Watchdog */
    (uint32_t)TIM0_IRQHandler,             /* IRQ 11: Timer 0 */
    (uint32_t)TIM1_IRQHandler,             /* IRQ 12: Timer 1 */
    (uint32_t)TIM2_IRQHandler,             /* IRQ 13: Timer 2 */
    (uint32_t)TIM3_IRQHandler,             /* IRQ 14: Timer 3 */
    0,                                     /* IRQ 15: 保留 */
};
