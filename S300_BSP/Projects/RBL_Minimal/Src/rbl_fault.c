/**
 * @file rbl_fault.c
 * @brief RBL异常处理和调试辅助
 */

#include "rbl_hal.h"
#include "s300.h"

/* HardFault寄存器结构 */
typedef struct
{
    uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
} hardfault_context_t;

/**
 * @brief HardFault处理函数
 * @param sp 栈指针，包含异常上下文
 */
void HardFault_Handler_C(hardfault_context_t *ctx)
{
    volatile uint32_t cfsr = SCB->CFSR;
    volatile uint32_t hfsr = SCB->HFSR;
    volatile uint32_t dfsr = SCB->DFSR;
    volatile uint32_t afsr = SCB->AFSR;
    volatile uint32_t bfar = SCB->BFAR;
    volatile uint32_t mmfar = SCB->MMFAR;
    /* 尝试输出错误信息（如果UART还能工作） */
    RBL_LOG("\r\n=== HARDFAULT DETECTED ===\r\n");
    RBL_LOG("PC: 0x%08X\r\n", ctx->pc);
    RBL_LOG("LR: 0x%08X\r\n", ctx->lr);
    RBL_LOG("PSR: 0x%08X\r\n", ctx->psr);
    RBL_LOG("R0: 0x%08X\r\n", ctx->r0);
    RBL_LOG("R1: 0x%08X\r\n", ctx->r1);
    RBL_LOG("R2: 0x%08X\r\n", ctx->r2);
    RBL_LOG("R3: 0x%08X\r\n", ctx->r3);
    RBL_LOG("R12: 0x%08X\r\n", ctx->r12);
    RBL_LOG("CFSR: 0x%08X\r\n", cfsr);
    RBL_LOG("HFSR: 0x%08X\r\n", hfsr);
    RBL_LOG("DFSR: 0x%08X\r\n", dfsr);
    RBL_LOG("AFSR: 0x%08X\r\n", afsr);
    if (cfsr & 0x80)
    {
        RBL_LOG("MMFAR: 0x%08X\r\n", mmfar);
    }
    if (cfsr & 0x8000)
    {
        RBL_LOG("BFAR: 0x%08X\r\n", bfar);
    }
    /* 分析错误类型 */
    if (cfsr & 0xFF)
    {
        RBL_LOG("MemManage Fault:\r\n");
        if (cfsr & 0x01) RBL_LOG("- Instruction access violation\r\n");
        if (cfsr & 0x02) RBL_LOG("- Data access violation\r\n");
        if (cfsr & 0x08) RBL_LOG("- MemManage on unstacking\r\n");
        if (cfsr & 0x10) RBL_LOG("- MemManage on stacking\r\n");
        if (cfsr & 0x20) RBL_LOG("- MemManage on FP lazy state\r\n");
    }
    if (cfsr & 0xFF00)
    {
        RBL_LOG("BusFault:\r\n");
        if (cfsr & 0x0100) RBL_LOG("- Instruction bus error\r\n");
        if (cfsr & 0x0200) RBL_LOG("- Precise data bus error\r\n");
        if (cfsr & 0x0400) RBL_LOG("- Imprecise data bus error\r\n");
        if (cfsr & 0x0800) RBL_LOG("- BusFault on unstacking\r\n");
        if (cfsr & 0x1000) RBL_LOG("- BusFault on stacking\r\n");
        if (cfsr & 0x2000) RBL_LOG("- BusFault on FP lazy state\r\n");
    }
    if (cfsr & 0xFF0000)
    {
        RBL_LOG("UsageFault:\r\n");
        if (cfsr & 0x010000) RBL_LOG("- Undefined instruction\r\n");
        if (cfsr & 0x020000) RBL_LOG("- Invalid state\r\n");
        if (cfsr & 0x040000) RBL_LOG("- Invalid PC load\r\n");
        if (cfsr & 0x080000) RBL_LOG("- No coprocessor\r\n");
        if (cfsr & 0x1000000) RBL_LOG("- Unaligned access\r\n");
        if (cfsr & 0x2000000) RBL_LOG("- Divide by zero\r\n");
    }
    RBL_LOG("=== END HARDFAULT INFO ===\r\n");
    /* 无限循环等待调试器 */
    while (1)
    {
        __WFI();
    }
}

/**
 * @brief HardFault处理汇编入口
 */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b HardFault_Handler_C\n"
    );
}
