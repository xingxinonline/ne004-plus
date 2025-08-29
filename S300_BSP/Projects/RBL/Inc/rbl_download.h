/**
 * @file rbl_download.h
 * @brief RBL下载功能头文件
 */

#ifndef RBL_DOWNLOAD_H
#define RBL_DOWNLOAD_H

#include <stdint.h>
#include <stdbool.h>

/* 前向声明 */
typedef struct download_packet download_packet_t;

/* 函数声明 */
int rbl_download_mode(void);
void rbl_send_ready_signal(void);
int rbl_process_download_packet(void);

int rbl_receive_packet(download_packet_t *packet, int max_size);
int rbl_send_response(uint8_t status, uint8_t seq, const uint8_t *data, uint16_t length);
void rbl_send_error_response(uint8_t seq, const char *error_msg);

/* 命令处理函数 */
int rbl_handle_get_info(download_packet_t *packet);
int rbl_handle_erase_sector(download_packet_t *packet);
int rbl_handle_write_data(download_packet_t *packet);
int rbl_handle_read_data(download_packet_t *packet);
int rbl_handle_finish(download_packet_t *packet);

#endif // RBL_DOWNLOAD_H
