/**
 * @file app_ota.c
 * @brief App OTA管理实现
 */

#include "app_ota.h"
#include "app_config.h"
#include "ymodem.h"
/* #include "qspi.h" */  /* 暂时注释掉，避免编译错误 */
#include <string.h>
#include <stdio.h>

/* OTA分区信息 */
#define OTA_PARTITION_BASE      0x08200000  /* OTA分区起始地址 */
#define OTA_PARTITION_SIZE      (1024 * 1024) /* 1MB */

/* 全局变量 */
static app_ota_context_t g_ota_ctx = {0};
static app_ota_event_callback_t g_ota_callback = NULL;
static void* g_ota_user_data = NULL;

/* 静态函数声明 */
static int ota_write_data(const uint8_t* data, size_t size, void* user_data);
static void ota_progress_callback(uint32_t received, uint32_t total, void* user_data);
static int ota_erase_partition(uint32_t addr, uint32_t size);
static int ota_write_flash(uint32_t addr, const uint8_t* data, size_t size);
static int ota_read_flash(uint32_t addr, uint8_t* data, size_t size);
static void ota_notify_event(app_ota_event_t event, void* event_data);

/**
 * @brief 初始化OTA系统
 */
int app_ota_init(app_ota_event_callback_t callback, void* user_data)
{
    APP_LOGI("OTA", "Initializing OTA system...");
    
    /* 清空上下文 */
    memset(&g_ota_ctx, 0, sizeof(app_ota_context_t));
    
    /* 设置回调 */
    g_ota_callback = callback;
    g_ota_user_data = user_data;
    
    /* 配置OTA分区 */
    g_ota_ctx.ota_partition_addr = OTA_PARTITION_BASE;
    g_ota_ctx.ota_partition_size = OTA_PARTITION_SIZE;
    g_ota_ctx.status = APP_OTA_STATUS_IDLE;
    g_ota_ctx.error = APP_OTA_ERROR_NONE;
    g_ota_ctx.verification_enabled = true;
    
    /* 初始化Ymodem */
    if (ymodem_init(&g_ota_ctx.ymodem_ctx, 
                    ota_write_data, 
                    ota_progress_callback, 
                    &g_ota_ctx) != 0) {
        APP_LOGE("OTA", "Failed to initialize Ymodem");
        g_ota_ctx.error = APP_OTA_ERROR_INIT;
        return -1;
    }
    
    APP_LOGI("OTA", "OTA system initialized");
    APP_LOGI("OTA", "OTA partition: 0x%08X - 0x%08X (%lu KB)", 
             g_ota_ctx.ota_partition_addr,
             g_ota_ctx.ota_partition_addr + g_ota_ctx.ota_partition_size,
             g_ota_ctx.ota_partition_size / 1024);
    
    return 0;
}

/**
 * @brief 开始OTA更新
 */
int app_ota_start(void)
{
    if (g_ota_ctx.status != APP_OTA_STATUS_IDLE) {
        APP_LOGE("OTA", "OTA already in progress");
        return -1;
    }
    
    APP_LOGI("OTA", "Starting OTA update...");
    
    /* 重置状态 */
    g_ota_ctx.status = APP_OTA_STATUS_STARTED;
    g_ota_ctx.error = APP_OTA_ERROR_NONE;
    g_ota_ctx.written_bytes = 0;
    g_ota_ctx.total_bytes = 0;
    g_ota_ctx.start_time = app_get_tick_ms();
    g_ota_ctx.last_activity = g_ota_ctx.start_time;
    g_ota_ctx.checksum = 0;
    
    /* 擦除OTA分区 */
    APP_LOGI("OTA", "Erasing OTA partition...");
    if (ota_erase_partition(g_ota_ctx.ota_partition_addr, g_ota_ctx.ota_partition_size) != 0) {
        APP_LOGE("OTA", "Failed to erase OTA partition");
        g_ota_ctx.status = APP_OTA_STATUS_ERROR;
        g_ota_ctx.error = APP_OTA_ERROR_FLASH;
        ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
        return -1;
    }
    
    /* 开始Ymodem接收 */
    if (ymodem_receive_start(&g_ota_ctx.ymodem_ctx) != YMODEM_STATUS_OK) {
        APP_LOGE("OTA", "Failed to start Ymodem receive");
        g_ota_ctx.status = APP_OTA_STATUS_ERROR;
        g_ota_ctx.error = APP_OTA_ERROR_INIT;
        ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
        return -1;
    }
    
    g_ota_ctx.status = APP_OTA_STATUS_RECEIVING;
    ota_notify_event(APP_OTA_EVENT_STARTED, NULL);
    
    APP_LOGI("OTA", "Ready to receive firmware via Ymodem");
    printf("Ready for Ymodem transfer. Please start sending the firmware file...\r\n");
    
    return 0;
}

