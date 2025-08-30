/**
 * @file app_download_api_example.c
 * @brief 应用程序下载API使用示例
 * @version 1.0
 * @date 2025-08-30
 * 
 * 展示如何在应用程序中集成软件下载触发功能
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* 假设这些是从RBL导出的API */
typedef enum {
    DOWNLOAD_REASON_NONE = 0,
    DOWNLOAD_REASON_USER_REQUEST,
    DOWNLOAD_REASON_REMOTE,
    DOWNLOAD_REASON_BLUETOOTH,
    DOWNLOAD_REASON_BOOT_FAILURE,
    DOWNLOAD_REASON_APP_CRASH,
    DOWNLOAD_REASON_WATCHDOG,
} download_reason_t;

/* RBL API声明（实际项目中应该从头文件包含） */
extern void app_trigger_download_mode(download_reason_t reason);
extern void app_report_successful_boot(void);

/* 应用程序状态 */
static struct {
    bool boot_completed;
    uint32_t crash_count;
    char last_error[64];
} app_state = {0};

/**
 * @brief 应用程序主函数
 */
int app_main(void)
{
    printf("\r\n");
    printf("===============================================\r\n");
    printf("  S300 Application with Download API Demo\r\n");
    printf("===============================================\r\n");
    
    /* 初始化应用程序 */
    if (app_initialize() != 0) {
        printf("[APP] Application initialization failed!\r\n");
        return -1;
    }
    
    /* 报告成功启动给RBL */
    app_report_successful_boot();
    app_state.boot_completed = true;
    
    printf("[APP] Application started successfully\r\n");
    printf("[APP] Available commands:\r\n");
    printf("  DOWNLOAD    - Enter download mode\r\n");
    printf("  RECOVERY    - Enter recovery mode\r\n");
    printf("  CRASH       - Simulate app crash\r\n");
    printf("  STATUS      - Show system status\r\n");
    printf("  HELP        - Show this help\r\n");
    
    /* 主循环 */
    while (1) {
        app_process_commands();
        app_run_tasks();
        
        /* 模拟看门狗喂狗 */
        app_watchdog_feed();
        
        /* 延时 */
        app_delay_ms(100);
    }
    
    return 0;
}

/**
 * @brief 应用程序初始化
 */
int app_initialize(void)
{
    /* 模拟硬件初始化 */
    printf("[APP] Initializing hardware...\r\n");
    
    /* 初始化串口 */
    /* 初始化GPIO */
    /* 初始化外设 */
    
    /* 检查配置有效性 */
    if (!app_check_configuration()) {
        strcpy(app_state.last_error, "Invalid configuration");
        return -1;
    }
    
    /* 初始化网络（如果有） */
    if (app_init_network() != 0) {
        printf("[APP] Warning: Network initialization failed\r\n");
        /* 网络失败不影响应用启动 */
    }
    
    printf("[APP] Hardware initialization completed\r\n");
    return 0;
}

/**
 * @brief 处理串口命令
 */
void app_process_commands(void)
{
    static char cmd_buffer[64];
    static int cmd_index = 0;
    char ch;
    
    /* 简化的串口读取（实际项目中使用具体的串口API） */
    if (app_uart_read_char(&ch)) {
        if (ch == '\r' || ch == '\n') {
            if (cmd_index > 0) {
                cmd_buffer[cmd_index] = '\0';
                app_handle_command(cmd_buffer);
                cmd_index = 0;
            }
        } else if (ch >= ' ' && ch <= '~' && cmd_index < 63) {
            cmd_buffer[cmd_index++] = ch;
        }
    }
}

/**
 * @brief 处理具体命令
 */
void app_handle_command(const char *command)
{
    printf("[APP] Command received: %s\r\n", command);
    
    if (strcmp(command, "DOWNLOAD") == 0) {
        printf("[APP] User requested download mode\r\n");
        app_trigger_download_mode(DOWNLOAD_REASON_USER_REQUEST);
        
    } else if (strcmp(command, "RECOVERY") == 0) {
        printf("[APP] User requested recovery mode\r\n");
        app_trigger_download_mode(DOWNLOAD_REASON_APP_CRASH);
        
    } else if (strcmp(command, "CRASH") == 0) {
        printf("[APP] Simulating application crash...\r\n");
        app_simulate_crash();
        
    } else if (strcmp(command, "STATUS") == 0) {
        app_show_status();
        
    } else if (strcmp(command, "HELP") == 0) {
        app_show_help();
        
    } else {
        printf("[APP] Unknown command: %s\r\n", command);
        printf("[APP] Type HELP for available commands\r\n");
    }
}

