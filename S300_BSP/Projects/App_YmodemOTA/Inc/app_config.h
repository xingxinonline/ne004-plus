/**
 * @file app_config.h
 * @brief App配置文件 - Ymodem OTA应用程序
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* 标准库包含 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* CMSIS包含 */
#include "device.h"  /* 必须在core_cm4.h之前 */
#include "core_cm4.h"
#include "s300.h"

/* BSP驱动包含 */
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
/* #include "qspi.h" */  /* 暂时注释掉，避免编译错误 */

/* App版本信息 */
#define APP_VERSION_MAJOR       1
#define APP_VERSION_MINOR       0
#define APP_VERSION_PATCH       0
#define APP_VERSION_STRING      "1.0.0"

/* 硬件配置 */
#define APP_CPU_FREQ_HZ         192000000   /* 192MHz */
#define APP_SRAM_BASE           0x20000000  /* SRAM基址 */
#define APP_SRAM_SIZE           (364 * 1024) /* 364KB可用SRAM */

/* UART配置 */
#define APP_UART_PORT           3           /* UART3 */
#define APP_UART_BAUDRATE       115200      /* 波特率 */
#define APP_UART_TX_PIN         10          /* PA10 */
#define APP_UART_RX_PIN         11          /* PA11 */

/* Flash配置 */
#define APP_FLASH_BASE          0x08000000  /* Flash基址 */
#define APP_FLASH_SIZE          (16 * 1024 * 1024) /* 16MB */

/* OTA配置 */
#define APP_OTA_PARTITION_SIZE  (1024 * 1024)  /* 1MB OTA分区 */
#define APP_OTA_BUFFER_SIZE     1024        /* OTA缓冲区 */
#define APP_OTA_TIMEOUT_MS      30000       /* OTA超时30秒 */

/* Ymodem配置 */
#define YMODEM_PACKET_SIZE      1024        /* Ymodem数据包大小 */
#define YMODEM_TIMEOUT_MS       5000        /* Ymodem超时5秒 */
#define YMODEM_MAX_ERRORS       10          /* 最大错误次数 */

/* 系统配置 */
#define APP_SYSTICK_FREQ        1000        /* 1ms SysTick */
#define APP_LED_BLINK_FREQ      2           /* LED闪烁频率 */
#define APP_WATCHDOG_TIMEOUT    30000       /* 看门狗30秒 */

/* 调试配置 */
#define APP_DEBUG_ENABLED       1           /* 调试输出 */
#define APP_VERBOSE_OTA         1           /* OTA详细日志 */

/* 断言宏 */
#if APP_DEBUG_ENABLED
#define APP_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("[APP ASSERT] %s:%d - %s\r\n", __FILE__, __LINE__, #condition); \
            while(1); \
        } \
    } while(0)
#else
#define APP_ASSERT(condition) ((void)0)
#endif

/* 日志宏 */
#if APP_DEBUG_ENABLED
#define APP_LOGI(tag, format, ...) printf("[APP][%s] " format "\r\n", tag, ##__VA_ARGS__)
#define APP_LOGW(tag, format, ...) printf("[APP][%s] WARNING: " format "\r\n", tag, ##__VA_ARGS__)
#define APP_LOGE(tag, format, ...) printf("[APP][%s] ERROR: " format "\r\n", tag, ##__VA_ARGS__)
#define APP_LOGD(tag, format, ...) printf("[APP][%s] DEBUG: " format "\r\n", tag, ##__VA_ARGS__)
#else
#define APP_LOGI(tag, format, ...) ((void)0)
#define APP_LOGW(tag, format, ...) ((void)0)
#define APP_LOGE(tag, format, ...) ((void)0)
#define APP_LOGD(tag, format, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
