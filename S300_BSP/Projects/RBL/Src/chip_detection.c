/**
 * @file chip_detection.c
 * @brief 串口芯片检测和识别功能实现 - ESP32兼容
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

/* 内部常量 */
#define MAX_RESPONSE_SIZE       2048
#define DEFAULT_TIMEOUT_MS      3000
#define ESP32_SYNC_TIMEOUT_MS   1000
#define DETECTION_RETRIES       3
#define MAX_SERIAL_PORTS        16

/* ESP32特有的命令和响应 */
#define ESP32_SYNC_CMD          "\x00\x08\x24\x00\x00\x00\x00\x00\x07\x07\x12\x20"
#define ESP32_SYNC_RESP         "OHAI"
#define ESP32_CHIP_ID_CMD       "\x00\x0a\x24\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define ESP32_MAC_CMD           "\x00\x09\x24\x00\x00\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00"

/* 常用波特率 */
const uint32_t chip_common_baud_rates[] = {
    115200, 921600, 460800, 230400, 74880, 57600, 38400, 19200, 9600
};
const int chip_common_baud_rates_count = sizeof(chip_common_baud_rates) / sizeof(chip_common_baud_rates[0]);

/* 内部函数声明 */
static int serial_open(const char *port, uint32_t baud_rate);
static void serial_close(int fd);
static int serial_write_data(int fd, const char *data, int len);
static int serial_read_data(int fd, char *buffer, int size, int timeout_ms);
static void serial_flush(int fd);
static int serial_set_dtr_rts(int fd, bool dtr, bool rts);
static int detect_esp32_chip(int fd, chip_info_t *info);
static int detect_at_chip(int fd, chip_info_t *info);
static int detect_s300_chip(int fd, chip_info_t *info);
static int parse_esp32_response(const char *response, int len, chip_info_t *info);
static uint32_t detect_baud_rate(const char *port);

/* 芯片类型字符串映射 */
static const char *chip_type_strings[] = {
    [CHIP_TYPE_UNKNOWN]   = "Unknown",
    [CHIP_TYPE_S300]      = "PiMCHIP S300 (Cortex-M4)",
    [CHIP_TYPE_ESP32]     = "ESP32",
    [CHIP_TYPE_ESP32_C3]  = "ESP32-C3", 
    [CHIP_TYPE_ESP32_S3]  = "ESP32-S3",
    [CHIP_TYPE_ESP32_C6]  = "ESP32-C6",
    [CHIP_TYPE_ESP8266]   = "ESP8266",
    [CHIP_TYPE_STM32F4]   = "STM32F4xx",
    [CHIP_TYPE_STM32H7]   = "STM32H7xx",
    [CHIP_TYPE_GD32]      = "GD32 Series",
    [CHIP_TYPE_CH32]      = "CH32 Series",
    [CHIP_TYPE_RISC_V]    = "RISC-V",
    [CHIP_TYPE_ARDUINO]   = "Arduino Compatible",
};
    [CHIP_TYPE_ESP32_S3]  = "ESP32",
    [CHIP_TYPE_STM32F4]   = "STM32",
    [CHIP_TYPE_STM32H7]   = "STM32",
    [CHIP_TYPE_GD32]      = "GD32",
    [CHIP_TYPE_CH32]      = "CH32",
    [CHIP_TYPE_RISC_V]    = "RISC-V",
};

/* 默认波特率映射 */
static const int default_baud_rates[] = {
    [CHIP_TYPE_UNKNOWN]   = 115200,
    [CHIP_TYPE_S300]      = 115200,
    [CHIP_TYPE_ESP32]     = 115200,
    [CHIP_TYPE_ESP32_C3]  = 115200,
    [CHIP_TYPE_ESP32_S3]  = 115200,
    [CHIP_TYPE_STM32F4]   = 115200,
    [CHIP_TYPE_STM32H7]   = 115200,
    [CHIP_TYPE_GD32]      = 115200,
    [CHIP_TYPE_CH32]      = 115200,
    [CHIP_TYPE_RISC_V]    = 115200,
};

/**
 * @brief 自动检测芯片类型
 */
int chip_detect_auto(const char *port, chip_info_t *info)
{
    if (!port || !info) {
        return -1;
    }
    
    detect_config_t config = {
        .port = port,
        .baud_rate = 115200,
        .timeout_ms = DEFAULT_TIMEOUT_MS,
        .method = DETECT_METHOD_AUTO,
        .verbose = true
    };
    
    return chip_detect_with_config(&config, info);
}

