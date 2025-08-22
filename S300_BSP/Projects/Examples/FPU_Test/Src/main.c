#include "s300.h"
#include "s300_bsp.h"
#include "s300_uart.h"

static void u32_to_hex(uint32_t v, char *out)
{
    static const char hc[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) out[7 - i] = hc[(v >> (i * 4)) & 0xF];
    out[8] = 0;
}

static inline uint32_t f32_to_bits(float f)
{
    union
    {
        float f;
        uint32_t u;
    } v;
    v.f = f;
    return v.u;
}

static void fpu_selftest(void)
{
    /* 打印 CPACR */
    uint32_t cpacr = SCB->CPACR;
    char h[9];
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: CPACR=0x");
    u32_to_hex(cpacr, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    /* DWT 计数 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 打印 DEMCR 和 DWT->CTRL 诊断 */
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: DEMCR=0x");
    u32_to_hex(CoreDebug->DEMCR, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", DWT->CTRL=0x");
    u32_to_hex(DWT->CTRL, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    /* 如果 NOCYCCNT 置位，说明不支持硬件周期计数 */
    if (DWT->CTRL & (1u << 25))
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: DWT CYCCNT not implemented (NOCYCCNT=1)\n");
    }
    float a[64], b[64];
    for (int i = 0; i < 64; ++i)
    {
        a[i] = (float)(i + 1) * 0.0314159f;
        b[i] = 1.0f / (float)(i + 3);
    }
    volatile float acc_f = 0.0f;
    const int repeat = 200;
    uint32_t t0_ms = S300_SysTick_Millis();
    uint32_t start = DWT->CYCCNT;
    for (int r = 0; r < repeat; ++r)
    {
        float s = 0.0f;
        for (int i = 0; i < 64; ++i) s += a[i] * b[i];
        acc_f += s;
    }
    uint32_t cycles = DWT->CYCCNT - start;
    uint32_t dt_ms = S300_SysTick_Millis() - t0_ms;
    double s_ref = 0.0;
    for (int i = 0; i < 64; ++i) s_ref += (double)a[i] * (double)b[i];
    float avg_f = acc_f / (float)repeat;
    float s_ref_f = (float)s_ref;
    float diff = avg_f - s_ref_f;
    if (diff < 0) diff = -diff;
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: avg=0x");
    u32_to_hex(f32_to_bits(avg_f), h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", ref=0x");
    u32_to_hex(f32_to_bits(s_ref_f), h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", |diff|=0x");
    u32_to_hex(f32_to_bits(diff), h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    u32_to_hex(cycles, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: cycles=0x");
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, " for 0x");
    u32_to_hex((uint32_t)(repeat * 64), h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, " FMACs\n");
    if (cycles == 0)
    {
        /* 回退：打印基于 SysTick 的毫秒耗时 */
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: fallback dt_ms=");
        u32_to_hex(dt_ms, h);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, " ms (SysTick)\n");
        /* 估算周期数（基于 SystemCoreClock）*/
        uint32_t est_cycles = (SystemCoreClock / 1000u) * dt_ms;
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU_Test: est_cycles=0x");
        u32_to_hex(est_cycles, h);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    }
    const float tol = 1e-4f;
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, (diff < tol) ? "FPU_Test: PASS\n" : "FPU_Test: FAIL\n");
}

int main(void)
{
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    S300_SysTick_Init();
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "FPU Test Start\n");
    fpu_selftest();
    while (1)
    {
        S300_DelayMs(1000);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ".\n");
    }
}
