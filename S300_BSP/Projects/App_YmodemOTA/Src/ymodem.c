/**
 * @file ymodem.c
 * @brief Ymodem协议实现
 */

#include "ymodem.h"
#include "app_config.h"
#include "uart.h"
#include "s300.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 外部函数声明 */
extern uint32_t app_get_tick_ms(void);

/* CRC16表 */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/* 静态变量 */
static ymodem_context_t* g_ymodem_ctx = NULL;

/**
 * @brief 初始化Ymodem接收器
 */
int ymodem_init(ymodem_context_t* ctx,
                ymodem_write_callback_t write_callback,
                ymodem_progress_callback_t progress_callback,
                void* user_data)
{
    if (ctx == NULL || write_callback == NULL) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(ymodem_context_t));
    
    ctx->write_callback = write_callback;
    ctx->progress_callback = progress_callback;
    ctx->user_data = user_data;
    ctx->timeout_ms = YMODEM_TIMEOUT_MS;
    ctx->max_errors = YMODEM_MAX_ERRORS;
    
    g_ymodem_ctx = ctx;
    
    APP_LOGI("YMODEM", "Ymodem initialized");
    return 0;
}

/**
 * @brief 开始Ymodem接收
 */
ymodem_status_t ymodem_receive_start(ymodem_context_t* ctx)
{
    if (ctx == NULL) {
        return YMODEM_STATUS_ERROR;
    }
    
    APP_LOGI("YMODEM", "Starting Ymodem receive...");
    
    /* 重置文件信息 */
    memset(&ctx->file_info, 0, sizeof(ymodem_file_info_t));
    ctx->error_count = 0;
    
    /* 发送'C'字符启动CRC模式传输 */
    ymodem_putchar(YMODEM_CRC_CHR);
    
    return YMODEM_STATUS_OK;
}

/**
 * @brief 接收单个数据包
 */
ymodem_status_t ymodem_receive_packet(ymodem_context_t* ctx, 
                                     uint8_t* packet_data, 
                                     size_t* packet_size)
{
    if (ctx == NULL || packet_data == NULL || packet_size == NULL) {
        return YMODEM_STATUS_ERROR;
    }
    
    uint8_t header;
    uint8_t packet_num, packet_num_comp;
    uint16_t crc_received, crc_calculated;
    size_t data_size;
    
    /* 接收包头 */
    if (ymodem_getchar(&header, ctx->timeout_ms) != 0) {
        APP_LOGW("YMODEM", "Timeout waiting for packet header");
        return YMODEM_STATUS_TIMEOUT;
    }
    
    switch (header) {
    case YMODEM_SOH:
        data_size = YMODEM_PACKET_128;
        break;
        
    case YMODEM_STX:
        data_size = YMODEM_PACKET_1024;
        break;
        
    case YMODEM_EOT:
        APP_LOGI("YMODEM", "Received EOT");
        ymodem_putchar(YMODEM_ACK);
        return YMODEM_STATUS_OK;
        
    case YMODEM_CAN:
        APP_LOGW("YMODEM", "Received CAN - transfer cancelled");
        return YMODEM_STATUS_CANCEL;
        
    default:
        APP_LOGW("YMODEM", "Invalid packet header: 0x%02X", header);
        ymodem_putchar(YMODEM_NAK);
        ctx->error_count++;
        return YMODEM_STATUS_PACKET_ERROR;
    }
    
    /* 接收包序号 */
    if (ymodem_getchar(&packet_num, ctx->timeout_ms) != 0 ||
        ymodem_getchar(&packet_num_comp, ctx->timeout_ms) != 0) {
        APP_LOGW("YMODEM", "Timeout receiving packet number");
        ymodem_putchar(YMODEM_NAK);
        ctx->error_count++;
        return YMODEM_STATUS_TIMEOUT;
    }
    
    /* 验证包序号 */
    if (packet_num != (uint8_t)(~packet_num_comp)) {
        APP_LOGW("YMODEM", "Packet number mismatch: %d != ~%d", packet_num, packet_num_comp);
        ymodem_putchar(YMODEM_NAK);
        ctx->error_count++;
        return YMODEM_STATUS_PACKET_ERROR;
    }
    
    /* 接收数据 */
    for (size_t i = 0; i < data_size; i++) {
        if (ymodem_getchar(&packet_data[i], ctx->timeout_ms) != 0) {
            APP_LOGW("YMODEM", "Timeout receiving data at byte %zu", i);
            ymodem_putchar(YMODEM_NAK);
            ctx->error_count++;
            return YMODEM_STATUS_TIMEOUT;
        }
    }
    
    /* 接收CRC */
    uint8_t crc_high, crc_low;
    if (ymodem_getchar(&crc_high, ctx->timeout_ms) != 0 ||
        ymodem_getchar(&crc_low, ctx->timeout_ms) != 0) {
        APP_LOGW("YMODEM", "Timeout receiving CRC");
        ymodem_putchar(YMODEM_NAK);
        ctx->error_count++;
        return YMODEM_STATUS_TIMEOUT;
    }
    
    crc_received = (crc_high << 8) | crc_low;
    crc_calculated = ymodem_crc16(packet_data, data_size);
    
    /* 验证CRC */
    if (crc_received != crc_calculated) {
        APP_LOGW("YMODEM", "CRC mismatch: received=0x%04X, calculated=0x%04X", 
                 crc_received, crc_calculated);
        ymodem_putchar(YMODEM_NAK);
        ctx->error_count++;
        return YMODEM_STATUS_CRC_ERROR;
    }
    
    /* 检查错误次数 */
    if (ctx->error_count >= ctx->max_errors) {
        APP_LOGE("YMODEM", "Too many errors, aborting");
        ymodem_cancel(ctx);
        return YMODEM_STATUS_ABORT;
    }
    
    /* 处理文件头（第一个包） */
    if (packet_num == 0 && !ctx->file_info.header_received) {
        /* 解析文件名和大小 */
        if (packet_data[0] != 0) {
            strncpy(ctx->file_info.filename, (char*)packet_data, sizeof(ctx->file_info.filename) - 1);
            ctx->file_info.filename[sizeof(ctx->file_info.filename) - 1] = '\0';
            
            /* 查找文件大小 */
            char* size_str = strchr((char*)packet_data, ' ');
            if (size_str != NULL) {
                ctx->file_info.filesize = strtoul(size_str + 1, NULL, 10);
            }
            
            ctx->file_info.header_received = true;
            
            APP_LOGI("YMODEM", "File: %s, Size: %lu bytes", 
                     ctx->file_info.filename, ctx->file_info.filesize);
            
            ymodem_putchar(YMODEM_ACK);
            ymodem_putchar(YMODEM_CRC_CHR);
        } else {
            /* 空文件名表示传输结束 */
            ymodem_putchar(YMODEM_ACK);
            return YMODEM_STATUS_OK;
        }
    } else if (packet_num > 0) {
        /* 数据包 */
        size_t actual_size = data_size;
        
        /* 最后一个包可能不满 */
        if (ctx->file_info.received_bytes + data_size > ctx->file_info.filesize) {
            actual_size = ctx->file_info.filesize - ctx->file_info.received_bytes;
        }
        
        /* 写入数据 */
        if (ctx->write_callback != NULL && actual_size > 0) {
            if (ctx->write_callback(packet_data, actual_size, ctx->user_data) != 0) {
                APP_LOGE("YMODEM", "Write callback failed");
                ymodem_cancel(ctx);
                return YMODEM_STATUS_ERROR;
            }
        }
        
        ctx->file_info.received_bytes += actual_size;
        
        /* 报告进度 */
        if (ctx->progress_callback != NULL) {
            ctx->progress_callback(ctx->file_info.received_bytes, 
                                 ctx->file_info.filesize, 
                                 ctx->user_data);
        }
        
        ymodem_putchar(YMODEM_ACK);
        
        APP_LOGD("YMODEM", "Received packet %d, %zu bytes (%lu/%lu)", 
                 packet_num, actual_size, 
                 ctx->file_info.received_bytes, ctx->file_info.filesize);
    }
    
    ctx->file_info.packet_number = packet_num;
    *packet_size = data_size;
    
    return YMODEM_STATUS_OK;
}

