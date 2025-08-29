/**
 * @file rbl_system.c
 * @brief RBL系统初始化(基于S300 BSP)
 */

#include "rbl_system.h"
#include "rbl_config.h"
#include "s300.h"
#include "rcc.h"
#include <string.h>

/* 系统时钟 - 使用extern声明，避免重复定义 */
extern uint32_t SystemCoreClock;

/* SysTick计数器 */
static volatile uint32_t system_tick = 0;

/**
 * @brief 早期系统初始化
 */
void rbl_system_early_init(void)
{
    /* 设置向量表到SRAM */
    SCB->VTOR = RBL_SRAM_BASE;
    __DSB();
    
    /* 启用FPU (如果需要) */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif
    
    /* 配置系统优先级分组 */
    NVIC_SetPriorityGrouping(4); /* 4位抢占优先级，0位子优先级 */
}

/**
 * @brief 系统初始化
 */
int rbl_system_init(void)
{
    /* 调用CMSIS标准系统初始化 */
    SystemInit();
    
    /* 初始化系统PLL到168MHz */
    /* refdiv=1, fbdiv=7, frac=0, postdiv1=1, postdiv2=1 */
    /* 24MHz * 7 / 1 / 1 = 168MHz */
    int ret = rcc_init_cortex_m4_pll(1, 7, 0, 1, 1);
    if (ret != 0) {
        return ret;
    }
    
    /* 设置CM4系统时钟 */
    rcc_set_cortex_m4_sys_clock(0, 0, 0, true);
    
    /* 更新系统时钟变量 */
    SystemCoreClock = RBL_SYSTEM_CLOCK_HZ;
    
    /* 初始化SysTick - 1ms中断 */
    if (SysTick_Config(SystemCoreClock / 1000) != 0) {
        return -1;
    }
    
    /* 设置SysTick中断优先级 */
    NVIC_SetPriority(SysTick_IRQn, 0);
    
    return 0;
}

/**
 * @brief 获取系统滴答计数
 */
uint32_t rbl_get_tick(void)
{
    return system_tick;
}

/**
 * @brief 延时(毫秒)
 */
void rbl_delay_ms(uint32_t ms)
{
    uint32_t start = system_tick;
    
    while ((system_tick - start) < ms) {
        __WFI();
    }
}

/**
 * @brief 延时(微秒) - 简单循环实现
 */
void rbl_delay_us(uint32_t us)
{
    /* 简单的循环延时，假设168MHz时钟 */
    uint32_t cycles = us * (SystemCoreClock / 1000000) / 3;
    
    while (cycles--) {
        __NOP();
    }
}

/**
 * @brief SysTick中断处理程序
 */
void SysTick_Handler(void)
{
    system_tick++;
}

/**
 * @brief 硬件故障处理程序
 */
void HardFault_Handler(void)
{
    /* 打印故障信息(如果串口已初始化) */
    printf("[RBL] Hard Fault at PC=0x%08lX\r\n", 
           (unsigned long)((uint32_t*)SCB->CFSR)[14]); /* 故障PC */
    
    /* 复位系统 */
    NVIC_SystemReset();
}

/**
 * @brief 内存管理故障处理程序
 */
void MemManage_Handler(void)
{
    printf("[RBL] Memory Management Fault\r\n");
    NVIC_SystemReset();
}

/**
 * @brief 总线故障处理程序
 */
void BusFault_Handler(void)
{
    printf("[RBL] Bus Fault\r\n");
    NVIC_SystemReset();
}

/**
 * @brief 用法故障处理程序
 */
void UsageFault_Handler(void)
{
    printf("[RBL] Usage Fault\r\n");
    NVIC_SystemReset();
}

/**
 * @brief 检查复位原因
 */
uint32_t rbl_get_reset_reason(void)
{
    /* S300的复位状态寄存器 - 需要根据实际芯片手册调整 */
    /* 这里假设使用RCC_CSR寄存器的复位标志位 */
    uint32_t rcc_csr = 0;
    
    /* 尝试从RCC控制状态寄存器读取复位原因 */
    /* 地址需要根据S300实际的RCC寄存器映射调整 */
    volatile uint32_t *rcc_base = (volatile uint32_t *)0x40021000; // 假设的RCC基地址
    
    /* 读取CSR寄存器 (Control/Status Register) */
    rcc_csr = rcc_base[0x74 / 4]; // 假设CSR在偏移0x74
    
    uint32_t reset_flags = 0;
    
    /* 分析复位标志位 (基于常见ARM Cortex-M实现) */
    if (rcc_csr & (1U << 31)) reset_flags |= 0x01;  // Low-power reset
    if (rcc_csr & (1U << 30)) reset_flags |= 0x02;  // Window watchdog reset
    if (rcc_csr & (1U << 29)) reset_flags |= 0x04;  // Independent watchdog reset
    if (rcc_csr & (1U << 28)) reset_flags |= 0x08;  // Software reset
    if (rcc_csr & (1U << 27)) reset_flags |= 0x10;  // POR/PDR reset
    if (rcc_csr & (1U << 26)) reset_flags |= 0x20;  // Pin reset
    if (rcc_csr & (1U << 25)) reset_flags |= 0x40;  // BOR reset
    
    /* 清除复位标志位 (写1清除) */
    rcc_base[0x74 / 4] |= (1U << 24); // RMVF位
    
    /* 如果没有检测到任何标志，返回默认值 */
    if (reset_flags == 0) {
        reset_flags = 0x01; // 默认为Power-on reset
    }
    
    return reset_flags;
}

/**
 * @brief 进入低功耗模式
 */
void rbl_system_sleep(void)
{
    /* 等待中断 */
    __WFI();
}

/**
 * @brief 获取系统信息
 */
int rbl_get_system_info(rbl_system_info_t *info)
{
    if (!info) {
        return -1;
    }
    
    /* 清零结构体 */
    memset(info, 0, sizeof(rbl_system_info_t));
    
    /* CPU ID - 从CPUID寄存器读取 */
    info->cpu_id = SCB->CPUID;
    
    /* 系统时钟 */
    info->system_clock = SystemCoreClock;
    
    /* SRAM大小 - S300固定256KB */
    info->sram_size = 256 * 1024;
    
    /* Flash大小 - W25Q128固定16MB */
    info->flash_size = 16 * 1024 * 1024;
    
    /* 复位原因 */
    info->reset_reason = rbl_get_reset_reason();
    
    /* 系统运行时间 */
    info->tick_count = system_tick;
    
    return 0;
}

/**
/**
 * @brief 系统软复位
 */
void rbl_system_reset(void)
{
    printf("[RBL] System Reset\r\n");
    
    /* 等待串口发送完成 */
    rbl_delay_ms(100);
    
    /* 执行系统复位 */
    NVIC_SystemReset();
}

/**
 * @brief 禁用中断
 */
uint32_t rbl_disable_irq(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
 * @brief 恢复中断
 */
void rbl_restore_irq(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/**
 * @brief 系统信息结构体定义
 */
