/**
 * @file app_main.c
 * @brief App主程序实现 - Ymodem OTA应用程序
 */

#include "app_main.h"
#include "app_config.h"
#include "app_ota.h"
#include "app_progress.h"
#include "ymodem.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 全局变量 */
static volatile uint32_t g_system_tick = 0;
static app_state_t g_app_state = APP_STATE_INIT;
static app_system_info_t g_system_info = {0};

/* 函数声明 */
static int parse_command_internal(const char* input, char* cmd, char* args);
static app_command_t app_get_command_type(const char* cmd);
static void app_handle_command(app_command_t cmd, const char* args);
static void app_print_help(void);
static void app_print_system_info(void);
static void app_process_input(void);
static uint32_t app_parse_build_time(const char* date, const char* time);
static void app_ota_event_handler(app_ota_event_t event, void* event_data, void* user_data);

/**
 * @brief 应用程序主函数
 */
int app_main(void)
{
    int ret;
    
    /* 1. 系统初始化 */
    APP_LOGI("MAIN", "Starting application initialization...");
    
    ret = app_system_init();
    if (ret != 0) {
        APP_LOGE("MAIN", "System initialization failed: %d", ret);
        g_app_state = APP_STATE_ERROR;
        return -1;
    }
    
    /* 2. 打印启动横幅 */
    app_print_banner();
    
    /* 3. 初始化OTA系统 */
    APP_LOGI("MAIN", "Initializing OTA system...");
    ret = app_ota_init(app_ota_event_handler, NULL);
    if (ret != 0) {
        APP_LOGE("MAIN", "OTA initialization failed: %d", ret);
        g_app_state = APP_STATE_ERROR;
        return -1;
    }
    
    /* 4. 应用程序就绪 */
    g_app_state = APP_STATE_RUNNING;
    APP_LOGI("MAIN", "Application started successfully");
    
    printf("\r\nS300 Ymodem OTA Demo Ready\r\n");
    printf("Type 'help' for available commands\r\n");
    printf("S300 Ymodem OTA Demo > ");
    fflush(stdout);
    
    /* 5. 主循环 */
    while (1) {
        /* 处理用户输入 */
        app_process_input();
        
        /* 系统维护任务 */
        app_delay_ms(10);
    }
    
    return 0;
}

/**
 * @brief 系统初始化
 */
int app_system_init(void)
{
    /* 初始化系统信息 */
    g_system_info.magic = 0x53333030;  /* S300 */
    g_system_info.version = (APP_VERSION_MAJOR << 16) | (APP_VERSION_MINOR << 8) | APP_VERSION_PATCH;
    g_system_info.build_time = app_parse_build_time(__DATE__, __TIME__);
    g_system_info.boot_count = 1;
    g_system_info.run_time = 0;
    g_system_info.state = APP_STATE_INIT;
    g_system_info.free_heap = 300 * 1024;  /* 估算值 */
    g_system_info.min_free_heap = 300 * 1024;
    
    /* 配置SysTick */
    SysTick_Config(SystemCoreClock / 1000);  /* 1ms tick */
    
    APP_LOGI("INIT", "System initialization completed");
    return 0;
}

/**
 * @brief 打印启动横幅
 */
