/**
 * @file sbl_ota.c
 * @brief SBL OTA管理实现 - ESP32兼容的OTA更新系统
 */

#include "sbl_ota.h"
#include "sbl_config.h"
#include "sbl_partition.h"
#include <string.h>
#include <stdio.h>

/* OTA数据结构 */
typedef struct {
    esp_ota_data_t ota_data;
    bool data_valid;
    const esp_partition_info_t* ota_partition;
} sbl_ota_context_t;

/* 全局变量 */
static sbl_ota_context_t g_ota_ctx = {0};
static bool g_ota_initialized = false;

/* 静态函数声明 */
static int sbl_ota_read_data(esp_ota_data_t* ota_data);
static int sbl_ota_write_data(const esp_ota_data_t* ota_data);
static bool sbl_ota_validate_image_header(const esp_image_header_t* header);
static uint32_t sbl_ota_calculate_checksum(const void* data, size_t length);
static int sbl_ota_read_flash(uint32_t offset, void* buffer, size_t size);

/**
 * @brief 初始化OTA系统
 */
int sbl_ota_init(void)
{
    if (g_ota_initialized) {
        return 0;
    }
    
    SBL_LOGI("OTA", "Initializing OTA system...");
    
    /* 查找OTA数据分区 */
    g_ota_ctx.ota_partition = sbl_partition_find(ESP_PARTITION_TYPE_DATA, 
                                                ESP_PARTITION_SUBTYPE_DATA_OTA, 
                                                NULL);
    if (g_ota_ctx.ota_partition == NULL) {
        SBL_LOGE("OTA", "OTA data partition not found");
        return -1;
    }
    
    SBL_LOGI("OTA", "Found OTA data partition: %s (offset=0x%X, size=0x%X)",
             g_ota_ctx.ota_partition->label,
             g_ota_ctx.ota_partition->offset,
             g_ota_ctx.ota_partition->size);
    
    /* 读取OTA数据 */
    if (sbl_ota_read_data(&g_ota_ctx.ota_data) == 0) {
        g_ota_ctx.data_valid = true;
        SBL_LOGI("OTA", "OTA data loaded: seq=%lu, active_slot=%d, pending=%d",
                 g_ota_ctx.ota_data.seq_label,
                 g_ota_ctx.ota_data.active_slot,
                 g_ota_ctx.ota_data.update_pending);
    } else {
        /* 初始化默认OTA数据 */
        memset(&g_ota_ctx.ota_data, 0, sizeof(esp_ota_data_t));
        g_ota_ctx.ota_data.seq_label = 1;
        g_ota_ctx.ota_data.active_slot = ESP_OTA_SLOT_FACTORY;
        g_ota_ctx.ota_data.update_pending = false;
        g_ota_ctx.ota_data.crc = 0;
        g_ota_ctx.data_valid = false;
        
        SBL_LOGW("OTA", "OTA data not found, using defaults");
    }
    
    g_ota_initialized = true;
    SBL_LOGI("OTA", "OTA system initialized successfully");
    
    return 0;
}

/**
 * @brief 获取启动分区
 */
const esp_partition_info_t* sbl_ota_get_boot_partition(void)
{
    if (!g_ota_initialized) {
        SBL_LOGE("OTA", "OTA system not initialized");
        return NULL;
    }
    
    const esp_partition_info_t* boot_partition = NULL;
    
    /* 根据活动槽选择启动分区 */
    switch (g_ota_ctx.ota_data.active_slot) {
        case ESP_OTA_SLOT_FACTORY:
            boot_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                              ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                              "factory");
            break;
            
        case ESP_OTA_SLOT_0:
            boot_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                              ESP_PARTITION_SUBTYPE_APP_OTA_0,
                                              "ota_0");
            break;
            
        case ESP_OTA_SLOT_1:
            boot_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                              ESP_PARTITION_SUBTYPE_APP_OTA_1,
                                              "ota_1");
            break;
            
        default:
            SBL_LOGE("OTA", "Invalid active slot: %d", g_ota_ctx.ota_data.active_slot);
            return NULL;
    }
    
    if (boot_partition == NULL) {
        SBL_LOGW("OTA", "Boot partition not found for slot %d, trying factory",
                 g_ota_ctx.ota_data.active_slot);
        
        /* 回退到factory分区 */
        boot_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                          ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                          "factory");
        
        if (boot_partition != NULL) {
            /* 更新OTA数据 */
            g_ota_ctx.ota_data.active_slot = ESP_OTA_SLOT_FACTORY;
            sbl_ota_write_data(&g_ota_ctx.ota_data);
        }
    }
    
    return boot_partition;
}