/**
 * @brief 使用配置检测芯片
 */
int chip_detect_with_config(const detect_config_t *config, chip_info_t *info)
{
    if (!config || !info) {
        return -1;
    }
    
    printf("[DETECT] Starting chip detection on %s\n", config->port);
    
    /* 初始化芯片信息 */
    memset(info, 0, sizeof(chip_info_t));
    info->type = CHIP_TYPE_UNKNOWN;
    
    /* 尝试不同的检测方法 */
    if (config->method == DETECT_METHOD_AUTO) {
        /* 方法1: 串口命令检测 */
        if (detect_by_uart_command(config->port, config->baud_rate, info) == 0) {
            chip_print_detection_log(config->port, DETECT_METHOD_UART_CMD, true);
            return 0;
        }
        
        /* 方法2: 复位序列检测 */
        if (detect_by_reset_sequence(config->port, config->baud_rate, info) == 0) {
            chip_print_detection_log(config->port, DETECT_METHOD_RESET_SEQ, true);
            return 0;
        }
        
        /* 方法3: 特征码检测 */
        if (detect_by_signature(config->port, config->baud_rate, info) == 0) {
            chip_print_detection_log(config->port, DETECT_METHOD_SIGNATURE, true);
            return 0;
        }
        
        /* 方法4: USB描述符检测 */
        if (detect_by_usb_descriptor(config->port, info) == 0) {
            chip_print_detection_log(config->port, DETECT_METHOD_USB_DESC, true);
            return 0;
        }
    } else {
        /* 使用指定方法 */
        switch (config->method) {
            case DETECT_METHOD_UART_CMD:
                return detect_by_uart_command(config->port, config->baud_rate, info);
            case DETECT_METHOD_USB_DESC:
                return detect_by_usb_descriptor(config->port, info);
            case DETECT_METHOD_RESET_SEQ:
                return detect_by_reset_sequence(config->port, config->baud_rate, info);
            case DETECT_METHOD_SIGNATURE:
                return detect_by_signature(config->port, config->baud_rate, info);
            default:
                break;
        }
    }
    
    chip_print_detection_log(config->port, config->method, false);
    return -1;
}

/**
 * @brief 通过串口命令检测芯片
 */
int detect_by_uart_command(const char *port, int baud, chip_info_t *info)
{
    char response[MAX_RESPONSE_SIZE];
    int ret;
    
    printf("[DETECT] Trying UART command detection...\n");
    
    /* S300芯片检测命令 */
    const char *s300_commands[] = {
        "INFO\r\n",
        "VERSION\r\n", 
        "CHIP_ID\r\n",
        "STATUS\r\n"
    };
    
    for (int i = 0; i < sizeof(s300_commands)/sizeof(s300_commands[0]); i++) {
        memset(response, 0, sizeof(response));
        ret = chip_send_command(port, baud, s300_commands[i], response, sizeof(response));
        
        if (ret == 0 && strlen(response) > 0) {
            printf("[DETECT] Got response: %.100s\n", response);
            
            /* 分析响应内容 */
            if (strstr(response, "S300") || strstr(response, "PiMCHIP")) {
                info->type = CHIP_TYPE_S300;
                strcpy(info->name, "PiMCHIP S300");
                strcpy(info->family, "PiMCHIP");
                
                /* 尝试解析更多信息 */
                if (strstr(response, "RBL") || strstr(response, "bootloader")) {
                    info->bootloader_mode = true;
                }
                
                return 0;
            }
            
            /* ESP32检测 */
            if (strstr(response, "ESP32")) {
                if (strstr(response, "ESP32-C3")) {
                    info->type = CHIP_TYPE_ESP32_C3;
                    strcpy(info->name, "ESP32-C3");
                } else if (strstr(response, "ESP32-S3")) {
                    info->type = CHIP_TYPE_ESP32_S3;
                    strcpy(info->name, "ESP32-S3");
                } else {
                    info->type = CHIP_TYPE_ESP32;
                    strcpy(info->name, "ESP32");
                }
                strcpy(info->family, "ESP32");
                return 0;
            }
            
            /* STM32检测 */
            if (strstr(response, "STM32")) {
                if (strstr(response, "STM32F4")) {
                    info->type = CHIP_TYPE_STM32F4;
                    strcpy(info->name, "STM32F4");
                } else if (strstr(response, "STM32H7")) {
                    info->type = CHIP_TYPE_STM32H7;
                    strcpy(info->name, "STM32H7");
                }
                strcpy(info->family, "STM32");
                return 0;
            }
        }
    }
    
    return -1;
}