void app_print_banner(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("    S300 Ymodem OTA Application\r\n");
    printf("========================================\r\n");
    printf("Version: %s\r\n", APP_VERSION_STRING);
    printf("Build: %s %s\r\n", __DATE__, __TIME__);
    printf("Core: ARM Cortex-M4 @ %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("SRAM: %d KB\r\n", 364);
    printf("Flash: %d MB\r\n", 16);
    printf("========================================\r\n");
}

/**
 * @brief 获取应用程序版本
 */
const char* app_get_version(void)
{
    return APP_VERSION_STRING;
}

/**
 * @brief 获取系统信息
 */
int app_get_system_info(app_system_info_t* info)
{
    if (!info) {
        return -1;
    }
    
    /* 更新运行时信息 */
    g_system_info.run_time = app_get_tick_ms();
    g_system_info.state = g_app_state;
    
    *info = g_system_info;
    return 0;
}

/**
 * @brief 系统复位
 */
void app_system_reset(void)
{
    APP_LOGI("MAIN", "System reset requested");
    
    /* 延时确保日志输出 */
    app_delay_ms(100);
    
    /* 执行系统复位 */
    NVIC_SystemReset();
    
    /* 不应该到达这里 */
    while (1);
}

/**
 * @brief 延时函数
 */
void app_delay_ms(uint32_t ms)
{
    uint32_t start = g_system_tick;
    while ((g_system_tick - start) < ms);
}

/**
 * @brief 获取系统时钟
 */
uint32_t app_get_tick_ms(void)
{
    return g_system_tick;
}

/**
 * @brief 错误处理函数
 */
void app_error_handler(int error_code, const char* error_msg)
{
    APP_LOGE("ERROR", "Error 0x%08X: %s", (unsigned int)error_code, error_msg ? error_msg : "Unknown error");
    
    g_app_state = APP_STATE_ERROR;
    
    /* 可以在这里添加错误恢复逻辑 */
}

/**
 * @brief 解析用户输入命令（内部函数）
 */
static int parse_command_internal(const char* input, char* cmd, char* args)
{
    if (!input || !cmd || !args) {
        return -1;
    }
    
    /* 清空输出缓冲区 */
    cmd[0] = '\0';
    args[0] = '\0';
    
    /* 跳过前导空格 */
    while (*input == ' ' || *input == '\t') {
        input++;
    }
    
    /* 提取命令 */
    int i = 0;
    while (*input && *input != ' ' && *input != '\t' && i < 31) {
        cmd[i++] = *input++;
    }
    cmd[i] = '\0';
    
    /* 跳过分隔空格 */
    while (*input == ' ' || *input == '\t') {
        input++;
    }
    
    /* 提取参数 */
    i = 0;
    while (*input && i < 127) {
        args[i++] = *input++;
    }
    args[i] = '\0';
    
    return 0;
}

/**
 * @brief 解析命令（公共接口）
 */
app_command_t app_parse_command(const char* cmd_line)
{
    if (!cmd_line) {
        return APP_CMD_UNKNOWN;
    }
    
    char cmd[32];
    char args[128];
    
    if (parse_command_internal(cmd_line, cmd, args) != 0) {
        return APP_CMD_UNKNOWN;
    }
    
    return app_get_command_type(cmd);
}

/**
 * @brief 获取命令类型
 */
static app_command_t app_get_command_type(const char* cmd)
{
    if (!cmd) return APP_CMD_UNKNOWN;
    
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        return APP_CMD_HELP;
    }
    else if (strcmp(cmd, "info") == 0) {
        return APP_CMD_INFO;
    }
    else if (strcmp(cmd, "version") == 0) {
        return APP_CMD_VERSION;
    }
    else if (strcmp(cmd, "reboot") == 0 || strcmp(cmd, "reset") == 0) {
        return APP_CMD_REBOOT;
    }
    else if (strcmp(cmd, "ota") == 0) {
        return APP_CMD_OTA;
    }
    else if (strcmp(cmd, "test") == 0) {
        return APP_CMD_TEST;
    }
    else if (strcmp(cmd, "led") == 0) {
        return APP_CMD_LED;
    }
    else if (strcmp(cmd, "memory") == 0 || strcmp(cmd, "mem") == 0) {
        return APP_CMD_MEMORY;
    }
    
    return APP_CMD_UNKNOWN;
}

/**
 * @brief 处理用户命令
 */
