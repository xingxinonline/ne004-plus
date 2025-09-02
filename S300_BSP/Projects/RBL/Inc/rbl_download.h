/**
 * @file rbl_download.h
 * @brief RBL下载管理器
 * 
 * 集成YMODEM协议与Flash写入，管理下载流程
 */

#ifndef RBL_DOWNLOAD_H
#define RBL_DOWNLOAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 下载配置 */
// === 下载配置参数 ===

// YMODEM下载到Flash的起始地址
#define DOWNLOAD_FLASH_START_ADDR   0x80000000  /* Flash起始地址(QSPI Flash) - 下载完整镜像 */
#define DOWNLOAD_SECTOR_SIZE        4096        /* Flash扇区大小 */
#define DOWNLOAD_MAX_SIZE          (128 * 1024) /* 最大下载大小(128KB) */
#define DOWNLOAD_TIMEOUT_MS         30000       /* 下载超时（30秒） */

/* 下载状态 */
typedef enum {
    DOWNLOAD_STATE_IDLE = 0,
    DOWNLOAD_STATE_WAITING,
    DOWNLOAD_STATE_RECEIVING,
    DOWNLOAD_STATE_COMPLETED,
    DOWNLOAD_STATE_ERROR,
    DOWNLOAD_STATE_TIMEOUT
} download_state_t;

/* 下载统计 */
typedef struct {
    uint32_t bytes_received;
    uint32_t packets_received;
    uint32_t errors;
    uint32_t start_time;
    uint32_t end_time;
} download_stats_t;

/**
 * @brief 初始化下载管理器
 */
void rbl_download_init(void);

/**
 * @brief 启动下载模式
 * @return true表示启动成功
 */
bool rbl_download_start(void);

/**
 * @brief 下载主循环
 * @return 下载状态
 */
download_state_t rbl_download_process(void);

/**
 * @brief 获取下载统计
 * @param stats 统计信息指针
 */
void rbl_download_get_stats(download_stats_t *stats);

/**
 * @brief 停止下载
 */
void rbl_download_stop(void);

/**
 * @brief 检查下载是否超时
 * @return true表示超时
 */
bool rbl_download_is_timeout(void);

/**
 * @brief 重置下载管理器
 */
void rbl_download_reset(void);

/**
 * @brief 打印下载统计
 */
void rbl_download_print_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* RBL_DOWNLOAD_H */