/**
 * @brief 显示系统状态
 */
void app_show_status(void)
{
    printf("\r\n--- System Status ---\r\n");
    printf("Boot completed: %s\r\n", app_state.boot_completed ? "Yes" : "No");
    printf("Crash count: %lu\r\n", (unsigned long)app_state.crash_count);
    printf("Last error: %s\r\n", app_state.last_error[0] ? app_state.last_error : "None");
    printf("System uptime: %lu ms\r\n", (unsigned long)app_get_uptime_ms());
    printf("Free memory: %lu bytes\r\n", (unsigned long)app_get_free_memory());
    printf("--------------------\r\n");
}

/**
 * @brief 显示帮助信息
 */
void app_show_help(void)
{
    printf("\r\n--- Available Commands ---\r\n");
    printf("DOWNLOAD  - Trigger software download mode\r\n");
    printf("RECOVERY  - Trigger recovery download mode\r\n");
    printf("CRASH     - Simulate application crash\r\n");
    printf("STATUS    - Show system status information\r\n");
    printf("HELP      - Show this help message\r\n");
    printf("-------------------------\r\n");
}

/**
 * @brief 模拟应用程序崩溃
 */
void app_simulate_crash(void)
{
    app_state.crash_count++;
    strcpy(app_state.last_error, "Simulated crash");
    
    printf("[APP] Crash simulation - triggering recovery download\r\n");
    
    /* 记录崩溃信息到Flash */
    app_log_crash_info("Simulated crash for testing");
    
    /* 触发恢复下载模式 */
    app_trigger_download_mode(DOWNLOAD_REASON_APP_CRASH);
}

/**
 * @brief 运行应用程序任务
 */
void app_run_tasks(void)
{
    static uint32_t last_heartbeat = 0;
    static uint32_t last_status = 0;
    uint32_t current_time = app_get_uptime_ms();
    
    /* 每5秒打印心跳 */
    if (current_time - last_heartbeat > 5000) {
        printf("[APP] Heartbeat - System running normally\r\n");
        last_heartbeat = current_time;
    }
    
    /* 每30秒打印状态摘要 */
    if (current_time - last_status > 30000) {
        printf("[APP] Status: Uptime=%lus, Free=%lu bytes\r\n",
               (unsigned long)(current_time / 1000),
               (unsigned long)app_get_free_memory());
        last_status = current_time;
    }
    
    /* 检查内存泄漏 */
    if (app_get_free_memory() < 1024) {
        printf("[APP] Critical: Low memory detected!\r\n");
        strcpy(app_state.last_error, "Low memory");
        /* 可以选择触发重启或下载模式 */
    }
}

/**
 * @brief 网络下载触发器（示例）
 */
void app_network_download_handler(void)
{
    printf("[APP] Network download request received\r\n");
    
    /* 可以先做一些清理工作 */
    app_save_user_data();
    app_close_network_connections();
    
    /* 延时确保操作完成 */
    app_delay_ms(1000);
    
    /* 触发下载模式 */
    app_trigger_download_mode(DOWNLOAD_REASON_REMOTE);
}

/**
 * @brief 蓝牙下载触发器（示例）
 */
void app_bluetooth_download_handler(const char *device_id)
{
    printf("[APP] Bluetooth download request from: %s\r\n", device_id);
    
    /* 验证设备权限 */
    if (!app_verify_bluetooth_device(device_id)) {
        printf("[APP] Unauthorized bluetooth device\r\n");
        return;
    }
    
    /* 发送确认响应 */
    app_bluetooth_send_response("Download mode activated");
    
    /* 触发下载模式 */
    app_trigger_download_mode(DOWNLOAD_REASON_BLUETOOTH);
}

/* 辅助函数的简化实现（实际项目中需要具体实现） */

bool app_check_configuration(void) { return true; }
int app_init_network(void) { return 0; }
bool app_uart_read_char(char *ch) { return false; /* 需要实际实现 */ }
uint32_t app_get_uptime_ms(void) { return 0; /* 需要实际实现 */ }
uint32_t app_get_free_memory(void) { return 32768; }
void app_delay_ms(uint32_t ms) { /* 需要实际实现 */ }
void app_watchdog_feed(void) { /* 需要实际实现 */ }
void app_log_crash_info(const char *info) { /* 需要实际实现 */ }
void app_save_user_data(void) { /* 需要实际实现 */ }
void app_close_network_connections(void) { /* 需要实际实现 */ }
bool app_verify_bluetooth_device(const char *id) { return true; }
void app_bluetooth_send_response(const char *msg) { /* 需要实际实现 */ }
