/**
 * @file chip_detection.h
 * @brief 串口芯片检测和识别功能 - ESP32兼容
 * @version 2.0
 * @date 2025-08-30
 */

#ifndef CHIP_DETECTION_H
#define CHIP_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

/* 支持的芯片类型 */
typedef enum {
    CHIP_TYPE_UNKNOWN = 0,
    CHIP_TYPE_S300,           // PiMCHIP S300 (本芯片)
    CHIP_TYPE_ESP32,          // ESP32系列
    CHIP_TYPE_ESP32_C3,       // ESP32-C3
    CHIP_TYPE_ESP32_S3,       // ESP32-S3
    CHIP_TYPE_ESP32_C6,       // ESP32-C6
    CHIP_TYPE_ESP8266,        // ESP8266
    CHIP_TYPE_STM32F4,        // STM32F4系列
    CHIP_TYPE_STM32H7,        // STM32H7系列
    CHIP_TYPE_GD32,           // 兆易创新GD32
    CHIP_TYPE_CH32,           // 沁恒CH32
    CHIP_TYPE_RISC_V,         // 通用RISC-V
    CHIP_TYPE_ARDUINO,        // Arduino系列
    CHIP_TYPE_COUNT
} chip_type_t;

/* 芯片信息结构 */
typedef struct {
    chip_type_t type;         // 芯片类型
    char name[32];            // 芯片名称
    char family[16];          // 芯片系列
    uint32_t chip_id;         // 芯片ID
    uint32_t flash_size;      // Flash大小(字节)
    uint32_t ram_size;        // RAM大小(字节)
    uint32_t freq;            // 主频(Hz)
    char version[16];         // 版本信息
    char mac_addr[18];        // MAC地址 (ESP32特有)
    bool bootloader_mode;     // 是否在Bootloader模式
    uint8_t uart_port;        // 检测到的串口号
    uint32_t baud_rate;       // 检测到的波特率
} chip_info_t;

/* 串口检测结果 */
typedef struct {
    bool detected;            // 是否检测到设备
    char port_name[32];       // 串口名称 (/dev/ttyUSB0, COM3等)
    uint32_t baud_rate;       // 工作波特率
    chip_info_t chip;         // 检测到的芯片信息
    char response[256];       // 原始响应数据
} uart_detection_result_t;

/* 主要API函数 */

/**
 * @brief 自动检测串口设备并识别芯片类型 (ESP32兼容)
 * @param port_pattern 串口模式 (如"/dev/ttyUSB*", "COM*", NULL表示自动搜索)
 * @param result 检测结果输出
 * @return 0=成功, <0=失败
 */
int chip_detect_auto(const char *port_pattern, uart_detection_result_t *result);

/**
 * @brief 检测指定串口的芯片信息
 * @param port 串口名称 (如"/dev/ttyUSB0", "COM3")
 * @param baud_rate 波特率 (0=自动检测)
 * @param result 检测结果输出
 * @return 0=成功, <0=失败
 */
int chip_detect_port(const char *port, uint32_t baud_rate, uart_detection_result_t *result);

/**
 * @brief ESP32兼容的芯片信息获取
 * @param port 串口名称
 * @param result 检测结果
 * @return 0=成功, <0=失败
 */
int chip_detect_esp32_compatible(const char *port, uart_detection_result_t *result);

/**
 * @brief 检测S300芯片信息 (本地)
 * @param info 芯片信息输出
 * @return 0=成功, <0=失败
 */
int chip_detect_s300_local(chip_info_t *info);

/**
 * @brief 列出所有可用的串口
 * @param ports 串口列表输出 (调用者负责释放)
 * @param max_ports 最大串口数量
 * @return 实际找到的串口数量, <0=失败
 */
int chip_list_serial_ports(char ports[][32], int max_ports);

/**
 * @brief 打印芯片检测结果 (ESP32兼容格式)
 * @param result 检测结果
 */
void chip_print_detection_result(const uart_detection_result_t *result);

/**
 * @brief 获取芯片类型字符串
 * @param type 芯片类型
 * @return 芯片类型字符串
 */
const char *chip_get_type_string(chip_type_t type);

/**
 * @brief 获取错误描述
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char *chip_get_error_string(int error_code);

/* 错误码定义 */
#define CHIP_DETECT_OK              0    // 成功
#define CHIP_DETECT_ERR_PORT        -1   // 串口错误
#define CHIP_DETECT_ERR_TIMEOUT     -2   // 超时
#define CHIP_DETECT_ERR_NO_RESPONSE -3   // 无响应
#define CHIP_DETECT_ERR_PARSE       -4   // 解析失败
#define CHIP_DETECT_ERR_UNSUPPORTED -5   // 不支持的芯片
#define CHIP_DETECT_ERR_INVALID     -6   // 无效参数

/* 常用波特率列表 */
extern const uint32_t chip_common_baud_rates[];
extern const int chip_common_baud_rates_count;

#endif /* CHIP_DETECTION_H */

/* 检测配置 */
typedef struct {
    const char *port;         // 串口设备
    int baud_rate;           // 波特率
    int timeout_ms;          // 超时时间
    detect_method_t method;  // 检测方法
    bool verbose;            // 详细输出
} detect_config_t;

/* 函数声明 */

/* 主要检测函数 */
int chip_detect_auto(const char *port, chip_info_t *info);
int chip_detect_with_config(const detect_config_t *config, chip_info_t *info);

/* 专用检测函数 */
int chip_detect_s300(const char *port, int baud, chip_info_t *info);
int chip_detect_esp32(const char *port, int baud, chip_info_t *info);
int chip_detect_stm32(const char *port, int baud, chip_info_t *info);

/* 辅助函数 */
const char *chip_type_to_string(chip_type_t type);
const char *chip_get_family_name(chip_type_t type);
bool chip_is_supported(chip_type_t type);
int chip_get_default_baud_rate(chip_type_t type);

/* 检测方法实现 */
int detect_by_uart_command(const char *port, int baud, chip_info_t *info);
int detect_by_usb_descriptor(const char *port, chip_info_t *info);
int detect_by_reset_sequence(const char *port, int baud, chip_info_t *info);
int detect_by_signature(const char *port, int baud, chip_info_t *info);

/* 工具函数 */
int chip_send_command(const char *port, int baud, const char *cmd, char *response, int resp_size);
int chip_read_response(const char *port, int baud, char *buffer, int size, int timeout_ms);
bool chip_match_pattern(const char *text, const char *pattern);

/* 调试和信息函数 */
void chip_print_info(const chip_info_t *info);
void chip_print_detection_log(const char *port, detect_method_t method, bool success);

#endif /* CHIP_DETECTION_H */
