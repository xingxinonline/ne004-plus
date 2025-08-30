/**
 * @file chip_detection_impl.c
 * @brief 串口芯片检测主要实现函数
 * @version 2.0
 * @date 2025-08-30
 */

#include "chip_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include <glob.h>

/* ================================
 * 主要API实现
 * ================================ */

/**
 * @brief 自动检测串口设备并识别芯片类型
 */
int chip_detect_auto(const char *port_pattern, uart_detection_result_t *result) {
    if (!result) return CHIP_DETECT_ERR_INVALID;
    
    memset(result, 0, sizeof(*result));
    
    char ports[MAX_SERIAL_PORTS][32];
    int port_count = chip_list_serial_ports(ports, MAX_SERIAL_PORTS);
    
    if (port_count <= 0) {
        printf("[CHIP_DETECT] No serial ports found\n");
        return CHIP_DETECT_ERR_PORT;
    }
    
    printf("[CHIP_DETECT] Found %d serial ports, scanning...\n", port_count);
    
    // 遍历所有串口进行检测
    for (int i = 0; i < port_count; i++) {
        printf("[CHIP_DETECT] Trying port: %s\n", ports[i]);
        
        int ret = chip_detect_port(ports[i], 0, result);
        if (ret == CHIP_DETECT_OK && result->detected) {
            printf("[CHIP_DETECT] ✓ Detected chip on %s\n", ports[i]);
            return CHIP_DETECT_OK;
        }
        
        // 短暂延迟避免端口冲突
        usleep(100000); // 100ms
    }
    
    printf("[CHIP_DETECT] No chips detected on any port\n");
    return CHIP_DETECT_ERR_NO_RESPONSE;
}

/**
 * @brief 检测指定串口的芯片信息
 */
int chip_detect_port(const char *port, uint32_t baud_rate, uart_detection_result_t *result) {
    if (!port || !result) return CHIP_DETECT_ERR_INVALID;
    
    memset(result, 0, sizeof(*result));
    strncpy(result->port_name, port, sizeof(result->port_name) - 1);
    
    // 如果未指定波特率，自动检测
    if (baud_rate == 0) {
        baud_rate = detect_baud_rate(port);
        if (baud_rate == 0) {
            baud_rate = 115200; // 默认波特率
        }
    }
    
    result->baud_rate = baud_rate;
    
    int fd = serial_open(port, baud_rate);
    if (fd < 0) {
        printf("[CHIP_DETECT] Failed to open port %s\n", port);
        return CHIP_DETECT_ERR_PORT;
    }
    
    // 尝试多种检测方法
    int ret = CHIP_DETECT_ERR_NO_RESPONSE;
    
    // 1. 尝试ESP32检测
    ret = detect_esp32_chip(fd, &result->chip);
    if (ret == CHIP_DETECT_OK) {
        result->detected = true;
        goto cleanup;
    }
    
    // 2. 尝试AT命令检测
    ret = detect_at_chip(fd, &result->chip);
    if (ret == CHIP_DETECT_OK) {
        result->detected = true;
        goto cleanup;
    }
    
    // 3. 尝试S300检测
    ret = detect_s300_chip(fd, &result->chip);
    if (ret == CHIP_DETECT_OK) {
        result->detected = true;
        goto cleanup;
    }
    
cleanup:
    serial_close(fd);
    return ret;
}

/**
 * @brief ESP32兼容的芯片信息获取
 */
int chip_detect_esp32_compatible(const char *port, uart_detection_result_t *result) {
    if (!port || !result) return CHIP_DETECT_ERR_INVALID;
    
    // 专门针对ESP32的检测流程
    int fd = serial_open(port, 115200);
    if (fd < 0) return CHIP_DETECT_ERR_PORT;
    
    memset(result, 0, sizeof(*result));
    strncpy(result->port_name, port, sizeof(result->port_name) - 1);
    result->baud_rate = 115200;
    
    // ESP32进入下载模式的序列
    serial_set_dtr_rts(fd, false, true);  // DTR=0, RTS=1
    usleep(100000);                        // 100ms
    serial_set_dtr_rts(fd, true, false);   // DTR=1, RTS=0
    usleep(50000);                         // 50ms
    serial_set_dtr_rts(fd, false, false);  // DTR=0, RTS=0
    
    int ret = detect_esp32_chip(fd, &result->chip);
    if (ret == CHIP_DETECT_OK) {
        result->detected = true;
    }
    
    serial_close(fd);
    return ret;
}

/**
 * @brief 检测S300芯片信息 (本地)
 */
int chip_detect_s300_local(chip_info_t *info) {
    if (!info) return CHIP_DETECT_ERR_INVALID;
    
    memset(info, 0, sizeof(*info));
    
    // S300芯片本地信息
    info->type = CHIP_TYPE_S300;
    strncpy(info->name, "PiMCHIP S300", sizeof(info->name) - 1);
    strncpy(info->family, "PiMCHIP", sizeof(info->family) - 1);
    strncpy(info->version, "1.0", sizeof(info->version) - 1);
    
    // 从系统寄存器读取信息 (这里使用模拟值)
    info->chip_id = 0x530001; // S300芯片ID
    info->flash_size = 16 * 1024 * 1024; // 16MB
    info->ram_size = 384 * 1024; // 384KB
    info->freq = 200000000; // 200MHz
    info->bootloader_mode = true; // RBL模式
    
    return CHIP_DETECT_OK;
}

