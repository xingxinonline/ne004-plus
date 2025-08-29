/**
 * @file system_s300_app.c
 * @brief S300 App系统初始化
 * @version 1.0
 * @date 2024-01-15
 */

#include "app_config.h"

/* 系统时钟频率 */
uint32_t SystemCoreClock = 200000000; /* 200MHz */

/* 私有函数声明 */
static void SetupSystemClock(void);
static void ConfigureFPU(void);
static uint32_t GetSysTick(void);

/**
 * @brief 系统初始化
 */
void SystemInit(void)
{
    /* 配置FPU */
    ConfigureFPU();
    
    /* 设置系统时钟 */
    SetupSystemClock();
    
    /* 更新系统时钟变量 */
    SystemCoreClockUpdate();
}

/**
 * @brief 更新系统时钟变量
 */
void SystemCoreClockUpdate(void)
{
    /* TODO: 从寄存器读取实际时钟频率 */
    SystemCoreClock = 200000000; /* 200MHz */
}

/**
 * @brief 配置FPU
 */
static void ConfigureFPU(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    /* 启用CP10和CP11协处理器 */
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
    __DSB();
    __ISB();
#endif
}

/**
 * @brief 设置系统时钟
 */
static void SetupSystemClock(void)
{
    /* TODO: 配置具体的时钟设置 */
    /* 这里应该配置PLL、分频器等 */
    
    /* 配置系统时钟为200MHz */
    SystemCoreClock = 200000000;
}

/**
 * @brief 获取系统时钟频率
 * @return 系统时钟频率(Hz)
 */
uint32_t GetSystemClock(void)
{
    return SystemCoreClock;
}

/**
 * @brief 延时函数(ms)
 * @param ms 延时时间(毫秒)
 */
void DelayMs(uint32_t ms)
{
    uint32_t start = GetSysTick();
    uint32_t delay_ticks = ms * (SystemCoreClock / 1000);
    
    while ((GetSysTick() - start) < delay_ticks) {
        __NOP();
    }
}

/**
 * @brief 延时函数(us)
 * @param us 延时时间(微秒)
 */
void DelayUs(uint32_t us)
{
    uint32_t start = GetSysTick();
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
    
    while ((GetSysTick() - start) < delay_ticks) {
        __NOP();
    }
}

/**
 * @brief 获取系统滴答计数
 * @return 滴答计数
 */
static uint32_t GetSysTick(void)
{
    return SysTick->VAL;
}

/**
 * @brief 系统复位
 */
void SystemReset(void)
{
    NVIC_SystemReset();
}

/**
 * @brief 错误处理函数
 * @param file 文件名
 * @param line 行号
 */
void Error_Handler(char *file, int line)
{
    APP_LOGE("SYS", "Error in %s at line %d", file, line);
    
    /* 禁用所有中断 */
    __disable_irq();
    
    /* 无限循环 */
    while (1) {
        __NOP();
    }
}

/**
 * @brief 断言失败处理
 * @param file 文件名
 * @param line 行号
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    APP_LOGE("SYS", "Assert failed in %s at line %lu", file, line);
    Error_Handler((char*)file, line);
}

/**
 * @brief 硬件错误处理
 */
void HardFault_Handler(void)
{
    APP_LOGE("SYS", "HardFault Exception occurred!");
    
    /* 保存错误信息 */
    volatile uint32_t *hardfault_args;
    
    __asm volatile (
        "tst lr, #4                                                \n"
        "ite eq                                                    \n"
        "mrseq r0, msp                                             \n"
        "mrsne r0, psp                                             \n"
        "mov %0, r0                                                \n"
        : "=r" (hardfault_args)
    );
    
    APP_LOGE("SYS", "R0: 0x%08lX", hardfault_args[0]);
    APP_LOGE("SYS", "R1: 0x%08lX", hardfault_args[1]);
    APP_LOGE("SYS", "R2: 0x%08lX", hardfault_args[2]);
    APP_LOGE("SYS", "R3: 0x%08lX", hardfault_args[3]);
    APP_LOGE("SYS", "R12: 0x%08lX", hardfault_args[4]);
    APP_LOGE("SYS", "LR: 0x%08lX", hardfault_args[5]);
    APP_LOGE("SYS", "PC: 0x%08lX", hardfault_args[6]);
    APP_LOGE("SYS", "PSR: 0x%08lX", hardfault_args[7]);
    
    /* 重启系统 */
    SystemReset();
}

/**
 * @brief 内存管理错误处理
 */
void MemManage_Handler(void)
{
    APP_LOGE("SYS", "MemManage Exception occurred!");
    SystemReset();
}

/**
 * @brief 总线错误处理
 */
void BusFault_Handler(void)
{
    APP_LOGE("SYS", "BusFault Exception occurred!");
    SystemReset();
}

/**
 * @brief 使用错误处理
 */
void UsageFault_Handler(void)
{
    APP_LOGE("SYS", "UsageFault Exception occurred!");
    SystemReset();
}

/************************ (C) COPYRIGHT S300 BSP *****END OF FILE****/