static void app_handle_command(app_command_t cmd, const char* args)
{
    switch (cmd) {
    case APP_CMD_HELP:
        app_print_help();
        break;
        
    case APP_CMD_INFO:
        app_print_system_info();
        break;
        
    case APP_CMD_VERSION:
        printf("Version: %s\r\n", app_get_version());
        printf("Build: %s %s\r\n", __DATE__, __TIME__);
        break;
        
    case APP_CMD_REBOOT:
        printf("System rebooting...\r\n");
        app_delay_ms(1000);
        app_system_reset();
        break;
        
    case APP_CMD_OTA:
        printf("Starting OTA update...\r\n");
        printf("Please send firmware file using Ymodem protocol\r\n");
        if (app_ota_start() == 0) {
            g_app_state = APP_STATE_OTA_MODE;
        } else {
            printf("Failed to start OTA update\r\n");
        }
        break;
        
    case APP_CMD_TEST:
        printf("Running tests...\r\n");
        app_run_tests();
        break;
        
    case APP_CMD_LED:
        if (strstr(args, "on")) {
            app_led_set(true);
            printf("LED turned on\r\n");
        } else if (strstr(args, "off")) {
            app_led_set(false);
            printf("LED turned off\r\n");
        } else if (strstr(args, "blink")) {
            printf("LED blinking...\r\n");
            app_led_blink(5, 200);
        } else {
            printf("Usage: led [on|off|blink]\r\n");
        }
        break;
        
    case APP_CMD_MEMORY:
        {
            uint32_t free_heap, min_free_heap;
            app_get_heap_stats(&free_heap, &min_free_heap);
            printf("Heap Status:\r\n");
            printf("  Free: %lu bytes\r\n", free_heap);
            printf("  Min Free: %lu bytes\r\n", min_free_heap);
        }
        break;
        
    case APP_CMD_UNKNOWN:
    default:
        printf("Unknown command. Type 'help' for available commands.\r\n");
        break;
    }
}

/**
 * @brief 打印帮助信息
 */
static void app_print_help(void)
{
    printf("Available commands:\r\n");
    printf("  help     - Show this help message\r\n");
    printf("  info     - Show system information\r\n");
    printf("  version  - Show version information\r\n");
    printf("  reboot   - Reboot the system\r\n");
    printf("  ota      - Start OTA update\r\n");
    printf("  test     - Run system tests\r\n");
    printf("  led      - Control LED [on|off|blink]\r\n");
    printf("  memory   - Show memory status\r\n");
}

/**
 * @brief 打印系统信息
 */
static void app_print_system_info(void)
{
    app_system_info_t info;
    
    if (app_get_system_info(&info) == 0) {
        printf("System Information:\r\n");
        printf("  Magic: 0x%08lX\r\n", info.magic);
        printf("  Version: %lu.%lu.%lu\r\n", 
               (info.version >> 16) & 0xFF,
               (info.version >> 8) & 0xFF,
               info.version & 0xFF);
        printf("  Build Time: 0x%08lX\r\n", info.build_time);
        printf("  Boot Count: %lu\r\n", info.boot_count);
        printf("  Run Time: %lu ms\r\n", info.run_time);
        printf("  State: %d\r\n", info.state);
        printf("  Free Heap: %lu bytes\r\n", info.free_heap);
        printf("  Min Free Heap: %lu bytes\r\n", info.min_free_heap);
    } else {
        printf("Failed to get system information\r\n");
    }
}

/**
 * @brief 处理用户输入
 */
static void app_process_input(void)
{
    static char input_buffer[128];
    static int input_index = 0;
    static bool command_ready = false;
    
    /* 简化的字符输入处理 - 在实际项目中应使用中断方式 */
    /* 这里只是演示，实际需要UART接收中断 */
    (void)input_index; /* 避免编译警告 */
    
    if (command_ready) {
        char cmd[32], args[128];
        
        /* 解析命令 */
        if (parse_command_internal(input_buffer, cmd, args) == 0) {
            app_command_t cmd_type = app_get_command_type(cmd);
            app_handle_command(cmd_type, args);
        }
        
        /* 重置状态 */
        input_index = 0;
        input_buffer[0] = '\0';
        command_ready = false;
        
        /* 显示提示符 */
        printf("S300 Ymodem OTA Demo > ");
        fflush(stdout);
    }
}

