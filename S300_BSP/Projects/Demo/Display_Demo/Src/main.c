#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "video.h"

// 1ms 节拍计时
static volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void)
{
    g_tick_ms++;
}

static inline uint32_t millis(void)
{
    return g_tick_ms;
}

static void fill_buffer(volatile uint16_t *frame,
                        volatile uint8_t *alpha,
                        size_t pixel_count,
                        uint16_t color,
                        uint8_t alpha_value)
{
    for (size_t i = 0; i < pixel_count; ++i)
    {
        frame[i] = color;
        alpha[i] = alpha_value;
    }
}

int main(void)
{
    // 板级初始化：时钟 + UART3，printf 可用
    board_init();
    printf("\r\n[S300][DisplayDemo] Booting...\r\n");

    // 启动 SysTick 为 1ms 节拍
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        printf("[S300][DisplayDemo][ERR] SysTick_Config failed!\r\n");
    }

    // 初始化视频子系统（包含 ST77 SPI LCD 序列），此处未使用摄像头，仅演示显示路径
    printf("[S300][DisplayDemo] init video...\r\n");
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);

    volatile uint16_t* f0 = (volatile uint16_t*)DISP_RFRAME0_ADDR;
    volatile uint16_t* f1 = (volatile uint16_t*)DISP_RFRAME1_ADDR;
    volatile uint8_t*  a0 = (volatile uint8_t*)DISP_RALPHA0_ADDR;
    volatile uint8_t*  a1 = (volatile uint8_t*)DISP_RALPHA1_ADDR;

    printf("[S300][DisplayDemo] frame0=%p frame1=%p alpha0=%p alpha1=%p\r\n", (void*)f0, (void*)f1, (void*)a0, (void*)a1);

    const size_t pixels = (size_t)DISP_IMAGE_WIDTH * (size_t)DISP_IMAGE_HEIGHT;
    const uint16_t colors[] = {0xF800u, 0x07E0u, 0x001Fu, 0xFFFFu};
    const size_t ncolors = sizeof(colors)/sizeof(colors[0]);
    size_t i0 = 0, i1 = 1 % ncolors;

    fill_buffer(f0, a0, pixels, colors[i0], 0x80u);
    fill_buffer(f1, a1, pixels, colors[i1], 0xFFu);

    const uint32_t REG_F0 = (DSP_VIDEO_SS_BASE + 0x50u);
    const uint32_t REG_F1 = (DSP_VIDEO_SS_BASE + 0x54u);

    REG32(REG_F0) = 1u;
    REG32(REG_F1) = 0u;
    printf("[S300][DisplayDemo] start flipping (10 fps)...\r\n");

    uint32_t next_due = millis(); // 立即允许首次帧
    while (1)
    {
        // 仅在到达下一次 10fps 的时间点时，尝试准备一个空闲帧
        uint32_t now = millis();
        if ((int32_t)(now - next_due) < 0)
        {
            continue; // 尚未到下一帧时间
        }

        uint32_t s0 = REG32(REG_F0);
        uint32_t s1 = REG32(REG_F1);
        bool flipped = false;

        if (s0 == 0u && s1 == 1u)
        {
            i0 = (i0 + 1) % ncolors;
            fill_buffer(f0, a0, pixels, colors[i0], 0xFFu);
            REG32(REG_F0) = 1u;
            printf("[S300][DisplayDemo] F0 -> ready, color=0x%04X\r\n", colors[i0]);
            flipped = true;
        }
        else if (s1 == 0u && s0 == 1u)
        {
            i1 = (i1 + 1) % ncolors;
            fill_buffer(f1, a1, pixels, colors[i1], 0xFFu);
            REG32(REG_F1) = 1u;
            printf("[S300][DisplayDemo] F1 -> ready, color=0x%04X\r\n", colors[i1]);
            flipped = true;
        }
        else if (s0 == 0u && s1 == 0u)
        {
            // 两个都空闲，则交替启动
            if (i0 == i1)
            {
                i0 = (i0 + 1) % ncolors;
                fill_buffer(f0, a0, pixels, colors[i0], 0xFFu);
                REG32(REG_F0) = 1u;
                printf("[S300][DisplayDemo] both free -> F0 ready, color=0x%04X\r\n", colors[i0]);
                flipped = true;
            }
            else
            {
                i1 = (i1 + 1) % ncolors;
                fill_buffer(f1, a1, pixels, colors[i1], 0xFFu);
                REG32(REG_F1) = 1u;
                printf("[S300][DisplayDemo] both free -> F1 ready, color=0x%04X\r\n", colors[i1]);
                flipped = true;
            }
        }

        if (flipped)
        {
            next_due = now + 200u; // 10fps -> 每 100ms 一帧
        }
    }
}
