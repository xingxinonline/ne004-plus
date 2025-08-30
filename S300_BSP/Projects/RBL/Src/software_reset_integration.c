/**
 * @file software_reset_integration.c
 * @brief S300软件复位模块集成示例
 * @version 1.0
 * @date 2024
 * 
 * 此文件展示如何将软件复位模块集成到RBL和应用程序中
 */

#include "software_reset.h"
#include "boot_mode.h"
#include "uart.h"
#include "rcc.h"
#include <stdio.h>
#include <string.h>

/* ============================= RBL集成 ============================= */

/**
 * @brief RBL启动模式检测 - 包含软件复位检测
 * @return 启动模式
 */
boot_mode_t rbl_detect_boot_mode_with_software_reset(void) {
    printf("\\n");
    printf("[RBL] ==========================================\\n");
    printf("[RBL] S300 Enhanced Boot Mode Detection v2.0\\n");
    printf("[RBL] Built: %s %s\\n", __DATE__, __TIME__);
    printf("[RBL] ==========================================\\n");
    
    reset_reason_t reason;
    
    // 初始化软件复位模块
    if (software_reset_init() != 0) {
        printf("[RBL] Warning: Software reset init failed\\n");
    }
    
    // 1. 软件下载标志检测（最高优先级）
    if (software_reset_check_download_flag(&reason)) {
        printf("[RBL] ✓ Software download flag detected\\n");
        printf("[RBL]   Reason: %s\\n", software_reset_get_reason_string(reason));
        
        // 清除标志避免循环重启
        software_reset_clear_download_flag();
        
        switch (reason) {
            case RESET_REASON_USER_CMD:
                printf("[RBL] → User command triggered download\\n");
                return BOOT_MODE_DOWNLOAD_USER;
                
            case RESET_REASON_DOUBLE_RESET:
                printf("[RBL] → Double reset pattern detected\\n");
                return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
                
            case RESET_REASON_APP_FAILURE:
                printf("[RBL] → Application failure recovery\\n");
                return BOOT_MODE_RECOVERY;
                
            case RESET_REASON_REMOTE_CMD:
                printf("[RBL] → Remote command triggered\\n");
                return BOOT_MODE_DOWNLOAD_REMOTE;
                
            default:
                printf("[RBL] → Software download mode\\n");
                return BOOT_MODE_DOWNLOAD_SOFTWARE;
        }
    }
    
    // 2. 双重启检测
    if (software_reset_check_double_reset()) {
        printf("[RBL] ✓ Double reset pattern detected\\n");
        return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
    }
    
    // 3. 故障恢复检测
    if (software_reset_check_boot_failure()) {
        printf("[RBL] ✓ Boot failure recovery needed\\n");
        return BOOT_MODE_RECOVERY;
    }
    
    // 4. 传统串口窗口检测
    if (rbl_check_enhanced_serial_window()) {
        printf("[RBL] ✓ Serial download window triggered\\n");
        return BOOT_MODE_DOWNLOAD_SERIAL;
    }
    
    // 5. GPIO下载引脚检测
    if (rbl_check_download_pin()) {
        printf("[RBL] ✓ Hardware download pin active\\n");
        return BOOT_MODE_DOWNLOAD_GPIO;
    }
    
    // 6. 正常启动
    printf("[RBL] → Normal boot mode selected\\n");
    return BOOT_MODE_NORMAL;
}

/**
 * @brief RBL主函数 - 集成软件复位检测
 */
void rbl_main_with_software_reset(void) {
    printf("\\n[RBL] S300 Robust Boot Loader v2.0\\n");
    
    // 系统初始化
    rcc_init();
    uart_init();
    qspi_flash_init();
    
    // 检测启动模式
    boot_mode_t boot_mode = rbl_detect_boot_mode_with_software_reset();
    
    switch (boot_mode) {
        case BOOT_MODE_DOWNLOAD_SOFTWARE:
        case BOOT_MODE_DOWNLOAD_USER:
        case BOOT_MODE_DOWNLOAD_DOUBLE_RESET:
        case BOOT_MODE_DOWNLOAD_REMOTE:
        case BOOT_MODE_DOWNLOAD_SERIAL:
        case BOOT_MODE_DOWNLOAD_GPIO:
            printf("[RBL] Entering download mode...\\n");
            rbl_enter_download_mode();
            break;
            
        case BOOT_MODE_RECOVERY:
            printf("[RBL] Entering recovery mode...\\n");
            rbl_enter_recovery_mode();
            break;
            
        case BOOT_MODE_NORMAL:
        default:
            printf("[RBL] Loading application...\\n");
            rbl_load_application();
            break;
    }
}

/* =========================== 应用程序集成 =========================== */

/**
 * @brief 应用程序启动成功报告任务
 * @param param 任务参数
 */
