/**
 * @file app_software_reset.h
 * @brief S300应用程序软件复位功能头文件
 * @version 1.0
 * @date 2024
 */

#ifndef __APP_SOFTWARE_RESET_H__
#define __APP_SOFTWARE_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ========================== 应用程序接口 ========================== */

/**
 * @brief 应用程序软件复位功能初始化
 * @return 0=成功, <0=失败
 */
int app_software_reset_init(void);

/**
 * @brief 处理串口命令
 * @param cmd 命令字符串
 * @return 0=成功处理, <0=未知命令
 */
int app_handle_command(const char *cmd);

/**
 * @brief 应用主循环中的软件复位处理
 * 定期检查是否需要报告启动成功
 */
void app_software_reset_loop_handler(void);

/* ========================== 信息显示 ========================== */

/**
 * @brief 打印应用信息
 */
void app_print_info(void);

/**
 * @brief 打印内存信息
 */
void app_print_memory_info(void);

/**
 * @brief 运行自检测试
 */
void app_run_self_test(void);

/**
 * @brief 打印帮助信息
 */
void app_print_help(void);

/* ========================== 异常处理 ========================== */

/**
 * @brief 应用程序异常处理
 * @param fault_type 故障类型
 */
void app_fault_handler(uint32_t fault_type);

/**
 * @brief 看门狗超时处理
 */
void app_watchdog_timeout_handler(void);

/**
 * @brief 延迟报告启动成功任务
 * @param param 参数（未使用）
 */
void app_delayed_boot_success_task(void *param);

/* ========================== 网络/蓝牙接口 ========================== */

#ifdef CONFIG_NETWORK_SUPPORT
/**
 * @brief HTTP API处理 - 系统控制
 * @param action 操作类型
 */
void app_http_system_control_handler(const char *action);
#endif

#ifdef CONFIG_BLUETOOTH_SUPPORT
/**
 * @brief 蓝牙命令处理
 * @param command 命令字符串
 */
void app_bluetooth_command_handler(const char *command);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_SOFTWARE_RESET_H__ */