/**
 * @brief 通过USB描述符检测芯片
 */
int detect_by_usb_descriptor(const char *port, chip_info_t *info)
{
    char cmd[256];
    FILE *fp;
    char buffer[512];
    
    printf("[DETECT] Trying USB descriptor detection...\n");
    
    /* 从串口设备路径推断USB信息 */
    if (strstr(port, "ttyUSB") || strstr(port, "ttyACM")) {
        /* 提取USB设备信息 */
        snprintf(cmd, sizeof(cmd), "lsusb -v 2>/dev/null | grep -A 20 -B 5 'CH340\\|CP210\\|FT232\\|CDC'");
        
        fp = popen(cmd, "r");
        if (fp) {
            while (fgets(buffer, sizeof(buffer), fp)) {
                /* CH340检测 */
                if (strstr(buffer, "CH340")) {
                    info->type = CHIP_TYPE_S300; /* 通常CH340用于S300等国产芯片 */
                    strcpy(info->name, "Unknown with CH340");
                    pclose(fp);
                    return 0;
                }
                
                /* CP2102检测 */
                if (strstr(buffer, "CP210")) {
                    info->type = CHIP_TYPE_ESP32; /* ESP32常用CP2102 */
                    strcpy(info->name, "Unknown with CP2102");
                    pclose(fp);
                    return 0;
                }
                
                /* FTDI检测 */
                if (strstr(buffer, "FT232")) {
                    info->type = CHIP_TYPE_UNKNOWN;
                    strcpy(info->name, "Unknown with FTDI");
                    pclose(fp);
                    return 0;
                }
            }
            pclose(fp);
        }
        
        /* 检查设备属性 */
        snprintf(cmd, sizeof(cmd), "udevadm info --name=%s --query=property", port);
        fp = popen(cmd, "r");
        if (fp) {
            while (fgets(buffer, sizeof(buffer), fp)) {
                if (strstr(buffer, "ID_VENDOR_ID=")) {
                    /* 根据厂商ID判断 */
                    if (strstr(buffer, "1a86")) { /* CH340厂商ID */
                        info->type = CHIP_TYPE_S300;
                        strcpy(info->name, "Device with CH340");
                    } else if (strstr(buffer, "10c4")) { /* CP210x厂商ID */
                        info->type = CHIP_TYPE_ESP32;
                        strcpy(info->name, "Device with CP210x");
                    }
                    pclose(fp);
                    return 0;
                }
            }
            pclose(fp);
        }
    }
    
    return -1;
}

/**
 * @brief 通过复位序列检测芯片
 */
int detect_by_reset_sequence(const char *port, int baud, chip_info_t *info)
{
    char response[MAX_RESPONSE_SIZE];
    int fd;
    
    printf("[DETECT] Trying reset sequence detection...\n");
    
    fd = serial_open(port, baud);
    if (fd < 0) {
        return -1;
    }
    
    /* 发送复位序列 */
    /* DTR复位 */
    int status;
    ioctl(fd, TIOCMGET, &status);
    status |= TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
    usleep(100000); /* 100ms */
    
    status &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
    usleep(500000); /* 500ms */
    
    /* 读取启动信息 */
    memset(response, 0, sizeof(response));
    if (serial_read_data(fd, response, sizeof(response)-1, 3000) > 0) {
        printf("[DETECT] Reset response: %.200s\n", response);
        
        /* 分析启动信息 */
        if (strstr(response, "S300") || strstr(response, "RBL") || strstr(response, "PiMCHIP")) {
            info->type = CHIP_TYPE_S300;
            strcpy(info->name, "PiMCHIP S300");
            strcpy(info->family, "PiMCHIP");
            
            if (strstr(response, "RBL")) {
                info->bootloader_mode = true;
            }
            
            serial_close(fd);
            return 0;
        }
        
        if (strstr(response, "ESP32")) {
            info->type = CHIP_TYPE_ESP32;
            strcpy(info->name, "ESP32");
            strcpy(info->family, "ESP32");
            serial_close(fd);
            return 0;
        }
        
        if (strstr(response, "STM32")) {
            info->type = CHIP_TYPE_STM32F4;
            strcpy(info->name, "STM32");
            strcpy(info->family, "STM32");
            serial_close(fd);
            return 0;
        }
    }
    
    serial_close(fd);
    return -1;
}

