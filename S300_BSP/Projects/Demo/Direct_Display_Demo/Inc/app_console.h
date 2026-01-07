/**
 * @file app_console.h
 * @brief 串口命令处理模块 - 用于接收矩形框绘制命令
 */

#ifndef _APP_CONSOLE_H_
#define _APP_CONSOLE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化串口命令处理
 */
void app_console_init(void);

/**
 * @brief 轮询处理串口命令
 *   应在主循环中调用
 */
void app_console_poll(void);

/**
 * @brief 处理接收到的一行命令
 *
 * @param cmd 命令字符串
 * @param len 长度
 */
void app_console_process(const char *cmd, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _APP_CONSOLE_H_ */
