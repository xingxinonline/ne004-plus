#include "rbl_hal.h"
#include "s300.h"
#include <stdarg.h>
#include "rcc.h"

// 寄存器与地址：优先使用 s300_memmap.h 中的定义
#ifndef UART3_BASE
#define UART3_BASE   (0x40013000u)
#endif
#define UART_THR     (*(volatile uint32_t *)(UART3_BASE + 0x00))
#define UART_RBR     (*(volatile uint32_t *)(UART3_BASE + 0x00))  /* 接收缓冲器 */
#define UART_IER     (*(volatile uint32_t *)(UART3_BASE + 0x04))
#define UART_IIR_FCR (*(volatile uint32_t *)(UART3_BASE + 0x08))
#define UART_LCR     (*(volatile uint32_t *)(UART3_BASE + 0x0C))
#define UART_MCR     (*(volatile uint32_t *)(UART3_BASE + 0x10))
#define UART_LSR     (*(volatile uint32_t *)(UART3_BASE + 0x14))

#define APB1_BASE      (0x4000A000u)
#define APB1_CLK_EN    (*(volatile uint32_t *)(APB1_BASE + 0x000Cu))

#ifndef IO_MATRIX_BASE
#define IO_MATRIX_BASE (0x40008000u)
#endif
#define IO_MATRIX_CFG1 (*(volatile uint32_t *)(IO_MATRIX_BASE + 0x04u))

static inline void uart_send_char(char c) {
    while ((UART_LSR & 0x20u) == 0u) {
        __NOP();
    }
    UART_THR = (uint32_t)c;
}

void rbl_uart_init(void) {
    // 开启时钟：假设 bit8:UART3, bit3:IO matrix（与现有最小实现一致）
    APB1_CLK_EN |= (1u << 8) | (1u << 3);

    // 复用：将对应引脚切到 UART3 功能（保持与现实现有寄存器位一致）
    uint32_t v = IO_MATRIX_CFG1;
    v &= ~((0x3u << 20) | (0x3u << 22));
    v |=  ((0x3u << 20) | (0x3u << 22));
    IO_MATRIX_CFG1 = v;

    // UART 基本配置：115200, 8N1
    UART_IER = 0x00u;
    UART_IIR_FCR = 0x07u;   // 使能 FIFO 并清空
    UART_LCR = 0x03u;       // 8N1
    UART_MCR = 0x00u;

    // 动态设置波特率：根据 APB1 时钟计算分频
    uint32_t apb1_hz = rcc_get_clock(RCC_CLOCK_APB1);
    if (apb1_hz == 0u) {
        apb1_hz = 24000000u; // 回退到24MHz
    }
    const uint32_t baud = 115200u;
    uint32_t divisor = (apb1_hz + (16u * baud / 2u)) / (16u * baud); // 四舍五入
    if (divisor == 0u) divisor = 1u;
    UART_LCR |= 0x80u;      // DLAB=1
    *(volatile uint32_t *)(UART3_BASE + 0x00) = (divisor & 0xFFu);        // DLL
    *(volatile uint32_t *)(UART3_BASE + 0x04) = ((divisor >> 8) & 0xFFu); // DLH
    UART_LCR &= ~0x80u;     // DLAB=0
}

void rbl_uart_write(const char *buf, size_t len) {
    if (!buf || len == 0) return;
    for (size_t i = 0; i < len; ++i) {
        uart_send_char(buf[i]);
    }
}

size_t rbl_hal_uart_receive(uint8_t *buf, size_t max_len) {
    size_t received = 0;
    
    while (received < max_len) {
        /* 检查UART是否有数据可读 */
        if (UART_LSR & 0x01) {  /* 数据准备位 */
            buf[received] = (uint8_t)(UART_RBR & 0xFF);
            received++;
        } else {
            break;  /* 没有更多数据 */
        }
    }
    
    return received;
}

void rbl_delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; ++i) {
        __NOP();
    }
}

/* 简化的字符串长度计算 */
static size_t rbl_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void rbl_log_write_char(char c) {
    rbl_uart_write(&c, 1u);
}

static void rbl_log_write_padding(char pad_char, int count) {
    while (count-- > 0) {
        rbl_log_write_char(pad_char);
    }
}

