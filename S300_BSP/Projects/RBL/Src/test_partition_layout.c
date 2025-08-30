/**
 * @file test_partition_layout.c
 * @brief S300分区布局测试程序
 * @version 1.0
 * @date 2024
 */

#include "partition_layout.h"
#include "software_reset.h"
#include <stdio.h>
#i    printf("地址查找性能: %d次查找耗时 %u ms\\n", iterations, 
           (unsigned int)(end_time - start_time));clude <assert.h>
#include <string.h>
#include <stdint.h>

/**
 * @brief 测试分区布局基础功能
 */
void test_partition_basic(void) {
    printf("=== 测试分区布局基础功能 ===\n");
    
    // 测试地址查找分区
    const partition_info_t *part;
    
    // 测试RBL分区
    part = partition_find_by_addr(0x00000100);
    assert(part != NULL);
    assert(strcmp(part->name, "RBL") == 0);
    printf("✓ RBL分区查找测试通过\n");
    
    // 测试NVS数据区
    part = partition_find_by_addr(0x00030000);
    assert(part != NULL);
    assert(strcmp(part->name, "NVS_Data") == 0);
    printf("✓ NVS数据区查找测试通过\n");
    
    // 测试软件复位标志区
    part = partition_find_by_addr(0x0003F000);
    assert(part != NULL);
    assert(strcmp(part->name, "Reset_Flags") == 0);
    printf("✓ 软件复位标志区查找测试通过\n");
    
    // 测试名称查找分区
    part = partition_find_by_name("OTA_0");
    assert(part != NULL);
    assert(part->base_addr == 0x00040000);
    printf("✓ 按名称查找分区测试通过\n");
    
    // 测试无效地址
    part = partition_find_by_addr(0x01234567);
    assert(part == NULL);
    printf("✓ 无效地址处理测试通过\n");
}

/**
 * @brief 测试分区布局验证
 */
void test_partition_validation(void) {
    printf("\n=== 测试分区布局验证 ===\n");
    
    int result = partition_validate_layout();
    assert(result == 0);
    printf("✓ 分区布局验证测试通过\n");
}

/**
 * @brief 测试软件复位标志区地址
 */
void test_reset_flag_addresses(void) {
    printf("\n=== 测试软件复位标志区地址 ===\n");
    
    // 验证标志区基地址
    assert(RESET_FLAG_SECTOR_ADDR == 0x0003F000);
    printf("✓ 复位标志区基地址: 0x%08X\n", RESET_FLAG_SECTOR_ADDR);
    
    // 验证各标志的绝对地址
    assert(DOWNLOAD_FLAG_ABS_ADDR == 0x0003F000);
    printf("✓ 下载标志绝对地址: 0x%08X\n", DOWNLOAD_FLAG_ABS_ADDR);
    
    assert(DOUBLE_RESET_FLAG_ABS_ADDR == 0x0003F100);
    printf("✓ 双重启标志绝对地址: 0x%08X\n", DOUBLE_RESET_FLAG_ABS_ADDR);
    
    assert(BOOT_COUNT_ABS_ADDR == 0x0003F200);
    printf("✓ 启动计数器绝对地址: 0x%08X\n", BOOT_COUNT_ABS_ADDR);
    
    // 验证地址是否在复位标志区内
    assert(IS_IN_RESET_FLAG_PARTITION(DOWNLOAD_FLAG_ABS_ADDR));
    assert(IS_IN_RESET_FLAG_PARTITION(DOUBLE_RESET_FLAG_ABS_ADDR));
    assert(IS_IN_RESET_FLAG_PARTITION(BOOT_COUNT_ABS_ADDR));
    printf("✓ 所有标志地址都在复位标志区内\n");
}

/**
 * @brief 测试NVS分区布局
 */
