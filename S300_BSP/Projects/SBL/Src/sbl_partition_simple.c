/**
 * @file sbl_partition_simple.c
 * @brief SBL分区管理简化版本
 */

#include "sbl_partition.h"
#include "sbl_config.h"
#include <stdio.h>
#include <string.h>

/* 全局变量 */
static bool g_partition_initialized = false;

/**
 * @brief 初始化分区表系统（简化版）
 */
int sbl_partition_init(void)
{
    if (g_partition_initialized) {
        return 0;
    }
    
    SBL_LOGI("PART", "Initializing partition table (simplified)...");
    
    g_partition_initialized = true;
    SBL_LOGI("PART", "Partition table initialized successfully (simplified)");
    
    return 0;
}

/**
 * @brief 查找分区（简化版）
 */
esp_partition_iterator_t sbl_partition_find(esp_partition_type_t type,
                                          esp_partition_subtype_t subtype,
                                          const char* label)
{
    /* 简化版本：返回空的迭代器 */
    static esp_partition_iterator_t dummy_iterator = {0};
    return dummy_iterator;
}

/**
 * @brief 打印分区表信息（简化版）
 */
void sbl_partition_table_print(void)
{
    if (!g_partition_initialized) {
        SBL_LOGE("PART", "Partition table not initialized");
        return;
    }
    
    printf("\r\n");
    printf("========================================\r\n");
    printf("Partition Table (Simplified)\r\n");
    printf("========================================\r\n");
    printf("Type | Sub | Offset   | Size     | Label\r\n");
    printf("----------------------------------------\r\n");
    printf("app  | fac | 0x010000 | 0x100000 | factory\r\n");
    printf("data | ota | 0x110000 | 0x002000 | otadata\r\n");
    printf("app  | ot0 | 0x120000 | 0x100000 | ota_0\r\n");
    printf("app  | ot1 | 0x220000 | 0x100000 | ota_1\r\n");
    printf("========================================\r\n");
}
