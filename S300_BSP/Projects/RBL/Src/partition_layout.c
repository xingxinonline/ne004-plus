/**
 * @file partition_layout.c
 * @brief S300 Flash分区布局实现
 * @version 2.0
 * @date 2024
 */

#include "partition_layout.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ============================= 分区信息表 ============================= */

/**
 * @brief 所有分区信息表
 */
const partition_info_t g_partition_table[] = {
    {
        .name = "RBL_Header",
        .base_addr = RBL_HEADER_ADDR,
        .size = RBL_HEADER_SIZE,
        .end_addr = RBL_HEADER_ADDR + RBL_HEADER_SIZE - 1,
        .description = "RBL元信息头"
    },
    {
        .name = "RBL",
        .base_addr = RBL_BASE_ADDR,
        .size = RBL_SIZE,
        .end_addr = RBL_END_ADDR,
        .description = "ROM Bootloader (64KB)"
    },
    {
        .name = "SBL",
        .base_addr = SBL_BASE_ADDR,
        .size = SBL_SIZE,
        .end_addr = SBL_END_ADDR,
        .description = "Secondary Bootloader (128KB)"
    },
    {
        .name = "NVS_Data",
        .base_addr = NVS_BASE_ADDR,
        .size = NVS_DATA_SIZE,
        .end_addr = NVS_DATA_END_ADDR,
        .description = "非易失性存储数据区 (56KB)"
    },
    {
        .name = "Reset_Flags",
        .base_addr = RESET_FLAG_SECTOR_ADDR,
        .size = RESET_FLAG_SECTOR_SIZE,
        .end_addr = NVS_END_ADDR,
        .description = "软件复位标志区 (4KB)"
    },
    {
        .name = "OTA_0",
        .base_addr = OTA_0_BASE_ADDR,
        .size = OTA_0_SIZE,
        .end_addr = OTA_0_END_ADDR,
        .description = "应用分区A (6MB)"
    },
    {
        .name = "OTA_1",
        .base_addr = OTA_1_BASE_ADDR,
        .size = OTA_1_SIZE,
        .end_addr = OTA_1_END_ADDR,
        .description = "应用分区B (6MB)"
    },
    {
        .name = "User_Data",
        .base_addr = USER_DATA_BASE_ADDR,
        .size = USER_DATA_SIZE,
        .end_addr = USER_DATA_END_ADDR,
        .description = "用户数据区 (3.75MB)"
    }
};

const int g_partition_count = sizeof(g_partition_table) / sizeof(partition_info_t);

/* ============================= 公共函数 ============================= */

/**
 * @brief 打印分区布局信息
 */
void partition_print_layout(void) {
    printf("\\n");
    printf("================== S300 Flash分区布局 (16MB W25Q128) ==================\\n");
    printf("分区名称        | 起始地址   | 结束地址   | 大小      | 描述\\n");
    printf("----------------|------------|------------|-----------|------------------\\n");
    
    for (int i = 0; i < g_partition_count; i++) {
        const partition_info_t *part = &g_partition_table[i];
        
        // 格式化大小显示
        char size_str[16];
        if (part->size >= 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1fMB", part->size / (1024.0 * 1024.0));
        } else if (part->size >= 1024) {
            snprintf(size_str, sizeof(size_str), "%uKB", (unsigned int)(part->size / 1024));
        } else {
            snprintf(size_str, sizeof(size_str), "%uB", (unsigned int)part->size);
        }
        
        printf("%-15s | 0x%08X | 0x%08X | %-9s | %s\\n",
               part->name,
               (unsigned int)part->base_addr,
               (unsigned int)part->end_addr,
               size_str,
               part->description);
    }
    
    printf("========================================================================\\n");
    printf("总计Flash使用: %.1fMB / 16MB\\n", FLASH_TOTAL_SIZE / (1024.0 * 1024.0));
    printf("\\n");
    
    // 详细显示NVS分区内部布局
    printf("=================== NVS分区详细布局 (60KB) ===================\\n");
    printf("子分区          | 起始地址   | 结束地址   | 大小   | 用途\\n");
    printf("----------------|------------|------------|--------|------------------\\n");
    printf("NVS_Data        | 0x%08X | 0x%08X | 56KB   | 配置参数、证书等\\n",
           NVS_BASE_ADDR, NVS_DATA_END_ADDR);
    printf("Reset_Flags     | 0x%08X | 0x%08X | 4KB    | 软件复位标志\\n",
           RESET_FLAG_SECTOR_ADDR, NVS_END_ADDR);
    printf("===============================================================\\n");
    printf("\\n");
    
    // 详细显示软件复位标志区布局
    printf("============== 软件复位标志区布局 (4KB) ==============\\n");
    printf("标志类型        | 偏移   | 大小  | 绝对地址   | 用途\\n");
    printf("----------------|--------|-------|------------|------------------\\n");
    printf("Download_Flag   | 0x%03X  | 24B   | 0x%08X | 下载模式标志\\n",
           DOWNLOAD_FLAG_OFFSET, DOWNLOAD_FLAG_ABS_ADDR);
    printf("DoubleRst_Flag  | 0x%03X  | 16B   | 0x%08X | 双重启标志\\n",
           DOUBLE_RESET_FLAG_OFFSET, DOUBLE_RESET_FLAG_ABS_ADDR);
    printf("Boot_Counter    | 0x%03X  | 24B   | 0x%08X | 启动计数器\\n",
           BOOT_COUNT_OFFSET, BOOT_COUNT_ABS_ADDR);
    printf("Reserved        | 0x%03X  | 2.5KB | 0x%08X | 预留扩展\\n",
           RESET_FLAG_RESERVED_OFFSET, RESET_FLAG_ABS_ADDR(RESET_FLAG_RESERVED_OFFSET));
    printf("====================================================\\n");
    printf("\\n");
}