/**
 * @brief 验证镜像
 */
bool sbl_ota_verify_image(const esp_partition_info_t* partition)
{
    if (partition == NULL) {
        SBL_LOGE("OTA", "Invalid partition for image verification");
        return false;
    }
    
    SBL_LOGI("OTA", "Verifying image in partition: %s", partition->label);
    
    /* 读取镜像头部 */
    esp_image_header_t image_header;
    if (sbl_ota_read_flash(partition->offset, &image_header, sizeof(image_header)) != 0) {
        SBL_LOGE("OTA", "Failed to read image header");
        return false;
    }
    
    /* 验证镜像头部 */
    if (!sbl_ota_validate_image_header(&image_header)) {
        SBL_LOGE("OTA", "Invalid image header");
        return false;
    }
    
    SBL_LOGI("OTA", "Image header validation passed");
    SBL_LOGI("OTA", "Image size: %lu bytes", image_header.image_size);
    SBL_LOGI("OTA", "Entry point: 0x%08X", image_header.entry_addr);
    
    /* 检查镜像大小 */
    if (image_header.image_size > partition->size) {
        SBL_LOGE("OTA", "Image size (%lu) exceeds partition size (%lu)",
                 image_header.image_size, partition->size);
        return false;
    }
    
    /* TODO: 添加更详细的镜像验证（CRC、签名等） */
    
    return true;
}

/**
 * @brief 设置活动槽
 */
int sbl_ota_set_active_slot(esp_ota_slot_t slot)
{
    if (!g_ota_initialized) {
        return -1;
    }
    
    if (slot != ESP_OTA_SLOT_FACTORY && slot != ESP_OTA_SLOT_0 && slot != ESP_OTA_SLOT_1) {
        SBL_LOGE("OTA", "Invalid OTA slot: %d", slot);
        return -1;
    }
    
    SBL_LOGI("OTA", "Setting active slot to: %d", slot);
    
    g_ota_ctx.ota_data.active_slot = slot;
    g_ota_ctx.ota_data.seq_label++;
    g_ota_ctx.ota_data.update_pending = true;
    
    return sbl_ota_write_data(&g_ota_ctx.ota_data);
}

/**
 * @brief 确认启动
 */
int sbl_ota_confirm_boot(void)
{
    if (!g_ota_initialized) {
        return -1;
    }
    
    if (!g_ota_ctx.ota_data.update_pending) {
        SBL_LOGI("OTA", "No pending update to confirm");
        return 0;
    }
    
    SBL_LOGI("OTA", "Confirming boot for slot %d", g_ota_ctx.ota_data.active_slot);
    
    g_ota_ctx.ota_data.update_pending = false;
    g_ota_ctx.ota_data.seq_label++;
    
    return sbl_ota_write_data(&g_ota_ctx.ota_data);
}

/**
 * @brief 获取下一个OTA槽
 */
esp_ota_slot_t sbl_ota_get_next_update_slot(void)
{
    if (!g_ota_initialized) {
        return ESP_OTA_SLOT_FACTORY;
    }
    
    switch (g_ota_ctx.ota_data.active_slot) {
        case ESP_OTA_SLOT_FACTORY:
        case ESP_OTA_SLOT_1:
            return ESP_OTA_SLOT_0;
            
        case ESP_OTA_SLOT_0:
            return ESP_OTA_SLOT_1;
            
        default:
            return ESP_OTA_SLOT_0;
    }
}

/**
 * @brief 获取OTA数据
 */