static size_t rbl_uint_to_str(uint32_t value, unsigned base, int uppercase, char *out, size_t out_cap) {
    static const char digits_low[] = "0123456789abcdef";
    static const char digits_up[]  = "0123456789ABCDEF";
    const char *digits = uppercase ? digits_up : digits_low;
    char tmp[32];
    size_t tmp_len = 0;

    if (base < 2u || base > 16u || out_cap == 0u) {
        return 0u;
    }

    do {
        tmp[tmp_len++] = digits[value % base];
        value /= base;
    } while (value != 0u && tmp_len < sizeof(tmp));

    if (tmp_len == 0u) {
        tmp[tmp_len++] = '0';
    }

    size_t copy_len = (tmp_len < (out_cap - 1u)) ? tmp_len : (out_cap - 1u);
    for (size_t i = 0; i < copy_len; ++i) {
        out[i] = tmp[tmp_len - 1u - i];
    }
    out[copy_len] = '\0';
    return copy_len;
}

static void rbl_log_write_uint(uint32_t value, unsigned base, int uppercase, char pad_char, int width) {
    char buf[32];
    size_t len = rbl_uint_to_str(value, base, uppercase, buf, sizeof(buf));
    int pad = width - (int)len;
    if (pad < 0) {
        pad = 0;
    }
    rbl_log_write_padding(pad_char, pad);
    rbl_uart_write(buf, len);
}

static void rbl_log_write_int(int32_t value, char pad_char, int width) {
    uint32_t magnitude;
    int negative = 0;
    if (value < 0) {
        negative = 1;
        magnitude = (uint32_t)(-(value + 1)) + 1u;
    } else {
        magnitude = (uint32_t)value;
    }

    char buf[32];
    size_t len = rbl_uint_to_str(magnitude, 10u, 0, buf, sizeof(buf));
    int total = (int)len + (negative ? 1 : 0);
    int pad = width - total;
    if (pad < 0) {
        pad = 0;
    }

    if (negative && pad_char == '0') {
        rbl_log_write_char('-');
        rbl_log_write_padding('0', pad);
    } else {
        rbl_log_write_padding(pad_char, pad);
        if (negative) {
            rbl_log_write_char('-');
        }
    }

    rbl_uart_write(buf, len);
}

static void rbl_log_write_string(const char *s, char pad_char, int width) {
    const char *text = s ? s : "(null)";
    size_t len = rbl_strlen(text);
    int pad = width - (int)len;
    if (pad < 0) {
        pad = 0;
    }
    rbl_log_write_padding(pad_char, pad);
    rbl_uart_write(text, len);
}

static void rbl_log_write_pointer(uintptr_t value, int width) {
    char buf[32];
    size_t len = rbl_uint_to_str((uint32_t)value, 16u, 0, buf, sizeof(buf));
    int target_width = width > 0 ? width : (int)(sizeof(void*) * 2u);
    int pad = target_width - (int)len;
    if (pad < 0) {
        pad = 0;
    }
    rbl_uart_write("0x", 2u);
    rbl_log_write_padding('0', pad);
    rbl_uart_write(buf, len);
}

static void rbl_log_vprintf(const char *format, va_list args) {
    while (format && *format) {
        if (*format != '%') {
            rbl_log_write_char(*format++);
            continue;
        }

        ++format;
        char pad_char = ' ';
        int width = 0;

        if (*format == '0') {
            pad_char = '0';
            ++format;
        }

        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            ++format;
        }

        char spec = *format ? *format : '\0';
        if (spec == '\0') {
            break;
        }

        switch (spec) {
            case '%':
                rbl_log_write_char('%');
                break;
            case 'c': {
                char value = (char)va_arg(args, int);
                rbl_log_write_char(value);
                break;
            }
            case 's':
                rbl_log_write_string(va_arg(args, const char *), pad_char, width);
                break;
            case 'd':
            case 'i':
                rbl_log_write_int(va_arg(args, int32_t), pad_char, width);
                break;
            case 'u':
                rbl_log_write_uint(va_arg(args, uint32_t), 10u, 0, pad_char, width);
                break;
            case 'x':
                rbl_log_write_uint(va_arg(args, uint32_t), 16u, 0, pad_char, width);
                break;
            case 'X':
                rbl_log_write_uint(va_arg(args, uint32_t), 16u, 1, pad_char, width);
                break;
            case 'p':
                rbl_log_write_pointer((uintptr_t)va_arg(args, void *), width);
                break;
            default:
                rbl_log_write_char('%');
                rbl_log_write_char(spec);
                break;
        }

        if (*format != '\0') {
            ++format;
        }
    }
}

void rbl_log_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    rbl_log_vprintf(format, args);
    va_end(args);
}

/* 空的printf替代函数，用于替换QSPI驱动中的调试输出 */
int rbl_printf_stub(const char *format, ...) {
    (void)format;  /* 静默编译器警告 */
    return 0;
}