/**
 * @brief 验证分区布局的合理性
 */
int partition_validate_layout(void) {
    int error_count = 0;
    
    printf("验证分区布局...\\n");
    
    // 检查分区是否按地址排序
    for (int i = 1; i < g_partition_count; i++) {
        if (g_partition_table[i].base_addr <= g_partition_table[i-1].end_addr) {
            printf("错误: 分区 %s 与 %s 重叠或顺序错误\\n",
                   g_partition_table[i-1].name, g_partition_table[i].name);
            error_count++;
        }
    }
    
    // 检查分区是否超出Flash范围
    for (int i = 0; i < g_partition_count; i++) {
        const partition_info_t *part = &g_partition_table[i];
        if (part->end_addr >= FLASH_TOTAL_SIZE) {
            printf("错误: 分区 %s 超出Flash范围 (0x%08X >= 0x%08X)\\n",
                   part->name, (unsigned int)part->end_addr, (unsigned int)FLASH_TOTAL_SIZE);
            error_count++;
        }
    }
    
    // 检查分区大小计算是否正确
    for (int i = 0; i < g_partition_count; i++) {
        const partition_info_t *part = &g_partition_table[i];
        uint32_t calc_size = part->end_addr - part->base_addr + 1;
        if (calc_size != part->size) {
            printf("错误: 分区 %s 大小计算错误 (实际: %u, 声明: %u)\\n",
                   part->name, (unsigned int)calc_size, (unsigned int)part->size);
            error_count++;
        }
    }
    
    // 特殊检查: NVS分区内部布局
    if (NVS_DATA_END_ADDR + 1 != RESET_FLAG_SECTOR_ADDR) {
        printf("错误: NVS数据区与复位标志区不连续\\n");
        error_count++;
    }
    
    if (NVS_DATA_SIZE + RESET_FLAG_SECTOR_SIZE != NVS_TOTAL_SIZE) {
        printf("错误: NVS子分区大小总和不等于NVS总大小\\n");
        error_count++;
    }
    
    if (error_count == 0) {
        printf("✓ 分区布局验证通过\\n");
    } else {
        printf("✗ 分区布局验证失败，发现 %d 个错误\\n", error_count);
    }
    
    return error_count == 0 ? 0 : -1;
}

/**
 * @brief 根据地址查找分区信息
 */
const partition_info_t* partition_find_by_addr(uint32_t addr) {
    for (int i = 0; i < g_partition_count; i++) {
        const partition_info_t *part = &g_partition_table[i];
        if (addr >= part->base_addr && addr <= part->end_addr) {
            return part;
        }
    }
    return NULL;
}

/**
 * @brief 根据名称查找分区信息
 */
const partition_info_t* partition_find_by_name(const char *name) {
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; i < g_partition_count; i++) {
        const partition_info_t *part = &g_partition_table[i];
        if (strcmp(part->name, name) == 0) {
            return part;
        }
    }
    return NULL;
}

/**
 * @brief 地址转换为可读字符串
 * @param addr Flash地址
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 缓冲区指针
 */
const char* partition_addr_to_string(uint32_t addr, char *buffer, size_t size) {
    const partition_info_t *part = partition_find_by_addr(addr);
    
    if (part) {
        uint32_t offset = addr - part->base_addr;
        snprintf(buffer, size, "%s+0x%X (0x%08X)", part->name, 
                (unsigned int)offset, (unsigned int)addr);
    } else {
        snprintf(buffer, size, "Unknown (0x%08X)", (unsigned int)addr);
    }
    
    return buffer;
}

/**
 * @brief 获取分区使用率统计
 * @param used_size 输出已使用大小
 * @param total_size 输出总大小
 */
void partition_get_usage_stats(uint32_t *used_size, uint32_t *total_size) {
    if (used_size) {
        *used_size = 0;
        for (int i = 0; i < g_partition_count; i++) {
            *used_size += g_partition_table[i].size;
        }
    }
    
    if (total_size) {
        *total_size = FLASH_TOTAL_SIZE;
    }
}
