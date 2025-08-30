/**
 * @file rbl_download_flags.c
 * @brief 软件下载标志管理实现
 * @version 1.0
 * @date 2025-08-30
 */

#include "rbl_download_flags.h"
#include "rbl_qspi.h"
#include "rbl_system.h"
#include <string.h>

/* 内部函数声明 */
static int flash_erase_flags_sector(void);
static int flash_read_safe(uint32_t addr, void *data, uint32_t size);
static int flash_write_safe(uint32_t addr, const void *data, uint32_t size);

/**
 * @brief 读取下载标志
 */
int rbl_flash_read_download_flag(download_flag_t *flag)
{
    if (!flag) {
        return -1;
    }

    return flash_read_safe(DOWNLOAD_FLAGS_SECTOR_ADDR, flag, sizeof(download_flag_t));
}

/**
 * @brief 写入下载标志
 */
int rbl_flash_write_download_flag(const download_flag_t *flag)
{
    if (!flag) {
        return -1;
    }

    /* 先擦除扇区 */
    int ret = flash_erase_flags_sector();
    if (ret != 0) {
        return ret;
    }

    /* 写入新标志 */
    return flash_write_safe(DOWNLOAD_FLAGS_SECTOR_ADDR, flag, sizeof(download_flag_t));
}

/**
 * @brief 设置下载标志
 */
int rbl_flash_set_download_flag(download_reason_t reason)
{
    download_flag_t flag = {
        .magic = DOWNLOAD_FLAG_MAGIC,
        .reason = reason,
        .timestamp = rbl_get_system_uptime_ms(),
        .retry_count = 0,
    };

    printf("[FLAGS] Setting download flag, reason: %s\r\n", 
           rbl_get_download_reason_string(reason));

    return rbl_flash_write_download_flag(&flag);
}

/**
 * @brief 清除下载标志
 */
int rbl_flash_clear_download_flag(void)
{
    printf("[FLAGS] Clearing download flag\r\n");
    return flash_erase_flags_sector();
}

/**
 * @brief 检查下载标志
 */
bool rbl_flash_check_download_flag(void)
{
    download_flag_t flag;
    
    if (rbl_flash_read_download_flag(&flag) != 0) {
        return false;
    }

    /* 检查魔数 */
    if (flag.magic != DOWNLOAD_FLAG_MAGIC) {
        return false;
    }

    /* 检查重试次数 */
    if (flag.retry_count > MAX_DOWNLOAD_RETRIES) {
        printf("[FLAGS] Too many download retries (%lu), clearing flag\r\n", 
               (unsigned long)flag.retry_count);
        rbl_flash_clear_download_flag();
        return false;
    }

    /* 更新重试次数 */
    flag.retry_count++;
    rbl_flash_write_download_flag(&flag);

    printf("[FLAGS] Download flag detected: %s (retry: %lu)\r\n",
           rbl_get_download_reason_string(flag.reason),
           (unsigned long)flag.retry_count);

    return true;
}

/**
 * @brief 检查双重启动
 */
bool rbl_check_double_reset(void)
{
    double_reset_flag_t flag;
    uint32_t current_time = rbl_get_system_uptime_ms();
    bool is_double_reset = false;

    /* 读取双重启标志 */
    if (flash_read_safe(DOUBLE_RESET_FLAG_ADDR, &flag, sizeof(flag)) != 0) {
        memset(&flag, 0, sizeof(flag));
    }

    /* 检查是否在时间窗口内 */
    if (flag.magic == DOUBLE_RESET_MAGIC) {
        uint32_t elapsed = current_time - flag.timestamp;
        
        if (elapsed < DOUBLE_RESET_TIMEOUT_MS) {
            flag.count++;
            printf("[FLAGS] Reset #%lu within %lu ms\r\n", 
                   (unsigned long)flag.count, (unsigned long)elapsed);
                   
            if (flag.count >= 2) {
                printf("[FLAGS] Double reset detected! Triggering download mode\r\n");
                is_double_reset = true;
                
                /* 清除标志，避免下次误触发 */
                flash_erase_flags_sector();
                return true;
            }
        } else {
            /* 超时，重置计数 */
            printf("[FLAGS] Reset timeout (%lu ms), restarting count\r\n", 
                   (unsigned long)elapsed);
            flag.count = 1;
        }
    } else {
        /* 首次启动或标志损坏 */
        printf("[FLAGS] First reset detected\r\n");
        flag.magic = DOUBLE_RESET_MAGIC;
        flag.count = 1;
    }

    /* 更新时间戳和计数 */
    flag.timestamp = current_time;

    /* 写回标志 */
    flash_write_safe(DOUBLE_RESET_FLAG_ADDR, &flag, sizeof(flag));

    return is_double_reset;
}

/**
 * @brief 读取启动环境
 */
int rbl_read_boot_environment(boot_environment_t *env)
{
    if (!env) {
        return -1;
    }

    const uint32_t env_addr = DOWNLOAD_FLAGS_SECTOR_ADDR + 0x200;
    return flash_read_safe(env_addr, env, sizeof(boot_environment_t));
}

/**
 * @brief 写入启动环境
 */