void app_boot_success_task(void *param) {
    printf("[APP] Boot success monitoring started\\n");
    
    // 等待5秒确保系统稳定运行
    rcc_delay_ms(5000);
    
    // 报告启动成功
    software_reset_update_boot_counter(true);
    printf("[APP] Boot success reported to reset manager\\n");
    
    // 任务结束
    vTaskDelete(NULL);
}

/**
 * @brief 应用程序命令处理任务
 * @param param 任务参数
 */
void app_command_task(void *param) {
    char cmd_buffer[64];
    
    printf("[APP] Command processor started\\n");
    printf("[APP] Available commands: reset, download, status, help\\n");
    
    while (1) {
        // 读取串口命令
        if (uart_read_line(cmd_buffer, sizeof(cmd_buffer)) > 0) {
            printf("[APP] Command received: %s\\n", cmd_buffer);
            
            // 先尝试软件复位命令
            if (software_reset_handle_command(cmd_buffer) == 0) {
                printf("[APP] Reset command processed\\n");
                continue;
            }
            
            // 处理其他应用命令
            if (strcmp(cmd_buffer, "info") == 0) {
                app_print_info();
            }
            else if (strcmp(cmd_buffer, "mem") == 0) {
                app_print_memory_info();
            }
            else if (strcmp(cmd_buffer, "test") == 0) {
                app_run_self_test();
            }
            else {
                printf("[APP] Unknown command: %s\\n", cmd_buffer);
                printf("[APP] Type 'help' for available commands\\n");
            }
        }
        
        rcc_delay_ms(100);
    }
}

/**
 * @brief 看门狗超时处理函数
 */
void app_watchdog_timeout_handler(void) {
    printf("[APP] WATCHDOG TIMEOUT!\\n");
    printf("[APP] System appears to be hung, triggering recovery\\n");
    
    // 触发看门狗复位恢复
    software_reset_enter_download_mode(RESET_REASON_WATCHDOG);
}

/**
 * @brief 应用程序异常处理
 * @param fault_type 故障类型
 */
void app_fault_handler(uint32_t fault_type) {
    printf("[APP] FAULT DETECTED! Type: 0x%08lX\\n", fault_type);
    
    // 报告启动失败
    software_reset_update_boot_counter(false);
    
    // 延时后触发恢复
    rcc_delay_ms(1000);
    software_reset_enter_download_mode(RESET_REASON_APP_FAILURE);
}

/**
 * @brief 应用程序主函数 - 集成软件复位
 */
void app_main_with_software_reset(void) {
    printf("\\n[APP] S300 Application v1.0\\n");
    
    // 初始化软件复位模块
    if (software_reset_init() != 0) {
        printf("[APP] Warning: Software reset init failed\\n");
    }
    
    // 创建启动成功监控任务
    if (xTaskCreate(app_boot_success_task, "boot_success", 
                   configMINIMAL_STACK_SIZE, NULL, 1, NULL) != pdPASS) {
        printf("[APP] Failed to create boot success task\\n");
    }
    
    // 创建命令处理任务
    if (xTaskCreate(app_command_task, "command", 
                   configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL) != pdPASS) {
        printf("[APP] Failed to create command task\\n");
    }
    
    // 安装故障处理函数
    register_fault_handler(app_fault_handler);
    register_watchdog_handler(app_watchdog_timeout_handler);
    
    printf("[APP] System initialization complete\\n");
    software_reset_print_status();
    
    // 主业务循环
    while (1) {
        // 执行主要业务逻辑
        app_business_logic();
        
        // 喂看门狗
        watchdog_feed();
        
        // 短暂延时
        rcc_delay_ms(100);
    }
}

/* ========================== 网络集成示例 ========================== */

#ifdef CONFIG_NETWORK_SUPPORT

/**
 * @brief HTTP API处理 - 系统控制
 * @param request HTTP请求
 * @param response HTTP响应
 */
void http_api_system_control(http_request_t *request, http_response_t *response) {
    if (strstr(request->body, "action=reset")) {
        http_send_response(response, 200, "System resetting...");
        rcc_delay_ms(1000);
        software_reset_system_now();
    }
    else if (strstr(request->body, "action=download")) {
        http_send_response(response, 200, "Entering download mode...");
        rcc_delay_ms(1000);
        software_reset_enter_download_mode(RESET_REASON_REMOTE_CMD);
    }
    else if (strstr(request->body, "action=status")) {
        software_reset_status_t status;
        if (software_reset_get_status(&status) == 0) {
            char json_response[256];
            snprintf(json_response, sizeof(json_response),
                    "{"
                    "\\"total_boots\\": %lu,"
                    "\\"failure_count\\": %lu,"
                    "\\"last_reason\\": \\"%s\\","
                    "\\"download_active\\": %s"
                    "}",
                    status.total_boots,
                    status.failure_count,
                    software_reset_get_reason_string(status.last_reason),
                    status.download_flag_active ? "true" : "false");
            http_send_response(response, 200, json_response);
        } else {
            http_send_response(response, 500, "Failed to get status");
        }
    }
    else {
        http_send_response(response, 400, "Invalid action");
    }
}

