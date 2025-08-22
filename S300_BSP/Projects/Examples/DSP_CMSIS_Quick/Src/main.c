#include "s300.h"
#include "s300_bsp.h"
#include "s300_uart.h"
#include <stdint.h>
#include <string.h>
#include "arm_math.h"

static void put_hex32(const char *prefix, uint32_t v)
{
    static const char hc[] = "0123456789ABCDEF";
    char h[9];
    for (int i = 7; i >= 0; --i) h[7 - i] = hc[(v >> (i * 4)) & 0xF];
    h[8] = 0;
    if (prefix) S300_UART_PutStringI(BOARD_UART_DEBUG_ID, prefix);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
}

static void quick_vector_add_check(void)
{
    // q15 向量加法: dst = srcA + srcB
    q15_t a[8] = {1000, -2000, 32760, -32760, 123, -456, 789, -1000};
    q15_t b[8] = {24,   35,   100,   -200,   -123, 456,  30000, -30000};
    q15_t dst[8];
    arm_add_q15(a, b, dst, 8);
    // 汇总校验值（避免影响现有 UART 打印节奏）
    int32_t acc = 0;
    for (int i = 0; i < 8; ++i) acc += dst[i];
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "CMSIS-DSP add_q15 acc=0x");
    put_hex32(NULL, (uint32_t)acc);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
}

static void quick_sin_check(void)
{
    // 快速三角函数: sin(0.5 rad)
    float32_t x = 0.5f;
    float32_t y = arm_sin_f32(x);
    // 简单打印浮点的位表示
    uint32_t bits;
    memcpy(&bits, &y, sizeof(bits));
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "CMSIS-DSP sin(0.5) bits=0x");
    put_hex32(NULL, bits);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
}

static void quick_fft_check(void)
{
    // 64 点 RFFT 快测：输入为单频正弦，输出主峰应在 bin=8
    const uint32_t N = 64;
    const float32_t fs = 1024.0f;  // 采样率
    const float32_t f0 = 128.0f;   // 目标频率（应落在 bin=8）
    float32_t in[N];
    float32_t spectrum[N]; // RFFT 输出数组
    // 生成正弦：x[n] = sin(2*pi*f0*n/fs)
    for (uint32_t n = 0; n < N; ++n)
    {
        in[n] = arm_sin_f32(2.0f * PI * f0 * (float32_t)n / fs);
    }
    arm_rfft_fast_instance_f32 S;
    if (arm_rfft_fast_init_f32(&S, N) != ARM_MATH_SUCCESS)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "RFFT init fail\n");
        return;
    }
    arm_rfft_fast_f32(&S, in, spectrum, 0);
    // 计算幅值谱（只取 [0..N/2]）
    float32_t mag_max = 0.0f;
    uint32_t  idx_max = 0;
    for (uint32_t k = 0; k <= N / 2; ++k)
    {
        float32_t re, im;
        if (k == 0)
        {
            re = spectrum[0];
            im = 0.0f;
        }
        else if (k == N / 2)
        {
            re = spectrum[1];
            im = 0.0f;
        }
        else
        {
            re = spectrum[2U * k];
            im = spectrum[2U * k + 1U];
        }
        float32_t mag_k;
        if (arm_sqrt_f32(re * re + im * im, &mag_k) != ARM_MATH_SUCCESS)
        {
            mag_k = 0.0f;
        }
        if (mag_k > mag_max)
        {
            mag_max = mag_k;
            idx_max = k;
        }
    }
    // 打印主峰 bin 和幅值位表示（防止浮点格式化开销）
    uint32_t mag_bits;
    memcpy(&mag_bits, &mag_max, sizeof(mag_bits));
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "RFFT64 peak_bin=0x");
    put_hex32(NULL, idx_max);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", mag_bits=0x");
    put_hex32(NULL, mag_bits);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
}

int main(void)
{
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    S300_SysTick_Init();
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_CMSIS_Quick start\n");
    quick_vector_add_check();
    quick_sin_check();
    quick_fft_check();
    // 保持与现有 UART 演示一致的心跳输出
    while (1)
    {
        S300_DelayMs(1000);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ".\n");
    }
}