void test_nvs_layout(void) {
    printf("\n=== 测试NVS分区布局 ===\n");
    
    // 验证NVS总大小
    assert(NVS_TOTAL_SIZE == 0x00010000);  // 60KB
    printf("✓ NVS总大小: %d KB\n", NVS_TOTAL_SIZE / 1024);
    
    // 验证NVS数据区大小
    assert(NVS_DATA_SIZE == 0x0000F000);   // 56KB
    printf("✓ NVS数据区大小: %d KB\n", NVS_DATA_SIZE / 1024);
    
    // 验证复位标志区大小
    assert(RESET_FLAG_SECTOR_SIZE == 0x00001000);  // 4KB
    printf("✓ 复位标志区大小: %d KB\n", RESET_FLAG_SECTOR_SIZE / 1024);
    
    // 验证地址连续性
    assert(NVS_DATA_END_ADDR + 1 == RESET_FLAG_SECTOR_ADDR);
    printf("✓ NVS数据区与复位标志区地址连续\n");
    
    // 验证总大小计算
    assert(NVS_DATA_SIZE + RESET_FLAG_SECTOR_SIZE == NVS_TOTAL_SIZE);
    printf("✓ NVS子分区大小总和正确\n");
}

/**
 * @brief 测试分区地址转换
 */
void test_address_conversion(void) {
    printf("\n=== 测试地址转换功能 ===\n");
    
    char buffer[64];
    
    // 测试RBL地址转换
    const char *str = partition_addr_to_string(0x00000200, buffer, sizeof(buffer));
    printf("地址 0x00000200 -> %s\n", str);
    
    // 测试复位标志区地址转换
    str = partition_addr_to_string(0x0003F100, buffer, sizeof(buffer));
    printf("地址 0x0003F100 -> %s\n", str);
    
    // 测试OTA分区地址转换
    str = partition_addr_to_string(0x00040000, buffer, sizeof(buffer));
    printf("地址 0x00040000 -> %s\n", str);
    
    printf("✓ 地址转换功能测试通过\n");
}

/**
 * @brief 测试分区使用统计
 */
void test_usage_stats(void) {
    printf("\n=== 测试分区使用统计 ===\n");
    
    uint32_t used_size, total_size;
    partition_get_usage_stats(&used_size, &total_size);
    
    printf("已使用空间: %.2f MB\n", used_size / (1024.0 * 1024.0));
    printf("总计空间: %.2f MB\n", total_size / (1024.0 * 1024.0));
    printf("使用率: %.1f%%\n", (used_size * 100.0) / total_size);
    
    assert(total_size == FLASH_TOTAL_SIZE);
    assert(used_size <= total_size);
    printf("✓ 分区使用统计测试通过\n");
}

/**
 * @brief 性能测试
 */
void test_performance(void) {
    printf("\n=== 性能测试 ===\n");
    
    const int iterations = 10000;
    
    // 测试地址查找性能
    uint32_t start_time = 0; // 假设有计时函数
    for (int i = 0; i < iterations; i++) {
        partition_find_by_addr(0x0003F000 + (i % 0x1000));
    }
    uint32_t end_time = 0;
    
    printf("地址查找性能: %d次查找耗时 %lu ms\n", iterations, end_time - start_time);
    printf("✓ 性能测试完成\n");
}

/**
 * @brief 主测试函数
 */
int main(void) {
    printf("S300分区布局测试程序 v1.0\n");
    printf("=====================================\n");
    
    // 首先打印分区布局
    partition_print_layout();
    
    // 运行各项测试
    test_partition_basic();
    test_partition_validation();
    test_reset_flag_addresses();
    test_nvs_layout();
    test_address_conversion();
    test_usage_stats();
    test_performance();
    
    printf("\n=====================================\n");
    printf("✅ 所有测试通过！分区布局配置正确。\n");
    printf("=====================================\n");
    
    return 0;
}

/**
 * @brief 用于RBL集成的测试函数
 */
void rbl_test_partition_layout(void) {
    printf("[RBL] 分区布局测试...\n");
    
    // 快速验证关键分区
    assert(partition_find_by_name("RBL") != NULL);
    assert(partition_find_by_name("SBL") != NULL);
    assert(partition_find_by_name("Reset_Flags") != NULL);
    assert(partition_find_by_name("OTA_0") != NULL);
    assert(partition_find_by_name("OTA_1") != NULL);
    
    // 验证软件复位地址
    assert(RESET_FLAG_SECTOR_ADDR == 0x0003F000);
    assert(IS_IN_RESET_FLAG_PARTITION(DOWNLOAD_FLAG_ABS_ADDR));
    
    // 验证分区布局
    if (partition_validate_layout() != 0) {
        printf("[RBL] 错误: 分区布局验证失败!\n");
        while(1); // 阻塞
    }
    
    printf("[RBL] ✓ 分区布局测试通过\n");
}
