/*
 * 文件：display_demo_app.c
 * 说明：本 Demo 的“应用编排层”，将多子模块按顺序初始化，并提供统一的 tick 调度入口。
 * 初始化顺序：
 *   1) 摄像头预上电/探测（失败不致命，继续显示链路验证）；
 *   2) 视频子系统（面板/时序/显存绑定）；
 *   3) M4<->DSP 邮箱握手（复位DSP并发送启动令牌，保持与算法侧一致）；
 *   4) LVGL 初始化与显示绑定（双缓冲整帧）；
 *   5) Eyes UI 创建与参数设置（眼距等）；
 *   6) Face Tracker 初始化（依赖毫秒节拍与 eyes）。
 * 运行期：
 *   - display_demo_app_tick() 负责人脸跟踪轮询与 LVGL 定时器处理。
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "s300.h"
#include "rcc.h"
#include "video.h"
#include "lvgl.h"
#include "ui_display.h"
#include "mailbox.h"
#include "camera_ov5640.h"
#include "face_tracker.h"
#include "eyes.h"
#include "display_demo_app.h"
#include "gpio.h"

void background_light();
print_psram_dat(uint16_t *dest, uint32_t count);

void display_demo_app_init(uint32_t (*get_millis)(void))
{
    /* 摄像头上电与探测（失败则仅初始化显示链路） */
    int cam_ret = camera_ov5640_preinit();
    if (cam_ret != 0) {
        printf("[S300][DisplayDemo][WARN] OV5640 init failed (%d), continue to init video for display path only.\r\n", cam_ret);
    }

    /* 视频子系统（包含面板初始化） */
    printf("[S300][DisplayDemo] init video...\r\n");
    background_light();
    // embedded_memset((uint8_t *)DISP_RALPHA0_ADDR, 0xFF, 320*240);
    // embedded_memset((uint8_t *)DISP_RALPHA1_ADDR, 0xFF, 320*240); 

    // embedded_memset((uint8_t *)DISP_WFRAME0_ADDR, 0x00, 320*240 * 2);
    // embedded_memset((uint8_t *)DISP_WFRAME1_ADDR, 0x00, 320*240 * 2);
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);

    // while (1)
    // {
    //     // embedded_memset((uint16_t *)DISP_RFRAME0_ADDR, 0x0000, 320*240 * 2);
    //     // REG32(DSP_VIDEO_SS_BASE + 0x50) = 0x1;//0;
    //     // print_psram_dat((uint16_t *)DISP_RFRAME0_ADDR, 100);

    //     // embedded_memset((uint16_t *)DISP_RFRAME1_ADDR, 0xFFFF, 320*240 * 2); 
    //     // REG32(DSP_VIDEO_SS_BASE + 0x54) = 0x1;//0;
    //     // print_psram_dat((uint16_t *)DISP_RFRAME1_ADDR, 100);

    // }
    
    /* M4 <-> DSP 邮箱通信与握手 */
    init_mailbox(MAILBOX_BASE, 4, MAILBOX_IRQ_NONE);
    set_dsp_warm_reset(true);
    write_mailbox(MAILBOX_BASE, 0x5A5A5A5A);

    /* LVGL + 显示绑定与背景色 */
    (void)ui_display_init();
    printf("[S300][DisplayDemo] LVGL %d.%d.%d (%s)\r\n", lv_version_major(), lv_version_minor(), lv_version_patch(), lv_version_info());
    ui_display_set_bg_color(0xffc21e);

    /* 眼睛 UI */
    eyes_set_spacing(48);
    eyes_create();

    /* 人脸追踪初始化（依赖 eyes + mailbox；提供时间回调实现） */
    face_tracker_init(get_millis);

    printf("[S300][DisplayDemo] LVGL started.\r\n");
    printf("[S300][DisplayDemo] UART echo enabled on debug UART (CR->CRLF).\r\n");
    printf("[S300][DisplayDemo] Command: goto <y_mid>  (move eyes midpoint vertically)\r\n");
}


void background_light()
{
    set_gpio_function(GPIOA,24,FUNCTION_2);//gpio alternate function
    set_gpio_mode(GPIOA,24,GPIO_UP);// choose pull-up for gpio
    set_gpio_direction(GPIOA,24,1);//gpio select output mode
    set_gpio_data(GPIOA,24,0);//gpio out 0    
}

void display_demo_app_tick(void)
{
    /* uart_cmd_poll(); // 如需命令控制可启用 */
    face_tracker_poll();
    lv_timer_handler();
}

void *embedded_memset(void *dest, int value, uint32_t count)
{
    // 参数检查
    if (dest == NULL || count == 0) {
        return dest;
    }
    
    uint16_t *byte_dest = (uint16_t *)dest;
    uint16_t byte_value = (uint16_t)value;
    
    // 简单循环实现
    while (count--) {
        *byte_dest++ = byte_value;
    }
    
    return dest;
}


print_psram_dat(uint16_t *dest, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        printf("addr: 0x%08X, val: 0x%02X\r\n", 
            (unsigned int)(dest + i), 
            dest[i]);
    }
}
