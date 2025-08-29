/**
 * @file rbl_crc.h
 * @brief CRC32计算模块头文件
 */

#ifndef __RBL_CRC_H__
#define __RBL_CRC_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CRC32计算函数 */
void rbl_crc32_init(void);
void rbl_crc32_update(const uint8_t *data, uint32_t length);
uint32_t rbl_crc32_final(void);

/* 便捷函数 */
uint32_t rbl_crc32_calculate(const uint8_t *data, uint32_t length);
bool rbl_crc32_verify(const uint8_t *data, uint32_t length, uint32_t expected_crc);

#ifdef __cplusplus
}
#endif

#endif /* __RBL_CRC_H__ */
