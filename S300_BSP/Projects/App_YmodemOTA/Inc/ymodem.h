/**
 * @file ymodem.h
 * @brief Ymodem协议实现头文件
 */

#ifndef YMODEM_H
#define YMODEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ymodem协议常量 */
#define YMODEM_SOH          0x01    /* Start of Header (128 bytes) */
#define YMODEM_STX          0x02    /* Start of Header (1024 bytes) */
#define YMODEM_EOT          0x04    /* End of Transmission */
#define YMODEM_ACK          0x06    /* Acknowledge */
#define YMODEM_NAK          0x15    /* Negative Acknowledge */
#define YMODEM_CAN          0x18    /* Cancel */
#define YMODEM_CRC_CHR      'C'     /* CRC mode */

#define YMODEM_PACKET_128   128     /* 128字节数据包 */
#define YMODEM_PACKET_1024  1024    /* 1024字节数据包 */

/* Ymodem状态 */
typedef enum {
    YMODEM_STATUS_OK = 0,           /* 成功 */
    YMODEM_STATUS_ERROR,            /* 错误 */
    YMODEM_STATUS_TIMEOUT,          /* 超时 */
    YMODEM_STATUS_CANCEL,           /* 取消 */
    YMODEM_STATUS_CRC_ERROR,        /* CRC错误 */
    YMODEM_STATUS_PACKET_ERROR,     /* 数据包错误 */
    YMODEM_STATUS_ABORT,            /* 中止 */
} ymodem_status_t;

/* Ymodem文件信息 */
typedef struct {
    char filename[256];             /* 文件名 */
    uint32_t filesize;              /* 文件大小 */
    uint32_t received_bytes;        /* 已接收字节数 */
    uint8_t packet_number;          /* 当前包序号 */
    bool header_received;           /* 是否已接收头部 */
} ymodem_file_info_t;

/* Ymodem回调函数类型 */
typedef int (*ymodem_write_callback_t)(const uint8_t* data, size_t size, void* user_data);
typedef void (*ymodem_progress_callback_t)(uint32_t received, uint32_t total, void* user_data);

/* Ymodem上下文 */
typedef struct {
    ymodem_file_info_t file_info;          /* 文件信息 */
    ymodem_write_callback_t write_callback; /* 写入回调 */
    ymodem_progress_callback_t progress_callback; /* 进度回调 */
    void* user_data;                        /* 用户数据 */
    uint32_t timeout_ms;                    /* 超时时间 */
    uint32_t max_errors;                    /* 最大错误次数 */
    uint32_t error_count;                   /* 错误计数 */
} ymodem_context_t;

/* 函数声明 */

/**
 * @brief 初始化Ymodem接收器
 * @param ctx Ymodem上下文
 * @param write_callback 数据写入回调函数
 * @param progress_callback 进度报告回调函数
 * @param user_data 用户数据指针
 * @return 0成功，非0失败
 */
int ymodem_init(ymodem_context_t* ctx,
                ymodem_write_callback_t write_callback,
                ymodem_progress_callback_t progress_callback,
                void* user_data);

/**
 * @brief 开始Ymodem接收
 * @param ctx Ymodem上下文
 * @return Ymodem状态
 */
ymodem_status_t ymodem_receive_start(ymodem_context_t* ctx);

/**
 * @brief 接收单个数据包
 * @param ctx Ymodem上下文
 * @param packet_data 数据包缓冲区
 * @param packet_size 数据包大小
 * @return Ymodem状态
 */
ymodem_status_t ymodem_receive_packet(ymodem_context_t* ctx, 
                                     uint8_t* packet_data, 
                                     size_t* packet_size);

/**
 * @brief 完成Ymodem接收
 * @param ctx Ymodem上下文
 * @return Ymodem状态
 */
ymodem_status_t ymodem_receive_finish(ymodem_context_t* ctx);

/**
 * @brief 取消Ymodem传输
 * @param ctx Ymodem上下文
 */
void ymodem_cancel(ymodem_context_t* ctx);

/**
 * @brief 获取文件信息
 * @param ctx Ymodem上下文
 * @return 文件信息指针
 */
const ymodem_file_info_t* ymodem_get_file_info(const ymodem_context_t* ctx);

/**
 * @brief 计算CRC16校验和
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16值
 */
uint16_t ymodem_crc16(const uint8_t* data, size_t length);

/**
 * @brief 发送字符
 * @param ch 字符
 * @return 0成功，非0失败
 */
int ymodem_putchar(uint8_t ch);

/**
 * @brief 接收字符
 * @param ch 字符指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 0成功，非0失败
 */
int ymodem_getchar(uint8_t* ch, uint32_t timeout_ms);

/**
 * @brief 清空接收缓冲区
 */
void ymodem_flush_input(void);

#ifdef __cplusplus
}
#endif

#endif /* YMODEM_H */
