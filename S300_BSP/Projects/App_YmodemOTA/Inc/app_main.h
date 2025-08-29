/**
 * @file app_main.h
 * @brief App主程序头文件
 */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 应用程序状态 */
typedef enum {
    APP_STATE_INIT = 0,             /* 初始化 */
    APP_STATE_RUNNING,              /* 运行中 */
    APP_STATE_OTA_MODE,             /* OTA模式 */
    APP_STATE_ERROR,                /* 错误状态 */
    APP_STATE_SHUTDOWN,             /* 关机状态 */
} app_state_t;

/* 系统信息 */
typedef struct {
    uint32_t magic;                 /* 魔数 */
    uint32_t version;               /* 版本 */
    uint32_t build_time;            /* 构建时间 */
    uint32_t boot_count;            /* 启动计数 */
    uint32_t run_time;              /* 运行时间 */
    app_state_t state;              /* 当前状态 */
    uint32_t free_heap;             /* 可用堆内存 */
    uint32_t min_free_heap;         /* 最小可用堆内存 */
} app_system_info_t;

/* 命令类型 */
typedef enum {
    APP_CMD_HELP = 0,               /* 帮助 */
    APP_CMD_INFO,                   /* 系统信息 */
    APP_CMD_VERSION,                /* 版本信息 */
    APP_CMD_REBOOT,                 /* 重启 */
    APP_CMD_OTA,                    /* OTA更新 */
    APP_CMD_TEST,                   /* 测试 */
    APP_CMD_LED,                    /* LED控制 */
    APP_CMD_MEMORY,                 /* 内存信息 */
    APP_CMD_UNKNOWN,                /* 未知命令 */
} app_command_t;

/* 函数声明 */

/**
 * @brief 应用程序主函数
 * @return 返回值（通常不会返回）
 */
int app_main(void);

/**
 * @brief 系统初始化
 * @return 0成功，非0失败
 */
int app_system_init(void);

/**
 * @brief 打印启动横幅
 */
void app_print_banner(void);

/**
 * @brief 获取应用程序版本
 * @return 版本字符串
 */
const char* app_get_version(void);

/**
 * @brief 获取系统信息
 * @param info 系统信息结构体
 * @return 0成功，非0失败
 */
int app_get_system_info(app_system_info_t* info);

/**
 * @brief 系统复位
 */
void app_system_reset(void) __attribute__((noreturn));

/**
 * @brief 延时函数
 * @param ms 延时时间（毫秒）
 */
void app_delay_ms(uint32_t ms);

/**
 * @brief 获取系统时钟
 * @return 系统时钟（毫秒）
 */
uint32_t app_get_tick_ms(void);

/**
 * @brief 错误处理函数
 * @param error_code 错误代码
 * @param error_msg 错误消息
 */
void app_error_handler(int error_code, const char* error_msg);

/**
 * @brief 看门狗初始化
 * @param timeout_ms 超时时间（毫秒）
 * @return 0成功，非0失败
 */
int app_watchdog_init(uint32_t timeout_ms);

/**
 * @brief 喂看门狗
 */
void app_watchdog_feed(void);

/**
 * @brief LED初始化
 * @return 0成功，非0失败
 */
int app_led_init(void);

/**
 * @brief LED控制
 * @param on true开启，false关闭
 */
void app_led_set(bool on);

/**
 * @brief LED闪烁
 * @param times 闪烁次数
 * @param interval_ms 闪烁间隔（毫秒）
 */
void app_led_blink(int times, uint32_t interval_ms);

/**
 * @brief 命令行初始化
 * @return 0成功，非0失败
 */
int app_console_init(void);

/**
 * @brief 处理命令行输入
 */
void app_console_process(void);

/**
 * @brief 解析命令
 * @param cmd_line 命令行字符串
 * @return 命令类型
 */
app_command_t app_parse_command(const char* cmd_line);

/**
 * @brief 执行命令
 * @param cmd 命令类型
 * @param args 命令参数
 * @return 0成功，非0失败
 */
int app_execute_command(app_command_t cmd, const char* args);

/**
 * @brief 显示帮助信息
 */
void app_show_help(void);

/**
 * @brief 运行测试
 * @return 0成功，非0失败
 */
int app_run_tests(void);

/**
 * @brief 获取堆内存统计
 * @param free_size 可用内存大小
 * @param min_free_size 最小可用内存大小
 * @return 0成功，非0失败
 */
int app_get_heap_stats(uint32_t* free_size, uint32_t* min_free_size);

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */
