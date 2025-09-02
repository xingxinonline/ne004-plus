/**
 * @file rbl_download.c
 * @brief RBL下载管理器实现
 */

#include "rbl_download.h"
#include "rbl_ymodem.h"
#include "rbl_hal.h"
#include "rbl_qspi.h"
#include "s300.h"
#include <string.h>

/* 静态变量 */
static download_state_t s_download_state = DOWNLOAD_STATE_IDLE;
static download_stats_t s_download_stats;
static ymodem_receiver_t s_ymodem_receiver;
static uint32_t s_start_tick = 0;

/* 内部函数声明 */
static void download_data_callback(uint32_t address, const uint8_t *data, uint32_t size);
static void download_progress_callback(uint32_t bytes_received, uint32_t total_bytes);
static uint32_t get_tick_ms(void);
static bool erase_flash_sectors(uint32_t start_addr, uint32_t size);

/**
 * @brief 初始化下载管理器
 */
void rbl_download_init(void)
{
    s_download_state = DOWNLOAD_STATE_IDLE;
    memset(&s_download_stats, 0, sizeof(s_download_stats));
    
    /* 设置YMODEM回调 */
    rbl_ymodem_set_data_callback(download_data_callback);
    rbl_ymodem_set_progress_callback(download_progress_callback);
    
    RBL_LOG("Download manager initialized\r\n");
}

/**
 * @brief 启动下载模式
 */
bool rbl_download_start(void)
{
    if (s_download_state != DOWNLOAD_STATE_IDLE) {
        return false;
    }
    
    RBL_LOG("\r\n=== Entering Download Mode ===\r\n");
    RBL_LOG("Waiting for YMODEM transfer...\r\n");
    RBL_LOG("Please use YMODEM compatible terminal to send file\r\n");
    
    /* 初始化YMODEM接收器 */
    rbl_ymodem_init(&s_ymodem_receiver, DOWNLOAD_FLASH_START_ADDR);
    
    /* 启动YMODEM接收 */
    if (!rbl_ymodem_start_receive(&s_ymodem_receiver)) {
        RBL_LOG("Start YMODEM receive failed\r\n");
        return false;
    }
    
    /* 重置统计 */
    memset(&s_download_stats, 0, sizeof(s_download_stats));
    s_download_stats.start_time = get_tick_ms();
    s_start_tick = s_download_stats.start_time;
    
    s_download_state = DOWNLOAD_STATE_WAITING;
    return true;
}

/**
 * @brief 下载主循环
 */
download_state_t rbl_download_process(void)
{
    if (s_download_state == DOWNLOAD_STATE_IDLE) {
        return s_download_state;
    }
    
    /* 检查超时 */
    if (rbl_download_is_timeout()) {
        RBL_LOG("Download timeout\r\n");
        s_download_state = DOWNLOAD_STATE_TIMEOUT;
        return s_download_state;
    }
    
    /* 检查串口数据 */
    uint8_t byte;
    if (rbl_hal_uart_receive(&byte, 1) > 0) {
        if (s_download_state == DOWNLOAD_STATE_WAITING) {
            s_download_state = DOWNLOAD_STATE_RECEIVING;
            RBL_LOG("Start receiving data...\r\n");
        }
        
        /* 处理接收到的字节 */
        if (!rbl_ymodem_process_byte(&s_ymodem_receiver, byte)) {
            /* YMODEM接收结束 */
            ymodem_state_t ymodem_state = rbl_ymodem_get_state(&s_ymodem_receiver);
            
            if (ymodem_state == YMODEM_STATE_COMPLETED) {
                s_download_stats.end_time = get_tick_ms();
                s_download_state = DOWNLOAD_STATE_COMPLETED;
                RBL_LOG("Download completed!\r\n");
                rbl_download_print_stats();
            } else if (ymodem_state == YMODEM_STATE_CANCELLED) {
                s_download_state = DOWNLOAD_STATE_ERROR;
                RBL_LOG("下载被取消\r\n");
            } else {
                s_download_state = DOWNLOAD_STATE_ERROR;
                s_download_stats.errors++;
                RBL_LOG("下载错误\r\n");
            }
        }
    }
    
    return s_download_state;
}

/**
 * @brief 数据写入回调
 */
static void download_data_callback(uint32_t address, const uint8_t *data, uint32_t size)
{
    /* 检查地址范围 */
    if (address < DOWNLOAD_FLASH_START_ADDR || 
        (address + size) > (DOWNLOAD_FLASH_START_ADDR + DOWNLOAD_MAX_SIZE)) {
        RBL_LOG("地址超出范围: 0x%08X\r\n", address);
        s_download_stats.errors++;
        return;
    }
    
    /* 如果是第一次写入，需要擦除扇区 */
    static bool first_write = true;
    if (first_write) {
        RBL_LOG("擦除Flash扇区...\r\n");
        if (!erase_flash_sectors(DOWNLOAD_FLASH_START_ADDR, DOWNLOAD_MAX_SIZE)) {
            RBL_LOG("擦除Flash失败\r\n");
            s_download_stats.errors++;
            return;
        }
        first_write = false;
    }
    
    /* 写入数据到Flash */
    if (rbl_qspi_page_program(address, data, size) == 0) {
        s_download_stats.bytes_received += size;
        s_download_stats.packets_received++;
        
        /* 可选：写入后验证 */
        uint8_t verify_buffer[256];
        if (size <= sizeof(verify_buffer)) {
            if (rbl_qspi_read(address, verify_buffer, size) == 0) {
                for (uint32_t i = 0; i < size; i++) {
                    if (verify_buffer[i] != data[i]) {
                        RBL_LOG("写入验证失败，地址=0x%08X\r\n", address + i);
                        s_download_stats.errors++;
                        return;
                    }
                }
            }
        }
    } else {
        RBL_LOG("Flash写入失败，地址=0x%08X\r\n", address);
        s_download_stats.errors++;
    }
}

/**
 * @brief 进度回调
 */
static void download_progress_callback(uint32_t bytes_received, uint32_t total_bytes)
{
    (void)total_bytes;  /* 暂时未使用 */
    
    /* 每1KB显示一次进度 */
    static uint32_t last_kb = 0;
    uint32_t current_kb = bytes_received / 1024;
    if (current_kb > last_kb) {
        RBL_LOG("已接收: %dKB\r\n", current_kb);
        last_kb = current_kb;
    }
}

/**
 * @brief 获取系统时钟（简单实现）
 */
static uint32_t get_tick_ms(void)
{
    /* 简单的时钟实现，实际应该使用系统时钟 */
    static uint32_t tick_counter = 0;
    return tick_counter++;
}

/**
 * @brief 擦除Flash扇区
 */
static bool erase_flash_sectors(uint32_t start_addr, uint32_t size)
{
    uint32_t sector_count = (size + DOWNLOAD_SECTOR_SIZE - 1) / DOWNLOAD_SECTOR_SIZE;
    
    for (uint32_t i = 0; i < sector_count; i++) {
        uint32_t sector_addr = start_addr + (i * DOWNLOAD_SECTOR_SIZE);
        if (rbl_qspi_erase_4k(sector_addr) != 0) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 打印下载统计
 */
void rbl_download_print_stats(void)
{
    uint32_t duration = s_download_stats.end_time - s_download_stats.start_time;
    if (duration == 0) duration = 1;  /* 避免除零 */
    
    RBL_LOG("\r\n=== 下载统计 ===\r\n");
    RBL_LOG("接收字节: %d\r\n", s_download_stats.bytes_received);
    RBL_LOG("接收包数: %d\r\n", s_download_stats.packets_received);
    RBL_LOG("错误次数: %d\r\n", s_download_stats.errors);
    RBL_LOG("传输时间: %d ticks\r\n", duration);
    RBL_LOG("平均速度: %d bytes/tick\r\n", s_download_stats.bytes_received / duration);
}

/**
 * @brief 获取下载统计
 */
void rbl_download_get_stats(download_stats_t *stats)
{
    if (stats) {
        *stats = s_download_stats;
    }
}

/**
 * @brief 停止下载
 */
void rbl_download_stop(void)
{
    if (s_download_state == DOWNLOAD_STATE_RECEIVING || 
        s_download_state == DOWNLOAD_STATE_WAITING) {
        rbl_ymodem_send_can();
        RBL_LOG("下载被停止\r\n");
    }
    
    s_download_state = DOWNLOAD_STATE_IDLE;
}

/**
 * @brief 检查下载是否超时
 */
bool rbl_download_is_timeout(void)
{
    if (s_download_state == DOWNLOAD_STATE_IDLE) {
        return false;
    }
    
    uint32_t current_time = get_tick_ms();
    return (current_time - s_start_tick) > (DOWNLOAD_TIMEOUT_MS / 10);  /* 简化时间计算 */
}

/**
 * @brief 重置下载管理器
 */
void rbl_download_reset(void)
{
    s_download_state = DOWNLOAD_STATE_IDLE;
    memset(&s_download_stats, 0, sizeof(s_download_stats));
    rbl_ymodem_reset(&s_ymodem_receiver);
}
