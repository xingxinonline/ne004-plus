/**
 * @file sbl_config.h
 * @brief SBL配置文件 - ESP32兼容的二级引导程序
 */

#ifndef SBL_CONFIG_H
#define SBL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* SBL版本信息 */
#define SBL_VERSION_MAJOR       1
#define SBL_VERSION_MINOR       0
#define SBL_VERSION_PATCH       0
#define SBL_VERSION_STRING      "1.0.0"

/* Flash和内存配置 */
#define SBL_FLASH_BASE_ADDR         0x08000000  /* QSPI Flash基址(与RBL保持一致) */
#define SBL_FLASH_SIZE              (16 * 1024 * 1024)  /* 16MB */
#define SBL_SRAM_BASE_ADDR          0x20000000  /* SRAM基址 */
#define SBL_SRAM_SIZE               (384 * 1024)  /* 384KB */

/* SBL在Flash中的位置 */
#define SBL_FLASH_OFFSET            0x10000     /* SBL在Flash中的偏移 */
#define SBL_FLASH_SIZE_LIMIT        (64 * 1024) /* SBL最大64KB */
#define SBL_XIP_ADDR                (SBL_FLASH_BASE_ADDR + SBL_FLASH_OFFSET) /* SBL XIP执行地址 */

/* APP完整SRAM可用 - SBL仅使用顶部栈空间 */
#define SBL_APP_SRAM_START          0x20000000  /* APP SRAM起始地址 */
#define SBL_APP_SRAM_SIZE           (364 * 1024) /* APP可用~364KB SRAM */
#define SBL_STACK_SIZE              (20 * 1024)  /* SBL栈空间20KB */

/* 分区表配置 */
#define PARTITION_TABLE_OFFSET  0x20000     /* 分区表偏移(相对SBL) */
#define PARTITION_TABLE_SIZE    0x1000      /* 分区表大小: 4KB */

/* OTA数据配置 */
#define OTA_DATA_OFFSET         0x21000     /* OTA数据偏移(相对SBL) */
#define OTA_DATA_SIZE           0x2000      /* OTA数据大小: 8KB */

/* 应用分区配置 */
#define APP_PARTITION_SIZE      0x400000    /* 每个APP分区: 4MB */
#define OTA_0_OFFSET            0x30000     /* OTA_0偏移(相对SBL) */
#define OTA_1_OFFSET            0x430000    /* OTA_1偏移(相对SBL) */

/* 启动配置 */
#define SBL_BOOT_TIMEOUT_MS     30000       /* 启动超时: 30秒 */
#define SBL_MAX_RETRY_COUNT     3           /* 最大重试次数 */
#define SBL_WATCHDOG_TIMEOUT_MS 30000       /* 看门狗超时 */

/* 调试配置 */
#define SBL_DEBUG_ENABLED       1           /* 调试输出 */
#define SBL_VERBOSE_BOOT        1           /* 详细启动信息 */
#define SBL_UART_DEBUG_PORT     3           /* 调试串口 */
#define SBL_UART_BAUDRATE       115200      /* 串口波特率 */

/* 安全配置 */
#define SBL_SECURE_BOOT         0           /* 安全启动(暂时禁用) */
#define SBL_IMAGE_VERIFICATION  1           /* 镜像验证 */
#define SBL_CRC32_CHECK         1           /* CRC32校验 */

/* ESP32兼容性配置 */
#define ESP_IMAGE_MAGIC         0xE9        /* ESP32镜像魔数 */
#define ESP_IMAGE_SEGMENT_MAGIC 0xAE        /* ESP32段魔数 */
#define ESP_CHECKSUM_MAGIC      0xEF        /* ESP32校验魔数 */

/* 内存配置 */
#define SBL_STACK_SIZE          8192        /* SBL栈大小: 8KB */
#define SBL_HEAP_SIZE           16384       /* SBL堆大小: 16KB */

/* 断言宏 */
#if SBL_DEBUG_ENABLED
#define SBL_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("[SBL ASSERT] %s:%d - %s\r\n", __FILE__, __LINE__, #condition); \
            while(1); \
        } \
    } while(0)
#else
#define SBL_ASSERT(condition) ((void)0)
#endif

/* 日志宏 */
#if SBL_DEBUG_ENABLED
#define SBL_LOGI(tag, format, ...) printf("[SBL][%s] " format "\r\n", tag, ##__VA_ARGS__)
#define SBL_LOGW(tag, format, ...) printf("[SBL][%s] WARNING: " format "\r\n", tag, ##__VA_ARGS__)
#define SBL_LOGE(tag, format, ...) printf("[SBL][%s] ERROR: " format "\r\n", tag, ##__VA_ARGS__)
#else
#define SBL_LOGI(tag, format, ...) ((void)0)
#define SBL_LOGW(tag, format, ...) ((void)0)
#define SBL_LOGE(tag, format, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SBL_CONFIG_H */