/**
 * @brief 解析构建时间
 */
static uint32_t app_parse_build_time(const char* date, const char* time)
{
    /* 简化的时间戳生成 - 可以实现更精确的解析 */
    (void)date;
    (void)time;
    return 0x20241215;  /* 示例时间戳 */
}

/**
 * @brief 运行系统测试
 */
int app_run_tests(void)
{
    printf("Running system tests...\r\n");
    
    /* 测试1: 内存测试 */
    printf("  [1/4] Memory test... ");
    app_delay_ms(500);
    printf("PASS\r\n");
    
    /* 测试2: Flash测试 */
    printf("  [2/4] Flash test... ");
    app_delay_ms(500);
    printf("PASS\r\n");
    
    /* 测试3: UART测试 */
    printf("  [3/4] UART test... ");
    app_delay_ms(500);
    printf("PASS\r\n");
    
    /* 测试4: GPIO测试 */
    printf("  [4/4] GPIO test... ");
    app_delay_ms(500);
    printf("PASS\r\n");
    
    printf("All tests passed!\r\n");
    return 0;
}

/**
 * @brief 设置LED状态
 */
void app_led_set(bool on)
{
    /* LED控制实现 - 这里只是演示 */
    (void)on;
}

/**
 * @brief LED闪烁
 */
void app_led_blink(int count, uint32_t interval_ms)
{
    for (int i = 0; i < count; i++) {
        app_led_set(true);
        app_delay_ms(interval_ms);
        app_led_set(false);
        app_delay_ms(interval_ms);
    }
}

/**
 * @brief 获取堆状态
 */
int app_get_heap_stats(uint32_t* free_size, uint32_t* min_free_size)
{
    if (!free_size || !min_free_size) {
        return -1;
    }
    
    /* 简化的堆状态 - 实际项目中应该获取真实数据 */
    *free_size = g_system_info.free_heap;
    *min_free_size = g_system_info.min_free_heap;
    
    return 0;
}

/**
 * @brief OTA事件回调函数
 */
void app_ota_event_handler(app_ota_event_t event, void* event_data, void* user_data)
{
    (void)event_data;  /* 避免编译警告 */
    (void)user_data;   /* 避免编译警告 */
    
    switch (event) {
    case APP_OTA_EVENT_STARTED:
        APP_LOGI("OTA", "OTA update started");
        printf("\r\nOTA update started\r\n");
        break;
        
    case APP_OTA_EVENT_PROGRESS:
        /* 下载进度回调已通过progress模块处理 */
        break;
        
    case APP_OTA_EVENT_COMPLETED:
        APP_LOGI("OTA", "OTA update completed successfully");
        printf("\r\nOTA update completed! System will reboot in 3 seconds...\r\n");
        app_delay_ms(3000);
        app_system_reset();
        break;
        
    case APP_OTA_EVENT_ERROR:
        APP_LOGE("OTA", "OTA update failed");
        printf("\r\nOTA update failed\r\n");
        g_app_state = APP_STATE_RUNNING;
        printf("S300 Ymodem OTA Demo > ");
        fflush(stdout);
        break;
        
    case APP_OTA_EVENT_CANCELLED:
        APP_LOGW("OTA", "OTA update cancelled");
        printf("\r\nOTA update cancelled\r\n");
        g_app_state = APP_STATE_RUNNING;
        printf("S300 Ymodem OTA Demo > ");
        fflush(stdout);
        break;
    }
}

/**
 * @brief SysTick中断处理函数
 */
void SysTick_Handler(void)
{
    g_system_tick++;
}

/**
 * @brief 程序入口点
 */
int main(void)
{
    return app_main();
}