/**
 * @brief 列出所有可用的串口
 */
int chip_list_serial_ports(char ports[][32], int max_ports) {
    if (!ports || max_ports <= 0) return CHIP_DETECT_ERR_INVALID;
    
    int count = 0;
    
#ifdef __linux__
    // Linux: 检查/dev/ttyUSB*, /dev/ttyACM*, /dev/ttyS*
    const char *patterns[] = {
        "/dev/ttyUSB*",
        "/dev/ttyACM*", 
        "/dev/ttyS*"
    };
    
    for (int i = 0; i < 3 && count < max_ports; i++) {
        glob_t glob_result;
        if (glob(patterns[i], GLOB_NOSORT, NULL, &glob_result) == 0) {
            for (size_t j = 0; j < glob_result.gl_pathc && count < max_ports; j++) {
                strncpy(ports[count], glob_result.gl_pathv[j], 31);
                ports[count][31] = '\0';
                count++;
            }
            globfree(&glob_result);
        }
    }
#elif defined(_WIN32)
    // Windows: 检查COM1-COM32
    for (int i = 1; i <= 32 && count < max_ports; i++) {
        char port_name[32];
        snprintf(port_name, sizeof(port_name), "COM%d", i);
        
        // 尝试打开端口验证存在性
        HANDLE h = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            strncpy(ports[count], port_name, 31);
            ports[count][31] = '\0';
            count++;
        }
    }
#elif defined(__APPLE__)
    // macOS: 检查/dev/cu.*
    glob_t glob_result;
    if (glob("/dev/cu.*", GLOB_NOSORT, NULL, &glob_result) == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc && count < max_ports; i++) {
            strncpy(ports[count], glob_result.gl_pathv[i], 31);
            ports[count][31] = '\0';
            count++;
        }
        globfree(&glob_result);
    }
#endif
    
    return count;
}

/**
 * @brief 打印芯片检测结果 (ESP32兼容格式)
 */
void chip_print_detection_result(const uart_detection_result_t *result) {
    if (!result) return;
    
    printf("\n=== Chip Detection Result ===\n");
    
    if (!result->detected) {
        printf("Status: No chip detected on %s\n", result->port_name);
        return;
    }
    
    const chip_info_t *chip = &result->chip;
    
    printf("Port: %s @ %u baud\n", result->port_name, result->baud_rate);
    printf("Chip Type: %s\n", chip_get_type_string(chip->type));
    printf("Chip Name: %s\n", chip->name);
    printf("Chip Family: %s\n", chip->family);
    
    if (chip->chip_id != 0) {
        printf("Chip ID: 0x%08X\n", chip->chip_id);
    }
    
    if (chip->flash_size > 0) {
        printf("Flash Size: %u KB\n", chip->flash_size / 1024);
    }
    
    if (chip->ram_size > 0) {
        printf("RAM Size: %u KB\n", chip->ram_size / 1024);
    }
    
    if (chip->freq > 0) {
        printf("CPU Frequency: %u MHz\n", chip->freq / 1000000);
    }
    
    if (strlen(chip->version) > 0) {
        printf("Version: %s\n", chip->version);
    }
    
    if (strlen(chip->mac_addr) > 0) {
        printf("MAC Address: %s\n", chip->mac_addr);
    }
    
    printf("Bootloader Mode: %s\n", chip->bootloader_mode ? "Yes" : "No");
    printf("==============================\n");
}

/**
 * @brief 获取芯片类型字符串
 */
const char *chip_get_type_string(chip_type_t type) {
    if (type >= CHIP_TYPE_COUNT) return "Invalid";
    return chip_type_strings[type];
}

/**
 * @brief 获取错误描述
 */
const char *chip_get_error_string(int error_code) {
    switch (error_code) {
        case CHIP_DETECT_OK:              return "Success";
        case CHIP_DETECT_ERR_PORT:        return "Serial port error";
        case CHIP_DETECT_ERR_TIMEOUT:     return "Communication timeout";
        case CHIP_DETECT_ERR_NO_RESPONSE: return "No response from device";
        case CHIP_DETECT_ERR_PARSE:       return "Response parse error";
        case CHIP_DETECT_ERR_UNSUPPORTED: return "Unsupported chip type";
        case CHIP_DETECT_ERR_INVALID:     return "Invalid parameters";
        default:                          return "Unknown error";
    }
}

/* ================================
 * 内部检测函数实现
 * ================================ */

/**
 * @brief 检测ESP32芯片
 */
