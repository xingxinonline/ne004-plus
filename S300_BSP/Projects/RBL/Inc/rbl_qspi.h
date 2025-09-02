// Minimal QSPI skeleton for RBL: init and read JEDEC ID
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 QSPI 控制器（假设时钟已开启，禁用 XIP/DIRECT，配置波特率、模式、尺寸）
void rbl_qspi_init(uint32_t ref_clk_hz, uint32_t sclk_hz);

// 读取 JEDEC ID（0x9F），返回 0 表示成功
int rbl_qspi_read_jedec_id(uint8_t id[3]);

#ifdef __cplusplus
}
#endif