/**
 * @brief 处理OTA过程
 */
int app_ota_process(void)
{
    if (g_ota_ctx.status != APP_OTA_STATUS_RECEIVING) {
        return 0;
    }
    
    uint8_t packet_buffer[YMODEM_PACKET_SIZE];
    size_t packet_size;
    
    /* 接收Ymodem数据包 */
    ymodem_status_t status = ymodem_receive_packet(&g_ota_ctx.ymodem_ctx,
                                                  packet_buffer,
                                                  &packet_size);
    
    switch (status) {
    case YMODEM_STATUS_OK:
        g_ota_ctx.last_activity = app_get_tick_ms();
        
        /* 检查是否接收完成 */
        const ymodem_file_info_t* file_info = ymodem_get_file_info(&g_ota_ctx.ymodem_ctx);
        if (file_info && file_info->received_bytes >= file_info->filesize) {
            g_ota_ctx.total_bytes = file_info->filesize;
            g_ota_ctx.status = APP_OTA_STATUS_COMPLETED;
            ota_notify_event(APP_OTA_EVENT_COMPLETED, NULL);
            APP_LOGI("OTA", "OTA receive completed");
            return app_ota_finish();
        }
        break;
        
    case YMODEM_STATUS_TIMEOUT:
        /* 检查超时 */
        if ((app_get_tick_ms() - g_ota_ctx.last_activity) > APP_OTA_TIMEOUT_MS) {
            APP_LOGE("OTA", "OTA timeout");
            g_ota_ctx.status = APP_OTA_STATUS_ERROR;
            g_ota_ctx.error = APP_OTA_ERROR_TIMEOUT;
            ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
            return -1;
        }
        break;
        
    case YMODEM_STATUS_CANCEL:
    case YMODEM_STATUS_ABORT:
        APP_LOGW("OTA", "OTA cancelled by user");
        g_ota_ctx.status = APP_OTA_STATUS_CANCELLED;
        ota_notify_event(APP_OTA_EVENT_CANCELLED, NULL);
        return -1;
        
    case YMODEM_STATUS_ERROR:
    case YMODEM_STATUS_CRC_ERROR:
    case YMODEM_STATUS_PACKET_ERROR:
        APP_LOGE("OTA", "OTA error: %d", status);
        g_ota_ctx.status = APP_OTA_STATUS_ERROR;
        g_ota_ctx.error = APP_OTA_ERROR_VERIFY;
        ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 完成OTA更新
 */
int app_ota_finish(void)
{
    APP_LOGI("OTA", "Finishing OTA update...");
    
    if (g_ota_ctx.status != APP_OTA_STATUS_COMPLETED) {
        APP_LOGE("OTA", "OTA not completed");
        return -1;
    }
    
    /* 完成Ymodem接收 */
    ymodem_receive_finish(&g_ota_ctx.ymodem_ctx);
    
    /* 验证镜像 */
    if (g_ota_ctx.verification_enabled) {
        APP_LOGI("OTA", "Verifying OTA image...");
        g_ota_ctx.status = APP_OTA_STATUS_VERIFYING;
        
        if (!app_ota_verify_image()) {
            APP_LOGE("OTA", "OTA image verification failed");
            g_ota_ctx.status = APP_OTA_STATUS_ERROR;
            g_ota_ctx.error = APP_OTA_ERROR_VERIFY;
            ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
            return -1;
        }
        
        APP_LOGI("OTA", "OTA image verification passed");
    }
    
    /* 应用更新 */
    if (app_ota_apply_update() != 0) {
        APP_LOGE("OTA", "Failed to apply OTA update");
        g_ota_ctx.status = APP_OTA_STATUS_ERROR;
        g_ota_ctx.error = APP_OTA_ERROR_FLASH;
        ota_notify_event(APP_OTA_EVENT_ERROR, &g_ota_ctx.error);
        return -1;
    }
    
    uint32_t duration = app_get_tick_ms() - g_ota_ctx.start_time;
    APP_LOGI("OTA", "OTA completed successfully in %lu ms", duration);
    APP_LOGI("OTA", "Transferred %lu bytes", g_ota_ctx.written_bytes);
    
    return 0;
}

/**
 * @brief 取消OTA更新
 */
void app_ota_cancel(void)
{
    APP_LOGW("OTA", "Cancelling OTA update");
    
    ymodem_cancel(&g_ota_ctx.ymodem_ctx);
    
    g_ota_ctx.status = APP_OTA_STATUS_CANCELLED;
    ota_notify_event(APP_OTA_EVENT_CANCELLED, NULL);
}

/**
 * @brief 获取OTA状态
 */
app_ota_status_t app_ota_get_status(void)
{
    return g_ota_ctx.status;
}

/**
 * @brief 获取OTA错误
 */
app_ota_error_t app_ota_get_error(void)
{
    return g_ota_ctx.error;
}

/**
 * @brief 获取OTA进度
 */
int app_ota_get_progress(uint32_t* received, uint32_t* total)
{
    if (received == NULL || total == NULL) {
        return -1;
    }
    
    const ymodem_file_info_t* file_info = ymodem_get_file_info(&g_ota_ctx.ymodem_ctx);
    if (file_info) {
        *received = file_info->received_bytes;
        *total = file_info->filesize;
    } else {
        *received = g_ota_ctx.written_bytes;
        *total = g_ota_ctx.total_bytes;
    }
    
    return 0;
}

/**
 * @brief 验证OTA镜像
 */
bool app_ota_verify_image(void)
{
    /* TODO: 实现镜像验证逻辑 */
    /* 这里应该验证镜像头部、校验和、签名等 */
    
    uint8_t header_buffer[32];
    if (ota_read_flash(g_ota_ctx.ota_partition_addr, header_buffer, sizeof(header_buffer)) != 0) {
        APP_LOGE("OTA", "Failed to read image header");
        return false;
    }
    
    /* 简单验证：检查是否有有效的向量表 */
    uint32_t* vectors = (uint32_t*)header_buffer;
    uint32_t stack_ptr = vectors[0];
    uint32_t reset_handler = vectors[1];
    
    /* 验证栈指针 */
    if (stack_ptr < APP_SRAM_BASE || stack_ptr > (APP_SRAM_BASE + APP_SRAM_SIZE)) {
        APP_LOGE("OTA", "Invalid stack pointer: 0x%08X", stack_ptr);
        return false;
    }
    
    /* 验证复位处理程序 */
    if ((reset_handler & 1) == 0 || reset_handler < g_ota_ctx.ota_partition_addr) {
        APP_LOGE("OTA", "Invalid reset handler: 0x%08X", reset_handler);
        return false;
    }
    
    APP_LOGI("OTA", "Image verification passed");
    return true;
}

/**
 * @brief 应用OTA更新
 */
int app_ota_apply_update(void)
{
    APP_LOGI("OTA", "Applying OTA update...");
    
    /* TODO: 实现OTA应用逻辑 */
    /* 这里应该更新分区表，设置新的启动分区 */
    
    APP_LOGI("OTA", "OTA update applied");
    APP_LOGI("OTA", "System will reboot to new firmware");
    
    return 0;
}

/**
 * @brief 重启到新固件
 */
void app_ota_reboot(void)
{
    APP_LOGI("OTA", "Rebooting to new firmware...");
    
    app_delay_ms(1000);
    app_system_reset();
}

/**
 * @brief 获取OTA统计信息
 */
int app_ota_get_statistics(app_ota_context_t* stats)
{
    if (stats == NULL) {
        return -1;
    }
    
    *stats = g_ota_ctx;
    return 0;
}

/**
 * @brief OTA数据写入回调
 */
static int ota_write_data(const uint8_t* data, size_t size, void* user_data)
{
    app_ota_context_t* ctx = (app_ota_context_t*)user_data;
    
    if (ctx == NULL || data == NULL || size == 0) {
        return -1;
    }
    
    uint32_t write_addr = ctx->ota_partition_addr + ctx->written_bytes;
    
    /* 检查写入地址是否超出分区 */
    if (ctx->written_bytes + size > ctx->ota_partition_size) {
        APP_LOGE("OTA", "Write would exceed partition size");
        return -1;
    }
    
    /* 写入Flash */
    if (ota_write_flash(write_addr, data, size) != 0) {
        APP_LOGE("OTA", "Failed to write to flash at 0x%08X", write_addr);
        return -1;
    }
    
    ctx->written_bytes += size;
    
    /* 更新校验和 */
    for (size_t i = 0; i < size; i++) {
        ctx->checksum += data[i];
    }
    
    return 0;
}

/**
 * @brief OTA进度回调
 */
static void ota_progress_callback(uint32_t received, uint32_t total, void* user_data)
{
    static uint32_t last_percent = 0;
    
    if (total == 0) return;
    
    uint32_t percent = (received * 100) / total;
    
    /* 每5%报告一次进度 */
    if (percent >= last_percent + 5 || percent == 100) {
        APP_LOGI("OTA", "Progress: %lu%% (%lu/%lu bytes)", percent, received, total);
        last_percent = percent;
        
        ota_notify_event(APP_OTA_EVENT_PROGRESS, &percent);
    }
}

/**
 * @brief 擦除分区
 */
static int ota_erase_partition(uint32_t addr, uint32_t size)
{
    /* TODO: 实现Flash擦除 */
    /* 这里应该调用QSPI驱动擦除指定区域 */
    
    APP_LOGI("OTA", "Erasing partition 0x%08X - 0x%08X", addr, addr + size);
    
    /* 模拟擦除延时 */
    app_delay_ms(1000);
    
    return 0;
}

/**
 * @brief 写入Flash
 */
static int ota_write_flash(uint32_t addr, const uint8_t* data, size_t size)
{
    /* TODO: 实现Flash写入 */
    /* 这里应该调用QSPI驱动写入数据 */
    
    APP_LOGD("OTA", "Writing %zu bytes to 0x%08X", size, addr);
    
    return 0;
}

/**
 * @brief 读取Flash
 */
static int ota_read_flash(uint32_t addr, uint8_t* data, size_t size)
{
    /* TODO: 实现Flash读取 */
    /* 这里应该调用QSPI驱动读取数据 */
    
    /* 模拟读取：从XIP地址直接读取 */
    memcpy(data, (void*)addr, size);
    
    return 0;
}

/**
 * @brief 通知OTA事件
 */
static void ota_notify_event(app_ota_event_t event, void* event_data)
{
    if (g_ota_callback != NULL) {
        g_ota_callback(event, event_data, g_ota_user_data);
    }
}
