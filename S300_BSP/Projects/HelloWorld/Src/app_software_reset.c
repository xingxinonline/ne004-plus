/**
 * @file app_software_reset.c
 * @brief S300应用程序软件复位功能实现
 * @version 1.0
 * @date 2024
 * 
 * 集成软件复位功能到应用程序中，支持多种触发方式
 */

#include "software_reset.h"
#include "s300.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ================================ 私有变量 ================================ */

static bool g_app_initialized = false;
static uint32_t g_app_start_time = 0;

/* ================================ 命令处理 ================================ */

/**
 * @brief 处理串口命令
 * @param cmd 命令字符串
 * @return 0=成功处理, <0=未知命令
 */
int app_handle_command(const char *cmd) {
    if (!cmd) {
        return -1;
    }
    
    printf("[APP] Command: %s\n", cmd);
    
    // 优先处理软件复位相关命令
    if (software_reset_handle_command(cmd) == 0) {
        // 软件复位命令已处理
        return 0;
    }
    
    // 处理其他应用命令
    if (strcmp(cmd, "info") == 0) {
        app_print_info();
        return 0;
    }
    else if (strcmp(cmd, "mem") == 0) {
        app_print_memory_info();
        return 0;
    }
    else if (strcmp(cmd, "test") == 0) {
        app_run_self_test();
        return 0;
    }
    else if (strcmp(cmd, "version") == 0) {
        printf("S300 Application v1.0\n");
        printf("Build: %s %s\n", __DATE__, __TIME__);
        return 0;
    }
    else if (strcmp(cmd, "uptime") == 0) {
        uint32_t uptime = HAL_GetTick() - g_app_start_time;
        printf("Uptime: %lu ms (%.1f seconds)\n", uptime, uptime / 1000.0f);
        return 0;
    }
    else if (strcmp(cmd, "help") == 0) {
        app_print_help();
        return 0;
    }
    
    return -1; // 未知命令
}

/**
 * @brief 打印应用信息
 */
