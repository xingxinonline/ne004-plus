/**
 * @file rbl_ymodem.c
 * @brief Ymodem协议实现
 */

#include "rbl_ymodem.h"
#include "rbl_uart.h"
#include "rbl_system.h"
#include "rbl_flash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 静态变量 */
static uint8_t ymodem_packet_buffer[YMODEM_PACKET_SIZE_1K + YMODEM_PACKET_HEADER_SIZE + YMODEM_PACKET_CRC_SIZE];

/**
 * @brief 计算CRC16校验值
 */
uint16_t ymodem_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0;
    
    while (length--) {
        crc ^= (*data++) << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief 发送ACK确认
 */
void ymodem_send_ack(void)
{
    uint8_t ack = YMODEM_ACK;
    rbl_uart_write(&ack, 1);
    rbl_uart_flush();
}

/**
 * @brief 发送NAK否认
 */
void ymodem_send_nak(void)
{
    uint8_t nak = YMODEM_NAK;
    rbl_uart_write(&nak, 1);
    rbl_uart_flush();
}

/**
 * @brief 发送C请求CRC模式
 */
void ymodem_send_c(void)
{
    uint8_t c = YMODEM_C;
    rbl_uart_write(&c, 1);
    rbl_uart_flush();
}

/**
 * @brief 发送CAN取消传输
 */
void ymodem_send_can(void)
{
    uint8_t can = YMODEM_CAN;
    rbl_uart_write(&can, 1);
    rbl_uart_write(&can, 1);  /* 发送两次CAN */
    rbl_uart_flush();
}

/**
 * @brief 接收Ymodem数据包
 */
ymodem_result_t ymodem_receive_packet(uint8_t *packet, uint32_t *packet_size, uint8_t expected_seq)
{
    uint32_t received = 0;
    uint32_t start_time = rbl_system_get_tick_ms();
    uint8_t header[3];
    uint32_t data_size;
    uint16_t crc_received, crc_calculated;
    
    /* 接收包头 (SOH/STX + 包号 + 包号补码) */
    while (received < 3) {
        if ((rbl_system_get_tick_ms() - start_time) > YMODEM_TIMEOUT_MS) {
            return YMODEM_TIMEOUT;
        }
        
        int bytes = rbl_uart_read_nonblock(&header[received], 3 - received);
        if (bytes > 0) {
            received += bytes;
        }
        rbl_system_delay_ms(1);
    }
    
    /* 检查包类型 */
    if (header[0] == YMODEM_EOT) {
        *packet_size = 0;
        return YMODEM_OK;  /* 传输结束 */
    } else if (header[0] == YMODEM_CAN) {
        return YMODEM_CANCEL;  /* 传输取消 */
    } else if (header[0] == YMODEM_SOH) {
        data_size = YMODEM_PACKET_SIZE_128;
    } else if (header[0] == YMODEM_STX) {
        data_size = YMODEM_PACKET_SIZE_1K;
    } else {
        return YMODEM_PACKET_ERROR;
    }
    
    /* 检查包序号 */
    uint8_t expected_seq_complement = (uint8_t)(~expected_seq);
    if (header[1] != expected_seq || header[2] != expected_seq_complement) {
        return YMODEM_PACKET_ERROR;
    }
    
    /* 接收数据部分 */
    received = 0;
    start_time = rbl_system_get_tick_ms();
    while (received < data_size + 2) {  /* 数据 + CRC16 */
        if ((rbl_system_get_tick_ms() - start_time) > YMODEM_TIMEOUT_MS) {
            return YMODEM_TIMEOUT;
        }
        
        int bytes = rbl_uart_read_nonblock(&packet[received], data_size + 2 - received);
        if (bytes > 0) {
            received += bytes;
        }
        rbl_system_delay_ms(1);
    }
    
    /* 验证CRC */
    crc_received = (packet[data_size] << 8) | packet[data_size + 1];
    crc_calculated = ymodem_crc16(packet, data_size);
    
    if (crc_received != crc_calculated) {
        return YMODEM_CRC_ERROR;
    }
    
    *packet_size = data_size;
    return YMODEM_OK;
}

/**
 * @brief 解析文件信息包(包0)
 */
static ymodem_result_t ymodem_parse_file_info(const uint8_t *packet, ymodem_file_info_t *file_info)
{
    const char *ptr = (const char *)packet;
    
    /* 如果第一个字节是0，表示传输结束 */
    if (packet[0] == 0) {
        return YMODEM_OK;
    }
    
    /* 解析文件名 */
    strncpy(file_info->filename, ptr, sizeof(file_info->filename) - 1);
    file_info->filename[sizeof(file_info->filename) - 1] = '\0';
    
    /* 跳过文件名，查找文件大小 */
    ptr += strlen(file_info->filename) + 1;
    if (*ptr) {
        file_info->filesize = strtoul(ptr, NULL, 10);
    } else {
        file_info->filesize = 0;  /* 未知大小 */
    }
    
    file_info->received_bytes = 0;
    
    printf("[YMODEM] File: %s, Size: %lu bytes\r\n", 
           file_info->filename, (unsigned long)file_info->filesize);
    
    return YMODEM_OK;
}

/**
 * @brief Ymodem接收文件
 */
ymodem_result_t ymodem_receive_file(ymodem_file_info_t *file_info,
                                   ymodem_write_callback_t write_cb,
                                   ymodem_progress_callback_t progress_cb)
{
    ymodem_result_t result;
    uint32_t packet_size;
    uint8_t packet_seq = 0;
    int retry_count = 0;
    bool file_started = false;
    uint32_t write_addr = file_info->flash_address;
    
    printf("[YMODEM] Starting file reception...\r\n");
    
    while (retry_count < YMODEM_MAX_RETRIES) {
        /* 发送C请求CRC模式传输 */
        ymodem_send_c();
        
        /* 接收数据包 */
        result = ymodem_receive_packet(ymodem_packet_buffer, &packet_size, packet_seq);
        
        switch (result) {
            case YMODEM_OK:
                if (packet_size == 0) {
                    /* 收到EOT，传输结束 */
                    ymodem_send_ack();
                    printf("[YMODEM] File reception completed\r\n");
                    return YMODEM_OK;
                }
                
                if (packet_seq == 0) {
                    /* 文件信息包 */
                    result = ymodem_parse_file_info(ymodem_packet_buffer, file_info);
                    if (result != YMODEM_OK) {
                        ymodem_send_can();
                        return result;
                    }
                    
                    if (file_info->filename[0] == '\0') {
                        /* 空文件名表示传输结束 */
                        ymodem_send_ack();
                        return YMODEM_OK;
                    }
                    
                    file_started = true;
                    ymodem_send_ack();
                    packet_seq++;
                    retry_count = 0;
                } else {
                    /* 数据包 */
                    if (!file_started) {
                        ymodem_send_can();
                        return YMODEM_ERROR;
                    }
                    
                    /* 写入Flash */
                    uint32_t write_size = packet_size;
                    if (file_info->filesize > 0) {
                        /* 如果已知文件大小，限制写入长度 */
                        uint32_t remaining = file_info->filesize - file_info->received_bytes;
                        if (write_size > remaining) {
                            write_size = remaining;
                        }
                    }
                    
                    if (write_size > 0 && write_cb) {
                        result = write_cb(write_addr, ymodem_packet_buffer, write_size);
                        if (result != YMODEM_OK) {
                            ymodem_send_can();
                            return YMODEM_FLASH_ERROR;
                        }
                        
                        write_addr += write_size;
                        file_info->received_bytes += write_size;
                        
                        /* 进度回调 */
                        if (progress_cb) {
                            progress_cb(file_info->received_bytes, file_info->filesize);
                        }
                    }
                    
                    ymodem_send_ack();
                    packet_seq++;
                    retry_count = 0;
                }
                break;
                
            case YMODEM_CANCEL:
                printf("[YMODEM] Transfer cancelled by sender\r\n");
                return YMODEM_CANCEL;
                
            case YMODEM_TIMEOUT:
            case YMODEM_CRC_ERROR:
            case YMODEM_PACKET_ERROR:
                printf("[YMODEM] Packet error, retrying... (%d/%d)\r\n", 
                       retry_count + 1, YMODEM_MAX_RETRIES);
                ymodem_send_nak();
                retry_count++;
                rbl_system_delay_ms(100);
                break;
                
            default:
                ymodem_send_can();
                return result;
        }
    }
    
    printf("[YMODEM] Too many retries, giving up\r\n");
    ymodem_send_can();
    return YMODEM_ERROR;
}

/**
 * @brief Ymodem接收(指定Flash地址)
 */
ymodem_result_t ymodem_receive(uint32_t flash_addr, 
                              ymodem_write_callback_t write_cb,
                              ymodem_progress_callback_t progress_cb)
{
    ymodem_file_info_t file_info;
    
    memset(&file_info, 0, sizeof(file_info));
    file_info.flash_address = flash_addr;
    
    return ymodem_receive_file(&file_info, write_cb, progress_cb);
}