#endif /* CONFIG_NETWORK_SUPPORT */

/* ========================== 蓝牙集成示例 ========================== */

#ifdef CONFIG_BLUETOOTH_SUPPORT

/**
 * @brief 蓝牙命令处理
 * @param command 收到的命令
 */
void bt_command_handler(const char *command) {
    printf("[BT] Command received: %s\\n", command);
    
    if (strcmp(command, "RESET") == 0) {
        bt_send_response("RESETTING");
        rcc_delay_ms(500);
        software_reset_system_now();
    }
    else if (strcmp(command, "DOWNLOAD") == 0) {
        bt_send_response("ENTERING_DOWNLOAD_MODE");
        rcc_delay_ms(500);
        software_reset_enter_download_mode(RESET_REASON_REMOTE_CMD);
    }
    else if (strcmp(command, "STATUS") == 0) {
        software_reset_status_t status;
        if (software_reset_get_status(&status) == 0) {
            char response[128];
            snprintf(response, sizeof(response),
                    "BOOTS:%lu,FAILURES:%lu,REASON:%s",
                    status.total_boots,
                    status.failure_count,
                    software_reset_get_reason_string(status.last_reason));
            bt_send_response(response);
        } else {
            bt_send_response("STATUS_ERROR");
        }
    }
    else {
        bt_send_response("UNKNOWN_COMMAND");
    }
}

#endif /* CONFIG_BLUETOOTH_SUPPORT */

/* ========================== 调试和测试 ========================== */

/**
 * @brief 软件复位功能测试
 */
void software_reset_test_suite(void) {
    printf("\\n[TEST] Software Reset Test Suite\\n");
    printf("==================================\\n");
    
    // 初始化测试
    printf("1. Initializing module...\\n");
    int init_result = software_reset_init();
    printf("   Result: %s\\n", init_result == 0 ? "PASS" : "FAIL");
    
    // 状态测试
    printf("2. Status check...\\n");
    software_reset_print_status();
    
    // 标志写入读取测试
    printf("3. Flag write/read test...\\n");
    software_reset_enter_download_mode(RESET_REASON_USER_CMD);
    // 这里不会执行到，因为会复位
}

/**
 * @brief 模拟应用故障进行测试
 */
void simulate_app_failure_test(void) {
    printf("[TEST] Simulating application failure...\\n");
    
    // 模拟3次启动失败
    for (int i = 0; i < 3; i++) {
        printf("[TEST] Simulating boot failure %d/3\\n", i + 1);
        software_reset_update_boot_counter(false);
        rcc_delay_ms(1000);
    }
    
    // 下一次失败应该触发恢复模式
    printf("[TEST] Next failure should trigger recovery...\\n");
    software_reset_update_boot_counter(false);
}

/**
 * @brief 双重启测试
 */
void double_reset_test(void) {
    printf("[TEST] Double reset test - Press reset twice within 2 seconds\\n");
    
    // 第一次复位
    printf("[TEST] First reset...\\n");
    software_reset_system_now();
}

/* ========================== 生产测试工具 ========================== */

/**
 * @brief 生产测试 - 验证软件复位功能
 * @return 0=测试通过, <0=测试失败
 */
int production_test_software_reset(void) {
    printf("\\n[PROD] Software Reset Production Test\\n");
    printf("=====================================\\n");
    
    int error_count = 0;
    
    // 测试1: 模块初始化
    if (software_reset_init() != 0) {
        printf("[PROD] FAIL: Module initialization\\n");
        error_count++;
    } else {
        printf("[PROD] PASS: Module initialization\\n");
    }
    
    // 测试2: Flash读写
    download_flag_t test_flag = {
        .magic = DOWNLOAD_FLAG_MAGIC,
        .reason = RESET_REASON_USER_CMD,
        .timestamp = 12345,
        .retry_count = 0,
        .user_data = 0xABCD
    };
    
    // 这里应该有Flash写入读取验证逻辑
    // ...
    
    // 测试3: 状态查询
    software_reset_status_t status;
    if (software_reset_get_status(&status) != 0) {
        printf("[PROD] FAIL: Status query\\n");
        error_count++;
    } else {
        printf("[PROD] PASS: Status query\\n");
    }
    
    printf("\\n[PROD] Test Summary: %d errors\\n", error_count);
    return error_count == 0 ? 0 : -1;
}