int sbl_ota_get_data(esp_ota_data_t* ota_data)
{
    if (!g_ota_initialized || ota_data == NULL) {
        return -1;
    }
    
    *ota_data = g_ota_ctx.ota_data;
    return 0;
}

/**
 * @brief 读取OTA数据
 */
static int sbl_ota_read_data(esp_ota_data_t* ota_data)
{
    if (ota_data == NULL || g_ota_ctx.ota_partition == NULL) {
        return -1;
    }
    
    /* 从Flash读取OTA数据 */
    if (sbl_ota_read_flash(g_ota_ctx.ota_partition->offset, ota_data, sizeof(esp_ota_data_t)) != 0) {
        SBL_LOGE("OTA", "Failed to read OTA data from flash");
        return -1;
    }
    
    /* 验证CRC */
    uint32_t calculated_crc = sbl_ota_calculate_checksum(ota_data, 
                                                        sizeof(esp_ota_data_t) - sizeof(uint32_t));
    if (calculated_crc != ota_data->crc) {
        SBL_LOGW("OTA", "OTA data CRC mismatch: calculated=0x%X, stored=0x%X",
                 calculated_crc, ota_data->crc);
        return -1;
    }
    
    SBL_LOGD("OTA", "OTA data read successfully");
    return 0;
}

/**
 * @brief 写入OTA数据
 */
static int sbl_ota_write_data(const esp_ota_data_t* ota_data)
{
    if (ota_data == NULL || g_ota_ctx.ota_partition == NULL) {
        return -1;
    }
    
    /* 计算CRC */
    esp_ota_data_t write_data = *ota_data;
    write_data.crc = sbl_ota_calculate_checksum(&write_data, 
                                               sizeof(esp_ota_data_t) - sizeof(uint32_t));
    
    /* TODO: 实现Flash写入功能 */
    SBL_LOGW("OTA", "Flash write not implemented yet");
    
    SBL_LOGI("OTA", "OTA data would be written: seq=%lu, slot=%d, pending=%d",
             write_data.seq_label, write_data.active_slot, write_data.update_pending);
    
    return 0;
}

/**
 * @brief 验证镜像头部
 */
static bool sbl_ota_validate_image_header(const esp_image_header_t* header)
{
    if (header == NULL) {
        return false;
    }
    
    /* 检查魔数 */
    if (header->magic != ESP_IMAGE_HEADER_MAGIC) {
        SBL_LOGE("OTA", "Invalid image magic: 0x%X", header->magic);
        return false;
    }
    
    /* 检查镜像大小 */
    if (header->image_size == 0 || header->image_size > SBL_MAX_APP_SIZE) {
        SBL_LOGE("OTA", "Invalid image size: %lu", header->image_size);
        return false;
    }
    
    /* 检查入口地址 */
    if (header->entry_addr < SBL_FLASH_BASE_ADDR || 
        header->entry_addr >= (SBL_FLASH_BASE_ADDR + SBL_FLASH_SIZE)) {
        SBL_LOGE("OTA", "Invalid entry address: 0x%08X", header->entry_addr);
        return false;
    }
    
    /* 检查Thumb模式（ARM Cortex-M要求） */
    if ((header->entry_addr & 1) == 0) {
        SBL_LOGE("OTA", "Entry address not in Thumb mode: 0x%08X", header->entry_addr);
        return false;
    }
    
    return true;
}

/**
 * @brief 计算校验和
 */
static uint32_t sbl_ota_calculate_checksum(const void* data, size_t length)
{
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < length; i++) {
        checksum += bytes[i];
    }
    
    return checksum;
}

/**
 * @brief 读取Flash数据
 */
static int sbl_ota_read_flash(uint32_t offset, void* buffer, size_t size)
{
    /* 计算绝对地址 */
    uint32_t flash_addr = SBL_FLASH_BASE_ADDR + offset;
    
    /* 检查地址对齐 */
    if ((flash_addr & 3) != 0 || (size & 3) != 0) {
        SBL_LOGE("OTA", "Flash read address or size not aligned");
        return -1;
    }
    
    /* 检查地址范围 */
    if (flash_addr < SBL_FLASH_BASE_ADDR || 
        (flash_addr + size) > (SBL_FLASH_BASE_ADDR + SBL_FLASH_SIZE)) {
        SBL_LOGE("OTA", "Flash read address out of range");
        return -1;
    }
    
    /* 直接从XIP地址读取数据 */
    memcpy(buffer, (void*)flash_addr, size);
    
    return 0;
}

