/**
 * @file software_reset.h
 * @brief S300软件复位解决方案 - 无硬件复位电路实现
 * @version 1.0
 * @date 2024
 * 
 * @copyright Copyright (c) 2024 S300 BSP Team
 * 
 * @note 此模块完全替代硬件复位电路功能，提供多种软件复位触发方式
 */

#ifndef __SOFTWARE_RESET_H__
#define __SOFTWARE_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "partition_layout.h"

/* ================================ 宏定义 ================================ */

// 使用分区布局中定义的地址常量
// 软件复位标志区位于NVS尾部4KB (0x0003F000-0x0003FFFF)

// 魔数定义
#define DOWNLOAD_FLAG_MAGIC       0x444C4654  ///< "DLFT"
#define DOUBLE_RESET_MAGIC        0x52535444  ///< "RSTD"
#define BOOT_COUNTER_MAGIC        0x424F4F54  ///< "BOOT"

// 时间窗口配置
#define DOUBLE_RESET_TIMEOUT_MS   2000       ///< 双重启检测时间窗口(ms)
#define MAX_BOOT_FAILURES         3          ///< 最大启动失败次数
#define MAX_RETRY_COUNT           3          ///< 最大重试次数

/* =============================== 类型定义 =============================== */

/**
 * @brief 复位触发原因枚举
 */
typedef enum {
    RESET_REASON_NONE = 0,          ///< 无原因
    RESET_REASON_USER_CMD,          ///< 用户命令触发
    RESET_REASON_SERIAL_CMD,        ///< 串口命令触发
    RESET_REASON_DOUBLE_RESET,      ///< 双重启触发
    RESET_REASON_APP_FAILURE,       ///< 应用故障触发
    RESET_REASON_WATCHDOG,          ///< 看门狗触发
    RESET_REASON_REMOTE_CMD,        ///< 远程命令触发
    RESET_REASON_POWER_ON,          ///< 上电复位
    RESET_REASON_UNKNOWN = 0xFF     ///< 未知原因
} reset_reason_t;

/**
 * @brief 下载模式标志结构
 */
typedef struct {
    uint32_t magic;           ///< 魔数验证
    uint32_t reason;          ///< 触发原因 @ref reset_reason_t
    uint32_t timestamp;       ///< 时间戳
    uint32_t retry_count;     ///< 重试次数
    uint32_t user_data;       ///< 用户数据
    uint32_t crc32;          ///< CRC32校验
} __attribute__((packed)) download_flag_t;

/**
 * @brief 双重启标志结构
 */
typedef struct {
    uint32_t magic;               ///< 魔数验证
    uint32_t first_reset_time;    ///< 第一次复位时间
    uint32_t reset_count;         ///< 复位计数
    uint32_t crc32;              ///< CRC32校验
} __attribute__((packed)) double_reset_flag_t;

/**
 * @brief 启动计数器结构
 */
typedef struct {
    uint32_t magic;               ///< 魔数验证
    uint32_t boot_count;          ///< 启动总次数
    uint32_t failure_count;       ///< 连续失败次数
    uint32_t last_success_time;   ///< 最后成功时间
    uint32_t crc32;              ///< CRC32校验
} __attribute__((packed)) boot_counter_t;

/**
 * @brief 软件复位状态信息
 */
typedef struct {
    reset_reason_t last_reason;   ///< 最后一次复位原因
    uint32_t total_boots;         ///< 总启动次数
    uint32_t failure_count;       ///< 连续失败次数
    bool download_flag_active;    ///< 下载标志是否激活
    bool double_reset_detected;   ///< 是否检测到双重启
} software_reset_status_t;

/* ============================= 函数声明 ============================== */

/**
 * @brief 初始化软件复位模块
 * @return 0=成功, <0=失败
 */
int software_reset_init(void);

/**
 * @brief 获取软件复位状态信息
 * @param status 输出状态信息
 * @return 0=成功, <0=失败
 */
int software_reset_get_status(software_reset_status_t *status);

/* ========================== 下载模式控制 =========================== */

/**
 * @brief 设置下载模式标志并软件复位
 * @param reason 触发原因
 * @return 0=成功, <0=失败 (不会返回，因为会复位)
 */
int software_reset_enter_download_mode(reset_reason_t reason);

/**
 * @brief 检查是否有下载模式标志
 * @param reason 输出触发原因
 * @return true=需要进入下载模式, false=正常启动
 */
bool software_reset_check_download_flag(reset_reason_t *reason);

/**
 * @brief 清除下载模式标志
 */
void software_reset_clear_download_flag(void);

/* ========================== 双重启检测 ============================= */

/**
 * @brief 检查双重启动模式
 * @return true=检测到双重启, false=未检测到
 */
bool software_reset_check_double_reset(void);

/**
 * @brief 清除双重启标志
 */
void software_reset_clear_double_reset_flag(void);

/* ========================== 故障检测恢复 =========================== */

/**
 * @brief 更新启动计数器
 * @param success true=启动成功, false=启动失败
 */
void software_reset_update_boot_counter(bool success);

/**
 * @brief 检查启动失败状态
 * @return true=有启动失败, false=正常
 */
bool software_reset_check_boot_failure(void);

/**
 * @brief 清除启动计数器
 */
void software_reset_clear_boot_counter(void);

/* ========================== 系统复位控制 =========================== */

/**
 * @brief 执行立即系统复位
 */
void software_reset_system_now(void) __attribute__((noreturn));

/**
 * @brief 延时系统复位
 * @param delay_ms 延时时间(毫秒)
 */
void software_reset_system_delayed(uint32_t delay_ms);

/* ========================== 调试和诊断 ============================= */

/**
 * @brief 打印软件复位状态信息
 */
void software_reset_print_status(void);

/**
 * @brief 获取复位原因字符串
 * @param reason 复位原因
 * @return 原因字符串
 */
const char* software_reset_get_reason_string(reset_reason_t reason);

/**
 * @brief 复位所有软件标志(危险操作，仅用于调试)
 */
void software_reset_factory_reset_flags(void);

/* ========================== 命令行接口 ============================= */

/**
 * @brief 处理软件复位相关命令
 * @param cmd 命令字符串
 * @return 0=成功处理, <0=无效命令
 */
int software_reset_handle_command(const char *cmd);

/**
 * @brief 打印命令帮助
 */
void software_reset_print_help(void);

#ifdef __cplusplus
}
#endif

#endif /* __SOFTWARE_RESET_H__ */
