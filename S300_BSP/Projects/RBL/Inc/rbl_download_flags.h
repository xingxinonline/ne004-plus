/**
 * @file rbl_download_flags.h
 * @brief 软件下载标志管理 - 硬件约束下的替代方案
 * @version 1.0
 * @date 2025-08-30
 */

#ifndef RBL_DOWNLOAD_FLAGS_H
#define RBL_DOWNLOAD_FLAGS_H

#include <stdint.h>
#include <stdbool.h>

/* Flash地址定义 */
#define DOWNLOAD_FLAGS_SECTOR_ADDR  0xFFF000    /* 最后4KB作为标志区 */
#define DOUBLE_RESET_FLAG_ADDR      0xFFF100    /* 双重启标志 */

/* 魔数定义 */
#define DOWNLOAD_FLAG_MAGIC         0x444F574E  /* "DOWN" */
#define DOUBLE_RESET_MAGIC          0x52455354  /* "REST" */

/* 配置参数 */
#define MAX_DOWNLOAD_RETRIES        3           /* 最大下载重试次数 */
#define DOUBLE_RESET_TIMEOUT_MS     3000        /* 双重启超时时间 */
#define MAX_BOOT_ATTEMPTS           3           /* 最大启动尝试次数 */

/* 下载原因枚举 */
typedef enum {
    DOWNLOAD_REASON_NONE = 0,
    DOWNLOAD_REASON_USER_REQUEST,     /* 用户主动请求 */
    DOWNLOAD_REASON_REMOTE,           /* 远程触发 */
    DOWNLOAD_REASON_BLUETOOTH,        /* 蓝牙触发 */
    DOWNLOAD_REASON_BOOT_FAILURE,     /* 启动故障 */
    DOWNLOAD_REASON_APP_CRASH,        /* 应用崩溃 */
    DOWNLOAD_REASON_WATCHDOG,         /* 看门狗复位 */
} download_reason_t;

/* 下载标志结构 */
typedef struct {
    uint32_t magic;                   /* 魔数验证 */
    download_reason_t reason;         /* 下载原因 */
    uint32_t timestamp;               /* 设置时间戳 */
    uint32_t retry_count;             /* 重试计数 */
    uint32_t reserved[4];             /* 保留字段 */
} download_flag_t;

/* 双重启标志结构 */
typedef struct {
    uint32_t magic;                   /* 魔数验证 */
    uint32_t timestamp;               /* 上次复位时间 */
    uint32_t count;                   /* 复位计数 */
    uint32_t reserved[5];             /* 保留字段 */
} double_reset_flag_t;

/* 启动环境结构（扩展） */
typedef struct {
    uint32_t magic;                   /* 魔数验证 */
    uint32_t boot_attempts;           /* 启动尝试次数 */
    uint32_t last_successful_boot;    /* 最后成功启动时间 */
    uint32_t crash_count;             /* 崩溃计数 */
    uint32_t total_boots;             /* 总启动次数 */
    uint32_t reserved[3];             /* 保留字段 */
} boot_environment_t;

/* 函数声明 */

/* 下载标志管理 */
int rbl_flash_read_download_flag(download_flag_t *flag);
int rbl_flash_write_download_flag(const download_flag_t *flag);
int rbl_flash_set_download_flag(download_reason_t reason);
int rbl_flash_clear_download_flag(void);
bool rbl_flash_check_download_flag(void);

/* 双重启检测 */
bool rbl_check_double_reset(void);
int rbl_update_reset_counter(void);

/* 启动环境管理 */
int rbl_read_boot_environment(boot_environment_t *env);
int rbl_write_boot_environment(const boot_environment_t *env);
int rbl_increment_boot_attempts(void);
int rbl_clear_boot_attempts(void);
bool rbl_check_boot_failure(void);

/* 工具函数 */
const char *rbl_get_download_reason_string(download_reason_t reason);
uint32_t rbl_get_system_uptime_ms(void);

#endif /* RBL_DOWNLOAD_FLAGS_H */
