/**
 * @file app_console.c
 * @brief 串口命令处理实现
 * 
 * 支持的命令:
 *   - RECT x,y,w,h          绘制矩形框(默认边框2像素,透明度0xC0)
 *   - RECT x,y,w,h,bw,alpha 绘制矩形框(指定边框宽度和透明度)
 *   - CLEAR                 清除所有矩形框
 *   - COLOR green/red/blue  设置图层颜色
 *   - ALPHA value           设置全局透明度
 *   - HELP                  显示帮助信息
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "app_console.h"
#include "direct_display.h"
#include "uart.h"

/* 命令缓冲区 */
#define CMD_BUF_SIZE 128
static char s_cmd_buf[CMD_BUF_SIZE];
static uint32_t s_cmd_idx = 0;

/* 将字符串转为小写 */
static void str_tolower(char *s)
{
    while (*s) {
        *s = tolower((unsigned char)*s);
        s++;
    }
}

void app_console_init(void)
{
    s_cmd_idx = 0;
    memset(s_cmd_buf, 0, sizeof(s_cmd_buf));
    
    printf("\r\n[SimpleDisplay] UART Command Interface Ready\r\n");
    printf("Commands:\r\n");
    printf("  RECT x,y,w,h          - Draw rectangle\r\n");
    printf("  RECT x,y,w,h,bw,alpha - Draw rectangle with border width and alpha\r\n");
    printf("  CLEAR                 - Clear all rectangles\r\n");
    printf("  COLOR green|red|blue  - Set layer color\r\n");
    printf("  ALPHA value           - Set global alpha (0-255)\r\n");
    printf("  REFRESH               - Refresh display\r\n");
    printf("  HELP                  - Show this help\r\n");
    printf("> ");
}

void app_console_process(const char *cmd, uint32_t len)
{
    if (cmd == NULL || len == 0) return;
    
    char buf[CMD_BUF_SIZE];
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, cmd, len);
    buf[len] = '\0';
    
    /* 去除首尾空格 */
    char *p = buf;
    while (*p && isspace((unsigned char)*p)) p++;
    char *end = p + strlen(p) - 1;
    while (end > p && isspace((unsigned char)*end)) *end-- = '\0';
    
    if (strlen(p) == 0) return;
    
    /* 获取命令关键字 */
    char cmd_word[32];
    int i = 0;
    while (*p && !isspace((unsigned char)*p) && i < 31) {
        cmd_word[i++] = tolower((unsigned char)*p);
        p++;
    }
    cmd_word[i] = '\0';
    
    /* 跳过空格 */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* 处理命令 */
    if (strcmp(cmd_word, "rect") == 0 || strcmp(cmd_word, "box") == 0) {
        direct_display_process_rect_cmd((const uint8_t *)p, strlen(p));
    }
    else if (strcmp(cmd_word, "clear") == 0) {
        direct_display_clear_boxes();
        printf("[SimpleDisplay] Cleared all boxes\r\n");
    }
    else if (strcmp(cmd_word, "color") == 0) {
        str_tolower(p);
        if (strcmp(p, "green") == 0) {
            direct_display_fill_color(COLOR_GREEN);
            printf("[SimpleDisplay] Color set to GREEN\r\n");
        } else if (strcmp(p, "red") == 0) {
            direct_display_fill_color(COLOR_RED);
            printf("[SimpleDisplay] Color set to RED\r\n");
        } else if (strcmp(p, "blue") == 0) {
            direct_display_fill_color(COLOR_BLUE);
            printf("[SimpleDisplay] Color set to BLUE\r\n");
        } else if (strcmp(p, "white") == 0) {
            direct_display_fill_color(COLOR_WHITE);
            printf("[SimpleDisplay] Color set to WHITE\r\n");
        } else if (strcmp(p, "black") == 0) {
            direct_display_fill_color(COLOR_BLACK);
            printf("[SimpleDisplay] Color set to BLACK\r\n");
        } else if (strcmp(p, "yellow") == 0) {
            direct_display_fill_color(COLOR_YELLOW);
            printf("[SimpleDisplay] Color set to YELLOW\r\n");
        } else {
            /* 尝试解析为16进制颜色值 */
            uint16_t color = (uint16_t)strtol(p, NULL, 16);
            direct_display_fill_color(color);
            printf("[SimpleDisplay] Color set to 0x%04X\r\n", color);
        }
    }
    else if (strcmp(cmd_word, "alpha") == 0) {
        int alpha = atoi(p);
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        direct_display_set_alpha((uint8_t)alpha);
        printf("[SimpleDisplay] Global alpha set to %d\r\n", alpha);
    }
    else if (strcmp(cmd_word, "refresh") == 0) {
        direct_display_refresh();
        printf("[SimpleDisplay] Display refreshed\r\n");
    }
    else if (strcmp(cmd_word, "help") == 0 || strcmp(cmd_word, "?") == 0) {
        printf("\r\nCommands:\r\n");
        printf("  RECT x,y,w,h          - Draw rectangle\r\n");
        printf("  RECT x,y,w,h,bw,alpha - Draw rectangle with border width and alpha\r\n");
        printf("  CLEAR                 - Clear all rectangles\r\n");
        printf("  COLOR green|red|blue  - Set layer color\r\n");
        printf("  ALPHA value           - Set global alpha (0-255)\r\n");
        printf("  REFRESH               - Refresh display\r\n");
        printf("  HELP                  - Show this help\r\n");
    }
    else {
        printf("[SimpleDisplay] Unknown command: %s\r\n", cmd_word);
        printf("Type 'HELP' for available commands.\r\n");
    }
}

void app_console_poll(void)
{
    /* 简单的非阻塞串口读取 */
    /* 这里假设有一个简单的方式检查串口是否有数据 */
    /* 具体实现取决于底层UART驱动 */
    
    /* 使用标准输入（假设printf/getchar已重定向到UART） */
    /* 如果底层支持非阻塞读取，可以在这里实现 */
    
    /* 这里提供一个简化的轮询实现框架 */
    /* 实际使用时需要根据底层UART驱动适配 */
    
    #if 0
    /* 示例：非阻塞读取字符 */
    int ch;
    while ((ch = uart_getchar_nonblock()) != -1) {
        if (ch == '\r' || ch == '\n') {
            if (s_cmd_idx > 0) {
                s_cmd_buf[s_cmd_idx] = '\0';
                printf("\r\n");
                app_console_process(s_cmd_buf, s_cmd_idx);
                s_cmd_idx = 0;
                printf("> ");
            }
        } else if (ch == '\b' || ch == 127) {
            /* 退格 */
            if (s_cmd_idx > 0) {
                s_cmd_idx--;
                printf("\b \b");
            }
        } else if (s_cmd_idx < CMD_BUF_SIZE - 1) {
            s_cmd_buf[s_cmd_idx++] = (char)ch;
            printf("%c", ch);
        }
    }
    #endif
}
