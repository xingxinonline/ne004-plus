/**
 * @file rbl_config.h
 * @brief RBL配置参数
 */

#ifndef RBL_CONFIG_H
#define RBL_CONFIG_H

/* 首先包含基础头文件 */
#include <stdint.h>
#include <stdbool.h>

/* 然后包含设备相关头文件 */
#include "device.h"
#include "s300_memmap.h"
#include "uart.h"

/* 系统配置 */
#define RBL_SYSTEM_CLOCK_HZ         168000000    // 系统时钟168MHz
#define RBL_AHB_CLOCK_HZ            168000000    // AHB时钟
#define RBL_APB_CLOCK_HZ            42000000     // APB时钟42MHz

/* SRAM配置 */
#define RBL_SRAM_BASE               0x20000000   // SRAM起始地址
#define RBL_SRAM_SIZE               (384 * 1024) // SRAM大小384KB
#define RBL_STACK_SIZE              (8 * 1024)   // 栈大小8KB

/* QSPI Flash配置 */
#define RBL_QSPI_BASE               0x08000000   // QSPI XIP基地址(S300实际地址)
#define RBL_QSPI_CFG_BASE           QSPI_CFG_BASE // QSPI配置寄存器基地址
#define RBL_FLASH_SIZE              (16 * 1024 * 1024) // Flash大小16MB
#define RBL_FLASH_SECTOR_SIZE       4096         // Flash扇区大小4KB
#define RBL_FLASH_PAGE_SIZE         256          // Flash页大小256B

/* 串口配置 */
#define RBL_UART_PORT               UART_IDX3    // 使用UART3
#define RBL_UART_BASE               UART3_BASE   // UART3基地址
#define RBL_UART_IRQ                UART3_IRQn   // UART3中断号
#define RBL_UART_BAUDRATE           115200       // 波特率

/* 下载配置 */
#define RBL_DOWNLOAD_WINDOW_MS      3000         // 下载窗口期3秒
#define RBL_DOWNLOAD_TIMEOUT_MS     30000        // 下载超时30秒
#define RBL_DOWNLOAD_CHUNK_SIZE     1024         // 下载块大小1KB
#define RBL_MAX_RETRY_COUNT         3            // 最大重试次数

/* SBL配置 */
#define RBL_SBL_FLASH_ADDR          0x10000      // SBL在Flash中的地址
#define RBL_SBL_MAX_SIZE            (128 * 1024) // SBL最大大小128KB
#define RBL_SBL_XIP_ADDR            0x80010000   // SBL XIP地址

/* GPIO配置 */
#define RBL_BOOT_MODE_GPIO          0            // 启动模式检测GPIO
#define RBL_LED_GPIO                1            // 状态LED GPIO

/* 调试配置 */
#define RBL_DEBUG_ENABLED           1            // 启用调试输出
#define RBL_VERBOSE_BOOT            1            // 详细启动信息
#define RBL_ENABLE_ASSERT           1            // 启用断言

/* 断言宏定义 */
#if RBL_ENABLE_ASSERT
#define RBL_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            printf("[RBL] ASSERT FAILED: %s:%d %s\r\n", __FILE__, __LINE__, #expr); \
            while(1) { rbl_system_delay_ms(1000); } \
        } \
    } while(0)
#else
#define RBL_ASSERT(expr) ((void)0)
#endif

/* 安全配置 */
#define RBL_ENABLE_CRC_CHECK        1            // 启用CRC校验
#define RBL_ENABLE_SIGNATURE_CHECK  0            // 签名校验(暂未实现)

/* 性能配置 */
#define RBL_ENABLE_ICACHE           1            // 启用指令缓存
#define RBL_ENABLE_DCACHE           0            // 数据缓存(SRAM无需)

#endif // RBL_CONFIG_H
