/**
 * @file rbl_ymodem.h
 * @brief RBL YMODEM简版协议实现
 * 
 * 实现YMODEM协议的简化版本，用于串口下载功能
 */

#ifndef RBL_YMODEM_H
#define RBL_YMODEM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* YMODEM协议定义 */
#define YMODEM_SOH          0x01    /* Start of Header (128字节包) */
#define YMODEM_STX          0x02    /* Start of Text (1024字节包) */
#define YMODEM_EOT          0x04    /* End of Transmission */
#define YMODEM_ACK          0x06    /* Acknowledge */
#define YMODEM_NAK          0x15    /* Negative Acknowledge */
#define YMODEM_CAN          0x18    /* Cancel */
#define YMODEM_CRC16        0x43    /* 'C' - CRC16模式 */

#define YMODEM_PACKET_SIZE_128   128
#define YMODEM_PACKET_SIZE_1024  1024
#define YMODEM_HEADER_SIZE       3      /* SOH/STX + SEQ + ~SEQ */
#define YMODEM_CRC_SIZE          2      /* CRC16 */

#define YMODEM_MAX_RETRIES       10
#define YMODEM_TIMEOUT_MS        3000

/* YMODEM状态定义 */
typedef enum {
    YMODEM_STATE_IDLE = 0,
    YMODEM_STATE_WAITING_HEADER,
    YMODEM_STATE_RECEIVING_DATA,
    YMODEM_STATE_COMPLETED,
    YMODEM_STATE_ERROR,
    YMODEM_STATE_CANCELLED
} ymodem_state_t;

/* YMODEM数据包结构 */
typedef struct {
    uint8_t header;                     /* SOH/STX */
    uint8_t sequence;                   /* 包序号 */
    uint8_t sequence_inv;               /* 包序号的反码 */
    uint8_t data[YMODEM_PACKET_SIZE_1024]; /* 数据（最大1024字节） */
    uint16_t crc;                       /* CRC校验 */
    uint16_t data_size;                 /* 实际数据大小 */
} ymodem_packet_t;

/* YMODEM接收器结构 */
typedef struct {
    ymodem_state_t state;               /* 当前状态 */
    uint8_t expected_seq;               /* 期望的包序号 */
    uint32_t total_bytes;               /* 总接收字节数 */
    uint32_t packet_count;              /* 包计数 */
    uint32_t retry_count;               /* 重试计数 */
    uint32_t flash_address;             /* Flash写入地址 */
    bool first_packet;                  /* 是否为第一个包（文件名包） */
} ymodem_receiver_t;

/* YMODEM回调函数类型 */
typedef void (*ymodem_data_callback_t)(uint32_t address, const uint8_t *data, uint32_t size);
typedef void (*ymodem_progress_callback_t)(uint32_t bytes_received, uint32_t total_bytes);

/**
 * @brief 初始化YMODEM接收器
 * @param receiver YMODEM接收器指针
 * @param flash_start_addr Flash起始地址
 */
void rbl_ymodem_init(ymodem_receiver_t *receiver, uint32_t flash_start_addr);

/**
 * @brief 启动YMODEM接收
 * @param receiver YMODEM接收器指针
 * @return true表示启动成功
 */
bool rbl_ymodem_start_receive(ymodem_receiver_t *receiver);

/**
 * @brief 处理接收到的字节
 * @param receiver YMODEM接收器指针
 * @param byte 接收到的字节
 * @return true表示需要继续接收
 */
bool rbl_ymodem_process_byte(ymodem_receiver_t *receiver, uint8_t byte);

/**
 * @brief 发送ACK
 */
void rbl_ymodem_send_ack(void);

/**
 * @brief 发送NAK
 */
void rbl_ymodem_send_nak(void);

/**
 * @brief 发送CAN（取消）
 */
void rbl_ymodem_send_can(void);

/**
 * @brief 发送'C'（请求CRC模式）
 */
void rbl_ymodem_send_c(void);

/**
 * @brief 计算CRC16
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16值
 */
uint16_t rbl_ymodem_crc16(const uint8_t *data, uint32_t length);

/**
 * @brief 设置数据写入回调
 * @param callback 回调函数
 */
void rbl_ymodem_set_data_callback(ymodem_data_callback_t callback);

/**
 * @brief 设置进度回调
 * @param callback 回调函数
 */
void rbl_ymodem_set_progress_callback(ymodem_progress_callback_t callback);

/**
 * @brief 获取当前状态
 * @param receiver YMODEM接收器指针
 * @return 当前状态
 */
ymodem_state_t rbl_ymodem_get_state(ymodem_receiver_t *receiver);

/**
 * @brief 重置接收器
 * @param receiver YMODEM接收器指针
 */
void rbl_ymodem_reset(ymodem_receiver_t *receiver);

#ifdef __cplusplus
}
#endif

#endif /* RBL_YMODEM_H */