int rbl_write_boot_environment(const boot_environment_t *env)
{
    if (!env) {
        return -1;
    }

    const uint32_t env_addr = DOWNLOAD_FLAGS_SECTOR_ADDR + 0x200;
    
    /* 读取当前扇区内容 */
    static uint8_t sector_buffer[4096];
    if (rbl_qspi_read(DOWNLOAD_FLAGS_SECTOR_ADDR, sector_buffer, 4096) != 0) {
        return -1;
    }

    /* 更新环境数据 */
    memcpy(sector_buffer + 0x200, env, sizeof(boot_environment_t));

    /* 擦除并重写整个扇区 */
    int ret = rbl_qspi_erase_sector(DOWNLOAD_FLAGS_SECTOR_ADDR);
    if (ret != 0) {
        return ret;
    }

    return rbl_qspi_write(DOWNLOAD_FLAGS_SECTOR_ADDR, sector_buffer, 4096);
}

/**
 * @brief 增加启动尝试计数
 */
int rbl_increment_boot_attempts(void)
{
    boot_environment_t env;
    
    /* 读取当前环境 */
    if (rbl_read_boot_environment(&env) != 0) {
        /* 初始化环境 */
        memset(&env, 0, sizeof(env));
        env.magic = 0x424F4F54; /* "BOOT" */
    }

    env.boot_attempts++;
    env.total_boots++;

    printf("[FLAGS] Boot attempts: %lu, total boots: %lu\r\n",
           (unsigned long)env.boot_attempts, 
           (unsigned long)env.total_boots);

    return rbl_write_boot_environment(&env);
}

/**
 * @brief 清除启动尝试计数
 */
int rbl_clear_boot_attempts(void)
{
    boot_environment_t env;
    
    if (rbl_read_boot_environment(&env) != 0) {
        return -1;
    }

    env.boot_attempts = 0;
    env.last_successful_boot = rbl_get_system_uptime_ms();

    printf("[FLAGS] Boot attempts cleared, successful boot recorded\r\n");

    return rbl_write_boot_environment(&env);
}

/**
 * @brief 检查启动故障
 */
bool rbl_check_boot_failure(void)
{
    boot_environment_t env;
    
    if (rbl_read_boot_environment(&env) != 0) {
        return false;
    }

    if (env.magic != 0x424F4F54) {
        return false;
    }

    if (env.boot_attempts >= MAX_BOOT_ATTEMPTS) {
        printf("[FLAGS] Boot failure detected (%lu attempts)\r\n",
               (unsigned long)env.boot_attempts);
        return true;
    }

    return false;
}

/**
 * @brief 获取下载原因字符串
 */
const char *rbl_get_download_reason_string(download_reason_t reason)
{
    switch (reason) {
        case DOWNLOAD_REASON_NONE:          return "None";
        case DOWNLOAD_REASON_USER_REQUEST:  return "User Request";
        case DOWNLOAD_REASON_REMOTE:        return "Remote Trigger";
        case DOWNLOAD_REASON_BLUETOOTH:     return "Bluetooth";
        case DOWNLOAD_REASON_BOOT_FAILURE:  return "Boot Failure";
        case DOWNLOAD_REASON_APP_CRASH:     return "App Crash";
        case DOWNLOAD_REASON_WATCHDOG:      return "Watchdog Reset";
        default:                            return "Unknown";
    }
}

/**
 * @brief 获取系统运行时间（毫秒）
 */
uint32_t rbl_get_system_uptime_ms(void)
{
    /* 这里应该返回系统启动后的毫秒数 */
    /* 暂时使用rbl_system_get_tick_ms()，实际使用中可能需要调整 */
    return rbl_system_get_tick_ms();
}

/* 内部函数实现 */

/**
 * @brief 擦除标志扇区
 */
static int flash_erase_flags_sector(void)
{
    printf("[FLAGS] Erasing flags sector at 0x%08lX\r\n", 
           (unsigned long)DOWNLOAD_FLAGS_SECTOR_ADDR);
    return rbl_qspi_erase_sector(DOWNLOAD_FLAGS_SECTOR_ADDR);
}

/**
 * @brief 安全Flash读取（带错误处理）
 */
static int flash_read_safe(uint32_t addr, void *data, uint32_t size)
{
    if (!data || size == 0) {
        return -1;
    }

    int ret = rbl_qspi_read(addr, (uint8_t*)data, size);
    if (ret != 0) {
        printf("[FLAGS] Flash read failed at 0x%08lX\r\n", (unsigned long)addr);
        return ret;
    }

    return 0;
}

/**
 * @brief 安全Flash写入（带错误处理）
 */
static int flash_write_safe(uint32_t addr, const void *data, uint32_t size)
{
    if (!data || size == 0) {
        return -1;
    }

    int ret = rbl_qspi_write(addr, (const uint8_t*)data, size);
    if (ret != 0) {
        printf("[FLAGS] Flash write failed at 0x%08lX\r\n", (unsigned long)addr);
        return ret;
    }

    printf("[FLAGS] Flash write successful at 0x%08lX (%lu bytes)\r\n", 
           (unsigned long)addr, (unsigned long)size);
    return 0;
}
