/**
 * @file rbl_ymodem.c
 * @brief RBL YMODEM简版协议实现
 */

#include "rbl_ymodem.h"
#include "rbl_hal.h"
#include "rbl_qspi.h"

/* 内部函数声明 */
static bool rbl_ymodem_process_packet(ymodem_receiver_t *receiver);

/* 静态变量 */
static ymodem_data_callback_t s_data_callback = NULL;
static ymodem_progress_callback_t s_progress_callback = NULL;
static ymodem_packet_t s_current_packet;
static uint8_t *s_packet_buffer = NULL;
static uint32_t s_packet_index = 0;

/* CRC16查找表（多项式: 0x1021） */
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

/**
 * @brief 初始化YMODEM接收器
 */
void rbl_ymodem_init(ymodem_receiver_t *receiver, uint32_t flash_start_addr)
{
    if (receiver == NULL) return;
    
    receiver->state = YMODEM_STATE_IDLE;
    receiver->expected_seq = 0;
    receiver->total_bytes = 0;
    receiver->packet_count = 0;
    receiver->retry_count = 0;
    receiver->flash_address = flash_start_addr;
    receiver->first_packet = true;
    
    s_packet_index = 0;
    s_packet_buffer = (uint8_t*)&s_current_packet;
    
    RBL_LOG("YMODEM: 初始化完成，Flash地址=0x%08X\r\n", flash_start_addr);
}

/**
 * @brief 启动YMODEM接收
 */
bool rbl_ymodem_start_receive(ymodem_receiver_t *receiver)
{
    if (receiver == NULL) return false;
    
    receiver->state = YMODEM_STATE_WAITING_HEADER;
    receiver->retry_count = 0;
    
    RBL_LOG("YMODEM: 启动接收，等待传输...\r\n");
    
    /* 发送'C'请求CRC模式 */
    rbl_ymodem_send_c();
    
    return true;
}

/**
 * @brief 处理接收到的字节
 */
