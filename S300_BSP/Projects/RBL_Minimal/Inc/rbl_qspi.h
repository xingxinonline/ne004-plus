// Minimal QSPI for RBL: init, ID read, and basic read/write/erase
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

// === Phase 2 扩展: 基本读写擦除功能 ===

// 等待Flash忙状态结束，返回0表示成功
int rbl_qspi_wait_ready(uint32_t timeout_ms);

// 读取数据（通用读取命令03h），返回0表示成功
int rbl_qspi_read(uint32_t addr, uint8_t *data, uint32_t len);

// 页编程（最大256字节），返回0表示成功
int rbl_qspi_page_program(uint32_t addr, const uint8_t *data, uint32_t len);

// 4KB扇区擦除，返回0表示成功  
int rbl_qspi_erase_4k(uint32_t addr);

// === 便捷的一页读写校验功能 ===

// 简单的一页读写校验测试（用于Phase 2验收）
int rbl_qspi_test_page_rw(uint32_t test_addr);

#ifdef __cplusplus
}
#endif
