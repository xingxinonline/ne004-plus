/**
 * @file sbl_ota.h
 * @brief ESP32兼容的OTA启动管理
 */

#ifndef SBL_OTA_H
#define SBL_OTA_H

#include <stdint.h>
#include <stdbool.h>
#include "sbl_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OTA状态定义 */
typedef enum {
    ESP_OTA_IMG_NEW = 0x0,          /* 新镜像，未验证 */
    ESP_OTA_IMG_PENDING_VERIFY = 0x1, /* 等待验证 */
    ESP_OTA_IMG_VALID = 0x2,        /* 镜像有效 */
    ESP_OTA_IMG_INVALID = 0x3,      /* 镜像无效 */
    ESP_OTA_IMG_ABORTED = 0x4,      /* 镜像中止 */
    ESP_OTA_IMG_UNDEFINED = 0xFFFFFFFF, /* 未定义 */
} esp_ota_img_states_t;

/* OTA选择数据结构 */
typedef struct {
    uint32_t ota_seq;               /* OTA序列号 */
    uint8_t  seq_label[20];         /* 序列标签 */
    uint32_t ota_state;             /* OTA状态 */
    uint32_t crc;                   /* CRC校验 */
} esp_ota_select_entry_t;

/* OTA数据结构 */
typedef struct {
    esp_ota_select_entry_t ota_select[2]; /* 双备份OTA选择数据 */
} esp_ota_data_t;

/* 启动环境结构 */
typedef struct {
    uint8_t  active_slot;           /* 当前活跃分区: 0=OTA_0, 1=OTA_1 */
    uint8_t  update_pending;        /* 待更新标志 */
    uint8_t  boot_count;            /* 启动计数 */
    uint8_t  retry_count;           /* 重试计数 */
    uint32_t last_boot_time;        /* 上次启动时间 */
    uint32_t flags;                 /* 标志位 */
} sbl_boot_env_t;

/* 镜像头结构(ESP32兼容) */
typedef struct {
    uint8_t magic;                  /* 魔数: 0xE9 */
    uint8_t segment_count;          /* 段数量 */
    uint8_t spi_mode;              /* SPI模式 */
    uint8_t spi_speed_size;        /* SPI速度和大小 */
    uint32_t entry_addr;           /* 入口地址 */
    uint8_t wp_pin;                /* WP引脚 */
    uint8_t spi_pin_drv[3];        /* SPI引脚驱动强度 */
    uint8_t chip_id;               /* 芯片ID */
    uint8_t min_chip_rev;          /* 最小芯片版本 */
    uint8_t reserved[8];           /* 保留字节 */
    uint8_t hash_appended;         /* 是否附加哈希 */
} __attribute__((packed)) esp_image_header_t;

/* 段头结构 */
typedef struct {
    uint32_t load_addr;            /* 加载地址 */
    uint32_t data_len;             /* 数据长度 */
} __attribute__((packed)) esp_image_segment_header_t;

/* 函数声明 */

/**
 * @brief 初始化OTA系统
 * @return 0成功，非0失败
 */
int sbl_ota_init(void);

/**
 * @brief 读取OTA数据
 * @param ota_data 输出OTA数据
 * @return 0成功，非0失败
 */
int sbl_ota_read_data(esp_ota_data_t* ota_data);

/**
 * @brief 写入OTA数据
 * @param ota_data OTA数据
 * @return 0成功，非0失败
 */
int sbl_ota_write_data(const esp_ota_data_t* ota_data);

/**
 * @brief 获取启动分区
 * @return 启动分区信息，NULL表示失败
 */
const esp_partition_info_t* sbl_ota_get_boot_partition(void);

/**
 * @brief 获取运行分区
 * @return 运行分区信息，NULL表示失败
 */
const esp_partition_info_t* sbl_ota_get_running_partition(void);

/**
 * @brief 验证镜像完整性
 * @param partition 分区信息
 * @return true有效，false无效
 */
bool sbl_ota_verify_image(const esp_partition_info_t* partition);

/**
 * @brief 设置启动分区
 * @param partition 要设置的分区
 * @return 0成功，非0失败
 */
int sbl_ota_set_boot_partition(const esp_partition_info_t* partition);

/**
 * @brief 标记应用为有效
 * @param partition 应用分区
 * @return 0成功，非0失败
 */
int sbl_ota_mark_app_valid_cancel_rollback(const esp_partition_info_t* partition);

/**
 * @brief 标记应用为无效(回滚)
 * @param partition 应用分区
 * @return 0成功，非0失败
 */
int sbl_ota_mark_app_invalid_rollback_and_reboot(const esp_partition_info_t* partition);

/**
 * @brief 读取启动环境
 * @param boot_env 输出启动环境
 * @return 0成功，非0失败
 */
int sbl_boot_read_env(sbl_boot_env_t* boot_env);

/**
 * @brief 写入启动环境
 * @param boot_env 启动环境
 * @return 0成功，非0失败
 */
int sbl_boot_write_env(const sbl_boot_env_t* boot_env);

/**
 * @brief 处理启动失败
 * @param boot_env 启动环境
 * @return 0成功，非0失败
 */
int sbl_boot_handle_failure(sbl_boot_env_t* boot_env);

/**
 * @brief 检查是否需要回滚
 * @param boot_env 启动环境
 * @return true需要回滚，false不需要
 */
bool sbl_boot_should_rollback(const sbl_boot_env_t* boot_env);

/**
 * @brief 执行回滚操作
 * @param boot_env 启动环境
 * @return 0成功，非0失败
 */
int sbl_boot_perform_rollback(sbl_boot_env_t* boot_env);

#ifdef __cplusplus
}
#endif

#endif /* SBL_OTA_H */