bool rbl_ymodem_process_byte(ymodem_receiver_t *receiver, uint8_t byte)
{
    if (receiver == NULL) return false;
    
    switch (receiver->state) {
        case YMODEM_STATE_WAITING_HEADER:
            if (byte == YMODEM_SOH || byte == YMODEM_STX) {
                /* 开始新包 */
                s_current_packet.header = byte;
                s_current_packet.data_size = (byte == YMODEM_SOH) ? 
                    YMODEM_PACKET_SIZE_128 : YMODEM_PACKET_SIZE_1024;
                s_packet_index = 1;
                receiver->state = YMODEM_STATE_RECEIVING_DATA;
                RBL_LOG("YMODEM: 开始接收数据包，大小=%d\r\n", s_current_packet.data_size);
            } else if (byte == YMODEM_EOT) {
                /* 传输结束 */
                RBL_LOG("YMODEM: 传输完成\r\n");
                rbl_ymodem_send_ack();
                receiver->state = YMODEM_STATE_COMPLETED;
                return false;
            } else if (byte == YMODEM_CAN) {
                /* 取消传输 */
                RBL_LOG("YMODEM: 传输被取消\r\n");
                receiver->state = YMODEM_STATE_CANCELLED;
                return false;
            }
            break;
            
        case YMODEM_STATE_RECEIVING_DATA:
            /* 接收数据到缓冲区 */
            s_packet_buffer[s_packet_index++] = byte;
            
            /* 检查是否接收完整个包 */
            uint32_t total_size = YMODEM_HEADER_SIZE + s_current_packet.data_size + YMODEM_CRC_SIZE;
            if (s_packet_index >= total_size) {
                /* 包接收完成，进行处理 */
                if (rbl_ymodem_process_packet(receiver)) {
                    receiver->state = YMODEM_STATE_WAITING_HEADER;
                } else {
                    receiver->state = YMODEM_STATE_ERROR;
                    return false;
                }
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

/**
 * @brief 处理完整的数据包
 */
static bool rbl_ymodem_process_packet(ymodem_receiver_t *receiver)
{
    /* 提取序号和反序号 */
    s_current_packet.sequence = s_packet_buffer[1];
    s_current_packet.sequence_inv = s_packet_buffer[2];
    
    /* 检查序号 */
    if ((s_current_packet.sequence + s_current_packet.sequence_inv) != 0xFF) {
        RBL_LOG("YMODEM: 序号校验失败\r\n");
        rbl_ymodem_send_nak();
        return false;
    }
    
    /* 复制数据 */
    uint8_t *data_start = &s_packet_buffer[3];
    for (uint32_t i = 0; i < s_current_packet.data_size; i++) {
        s_current_packet.data[i] = data_start[i];
    }
    
    /* 提取CRC */
    uint8_t *crc_start = &s_packet_buffer[3 + s_current_packet.data_size];
    s_current_packet.crc = (crc_start[0] << 8) | crc_start[1];
    
    /* 校验CRC */
    uint16_t calc_crc = rbl_ymodem_crc16(s_current_packet.data, s_current_packet.data_size);
    if (calc_crc != s_current_packet.crc) {
        RBL_LOG("YMODEM: CRC校验失败，期望=0x%04X，实际=0x%04X\r\n", 
                s_current_packet.crc, calc_crc);
        rbl_ymodem_send_nak();
        return false;
    }
    
    /* 检查序号是否连续 */
    if (s_current_packet.sequence != receiver->expected_seq) {
        if (s_current_packet.sequence == (receiver->expected_seq - 1)) {
            /* 重复包，发送ACK但不处理数据 */
            RBL_LOG("YMODEM: 重复包，序号=%d\r\n", s_current_packet.sequence);
            rbl_ymodem_send_ack();
            return true;
        } else {
            RBL_LOG("YMODEM: 序号错误，期望=%d，接收=%d\r\n", 
                    receiver->expected_seq, s_current_packet.sequence);
            rbl_ymodem_send_nak();
            return false;
        }
    }
    
    /* 处理数据 */
    if (receiver->first_packet) {
        /* 第一个包是文件名和大小信息 */
        receiver->first_packet = false;
        RBL_LOG("YMODEM: 接收到文件信息包\r\n");
    } else {
        /* 数据包，写入Flash */
        if (s_data_callback) {
            s_data_callback(receiver->flash_address, s_current_packet.data, s_current_packet.data_size);
        }
        receiver->flash_address += s_current_packet.data_size;
        receiver->total_bytes += s_current_packet.data_size;
        
        if (s_progress_callback) {
            s_progress_callback(receiver->total_bytes, 0);
        }
        
        RBL_LOG("YMODEM: 数据写入Flash，地址=0x%08X，大小=%d\r\n", 
                receiver->flash_address - s_current_packet.data_size, s_current_packet.data_size);
    }
    
    /* 更新序号 */
    receiver->expected_seq++;
    receiver->packet_count++;
    
    /* 发送ACK */
    rbl_ymodem_send_ack();
    
    return true;
}

/**
 * @brief 发送ACK
 */
void rbl_ymodem_send_ack(void)
{
    uint8_t ack = YMODEM_ACK;
    rbl_hal_uart_send(&ack, 1);
}

/**
 * @brief 发送NAK
 */
void rbl_ymodem_send_nak(void)
{
    uint8_t nak = YMODEM_NAK;
    rbl_hal_uart_send(&nak, 1);
}

/**
 * @brief 发送CAN（取消）
 */
void rbl_ymodem_send_can(void)
{
    uint8_t can = YMODEM_CAN;
    rbl_hal_uart_send(&can, 1);
    rbl_hal_uart_send(&can, 1);  /* 发送两次 */
}

/**
 * @brief 发送'C'（请求CRC模式）
 */
void rbl_ymodem_send_c(void)
{
    uint8_t c = YMODEM_CRC16;
    rbl_hal_uart_send(&c, 1);
}

/**
 * @brief 计算CRC16
 */
uint16_t rbl_ymodem_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0x0000;
    
    for (uint32_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    return crc;
}

/**
 * @brief 设置数据写入回调
 */
void rbl_ymodem_set_data_callback(ymodem_data_callback_t callback)
{
    s_data_callback = callback;
}

/**
 * @brief 设置进度回调
 */
void rbl_ymodem_set_progress_callback(ymodem_progress_callback_t callback)
{
    s_progress_callback = callback;
}

/**
 * @brief 获取当前状态
 */
ymodem_state_t rbl_ymodem_get_state(ymodem_receiver_t *receiver)
{
    return receiver ? receiver->state : YMODEM_STATE_ERROR;
}

/**
 * @brief 重置接收器
 */
void rbl_ymodem_reset(ymodem_receiver_t *receiver)
{
    if (receiver) {
        receiver->state = YMODEM_STATE_IDLE;
        receiver->expected_seq = 0;
        receiver->total_bytes = 0;
        receiver->packet_count = 0;
        receiver->retry_count = 0;
        receiver->first_packet = true;
        s_packet_index = 0;
    }
}
