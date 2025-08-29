/**
 * @file rbl_ymodem.h
 * @brief Ymodem协议头文件
 */

#ifndef RBL_YMODEM_H
#define RBL_YMODEM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ymodem协议常量 */
#define YMODEM_SOH          0x01    /* 128字节数据包开始 */
#define YMODEM_STX          0x02    /* 1024字节数据包开始 */
#define YMODEM_EOT          0x04    /* 传输结束 */
#define YMODEM_ACK          0x06    /* 确认 */
#define YMODEM_NAK          0x15    /* 否认 */
#define YMODEM_CAN          0x18    /* 取消 */
#define YMODEM_C            0x43    /* 请求CRC模式 */

#define YMODEM_PACKET_SIZE_128    128
#define YMODEM_PACKET_SIZE_1K     1024
#define YMODEM_PACKET_HEADER_SIZE 3     /* SOH/STX + 包号 + 包号补码 */
#define YMODEM_PACKET_CRC_SIZE    2     /* CRC16 */

#define YMODEM_MAX_RETRIES        10
#define YMODEM_TIMEOUT_MS         3000

/* Ymodem传输状态 */
typedef enum {
    YMODEM_OK = 0,
    YMODEM_ERROR,
    YMODEM_TIMEOUT,
    YMODEM_CANCEL,
    YMODEM_CRC_ERROR,
    YMODEM_PACKET_ERROR,
    YMODEM_FLASH_ERROR
} ymodem_result_t;

/* 文件信息结构 */
typedef struct {
    char filename[64];
    uint32_t filesize;
    uint32_t received_bytes;
    uint32_t flash_address;
} ymodem_file_info_t;

/* Ymodem回调函数类型 */
typedef ymodem_result_t (*ymodem_write_callback_t)(uint32_t addr, const uint8_t *data, uint32_t size);
typedef void (*ymodem_progress_callback_t)(uint32_t received, uint32_t total);

/* 函数声明 */
ymodem_result_t ymodem_receive(uint32_t flash_addr, 
                              ymodem_write_callback_t write_cb,
                              ymodem_progress_callback_t progress_cb);

ymodem_result_t ymodem_receive_file(ymodem_file_info_t *file_info,
                                   ymodem_write_callback_t write_cb,
                                   ymodem_progress_callback_t progress_cb);

/* 内部函数 */
uint16_t ymodem_crc16(const uint8_t *data, uint32_t length);
ymodem_result_t ymodem_receive_packet(uint8_t *packet, uint32_t *packet_size, uint8_t expected_seq);
void ymodem_send_ack(void);
void ymodem_send_nak(void);
void ymodem_send_c(void);
void ymodem_send_can(void);

#ifdef __cplusplus
}
#endif

#endif /* RBL_YMODEM_H */
