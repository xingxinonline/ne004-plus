/**
 * @file app_mailbox.c
 * @brief 邮箱数据处理实现 - 接收DSP发送的人脸框坐标
 * 
 * 协议说明:
 *   DSP通过邮箱发送 FaceRect 在共享内存中的偏移量
 *   CM4根据偏移量在共享内存中读取 FaceRect 结构体
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "app_mailbox.h"
#include "direct_display.h"
#include "mailbox.h"
#include "video.h"

/* DSP共享内存中的人脸数据结构 */
typedef struct FaceRect_ {
    float score;
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
    float lm[10];
} FaceRect;

#ifndef DSP_FACE_BASE_ADDR
#define DSP_FACE_BASE_ADDR 0x44800000u
#endif

/* 时间回调 */
static uint32_t (*s_get_ms)(void) = NULL;

/* 上次处理时间（限速用） */
static uint32_t s_last_process_ms = 0;
/* 降低限速，让反应更灵敏，实际由FIFO速度控制 */
#define PROCESS_INTERVAL_MS 0 

/* 上次收到有效人脸的时间 */
static uint32_t s_last_face_ms = 0;
#define FACE_TIMEOUT_MS 300

/* 人脸框样式 */
#define FACE_BOX_BORDER     2
#define FACE_BOX_ALPHA      0xC0

/* 是否已检测到过人脸（用于判断是否需要保留启动时的测试框） */
static uint8_t s_seen_face = 0;

void app_mailbox_init(void)
{
    /* 邮箱已在main中初始化 */
    printf("[AppMailbox] Handler initialized (SharedMem Mode)\r\n");
}

void app_mailbox_set_time_callback(uint32_t (*get_ms)(void))
{
    s_get_ms = get_ms;
}

void app_mailbox_poll(void)
{
    uint32_t now = (s_get_ms != NULL) ? s_get_ms() : 0;
    bool received_new_face = false;

    /* 循环读取邮箱数据，直到清空FIFO */
    /*mailbox_sta_empty_flag_is 返回0表示匹配，即非空*/
    while (mailbox_sta_empty_flag_is(MAILBOX_BASE, 0) == 0)
    {
        /* 读取偏移量 - 使用mailbox_read_u32直接读取，避免read_mailbox的0值陷阱 */
        uint32_t offset = 0;
        int ret = mailbox_read_u32(MAILBOX_BASE, &offset, 100);
        if (ret != 0) break;
        
        /* 计算共享内存地址 */
        uintptr_t addr = (uintptr_t)DSP_FACE_BASE_ADDR + (uintptr_t)offset;
        const FaceRect *fr = (const FaceRect*)addr;
        
        /* Debug: Print raw offset to confirm data arrival */
        // printf("[AppMailbox] Raw Offset: 0x%08X, x1=%d\r\n", offset, fr->x1);

        /* 基本合法性检查 */
        if (fr->x1 < 0 && fr->y1 < 0 && fr->x2 < 0 && fr->y2 < 0) {
            /* 这可能是无效帧 */
            continue;
        }

        /* 标记收到人脸 */
        s_seen_face = 1;
        s_last_face_ms = now;
        received_new_face = true;
        
        /* 获取坐标并进行正规化处理 */
        int32_t x1 = fr->x1;
        int32_t y1 = fr->y1;
        int32_t x2 = fr->x2;
        int32_t y2 = fr->y2;

        /* 确保 x1 <= x2 */
        if (x1 > x2) { int32_t temp = x1; x1 = x2; x2 = temp; }
        if (y1 > y2) { int32_t temp = y1; y1 = y2; y2 = temp; }
        
        int16_t w = x2 - x1;
        int16_t h = y2 - y1;
        
        /* 忽略太小的框 */
        if (w < 4 || h < 4) continue;
        
        /* 绘制 */
        direct_display_clear_boxes();
        direct_display_draw_rect((int16_t)x1, (int16_t)y1, w, h,
                                 FACE_BOX_BORDER, FACE_BOX_ALPHA);
                                 
        /* 简单打印调试信息（每1000ms最多打印一次） */
        static uint32_t last_print = 0;
        if (now - last_print > 1000) {
            printf("[AppMailbox] Face at (%d,%d) %dx%d score=%d\r\n", 
                   (int)x1, (int)y1, (int)w, (int)h, (int)(fr->score * 100));
            last_print = now;
        }
    }
    
    /* 如果收到新数据，刷新屏幕 */
    static bool s_is_cleared = false;
    
    if (received_new_face) {
        direct_display_refresh();
        s_is_cleared = false;
    }
    else if (s_seen_face && (now - s_last_face_ms > FACE_TIMEOUT_MS)) {
        /* 如果一段时间没人脸（且之前见过人脸），清除屏幕 */
        if (!s_is_cleared) {
            printf("[AppMailbox] Face lost, clearing...\r\n");
            direct_display_clear_boxes();
            direct_display_refresh();
            s_is_cleared = true;
        }
    }
}
