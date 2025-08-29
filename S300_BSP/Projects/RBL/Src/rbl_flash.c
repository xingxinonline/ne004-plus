/**
 * @file rbl_flash.c
 * @brief RBL Flash高级操作
 */

#include "rbl_flash.h"
#include "rbl_qspi.h"
#include "rbl_crc.h"
#include "rbl_config.h"

/* Flash恢复标志地址 */
#define RECOVERY_FLAG_ADDR          0xFFF000  /* 倒数第4KB */
#define RECOVERY_MAGIC              0x52454356 /* "RECV" */

/**
 * @brief 检查恢复标志
 */
bool rbl_flash_check_recovery_flag(void)
{
    uint32_t magic;
    
    /* 读取恢复标志 */
    if (rbl_qspi_read(RECOVERY_FLAG_ADDR, (uint8_t*)&magic, sizeof(magic)) != 0) {
        return false;
    }
    
    return (magic == RECOVERY_MAGIC);
}

/**
 * @brief 设置恢复标志
 */
int rbl_flash_set_recovery_flag(void)
{
    uint32_t magic = RECOVERY_MAGIC;
    
    /* 擦除扇区 */
    int ret = rbl_qspi_erase_sector(RECOVERY_FLAG_ADDR);
    if (ret != 0) {
        return ret;
    }
    
    /* 写入恢复标志 */
    return rbl_qspi_write(RECOVERY_FLAG_ADDR, (uint8_t*)&magic, sizeof(magic));
}

/**
 * @brief 清除恢复标志
 */
int rbl_flash_clear_recovery_flag(void)
{
    /* 擦除整个扇区即可清除标志 */
    return rbl_qspi_erase_sector(RECOVERY_FLAG_ADDR);
}

/**
 * @brief 计算Flash区域的CRC32
 */
int rbl_flash_calculate_crc32(uint32_t addr, uint32_t length, uint32_t *crc)
{
    static uint8_t buffer[1024];
    uint32_t remaining = length;
    uint32_t offset = 0;
    
    if (!crc) {
        return -1;
    }
    
    /* 初始化CRC */
    rbl_crc32_init();
    
    /* 分块计算CRC */
    while (remaining > 0) {
        uint32_t chunk_size = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        
        /* 读取数据块 */
        if (rbl_qspi_read(addr + offset, buffer, chunk_size) != 0) {
            return -1;
        }
        
        /* 更新CRC */
        rbl_crc32_update(buffer, chunk_size);
        
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    /* 获取最终CRC值 */
    *crc = rbl_crc32_final();
    
    return 0;
}

/**
 * @brief 验证Flash区域完整性
 */
int rbl_flash_verify_region(uint32_t addr, uint32_t length, uint32_t expected_crc)
{
    uint32_t calculated_crc;
    
    /* 计算实际CRC */
    int ret = rbl_flash_calculate_crc32(addr, length, &calculated_crc);
    if (ret != 0) {
        return ret;
    }
    
    /* 比较CRC */
    if (calculated_crc != expected_crc) {
        printf("[RBL] CRC mismatch: expected 0x%08X, got 0x%08X\r\n", 
               expected_crc, calculated_crc);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 安全写入Flash(带验证)
 */
int rbl_flash_write_safe(uint32_t addr, const uint8_t *data, uint32_t length)
{
    static uint8_t verify_buffer[256];
    uint32_t remaining = length;
    uint32_t offset = 0;
    
    if (!data) {
        return -1;
    }
    
    while (remaining > 0) {
        uint32_t chunk_size = (remaining > sizeof(verify_buffer)) ? 
                             sizeof(verify_buffer) : remaining;
        
        /* 写入数据 */
        int ret = rbl_qspi_write(addr + offset, data + offset, chunk_size);
        if (ret != 0) {
            return ret;
        }
        
        /* 读回验证 */
        ret = rbl_qspi_read(addr + offset, verify_buffer, chunk_size);
        if (ret != 0) {
            return ret;
        }
        
        /* 比较数据 */
        if (memcmp(data + offset, verify_buffer, chunk_size) != 0) {
            printf("[RBL] Write verification failed at 0x%08X\r\n", addr + offset);
            return -1;
        }
        
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    return 0;
}

/**
 * @brief 擦除Flash区域
 */
int rbl_flash_erase_region(uint32_t addr, uint32_t length)
{
    uint32_t sector_addr = addr & ~(RBL_FLASH_SECTOR_SIZE - 1);
    uint32_t end_addr = addr + length;
    
    while (sector_addr < end_addr) {
        printf("[RBL] Erasing sector at 0x%08X\r\n", sector_addr);
        
        int ret = rbl_qspi_erase_sector(sector_addr);
        if (ret != 0) {
            printf("[RBL] Failed to erase sector at 0x%08X\r\n", sector_addr);
            return ret;
        }
        
        sector_addr += RBL_FLASH_SECTOR_SIZE;
    }
    
    return 0;
}