/**
 * @brief 通过特征码检测芯片
 */
int detect_by_signature(const char *port, int baud, chip_info_t *info)
{
    char response[MAX_RESPONSE_SIZE];
    
    printf("[DETECT] Trying signature detection...\n");
    
    /* 发送通用查询命令 */
    const char *probe_commands[] = {
        "\r\n",           /* 简单回车 */
        "AT\r\n",         /* AT命令 */
        "?\r\n",          /* 查询命令 */
        "help\r\n",       /* 帮助命令 */
        " ",              /* 空格触发 */
    };
    
    for (int i = 0; i < sizeof(probe_commands)/sizeof(probe_commands[0]); i++) {
        memset(response, 0, sizeof(response));
        
        if (chip_send_command(port, baud, probe_commands[i], response, sizeof(response)) == 0) {
            if (strlen(response) > 10) { /* 有效响应 */
                printf("[DETECT] Probe response: %.100s\n", response);
                
                /* 基于响应内容的模式匹配 */
                if (chip_match_pattern(response, "S300") || 
                    chip_match_pattern(response, "RBL") ||
                    chip_match_pattern(response, "PiMCHIP")) {
                    info->type = CHIP_TYPE_S300;
                    strcpy(info->name, "PiMCHIP S300");
                    return 0;
                }
                
                if (chip_match_pattern(response, "ESP32")) {
                    info->type = CHIP_TYPE_ESP32;
                    strcpy(info->name, "ESP32");
                    return 0;
                }
                
                if (chip_match_pattern(response, "STM32")) {
                    info->type = CHIP_TYPE_STM32F4;
                    strcpy(info->name, "STM32");
                    return 0;
                }
                
                /* 如果有响应但无法识别，至少知道有设备 */
                info->type = CHIP_TYPE_UNKNOWN;
                strcpy(info->name, "Unknown Device");
                return 0;
            }
        }
    }
    
    return -1;
}

/**
 * @brief 发送命令并接收响应
 */
int chip_send_command(const char *port, int baud, const char *cmd, char *response, int resp_size)
{
    int fd = serial_open(port, baud);
    if (fd < 0) {
        return -1;
    }
    
    serial_flush(fd);
    
    /* 发送命令 */
    if (serial_write_data(fd, cmd, strlen(cmd)) < 0) {
        serial_close(fd);
        return -1;
    }
    
    /* 等待响应 */
    usleep(100000); /* 100ms delay */
    
    int bytes_read = serial_read_data(fd, response, resp_size - 1, 2000);
    serial_close(fd);
    
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
        return 0;
    }
    
    return -1;
}

/**
 * @brief 模式匹配
 */
bool chip_match_pattern(const char *text, const char *pattern)
{
    if (!text || !pattern) {
        return false;
    }
    
    /* 忽略大小写的字符串匹配 */
    char *text_lower = strdup(text);
    char *pattern_lower = strdup(pattern);
    
    for (int i = 0; text_lower[i]; i++) {
        text_lower[i] = tolower(text_lower[i]);
    }
    for (int i = 0; pattern_lower[i]; i++) {
        pattern_lower[i] = tolower(pattern_lower[i]);
    }
    
    bool found = strstr(text_lower, pattern_lower) != NULL;
    
    free(text_lower);
    free(pattern_lower);
    
    return found;
}

/**
 * @brief 打印芯片信息
 */
void chip_print_info(const chip_info_t *info)
{
    if (!info) {
        return;
    }
    
    printf("\n=== Chip Detection Result ===\n");
    printf("Type:           %s\n", chip_type_to_string(info->type));
    printf("Name:           %s\n", info->name);
    printf("Family:         %s\n", info->family);
    
    if (info->chip_id != 0) {
        printf("Chip ID:        0x%08X\n", info->chip_id);
    }
    
    if (info->flash_size > 0) {
        printf("Flash Size:     %u bytes (%.1f MB)\n", 
               info->flash_size, info->flash_size / (1024.0 * 1024.0));
    }
    
    if (info->ram_size > 0) {
        printf("RAM Size:       %u bytes (%.1f KB)\n", 
               info->ram_size, info->ram_size / 1024.0);
    }
    
    if (info->freq > 0) {
        printf("Frequency:      %u Hz (%.1f MHz)\n", 
               info->freq, info->freq / 1000000.0);
    }
    
    if (strlen(info->version) > 0) {
        printf("Version:        %s\n", info->version);
    }
    
    printf("Bootloader:     %s\n", info->bootloader_mode ? "Yes" : "No");
    printf("Default Baud:   %d\n", chip_get_default_baud_rate(info->type));
    printf("=============================\n\n");
}

