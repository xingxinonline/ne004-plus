/**
 * @file rbl_uart.c
 * @brief RBL UART驱动(基于S300 BSP)
 */

#include "rbl_uart.h"
#include "rbl_config.h"
#include "rbl_system.h"
#include "uart.h"
#include "rcc.h"
#include <stdarg.h>
#include <stdio.h>

/* UART配置 */
static volatile bool uart_initialized = false;

/**
 * @brief 初始化UART
 */
int rbl_uart_init(void)
{
    if (uart_initialized) {
        return 0;
    }
    
    /* 使能UART时钟 */
    rcc_set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    
    /* 初始化UART3 */
    int ret = uart_init(RBL_UART_PORT, UARTTYPE_STD_SERIAL, 
                       RBL_SYSTEM_CLOCK_HZ, RBL_UART_BAUDRATE);
    if (ret != 0) {
        return ret;
    }
    
    /* 配置UART属性：8位数据，1位停止位，无校验 */
    uart_set_property(RBL_UART_PORT, 8, 1, 0);
    
    /* 使能FIFO */
    uart_set_fifo(RBL_UART_PORT, UART_FIFO_EN | UART_FIFO_TX_1_2 | UART_FIFO_RX_1_2);
    
    uart_initialized = true;
    return 0;
}

/**
 * @brief 反初始化UART
 */
void rbl_uart_deinit(void)
{
    if (!uart_initialized) {
        return;
    }
    
    /* 禁用UART时钟 */
    rcc_set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, false);
    
    uart_initialized = false;
}

/**
 * @brief 发送一个字符
 */
int rbl_uart_putchar(int ch)
{
    if (!uart_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的UART写函数 */
    return uart_write(RBL_UART_PORT, UARTTYPE_STD_SERIAL, (uint16_t)ch);
}

/**
 * @brief 接收一个字符
 */
int rbl_uart_getchar(void)
{
    if (!uart_initialized) {
        return -1;
    }
    
    /* 使用S300 BSP的UART读函数 */
    uint16_t data = uart_read(RBL_UART_PORT, UARTTYPE_STD_SERIAL);
    
    /* 检查是否有数据 */
    if (data & 0xFF00) {
        return -1;  /* 无数据或有错误 */
    }
    
    return (int)(data & 0xFF);
}

/**
 * @brief 检查是否有可读数据
 */
bool rbl_uart_readable(void)
{
    if (!uart_initialized) {
        return false;
    }
    
    /* 尝试读取，如果没有数据会返回错误标志 */
    uint16_t data = uart_read(RBL_UART_PORT, UARTTYPE_STD_SERIAL);
    return !(data & 0xFF00);
}

/**
 * @brief 发送字符串
 */
int rbl_uart_puts(const char *str)
{
    if (!str || !uart_initialized) {
        return -1;
    }
    
    int count = 0;
    while (*str) {
        if (rbl_uart_putchar(*str++) < 0) {
            return -1;
        }
        count++;
    }
    
    return count;
}

/**
 * @brief 发送数据块
 */
int rbl_uart_write(const uint8_t *data, size_t length)
{
    if (!data || !uart_initialized) {
        return -1;
    }
    
    for (size_t i = 0; i < length; i++) {
        if (rbl_uart_putchar(data[i]) < 0) {
            return -1;
        }
    }
    
    return (int)length;
}

/**
 * @brief 接收数据块
 */
int rbl_uart_read(uint8_t *buffer, size_t length)
{
    if (!buffer || !uart_initialized) {
        return -1;
    }
    
    size_t received = 0;
    
    for (size_t i = 0; i < length; i++) {
        int ch = rbl_uart_getchar();
        if (ch < 0) {
            break;
        }
        buffer[i] = (uint8_t)ch;
        received++;
    }
    
    return (int)received;
}

/**
 * @brief 超时接收数据
 */
int rbl_uart_read_timeout(uint8_t *buffer, size_t length, uint32_t timeout_ms)
{
    if (!buffer || !uart_initialized) {
        return -1;
    }
    
    uint32_t start_time = rbl_get_tick();
    size_t received = 0;
    
    while (received < length) {
        if (rbl_uart_readable()) {
            int ch = rbl_uart_getchar();
            if (ch >= 0) {
                buffer[received++] = (uint8_t)ch;
            }
        }
        
        /* 检查超时 */
        if ((rbl_get_tick() - start_time) >= timeout_ms) {
            break;
        }
    }
    
    return (int)received;
}

/**
 * @brief 非阻塞接收数据
 */
int rbl_uart_read_nonblock(uint8_t *buffer, size_t length)
{
    if (!buffer || !uart_initialized) {
        return -1;
    }
    
    size_t received = 0;
    
    for (size_t i = 0; i < length; i++) {
        if (!rbl_uart_readable()) {
            break;
        }
        
        int ch = rbl_uart_getchar();
        if (ch < 0) {
            break;
        }
        
        buffer[i] = (uint8_t)ch;
        received++;
    }
    
    return (int)received;
}

/**
 * @brief 设置波特率
 */
int rbl_uart_set_baudrate(uint32_t baudrate)
{
    if (!uart_initialized) {
        return -1;
    }
    
    uart_set_baud(RBL_UART_PORT, RBL_SYSTEM_CLOCK_HZ, baudrate);
    return 0;
}

/**
 * @brief 刷新发送缓冲区
 */
void rbl_uart_flush(void)
{
    if (!uart_initialized) {
        return;
    }
    
    /* S300 UART驱动会自动处理发送完成 */
    /* 这里可以添加等待发送完成的代码 */
}

/**
 * @brief printf重定向到UART
 */
int rbl_printf(const char *format, ...)
{
    static char buffer[256];
    va_list args;
    int len;
    
    if (!uart_initialized) {
        return -1;
    }
    
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        rbl_uart_write((uint8_t*)buffer, len);
    }
    
    return len;
}

/* printf重定向支持 */
#ifdef __GNUC__
int _write(int file, char *ptr, int len)
{
    (void)file;
    return rbl_uart_write((uint8_t*)ptr, len);
}
#endif

#ifdef __ARMCC_VERSION
int fputc(int ch, FILE *f)
{
    (void)f;
    return rbl_uart_putchar(ch);
}
#endif
