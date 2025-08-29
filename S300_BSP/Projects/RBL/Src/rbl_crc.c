/**
 * @file rbl_crc.c
 * @brief CRC32计算模块
 */

#include "rbl_crc.h"

/* CRC32多项式 (IEEE 802.3) */
#define CRC32_POLY              0xEDB88320UL
#define CRC32_INIT              0xFFFFFFFFUL

/* CRC32查找表 */
static uint32_t crc32_table[256];
static bool table_initialized = false;
static uint32_t current_crc;

/**
 * @brief 初始化CRC32查找表
 */
static void crc32_init_table(void)
{
    uint32_t crc;
    int i, j;
    
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 8; j > 0; j--) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    
    table_initialized = true;
}

/**
 * @brief 初始化CRC32计算
 */
void rbl_crc32_init(void)
{
    if (!table_initialized) {
        crc32_init_table();
    }
    
    current_crc = CRC32_INIT;
}

/**
 * @brief 更新CRC32值
 */
void rbl_crc32_update(const uint8_t *data, uint32_t length)
{
    uint32_t i;
    
    if (!data) {
        return;
    }
    
    for (i = 0; i < length; i++) {
        current_crc = crc32_table[(current_crc ^ data[i]) & 0xFF] ^ (current_crc >> 8);
    }
}

/**
 * @brief 获取最终CRC32值
 */
uint32_t rbl_crc32_final(void)
{
    return current_crc ^ 0xFFFFFFFFUL;
}

/**
 * @brief 计算数据的CRC32(一次性计算)
 */
uint32_t rbl_crc32_calculate(const uint8_t *data, uint32_t length)
{
    rbl_crc32_init();
    rbl_crc32_update(data, length);
    return rbl_crc32_final();
}

/**
 * @brief 验证CRC32
 */
bool rbl_crc32_verify(const uint8_t *data, uint32_t length, uint32_t expected_crc)
{
    uint32_t calculated_crc = rbl_crc32_calculate(data, length);
    return (calculated_crc == expected_crc);
}