void app_print_info(void) {
    printf("\n[APP] ===== S300 Application Info =====\n");
    printf("Application: S300 Demo App\n");
    printf("Version: 1.0\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("Core: ARM Cortex-M4\n");
    printf("Clock: %lu MHz\n", SystemCoreClock / 1000000);
    printf("SRAM: 384KB\n");
    printf("Flash: 16MB (W25Q128)\n");
    printf("Initialized: %s\n", g_app_initialized ? "Yes" : "No");
    
    uint32_t uptime = HAL_GetTick() - g_app_start_time;
    printf("Uptime: %lu ms\n", uptime);
    printf("=====================================\n\n");
}

/**
 * @brief 打印内存信息
 */
void app_print_memory_info(void) {
    printf("\n[APP] ===== Memory Information =====\n");
    
    // 简单的堆栈使用检查
    extern uint32_t _sstack, _estack;
    uint32_t stack_size = (uint32_t)&_estack - (uint32_t)&_sstack;
    
    printf("Stack Size: %lu bytes\n", stack_size);
    printf("Stack Start: 0x%08lX\n", (uint32_t)&_sstack);
    printf("Stack End: 0x%08lX\n", (uint32_t)&_estack);
    
    // 获取当前堆栈指针
    uint32_t sp;
    __asm volatile ("mov %0, sp" : "=r" (sp));
    printf("Current SP: 0x%08lX\n", sp);
    printf("Stack Used: %lu bytes\n", (uint32_t)&_estack - sp);
    printf("Stack Free: %lu bytes\n", sp - (uint32_t)&_sstack);
    
    printf("===================================\n\n");
}

/**
 * @brief 运行自检测试
 */
void app_run_self_test(void) {
    printf("\n[APP] ===== Self Test =====\n");
    
    int error_count = 0;
    
    // 测试1: 软件复位模块
    printf("1. Testing software reset module...\n");
    if (software_reset_init() == 0) {
        printf("   ✓ Software reset module OK\n");
    } else {
        printf("   ✗ Software reset module FAIL\n");
        error_count++;
    }
    
    // 测试2: 系统时钟
    printf("2. Testing system clock...\n");
    if (SystemCoreClock > 0) {
        printf("   ✓ System clock OK (%lu Hz)\n", SystemCoreClock);
    } else {
        printf("   ✗ System clock FAIL\n");
        error_count++;
    }
    
    // 测试3: UART功能
    printf("3. Testing UART...\n");
    printf("   ✓ UART output OK (you can see this message)\n");
    
    // 测试4: Flash访问
    printf("4. Testing Flash access...\n");
    software_reset_status_t status;
    if (software_reset_get_status(&status) == 0) {
        printf("   ✓ Flash access OK\n");
    } else {
        printf("   ✗ Flash access FAIL\n");
        error_count++;
    }
    
    printf("\nTest Summary: %d errors\n", error_count);
    if (error_count == 0) {
        printf("✅ All tests PASSED\n");
    } else {
        printf("❌ %d tests FAILED\n", error_count);
    }
    printf("==========================\n\n");
}

/**
 * @brief 打印帮助信息
 */
void app_print_help(void) {
    printf("\n[APP] ===== Available Commands =====\n");
    printf("Application Commands:\n");
    printf("  info          - Show application information\n");
    printf("  mem           - Show memory information\n");
    printf("  test          - Run self test\n");
    printf("  version       - Show version\n");
    printf("  uptime        - Show uptime\n");
    printf("  help          - Show this help\n");
    printf("\nSoftware Reset Commands:\n");
    printf("  reset         - Immediate system reset\n");
    printf("  download      - Enter download mode\n");
    printf("  bootloader    - Enter bootloader mode\n");
    printf("  dfu           - Enter DFU mode\n");
    printf("  status        - Show reset status\n");
    printf("  clear         - Clear download flag\n");
    printf("===================================\n\n");
}

/* ================================ 启动管理 ================================ */

/**
 * @brief 延迟报告启动成功
 * @param param 参数（未使用）
 */
void app_delayed_boot_success_task(void *param) {
    (void)param;
    
    printf("[APP] Boot success monitoring started\n");
    
    // 等待5秒确保系统稳定运行
    HAL_Delay(5000);
    
    // 报告启动成功
    software_reset_update_boot_counter(true);
    printf("[APP] ✅ Boot success reported\n");
    
    // 任务结束（如果使用RTOS）
    // vTaskDelete(NULL);
}

/**
 * @brief 应用程序初始化
 */
int app_software_reset_init(void) {
    printf("\n[APP] Initializing software reset functionality...\n");
    
    g_app_start_time = HAL_GetTick();
    
    // 初始化软件复位模块
    if (software_reset_init() != 0) {
        printf("[APP] ❌ Software reset init failed\n");
        return -1;
    }
    
    // 创建延迟报告启动成功的任务
    // 如果使用裸机系统，可以用定时器或者在主循环中处理
    printf("[APP] Setting up boot success monitoring...\n");
    
    g_app_initialized = true;
    
    printf("[APP] ✅ Software reset functionality initialized\n");
    software_reset_print_status();
    
    return 0;
}

/* ================================ 异常处理 ================================ */

/**
 * @brief 应用程序异常处理
 * @param fault_type 故障类型
 */
void app_fault_handler(uint32_t fault_type) {
    printf("\n[APP] ❌ FAULT DETECTED! Type: 0x%08lX\n", fault_type);
    printf("[APP] Reporting boot failure...\n");
    
    // 报告启动失败
    software_reset_update_boot_counter(false);
    
    // 延时后触发恢复
    printf("[APP] Triggering recovery mode in 3 seconds...\n");
    HAL_Delay(3000);
    
    software_reset_enter_download_mode(RESET_REASON_APP_FAILURE);
    
    // 不会执行到这里
}

/**
 * @brief 看门狗超时处理
 */
void app_watchdog_timeout_handler(void) {
    printf("\n[APP] ❌ WATCHDOG TIMEOUT!\n");
    printf("[APP] System appears to be hung, triggering recovery\n");
    
    // 报告启动失败
    software_reset_update_boot_counter(false);
    
    // 触发看门狗复位恢复
    software_reset_enter_download_mode(RESET_REASON_WATCHDOG);
}

/* ================================ 主循环集成 ================================ */

/**
 * @brief 应用主循环中的软件复位处理
 * 定期检查是否需要报告启动成功
 */
void app_software_reset_loop_handler(void) {
    static bool boot_success_reported = false;
    static uint32_t last_check_time = 0;
    
    uint32_t current_time = HAL_GetTick();
    
    // 每1秒检查一次
    if (current_time - last_check_time >= 1000) {
        last_check_time = current_time;
        
        // 如果运行超过5秒且未报告过启动成功，则报告
        if (!boot_success_reported && (current_time - g_app_start_time >= 5000)) {
            printf("[APP] Auto-reporting boot success (5s elapsed)\n");
            software_reset_update_boot_counter(true);
            boot_success_reported = true;
        }
    }
}

/* ================================ 网络/蓝牙集成 ================================ */

#ifdef CONFIG_NETWORK_SUPPORT
/**
 * @brief HTTP API处理 - 系统控制
 */
void app_http_system_control_handler(const char *action) {
    if (!action) return;
    
    printf("[APP] HTTP system control: %s\n", action);
    
    if (strcmp(action, "reset") == 0) {
        printf("[APP] HTTP triggered reset\n");
        HAL_Delay(1000);
        software_reset_system_now();
    }
    else if (strcmp(action, "download") == 0) {
        printf("[APP] HTTP triggered download mode\n");
        HAL_Delay(1000);
        software_reset_enter_download_mode(RESET_REASON_REMOTE_CMD);
    }
    else if (strcmp(action, "status") == 0) {
        software_reset_print_status();
    }
}
#endif

#ifdef CONFIG_BLUETOOTH_SUPPORT
/**
 * @brief 蓝牙命令处理
 */
void app_bluetooth_command_handler(const char *command) {
    if (!command) return;
    
    printf("[APP] Bluetooth command: %s\n", command);
    
    if (strcmp(command, "RESET") == 0) {
        printf("[APP] Bluetooth triggered reset\n");
        HAL_Delay(500);
        software_reset_system_now();
    }
    else if (strcmp(command, "DOWNLOAD") == 0) {
        printf("[APP] Bluetooth triggered download mode\n");
        HAL_Delay(500);
        software_reset_enter_download_mode(RESET_REASON_REMOTE_CMD);
    }
    else if (strcmp(command, "STATUS") == 0) {
        software_reset_print_status();
    }
}
#endif
