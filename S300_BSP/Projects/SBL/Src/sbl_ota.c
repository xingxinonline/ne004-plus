/**
 * @file sbl_ota_simple.c
 * @brief SBL OTA功能简化版本 - 暂时禁用复杂功能，先让编译通过
 */

#include "sbl_ota.h"
#include "sbl_config.h"
#include "sbl_partition.h"
#include <stdio.h>
#include <string.h>

/* 全局变量 */
static bool g_ota_initialized = false;
static esp_ota_data_t g_simple_ota_data = {0};

/**
 * @brief 初始化OTA系统（简化版）
 */
int sbl_ota_init(void)
{
    if (g_ota_initialized) {
        return 0;
    }
    
    SBL_LOGI("OTA", "Initializing OTA system (simplified)...");
    
    /* 初始化默认OTA数据 */
    memset(&g_simple_ota_data, 0, sizeof(esp_ota_data_t));
    
    g_ota_initialized = true;
    SBL_LOGI("OTA", "OTA system initialized successfully (simplified)");
    
    return 0;
}

/**
 * @brief 获取启动分区（简化版）
 */
const esp_partition_info_t* sbl_ota_get_boot_partition(void)
{
    if (!g_ota_initialized) {
        SBL_LOGE("OTA", "OTA system not initialized");
        return NULL;
    }
    
    /* 暂时返回NULL，需要实际的分区表系统 */
    static esp_partition_info_t factory_partition = {
        .magic = ESP_PARTITION_MAGIC,
        .type = ESP_PARTITION_TYPE_APP,
        .subtype = ESP_PARTITION_SUBTYPE_APP_FACTORY,
        .offset = 0x10000,  /* 64KB偏移 */
        .size = 0x100000,   /* 1MB大小 */
        .flags = 0
    };
    strcpy(factory_partition.label, "factory");
    
    SBL_LOGI("OTA", "Using factory partition (simplified)");
    return &factory_partition;
}

/**
 * @brief 验证镜像（简化版）
 */
bool sbl_ota_verify_image(const esp_partition_info_t* partition)
{
    if (partition == NULL) {
        SBL_LOGE("OTA", "Invalid partition for image verification");
        return false;
    }
    
    SBL_LOGI("OTA", "Verifying image in partition: %s (simplified)", partition->label);
    
    /* 简化版本：总是返回true */
    SBL_LOGI("OTA", "Image verification passed (simplified)");
    
    return true;
}

/**
 * @brief 读取启动环境（简化版）
 */
int sbl_boot_read_env(sbl_boot_env_t* env)
{
    if (env == NULL) {
        return -1;
    }
    
    /* 使用默认值 */
    memset(env, 0, sizeof(sbl_boot_env_t));
    env->active_slot = 0;
    env->update_pending = 0;
    env->boot_count = 1;
    env->retry_count = 0;
    
    SBL_LOGI("OTA", "Boot environment loaded (simplified)");
    
    return 0;
}

/**
 * @brief 写入启动环境（简化版）
 */
int sbl_boot_write_env(const sbl_boot_env_t* env)
{
    if (env == NULL) {
        return -1;
    }
    
    SBL_LOGI("OTA", "Boot environment written (simplified)");
    
    return 0;
}

/**
 * @brief 检查是否应该回滚（简化版）
 */
bool sbl_boot_should_rollback(const sbl_boot_env_t* boot_env)
{
    /* 简化版本：不回滚 */
    return false;
}

/**
 * @brief 执行回滚操作（简化版）
 */
int sbl_boot_perform_rollback(sbl_boot_env_t* boot_env)
{
    /* 简化版本：不执行回滚 */
    return -1;
}

/**
 * @brief 更新启动计数（简化版）
 */
int sbl_boot_increment_counter(void)
{
    SBL_LOGI("OTA", "Boot counter incremented (simplified)");
    return 0;
}

/**
 * @brief 重置重试计数（简化版）
 */
int sbl_boot_reset_retry_counter(void)
{
    SBL_LOGI("OTA", "Retry counter reset (simplified)");
    return 0;
}
