/**
 * @file partition_layout.h
 * @brief S300 Flash分区布局定义
 * @version 2.0
 * @date 2024
 * 
 * @note 更新版本：将软件复位标志区移至NVS尾部4KB，NVS扩展为60KB
 */

#ifndef __PARTITION_LAYOUT_H__
#define __PARTITION_LAYOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ============================= Flash总体布局 ============================= */

#define FLASH_TOTAL_SIZE          0x01000000  ///< Flash总大小 16MB (W25Q128)

/* ============================= 各分区定义 ============================= */

/**
 * @brief RBL Header分区 (256B)
 */
#define RBL_HEADER_ADDR           0x00000000
#define RBL_HEADER_SIZE           0x00000100

/**
 * @brief RBL分区 (64KB)
 */
#define RBL_BASE_ADDR             0x00000100
#define RBL_SIZE                  0x0000FF00
#define RBL_END_ADDR              0x0000FFFF

/**
 * @brief SBL分区 (128KB)
 */
#define SBL_BASE_ADDR             0x00010000
#define SBL_SIZE                  0x00020000
#define SBL_END_ADDR              0x0002FFFF

/**
 * @brief NVS分区 (60KB总计 = 56KB数据区 + 4KB软件复位标志区)
 */
#define NVS_BASE_ADDR             0x00030000
#define NVS_TOTAL_SIZE            0x00010000  ///< NVS总大小 60KB
#define NVS_DATA_SIZE             0x0000F000  ///< NVS数据区 56KB
#define NVS_DATA_END_ADDR         0x0003EFFF

// 软件复位标志区 (NVS尾部4KB)
#define RESET_FLAG_SECTOR_ADDR    0x0003F000
#define RESET_FLAG_SECTOR_SIZE    0x00001000  ///< 4KB
#define NVS_END_ADDR              0x0003FFFF

/**
 * @brief OTA_0应用分区 (6MB)
 */
#define OTA_0_BASE_ADDR           0x00040000
#define OTA_0_SIZE                0x00600000  ///< 6MB
#define OTA_0_END_ADDR            0x0063FFFF

/**
 * @brief OTA_1应用分区 (6MB)
 */
#define OTA_1_BASE_ADDR           0x00640000
#define OTA_1_SIZE                0x00600000  ///< 6MB
#define OTA_1_END_ADDR            0x00C3FFFF

/**
 * @brief 用户数据分区 (3.75MB)
 */
#define USER_DATA_BASE_ADDR       0x00C40000
#define USER_DATA_SIZE            0x003C0000  ///< 3.75MB
#define USER_DATA_END_ADDR        0x00FFFFFF

/* ========================= 软件复位标志区详细布局 ========================= */

/**
 * @brief 软件复位标志区内部布局 (4KB)
 * 地址范围: 0x0003F000 - 0x0003FFFF
 */

// 下载模式标志 (24字节)
#define DOWNLOAD_FLAG_OFFSET      0x000
#define DOWNLOAD_FLAG_SIZE        0x018

// 双重启标志 (16字节) 
#define DOUBLE_RESET_FLAG_OFFSET  0x100
#define DOUBLE_RESET_FLAG_SIZE    0x010

// 启动计数器 (24字节)
#define BOOT_COUNT_OFFSET         0x200
#define BOOT_COUNT_SIZE           0x018

// 预留扩展区 (约2.5KB)
#define RESET_FLAG_RESERVED_OFFSET 0x300
#define RESET_FLAG_RESERVED_SIZE   0xD00

/* ============================= 分区验证宏 ============================= */

/**
 * @brief 检查地址是否在指定分区内
 */
#define IS_IN_RBL_PARTITION(addr) \
    ((addr) >= RBL_BASE_ADDR && (addr) <= RBL_END_ADDR)

#define IS_IN_SBL_PARTITION(addr) \
    ((addr) >= SBL_BASE_ADDR && (addr) <= SBL_END_ADDR)

#define IS_IN_NVS_DATA_PARTITION(addr) \
    ((addr) >= NVS_BASE_ADDR && (addr) <= NVS_DATA_END_ADDR)

#define IS_IN_RESET_FLAG_PARTITION(addr) \
    ((addr) >= RESET_FLAG_SECTOR_ADDR && (addr) <= NVS_END_ADDR)

#define IS_IN_OTA_0_PARTITION(addr) \
    ((addr) >= OTA_0_BASE_ADDR && (addr) <= OTA_0_END_ADDR)

#define IS_IN_OTA_1_PARTITION(addr) \
    ((addr) >= OTA_1_BASE_ADDR && (addr) <= OTA_1_END_ADDR)

#define IS_IN_USER_DATA_PARTITION(addr) \
    ((addr) >= USER_DATA_BASE_ADDR && (addr) <= USER_DATA_END_ADDR)

/* ============================= 便捷地址计算 ============================= */

/**
 * @brief 计算软件复位标志区内的绝对地址
 */
#define RESET_FLAG_ABS_ADDR(offset) \
    (RESET_FLAG_SECTOR_ADDR + (offset))

// 各标志的绝对地址
#define DOWNLOAD_FLAG_ABS_ADDR    RESET_FLAG_ABS_ADDR(DOWNLOAD_FLAG_OFFSET)
#define DOUBLE_RESET_FLAG_ABS_ADDR RESET_FLAG_ABS_ADDR(DOUBLE_RESET_FLAG_OFFSET)
#define BOOT_COUNT_ABS_ADDR       RESET_FLAG_ABS_ADDR(BOOT_COUNT_OFFSET)

/* ============================= 分区信息结构 ============================= */

/**
 * @brief 分区信息结构
 */
typedef struct {
    const char *name;       ///< 分区名称
    uint32_t base_addr;     ///< 起始地址
    uint32_t size;          ///< 分区大小
    uint32_t end_addr;      ///< 结束地址
    const char *description; ///< 描述
} partition_info_t;

/**
 * @brief 所有分区信息表
 */
extern const partition_info_t g_partition_table[];
extern const int g_partition_count;

/* ============================= 函数声明 ============================= */

/**
 * @brief 打印分区布局信息
 */
void partition_print_layout(void);

/**
 * @brief 验证分区布局的合理性
 * @return 0=正常, <0=有重叠或错误
 */
int partition_validate_layout(void);

/**
 * @brief 根据地址查找分区信息
 * @param addr Flash地址
 * @return 分区信息指针，未找到返回NULL
 */
const partition_info_t* partition_find_by_addr(uint32_t addr);

/**
 * @brief 根据名称查找分区信息
 * @param name 分区名称
 * @return 分区信息指针，未找到返回NULL
 */
const partition_info_t* partition_find_by_name(const char *name);

/**
 * @brief 地址转换为可读字符串
 * @param addr Flash地址
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 缓冲区指针
 */
const char* partition_addr_to_string(uint32_t addr, char *buffer, size_t size);

/**
 * @brief 获取分区使用率统计
 * @param used_size 输出已使用大小
 * @param total_size 输出总大小
 */
void partition_get_usage_stats(uint32_t *used_size, uint32_t *total_size);

#ifdef __cplusplus
}
#endif

#endif /* __PARTITION_LAYOUT_H__ */