/**
 * @brief 启动环境操作实现
 */

/**
 * @brief 读取启动环境
 */
int sbl_boot_read_env(sbl_boot_env_t* env)
{
    if (env == NULL) {
        return -1;
    }
    
    /* 从OTA数据转换到启动环境 */
    if (g_ota_initialized) {
        env->active_slot = g_ota_ctx.ota_data.active_slot;
        env->update_pending = g_ota_ctx.ota_data.update_pending;
        env->boot_count = 0; /* TODO: 从持久存储读取 */
        env->retry_count = 0; /* TODO: 从持久存储读取 */
        return 0;
    }
    
    return -1;
}

/**
 * @brief 写入启动环境
 */
int sbl_boot_write_env(const sbl_boot_env_t* env)
{
    if (env == NULL) {
        return -1;
    }
    
    /* TODO: 写入持久存储 */
    SBL_LOGD("OTA", "Boot env would be written: slot=%d, pending=%d, count=%d, retry=%d",
             env->active_slot, env->update_pending, env->boot_count, env->retry_count);
    
    return 0;
}

/**
 * @brief 检查是否应该回滚
 */
bool sbl_boot_should_rollback(const sbl_boot_env_t* env)
{
    if (env == NULL) {
        return false;
    }
    
    /* 如果有待处理的更新且重试次数超过阈值，则回滚 */
    return (env->update_pending && env->retry_count >= SBL_MAX_RETRY_COUNT);
}

/**
 * @brief 执行回滚
 */
int sbl_boot_perform_rollback(sbl_boot_env_t* env)
{
    if (env == NULL) {
        return -1;
    }
    
    SBL_LOGI("OTA", "Performing rollback from slot %d", env->active_slot);
    
    /* 根据当前槽选择回滚目标 */
    esp_ota_slot_t rollback_slot = ESP_OTA_SLOT_FACTORY;
    
    if (env->active_slot == ESP_OTA_SLOT_0) {
        rollback_slot = ESP_OTA_SLOT_1;
    } else if (env->active_slot == ESP_OTA_SLOT_1) {
        rollback_slot = ESP_OTA_SLOT_0;
    }
    
    /* 检查回滚目标分区是否存在 */
    const esp_partition_info_t* rollback_partition = NULL;
    
    switch (rollback_slot) {
        case ESP_OTA_SLOT_FACTORY:
            rollback_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                                  ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                                  "factory");
            break;
            
        case ESP_OTA_SLOT_0:
            rollback_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                                  ESP_PARTITION_SUBTYPE_APP_OTA_0,
                                                  "ota_0");
            break;
            
        case ESP_OTA_SLOT_1:
            rollback_partition = sbl_partition_find(ESP_PARTITION_TYPE_APP,
                                                  ESP_PARTITION_SUBTYPE_APP_OTA_1,
                                                  "ota_1");
            break;
    }
    
    if (rollback_partition == NULL) {
        SBL_LOGE("OTA", "Rollback partition not found for slot %d", rollback_slot);
        return -1;
    }
    
    /* 验证回滚目标镜像 */
    if (!sbl_ota_verify_image(rollback_partition)) {
        SBL_LOGE("OTA", "Rollback target image verification failed");
        return -1;
    }
    
    /* 执行回滚 */
    if (sbl_ota_set_active_slot(rollback_slot) != 0) {
        SBL_LOGE("OTA", "Failed to set rollback slot");
        return -1;
    }
    
    /* 更新启动环境 */
    env->active_slot = rollback_slot;
    env->update_pending = false;
    env->retry_count = 0;
    
    SBL_LOGI("OTA", "Rollback completed to slot %d", rollback_slot);
    
    return 0;
}