/**
 * @brief 获取芯片类型字符串
 */
const char *chip_type_to_string(chip_type_t type)
{
    if (type < CHIP_TYPE_COUNT) {
        return chip_type_strings[type];
    }
    return "Invalid";
}

/**
 * @brief 获取芯片系列名称
 */
const char *chip_get_family_name(chip_type_t type)
{
    if (type < CHIP_TYPE_COUNT) {
        return chip_family_names[type];
    }
    return "Unknown";
}

/**
 * @brief 检查是否支持的芯片
 */
bool chip_is_supported(chip_type_t type)
{
    return type > CHIP_TYPE_UNKNOWN && type < CHIP_TYPE_COUNT;
}

/**
 * @brief 获取默认波特率
 */
int chip_get_default_baud_rate(chip_type_t type)
{
    if (type < CHIP_TYPE_COUNT) {
        return default_baud_rates[type];
    }
    return 115200;
}

/**
 * @brief 打印检测日志
 */
void chip_print_detection_log(const char *port, detect_method_t method, bool success)
{
    const char *method_names[] = {
        "Auto Detection",
        "UART Command",
        "USB Descriptor", 
        "JTAG ID",
        "Reset Sequence",
        "Signature"
    };
    
    const char *method_name = (method < sizeof(method_names)/sizeof(method_names[0])) 
                             ? method_names[method] : "Unknown";
    
    printf("[DETECT] %s on %s: %s\n", method_name, port, success ? "SUCCESS" : "FAILED");
}

/* 内部串口操作函数实现 */

static int serial_open(const char *port, int baud_rate)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }
    
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }
    
    /* 设置波特率 */
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;
        default:     speed = B115200; break;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    /* 配置串口参数 */
    tty.c_cflag &= ~PARENB;        /* 无校验 */
    tty.c_cflag &= ~CSTOPB;        /* 1个停止位 */
    tty.c_cflag &= ~CSIZE;         /* 清除数据位设置 */
    tty.c_cflag |= CS8;            /* 8数据位 */
    tty.c_cflag &= ~CRTSCTS;       /* 无硬件流控 */
    tty.c_cflag |= CREAD | CLOCAL; /* 打开接收和本地模式 */
    
    tty.c_lflag &= ~ICANON;        /* 非规范模式 */
    tty.c_lflag &= ~ECHO;          /* 禁用回显 */
    tty.c_lflag &= ~ECHOE;         /* 禁用擦除字符回显 */
    tty.c_lflag &= ~ECHONL;        /* 禁用新行回显 */
    tty.c_lflag &= ~ISIG;          /* 禁用信号字符 */
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* 禁用软件流控 */
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    
    tty.c_oflag &= ~OPOST;         /* 原始输出 */
    tty.c_oflag &= ~ONLCR;         /* 禁用换行符转换 */
    
    tty.c_cc[VTIME] = 10;          /* 1秒超时 */
    tty.c_cc[VMIN] = 0;            /* 非阻塞读取 */
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

static void serial_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

static int serial_write_data(int fd, const char *data, int len)
{
    return write(fd, data, len);
}

static int serial_read_data(int fd, char *buffer, int size, int timeout_ms)
{
    fd_set readfds;
    struct timeval timeout;
    int total_bytes = 0;
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    time_t start_time = time(NULL);
    
    while (total_bytes < size - 1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(fd, &readfds)) {
            int bytes = read(fd, buffer + total_bytes, size - total_bytes - 1);
            if (bytes > 0) {
                total_bytes += bytes;
            }
        } else if (ret == 0) {
            break; /* 超时 */
        } else {
            break; /* 错误 */
        }
        
        /* 检查总超时 */
        if ((time(NULL) - start_time) * 1000 > timeout_ms) {
            break;
        }
    }
    
    return total_bytes;
}

static void serial_flush(int fd)
{
    tcflush(fd, TCIOFLUSH);
}