static int detect_esp32_chip(int fd, chip_info_t *info) {
    if (fd < 0 || !info) return CHIP_DETECT_ERR_INVALID;
    
    serial_flush(fd);
    
    // 发送ESP32 SYNC命令
    if (serial_write_data(fd, ESP32_SYNC_CMD, 12) != 12) {
        return CHIP_DETECT_ERR_PORT;
    }
    
    char response[MAX_RESPONSE_SIZE];
    int len = serial_read_data(fd, response, sizeof(response), ESP32_SYNC_TIMEOUT_MS);
    
    if (len <= 0) {
        return CHIP_DETECT_ERR_NO_RESPONSE;
    }
    
    // 检查是否收到ESP32响应
    if (strstr(response, ESP32_SYNC_RESP) != NULL) {
        return parse_esp32_response(response, len, info);
    }
    
    return CHIP_DETECT_ERR_PARSE;
}

/**
 * @brief 检测AT命令芯片 (ESP8266等)
 */
static int detect_at_chip(int fd, chip_info_t *info) {
    if (fd < 0 || !info) return CHIP_DETECT_ERR_INVALID;
    
    serial_flush(fd);
    
    // 发送AT命令
    const char *at_commands[] = {
        "AT\r\n",
        "AT+GMR\r\n",     // 获取版本信息
        "AT+CHIPID\r\n",  // 获取芯片ID
        NULL
    };
    
    for (int i = 0; at_commands[i]; i++) {
        serial_write_data(fd, at_commands[i], strlen(at_commands[i]));
        
        char response[512];
        int len = serial_read_data(fd, response, sizeof(response), 1000);
        
        if (len > 0 && (strstr(response, "OK") || strstr(response, "ESP"))) {
            // 解析AT响应
            memset(info, 0, sizeof(*info));
            
            if (strstr(response, "ESP8266")) {
                info->type = CHIP_TYPE_ESP8266;
                strncpy(info->name, "ESP8266", sizeof(info->name) - 1);
                strncpy(info->family, "ESP8266", sizeof(info->family) - 1);
            } else if (strstr(response, "ESP32")) {
                info->type = CHIP_TYPE_ESP32;
                strncpy(info->name, "ESP32", sizeof(info->name) - 1);
                strncpy(info->family, "ESP32", sizeof(info->family) - 1);
            } else {
                info->type = CHIP_TYPE_UNKNOWN;
                strncpy(info->name, "AT Compatible", sizeof(info->name) - 1);
            }
            
            info->bootloader_mode = false;
            return CHIP_DETECT_OK;
        }
    }
    
    return CHIP_DETECT_ERR_NO_RESPONSE;
}

/**
 * @brief 检测S300芯片
 */
static int detect_s300_chip(int fd, chip_info_t *info) {
    if (fd < 0 || !info) return CHIP_DETECT_ERR_INVALID;
    
    serial_flush(fd);
    
    // 发送S300特有的命令
    const char *s300_commands[] = {
        "INFO\r\n",
        "VERSION\r\n",
        "CHIPID\r\n",
        NULL
    };
    
    for (int i = 0; s300_commands[i]; i++) {
        serial_write_data(fd, s300_commands[i], strlen(s300_commands[i]));
        
        char response[512];
        int len = serial_read_data(fd, response, sizeof(response), 1000);
        
        if (len > 0 && (strstr(response, "S300") || strstr(response, "PiMCHIP"))) {
            // 检测到S300响应
            memset(info, 0, sizeof(*info));
            info->type = CHIP_TYPE_S300;
            strncpy(info->name, "PiMCHIP S300", sizeof(info->name) - 1);
            strncpy(info->family, "PiMCHIP", sizeof(info->family) - 1);
            info->bootloader_mode = true;
            
            return CHIP_DETECT_OK;
        }
    }
    
    return CHIP_DETECT_ERR_NO_RESPONSE;
}

/**
 * @brief 解析ESP32响应
 */
static int parse_esp32_response(const char *response, int len, chip_info_t *info) {
    if (!response || !info || len <= 0) return CHIP_DETECT_ERR_INVALID;
    
    memset(info, 0, sizeof(*info));
    
    // 基础ESP32信息
    info->type = CHIP_TYPE_ESP32;
    strncpy(info->name, "ESP32", sizeof(info->name) - 1);
    strncpy(info->family, "ESP32", sizeof(info->family) - 1);
    info->bootloader_mode = true;
    
    // 尝试从响应中提取更多信息
    // 这里需要根据实际的ESP32响应格式进行解析
    
    return CHIP_DETECT_OK;
}

/**
 * @brief 自动检测波特率
 */
static uint32_t detect_baud_rate(const char *port) {
    for (int i = 0; i < chip_common_baud_rates_count; i++) {
        int fd = serial_open(port, chip_common_baud_rates[i]);
        if (fd >= 0) {
            // 发送测试命令
            serial_write_data(fd, "AT\r\n", 4);
            
            char response[64];
            int len = serial_read_data(fd, response, sizeof(response), 500);
            
            serial_close(fd);
            
            if (len > 0) {
                return chip_common_baud_rates[i];
            }
        }
    }
    
    return 0; // 检测失败
}
