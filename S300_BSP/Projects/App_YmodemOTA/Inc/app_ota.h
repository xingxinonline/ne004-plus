/**
 * @file app_ota.h
 * @brief App OTA管理头文件
 */

#ifndef APP_OTA_H
#define APP_OTA_H

#include <stdint.h>
#include <stdbool.h>
#include "ymodem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OTA状态 */
typedef enum {
    APP_OTA_STATUS_IDLE = 0,        /* 空闲 */
    APP_OTA_STATUS_STARTED,         /* 已开始 */
    APP_OTA_STATUS_RECEIVING,       /* 正在接收 */
    APP_OTA_STATUS_VERIFYING,       /* 正在验证 */
    APP_OTA_STATUS_COMPLETED,       /* 已完成 */
    APP_OTA_STATUS_ERROR,           /* 错误 */
    APP_OTA_STATUS_CANCELLED,       /* 已取消 */
} app_ota_status_t;

/* OTA错误代码 */
typedef enum {
    APP_OTA_ERROR_NONE = 0,         /* 无错误 */
    APP_OTA_ERROR_INIT,             /* 初始化错误 */
    APP_OTA_ERROR_PARTITION,        /* 分区错误 */
    APP_OTA_ERROR_WRITE,            /* 写入错误 */
    APP_OTA_ERROR_VERIFY,           /* 验证错误 */
    APP_OTA_ERROR_SIZE,             /* 大小错误 */
    APP_OTA_ERROR_TIMEOUT,          /* 超时错误 */
    APP_OTA_ERROR_FLASH,            /* Flash错误 */
    APP_OTA_ERROR_CHECKSUM,         /* 校验和错误 */
} app_ota_error_t;

/* OTA上下文 */
typedef struct {
    app_ota_status_t status;        /* OTA状态 */
    app_ota_error_t error;          /* 错误代码 */
    uint32_t ota_partition_addr;    /* OTA分区地址 */
    uint32_t ota_partition_size;    /* OTA分区大小 */
    uint32_t written_bytes;         /* 已写入字节数 */
    uint32_t total_bytes;           /* 总字节数 */
    uint32_t start_time;            /* 开始时间 */
    uint32_t last_activity;         /* 最后活动时间 */
    bool verification_enabled;      /* 是否启用验证 */
    uint32_t checksum;              /* 校验和 */
    ymodem_context_t ymodem_ctx;    /* Ymodem上下文 */
} app_ota_context_t;

/* OTA事件类型 */
typedef enum {
    APP_OTA_EVENT_STARTED,          /* OTA开始 */
    APP_OTA_EVENT_PROGRESS,         /* OTA进度 */
    APP_OTA_EVENT_COMPLETED,        /* OTA完成 */
    APP_OTA_EVENT_ERROR,            /* OTA错误 */
    APP_OTA_EVENT_CANCELLED,        /* OTA取消 */
} app_ota_event_t;

/* OTA事件回调函数类型 */
typedef void (*app_ota_event_callback_t)(app_ota_event_t event, 
                                         void* event_data, 
                                         void* user_data);

/* 函数声明 */

/**
 * @brief 初始化OTA系统
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return 0成功，非0失败
 */
int app_ota_init(app_ota_event_callback_t callback, void* user_data);

/**
 * @brief 开始OTA更新
 * @return 0成功，非0失败
 */
int app_ota_start(void);

/**
 * @brief 处理OTA过程
 * @return 0成功，非0失败
 */
int app_ota_process(void);

/**
 * @brief 完成OTA更新
 * @return 0成功，非0失败
 */
int app_ota_finish(void);

/**
 * @brief 取消OTA更新
 */
void app_ota_cancel(void);

/**
 * @brief 获取OTA状态
 * @return OTA状态
 */
app_ota_status_t app_ota_get_status(void);

/**
 * @brief 获取OTA错误
 * @return OTA错误代码
 */
app_ota_error_t app_ota_get_error(void);

/**
 * @brief 获取OTA进度
 * @param received 已接收字节数
 * @param total 总字节数
 * @return 0成功，非0失败
 */
int app_ota_get_progress(uint32_t* received, uint32_t* total);

/**
 * @brief 验证OTA镜像
 * @return true有效，false无效
 */
bool app_ota_verify_image(void);

/**
 * @brief 应用OTA更新
 * @return 0成功，非0失败
 */
int app_ota_apply_update(void);

/**
 * @brief 重启到新固件
 */
void app_ota_reboot(void) __attribute__((noreturn));

/**
 * @brief 获取OTA统计信息
 * @param stats 统计信息结构体
 * @return 0成功，非0失败
 */
int app_ota_get_statistics(app_ota_context_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* APP_OTA_H */
