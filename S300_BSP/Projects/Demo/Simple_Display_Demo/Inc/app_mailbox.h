/**
 * @file app_mailbox.h
 * @brief 邮箱数据处理 - 接收DSP发送的人脸框坐标
 */

#ifndef _SIMPLE_MAILBOX_HANDLER_H_
#define _SIMPLE_MAILBOX_HANDLER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化邮箱处理
 */
void app_mailbox_init(void);

/**
 * @brief 轮询检查邮箱数据
 *   如果收到有效的人脸框坐标，自动绘制矩形框
 */
void app_mailbox_poll(void);

/**
 * @brief 设置时间获取回调（用于限速）
 *
 * @param get_ms 获取毫秒时间戳的函数
 */
void app_mailbox_set_time_callback(uint32_t (*get_ms)(void));

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_MAILBOX_HANDLER_H_ */
