/**
 * @file sbl_main.h
 * @brief SBL主程序头文件
 */

#ifndef SBL_MAIN_H
#define SBL_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "sbl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SBL启动阶段 */
typedef enum {
    SBL_STAGE_INIT = 0,         /* 初始化阶段 */
    SBL_STAGE_PARTITION,        /* 分区表阶段 */
    SBL_STAGE_OTA,              /* OTA选择阶段 */
    SBL_STAGE_VERIFY,           /* 镜像验证阶段 */
    SBL_STAGE_BOOT,             /* 启动应用阶段 */
    SBL_STAGE_ERROR,            /* 错误阶段 */
} sbl_stage_t;

/* SBL启动结果 */
typedef enum {
    SBL_BOOT_SUCCESS = 0,       /* 启动成功 */
    SBL_BOOT_PARTITION_ERROR,   /* 分区表错误 */
    SBL_BOOT_OTA_ERROR,         /* OTA数据错误 */
    SBL_BOOT_IMAGE_ERROR,       /* 镜像错误 */
    SBL_BOOT_VERIFY_ERROR,      /* 验证错误 */
    SBL_BOOT_TIMEOUT_ERROR,     /* 超时错误 */
    SBL_BOOT_UNKNOWN_ERROR,     /* 未知错误 */
} sbl_boot_result_t;

/* SBL系统信息 */
typedef struct {
    uint32_t magic;             /* 魔数 */
    uint32_t version;           /* SBL版本 */
    uint32_t build_time;        /* 构建时间 */
    uint32_t boot_count;        /* 启动计数 */
    uint32_t last_error;        /* 上次错误代码 */
    uint8_t  active_slot;       /* 活跃分区 */
    uint8_t  rollback_count;    /* 回滚计数 */
    uint8_t  reserved[2];       /* 保留字节 */
} sbl_system_info_t;

/* 函数声明 */

/**
 * @brief SBL主函数
 * @return 启动结果
 */
int sbl_main(void);

/**
 * @brief 系统初始化
 * @return 0成功，非0失败
 */
int sbl_system_init(void);

/**
 * @brief 打印启动横幅
 */
void sbl_print_banner(void);

/**
 * @brief 获取SBL版本字符串
 * @return 版本字符串
 */
const char* sbl_get_version_string(void);

/**
 * @brief 获取系统信息
 * @param info 输出系统信息
 * @return 0成功，非0失败
 */
int sbl_get_system_info(sbl_system_info_t* info);

/**
 * @brief 跳转到应用程序
 * @param app_address 应用程序地址
 * @param app_entry 应用程序入口点
 * @return 不应该返回
 */
void sbl_jump_to_app(uint32_t app_address, uint32_t app_entry) __attribute__((noreturn));

/**
 * @brief 系统复位
 */
void sbl_system_reset(void) __attribute__((noreturn));

/**
 * @brief 错误处理
 * @param error_code 错误代码
 * @param error_msg 错误消息
 */
void sbl_error_handler(sbl_boot_result_t error_code, const char* error_msg);

/**
 * @brief 启动看门狗
 * @param timeout_ms 超时时间(毫秒)
 */
void sbl_watchdog_start(uint32_t timeout_ms);

/**
 * @brief 停止看门狗
 */
void sbl_watchdog_stop(void);

/**
 * @brief 喂看门狗
 */
void sbl_watchdog_feed(void);

/**
 * @brief 延时函数
 * @param ms 延时时间(毫秒)
 */
void sbl_delay_ms(uint32_t ms);

/**
 * @brief 获取系统时钟
 * @return 系统时钟(毫秒)
 */
uint32_t sbl_get_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* SBL_MAIN_H */