/**
 * @brief 完成Ymodem接收
 */
ymodem_status_t ymodem_receive_finish(ymodem_context_t* ctx)
{
    if (ctx == NULL) {
        return YMODEM_STATUS_ERROR;
    }
    
    APP_LOGI("YMODEM", "Ymodem receive finished");
    APP_LOGI("YMODEM", "File: %s", ctx->file_info.filename);
    APP_LOGI("YMODEM", "Size: %lu bytes", ctx->file_info.filesize);
    APP_LOGI("YMODEM", "Received: %lu bytes", ctx->file_info.received_bytes);
    
    return YMODEM_STATUS_OK;
}

/**
 * @brief 取消Ymodem传输
 */
void ymodem_cancel(ymodem_context_t* ctx)
{
    APP_LOGW("YMODEM", "Cancelling Ymodem transfer");
    
    /* 发送取消信号 */
    for (int i = 0; i < 3; i++) {
        ymodem_putchar(YMODEM_CAN);
    }
    
    ymodem_flush_input();
}

/**
 * @brief 获取文件信息
 */
const ymodem_file_info_t* ymodem_get_file_info(const ymodem_context_t* ctx)
{
    if (ctx == NULL) {
        return NULL;
    }
    
    return &ctx->file_info;
}

/**
 * @brief 计算CRC16校验和
 */
uint16_t ymodem_crc16(const uint8_t* data, size_t length)
{
    uint16_t crc = 0;
    
    for (size_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    return crc;
}

/**
 * @brief 发送字符
 */
int ymodem_putchar(uint8_t ch)
{
    return uart_write(UART_IDX3, UARTTYPE_STD_SERIAL, ch);
}

/**
 * @brief 接收字符
 */
int ymodem_getchar(uint8_t* ch, uint32_t timeout_ms)
{
    if (ch == NULL) {
        return -1;
    }
    
    uint32_t start_time = app_get_tick_ms();
    
    while ((app_get_tick_ms() - start_time) < timeout_ms) {
        /* 检查是否有数据可读 */
        S300_UART_TypeDef* uart = (S300_UART_TypeDef*)UART3;
        if (uart->LSR & 0x01) { /* 数据就绪 */
            uint16_t data = uart_read(UART_IDX3, UARTTYPE_STD_SERIAL);
            *ch = (uint8_t)(data & 0xFF);
            return 0;
        }
        
        /* 短暂延时避免忙等待 */
        __WFI();
    }
    
    return -1; /* 超时 */
}

/**
 * @brief 清空接收缓冲区
 */
void ymodem_flush_input(void)
{
    /* 读取所有可用数据直到缓冲区为空 */
    S300_UART_TypeDef* uart = (S300_UART_TypeDef*)UART3;
    while (uart->LSR & 0x01) {
        uart_read(UART_IDX3, UARTTYPE_STD_SERIAL);
    }
}
