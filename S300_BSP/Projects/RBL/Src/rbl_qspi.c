/**
 * @file rbl_qspi.c
 * @brief RBL QSPI Flash驱动(基于S300 Cadence QSPI BSP)
 */

#include "rbl_qspi.h"
#include "rbl_config.h"
#include "qspi_cadence.h"
#include "rcc.h"
#include <string.h>

/* QSPI配置 */
static volatile bool qspi_initialized = false;

/**
 * @brief 初始化QSPI控制器
 */
int rbl_qspi_init(void)
{
    if (qspi_initialized) {
        return 0;
    }
    
    /* 使能QSPI时钟 */
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
    
    /* 初始化QSPI控制器 */
    qspi_cadence_init(RBL_SYSTEM_CLOCK_HZ, 50000000); /* 50MHz SCLK */
    
    /* 关闭调试打印以提高性能 */
    qspi_set_verbose(false);
    
    /* 软件复位Flash */
    qspi_software_reset();
    
    /* 读取Flash ID验证连接 */
    uint8_t flash_id[3];
    if (qspi_read_id(flash_id, 3) != 0) {
        return -1;
    }
    
    /* 验证W25Q128 ID (0xEF4018) */
    if (flash_id[0] != 0xEF || flash_id[1] != 0x40 || flash_id[2] != 0x18) {
        return -2;  /* Flash ID不匹配 */
    }
    
    qspi_initialized = true;
    return 0;
}

/**
 * @brief 反初始化QSPI
 */
void rbl_qspi_deinit(void)
{
    if (!qspi_initialized) {
        return;
    }
    
    /* 禁用QSPI时钟 */
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, false);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, false);
    
    qspi_initialized = false;
}

/**
 * @brief 读取Flash ID
 */
int rbl_qspi_read_id(uint32_t *id)
{
    if (!id || !qspi_initialized) {
        return -1;
    }
    
    uint8_t flash_id[3];
    if (qspi_read_id(flash_id, 3) != 0) {
        return -1;
    }
    
    *id = (flash_id[0] << 16) | (flash_id[1] << 8) | flash_id[2];
    return 0;
}

/**
 * @brief 读取Flash数据
 */
int rbl_qspi_read(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    if (!buffer || !qspi_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的读取函数 */
    return qspi_read(addr, buffer, length);
}

/**
 * @brief 写入Flash数据(页编程)
 */
int rbl_qspi_write(uint32_t addr, const uint8_t *data, uint32_t length)
{
    if (!data || !qspi_initialized) {
        return -1;
    }
    
    uint32_t remaining = length;
    uint32_t offset = 0;
    
    while (remaining > 0) {
        /* 计算当前页内可写字节数 */
        uint32_t page_offset = (addr + offset) % RBL_FLASH_PAGE_SIZE;
        uint32_t page_remaining = RBL_FLASH_PAGE_SIZE - page_offset;
        uint32_t write_size = (remaining < page_remaining) ? remaining : page_remaining;
        
        /* 页编程 */
        if (qspi_page_program(addr + offset, data + offset, write_size) != 0) {
            return -1;
        }
        
        /* 等待操作完成 */
        if (qspi_wait_ready(1000) != 0) {
            return -1;
        }
        
        offset += write_size;
        remaining -= write_size;
    }
    
    return 0;
}

/**
 * @brief 擦除Flash扇区(4KB)
 */
int rbl_qspi_erase_sector(uint32_t addr)
{
    if (!qspi_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的4KB扇区擦除 */
    if (qspi_erase_4k(addr) != 0) {
        return -1;
    }
    
    /* 等待擦除完成 */
    return qspi_wait_ready(5000);  /* 扇区擦除最大5秒 */
}

/**
 * @brief 擦除Flash块(64KB)
 */
int rbl_qspi_erase_block(uint32_t addr)
{
    if (!qspi_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的64KB块擦除 */
    if (qspi_erase_64k(addr) != 0) {
        return -1;
    }
    
    /* 等待擦除完成 */
    return qspi_wait_ready(10000);  /* 块擦除最大10秒 */
}

/**
 * @brief 擦除整个Flash芯片
 */
int rbl_qspi_erase_chip(void)
{
    if (!qspi_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的芯片擦除 */
    if (qspi_chip_erase() != 0) {
        return -1;
    }
    
    /* 等待擦除完成 */
    return qspi_wait_ready(120000);  /* 芯片擦除最大120秒 */
}

/**
 * @brief 获取Flash信息
 */
int rbl_qspi_get_info(rbl_flash_info_t *info)
{
    if (!info || !qspi_initialized) {
        return -1;
    }
    
    /* 读取Flash ID */
    uint32_t flash_id;
    if (rbl_qspi_read_id(&flash_id) != 0) {
        return -1;
    }
    
    info->id = flash_id;
    info->size = RBL_FLASH_SIZE;
    info->sector_size = RBL_FLASH_SECTOR_SIZE;
    info->page_size = RBL_FLASH_PAGE_SIZE;
    
    return 0;
}

/**
 * @brief 检查Flash是否忙碌
 */
bool rbl_qspi_is_busy(void)
{
    if (!qspi_initialized) {
        return true;
    }
    
    /* 读取状态寄存器 */
    uint8_t sr1, sr2, sr3;
    if (qspi_read_status(&sr1, &sr2, &sr3) != 0) {
        return true;
    }
    
    return (sr1 & 0x01) != 0;  /* BUSY位 */
}

/**
 * @brief 等待Flash操作完成
 */
int rbl_qspi_wait_ready(uint32_t timeout_ms)
{
    if (!qspi_initialized) {
        return -1;
    }
    
    return qspi_wait_ready(timeout_ms);
}

/**
 * @brief 设置Quad模式
 */
int rbl_qspi_set_quad_mode(bool enable)
{
    if (!qspi_initialized) {
        return -1;
    }
    
    /* 设置Quad使能位 */
    if (qspi_set_quad_enable(enable) != 0) {
        return -1;
    }
    
    /* 配置控制器的Quad读取模式 */
    if (enable) {
        qspi_configure_quad_read(true);
    } else {
        qspi_configure_quad_read(false);
    }
    
    return 0;
}
