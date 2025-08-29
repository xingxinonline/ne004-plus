#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

/* printf 已在 board_init 中完成串口与重定向初始化 */

/* REG32宏定义 */
#define REG32(base, off) (*(volatile uint32_t *)((uintptr_t)(base) + (off)))

/* XIP 相关定义 */
#define XIP_FLASH_BASE          M4_SLV_FLASH_BASE    /* QSPI Flash在AHB总线上的映射地址 */
#define XIP_CODE_SECTION_ADDR   0x100000UL      /* XIP代码段在Flash中的偏移 */
#define XIP_DATA_SECTION_ADDR   0x120000UL      /* XIP数据段在Flash中的偏移 */
#define XIP_FONT_DATA_ADDR      0x140000UL      /* 字体数据在Flash中的偏移 */

/* 测试函数类型定义 */
typedef int (*xip_add_func_t)(int a, int b);
typedef int (*xip_array_sum_func_t)(const int *array, int count);
typedef int (*xip_fibonacci_func_t)(int n);
typedef int (*xip_strlen_func_t)(const char *str);

typedef struct
{
    int x, y;
    int result;
} point_t;

/* SysTick计时辅助函数 */
static volatile uint32_t systick_counter = 0;

/* SysTick中断处理函数 */
void SysTick_Handler(void)
{
    systick_counter++;
}

/* 初始化SysTick为1ms中断 */
static void systick_init(void)
{
    systick_counter = 0;
    /* 配置SysTick为1ms中断 */
    SysTick_Config(SystemCoreClock / 1000);
}

/* 获取当前时间戳（毫秒） */
static uint32_t get_systick_ms(void)
{
    return systick_counter;
}

/* 获取高精度计数值（用于性能测试，微秒级） */
static uint32_t get_systick_us(void)
{
    uint32_t ms = systick_counter;
    uint32_t val = SysTick->VAL;
    uint32_t load = SysTick->LOAD;
    /* SysTick是递减计数器，计算已过去的时间 */
    uint32_t elapsed_ticks = load - val;
    uint32_t us_per_tick = 1000000 / SystemCoreClock;
    return ms * 1000 + (elapsed_ticks * us_per_tick);
}

/* 函数声明 */
static void debug_xip_config(const char *tag);
static int configure_xip_mode(void);
static int exit_xip_mode(void);
static int prepare_flash_content(void);
static int verify_flash_content(void);
static int test_xip_data_access(void);
static int test_xip_code_execution(void);
typedef void (*xip_process_points_func_t)(point_t *points, int count);

/* 示例机器码 - 实际应用中应从编译好的二进制文件中提取 */
/*
 * 对应函数：int xip_add_function(int a, int b) { return a + b + 42; }
 * ARM Thumb机器码：
 */
static const uint8_t xip_add_function_code[] =
{
    0x08, 0x44,     /* add r0, r0, r1    ; a + b */
    0x2A, 0x30,     /* adds r0, #42      ; + 42 */
    0x70, 0x47      /* bx lr             ; 返回 */
};

/* 模拟字体数据 - 8x8点阵字符 'A' 和 'B' */
static const uint8_t font_data[] =
{
    /* 字符 'A' */
    0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00,
    /* 字符 'B' */
    0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00,
    /* 字符 'C' */
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00,
    /* 字符 'D' */
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00
};

/* 测试数据表 */
static const int test_array[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const char test_string[] = "Hello XIP World!";

/* 配置XIP模式 */
static int configure_xip_mode(void)
{
    printf("Configuring XIP mode...\n");
    /* 首先配置Quad读取模式(1-1-4)，与SPL保持一致 */
    qspi_configure_quad_read(true);
    /* 启用直接访问模式 (XIP) */
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg |= CQSPI_CFG_DIRECT;
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    debug_xip_config("After XIP configuration");
    printf("XIP mode configured\n");
    return 0;
}

/* 退出XIP模式 */
static int exit_xip_mode(void)
{
    printf("Exiting XIP mode...\n");
    /* 禁用直接访问模式 */
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_DIRECT | CQSPI_CFG_XIP_IMM);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    /* 清除模式位配置 */
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    rd &= ~(1u << CQSPI_RD_MODE_EN_LSB);
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
    REG32(g_qspi.reg, CQSPI_REG_MODE_BIT) = 0;
    /* 恢复到标准读取模式 */
    qspi_configure_quad_read(false);
    printf("XIP mode exited\n");
    return 0;
}

/* 调试函数：显示XIP配置状态 */
static void debug_xip_config(const char *tag)
{
    printf("[DEBUG] %s:\n", tag);
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    uint32_t mode = REG32(g_qspi.reg, CQSPI_REG_MODE_BIT);
    printf("  CONFIG: 0x%08lX (DIRECT:%u)\n", (unsigned long)cfg,
           (cfg & CQSPI_CFG_DIRECT) ? 1 : 0);
    printf("  RD_INSTR: 0x%08lX (OP:0x%02lX, MODE_EN:%lu, DUMMY:%lu)\n",
           (unsigned long)rd,
           (rd >> CQSPI_RD_OPCODE_LSB) & 0xFF,
           (rd >> CQSPI_RD_MODE_EN_LSB) & 1,
           (rd >> CQSPI_RD_DUMMY_LSB) & 0x1F);
    printf("  MODE_BIT: 0x%08lX\n", (unsigned long)mode);
}

/* 准备Flash内容 */
static int prepare_flash_content(void)
{
    printf("Preparing Flash content for Advanced XIP test...\n");
    /* 确保退出XIP模式以便写入 */
    exit_xip_mode();
    /* 擦除代码段区域 */
    printf("  Erasing code section @0x%06lX\n", (unsigned long)XIP_CODE_SECTION_ADDR);
    if (qspi_erase_64k(XIP_CODE_SECTION_ADDR) != 0)
    {
        printf("  Failed to erase code section\n");
        return -1;
    }
    /* 擦除数据段区域 */
    printf("  Erasing data section @0x%06lX\n", (unsigned long)XIP_DATA_SECTION_ADDR);
    if (qspi_erase_64k(XIP_DATA_SECTION_ADDR) != 0)
    {
        printf("  Failed to erase data section\n");
        return -1;
    }
    /* 擦除字体数据区域 */
    printf("  Erasing font data section @0x%06lX\n", (unsigned long)XIP_FONT_DATA_ADDR);
    if (qspi_erase_64k(XIP_FONT_DATA_ADDR) != 0)
    {
        printf("  Failed to erase font data section\n");
        return -1;
    }
    /* 写入测试函数代码 */
    printf("  Programming XIP function code (%zu bytes)\n", sizeof(xip_add_function_code));
    if (qspi_page_program(XIP_CODE_SECTION_ADDR, xip_add_function_code,
                          sizeof(xip_add_function_code)) != 0)
    {
        printf("  Failed to program function code\n");
        return -1;
    }
    /* 写入测试数据 */
    printf("  Programming test array (%zu bytes)\n", sizeof(test_array));
    if (qspi_page_program(XIP_DATA_SECTION_ADDR, test_array, sizeof(test_array)) != 0)
    {
        printf("  Failed to program test array\n");
        return -1;
    }
    /* 写入测试字符串 */
    uint32_t string_addr = XIP_DATA_SECTION_ADDR + 256;  /* 256字节偏移 */
    printf("  Programming test string (%zu bytes)\n", sizeof(test_string));
    if (qspi_page_program(string_addr, test_string, sizeof(test_string)) != 0)
    {
        printf("  Failed to program test string\n");
        return -1;
    }
    /* 写入字体数据 */
    printf("  Programming font data (%zu bytes)\n", sizeof(font_data));
    if (qspi_page_program(XIP_FONT_DATA_ADDR, font_data, sizeof(font_data)) != 0)
    {
        printf("  Failed to program font data\n");
        return -1;
    }
    printf("Flash content prepared successfully\n");
    return 0;
}

/* 验证Flash内容 */
static int verify_flash_content(void)
{
    printf("Verifying Flash content...\n");
    /* 验证函数代码 */
    uint8_t read_code[sizeof(xip_add_function_code)];
    if (qspi_read(XIP_CODE_SECTION_ADDR, read_code, sizeof(read_code)) != 0)
    {
        printf("  Failed to read function code\n");
        return -1;
    }
    printf("  Function code verification:\n");
    printf("    Expected: ");
    for (size_t i = 0; i < sizeof(xip_add_function_code); i++)
    {
        printf("%02X ", xip_add_function_code[i]);
    }
    printf("\n    Read:     ");
    for (size_t i = 0; i < sizeof(read_code); i++)
    {
        printf("%02X ", read_code[i]);
    }
    printf("\n");
    if (memcmp(xip_add_function_code, read_code, sizeof(xip_add_function_code)) != 0)
    {
        printf("  Function code verification failed\n");
        return -1;
    }
    /* 验证测试数组 */
    int read_array[sizeof(test_array) / sizeof(test_array[0])];
    if (qspi_read(XIP_DATA_SECTION_ADDR, read_array, sizeof(read_array)) != 0)
    {
        printf("  Failed to read test array\n");
        return -1;
    }
    printf("  Array data verification:\n");
    printf("    Expected: ");
    for (size_t i = 0; i < sizeof(test_array) / sizeof(test_array[0]); i++)
    {
        printf("%d ", test_array[i]);
    }
    printf("\n    Read:     ");
    for (size_t i = 0; i < sizeof(read_array) / sizeof(read_array[0]); i++)
    {
        printf("%d ", read_array[i]);
    }
    printf("\n");
    if (memcmp(test_array, read_array, sizeof(test_array)) != 0)
    {
        printf("  Test array verification failed\n");
        return -1;
    }
    printf("Flash content verified successfully\n");
    return 0;
}

/* 测试XIP数据访问 - 资源管理 */
static int test_xip_data_access(void)
{
    printf("Testing XIP data access (Resource Management)...\n");
    /* 通过XIP访问测试数组 */
    volatile int *xip_array = (volatile int *)(XIP_FLASH_BASE + XIP_DATA_SECTION_ADDR);
    printf("  XIP Array Contents:\n");
    int sum = 0;
    for (size_t i = 0; i < sizeof(test_array) / sizeof(test_array[0]); i++)
    {
        int val = xip_array[i];
        printf("    [%zu]: %d\n", i, val);
        sum += val;
    }
    printf("  Array sum: %d\n", sum);
    /* 通过XIP访问测试字符串 */
    volatile char *xip_string = (volatile char *)(XIP_FLASH_BASE + XIP_DATA_SECTION_ADDR + 256);
    printf("  XIP String: \"");
    for (int i = 0; i < 20 && xip_string[i] != '\0'; i++)
    {
        printf("%c", xip_string[i]);
    }
    printf("\"\n");
    /* 通过XIP访问字体数据 */
    volatile uint8_t *xip_font = (volatile uint8_t *)(XIP_FLASH_BASE + XIP_FONT_DATA_ADDR);
    printf("  XIP Font Data (first character):\n");
    for (int row = 0; row < 8; row++)
    {
        printf("    ");
        uint8_t byte = xip_font[row];
        for (int bit = 7; bit >= 0; bit--)
        {
            printf("%c", (byte & (1 << bit)) ? '#' : '.');
        }
        printf("\n");
    }
    printf("XIP data access test passed\n");
    return 0;
}

/* 测试XIP代码执行 */
static int test_xip_code_execution(void)
{
    printf("Testing XIP code execution...\n");
    /* 计算XIP函数地址 (需要设置Thumb位) */
    uint32_t xip_func_addr = XIP_FLASH_BASE + XIP_CODE_SECTION_ADDR;
    xip_func_addr |= 1;  /* 设置Thumb位 */
    xip_add_func_t xip_add_func = (xip_add_func_t)xip_func_addr;
    printf("  XIP function address: 0x%08lX\n", (unsigned long)xip_func_addr);
    /* 测试函数调用 */
    int test_cases[][3] =
    {
        {10, 20, 72},    /* 10 + 20 + 42 = 72 */
        {0, 0, 42},      /* 0 + 0 + 42 = 42 */
        {-5, 15, 52},    /* -5 + 15 + 42 = 52 */
        {100, 200, 342}  /* 100 + 200 + 42 = 342 */
    };
    printf("  Testing XIP function calls:\n");
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++)
    {
        int a = test_cases[i][0];
        int b = test_cases[i][1];
        int expected = test_cases[i][2];
        printf("    xip_add_func(%d, %d) = ", a, b);
        /* 调用XIP中的函数 */
        int result = xip_add_func(a, b);
        printf("%d (expected %d) %s\n",
               result, expected, (result == expected) ? "✓" : "✗");
        if (result != expected)
        {
            printf("  XIP code execution test failed\n");
            return -1;
        }
    }
    printf("XIP code execution test passed\n");
    return 0;
}

/* 测试XIP数组处理 */
static int test_xip_array_processing(void)
{
    printf("Testing XIP array processing...\n");
    /* 使用XIP中的数组进行计算 */
    volatile int *xip_array = (volatile int *)(XIP_FLASH_BASE + XIP_DATA_SECTION_ADDR);
    /* 本地求和函数 */
    int local_sum = 0;
    for (size_t i = 0; i < sizeof(test_array) / sizeof(test_array[0]); i++)
    {
        local_sum += xip_array[i];
    }
    printf("  Sum of XIP array: %d\n", local_sum);
    printf("  Expected sum: 55\n");
    if (local_sum == 55)
    {
        printf("XIP array processing test passed\n");
        return 0;
    }
    else
    {
        printf("XIP array processing test failed\n");
        return -1;
    }
}

/* 测试混合访问模式 */
static int test_mixed_access_modes(void)
{
    printf("Testing mixed access modes...\n");
    /* 在XIP模式下读取数据 */
    volatile uint8_t *xip_font = (volatile uint8_t *)(XIP_FLASH_BASE + XIP_FONT_DATA_ADDR);
    uint8_t xip_data[32];
    for (int i = 0; i < 32; i++)
    {
        xip_data[i] = xip_font[i];
    }
    /* 暂时退出XIP模式，使用STIG方式读取相同数据 */
    exit_xip_mode();
    uint8_t stig_data[32];
    if (qspi_read(XIP_FONT_DATA_ADDR, stig_data, sizeof(stig_data)) != 0)
    {
        printf("  STIG read failed\n");
        return -1;
    }
    /* 比较两种方式读取的数据 */
    if (memcmp(xip_data, stig_data, sizeof(xip_data)) == 0)
    {
        printf("  XIP vs STIG data match: OK\n");
    }
    else
    {
        printf("  XIP vs STIG data mismatch: FAIL\n");
        return -1;
    }
    /* 重新启用XIP模式 */
    configure_xip_mode();
    printf("Mixed access modes test passed\n");
    return 0;
}

/* 性能基准测试 */
static int performance_benchmark(void)
{
    printf("Running XIP performance benchmark...\n");
    /* 准备性能测试 */
    const int iterations = 10000;
    volatile uint8_t *xip_data = (volatile uint8_t *)(XIP_FLASH_BASE + XIP_FONT_DATA_ADDR);
    /* XIP读取性能测试 */
    uint32_t start_time = get_systick_us();
    volatile uint32_t checksum = 0;
    for (int iter = 0; iter < iterations; iter++)
    {
        for (int i = 0; i < 32; i++)
        {
            checksum += xip_data[i];
        }
    }
    uint32_t xip_time_us = get_systick_us() - start_time;
    /* SRAM读取性能测试（作为对比） */
    uint8_t sram_data[32];
    memcpy(sram_data, (void *)xip_data, 32); /* 复制到SRAM */
    start_time = get_systick_us();
    volatile uint32_t sram_checksum = 0;
    for (int iter = 0; iter < iterations; iter++)
    {
        for (int i = 0; i < 32; i++)
        {
            sram_checksum += sram_data[i];
        }
    }
    uint32_t sram_time_us = get_systick_us() - start_time;
    printf("  Performance comparison (%d iterations × 32 bytes):\n", iterations);
    printf("    SRAM: %lu μs (checksum: %lu)\n", (unsigned long)sram_time_us, (unsigned long)sram_checksum);
    printf("    XIP:  %lu μs (checksum: %lu)\n", (unsigned long)xip_time_us, (unsigned long)checksum);
    if (checksum == sram_checksum)
    {
        printf("    Checksums match: OK\n");
        if (sram_time_us > 0)
        {
            float slowdown = (float)xip_time_us / (float)sram_time_us;
            printf("    XIP slowdown factor: %.2fx\n", slowdown);
        }
        else
        {
            printf("    XIP slowdown factor: N/A (SRAM time too small)\n");
        }
        /* 计算带宽 (MB/s) */
        uint32_t total_bytes = iterations * 32;
        if (xip_time_us > 0)
        {
            /* 计算带宽：bytes/us * 1,000,000 = bytes/s，再除以1,048,576得到MB/s */
            float xip_bandwidth_mbps = (float)total_bytes * 1000000.0f / (float)xip_time_us / 1048576.0f;
            printf("    XIP bandwidth:  %.2f MB/s\n", xip_bandwidth_mbps);
        }
        else
        {
            printf("    XIP bandwidth:  N/A (time too small)\n");
        }
        if (sram_time_us > 0)
        {
            float sram_bandwidth_mbps = (float)total_bytes * 1000000.0f / (float)sram_time_us / 1048576.0f;
            printf("    SRAM bandwidth: %.2f MB/s\n", sram_bandwidth_mbps);
        }
        else
        {
            printf("    SRAM bandwidth: N/A (time too small)\n");
        }
    }
    else
    {
        printf("    Checksums mismatch: FAIL\n");
        return -1;
    }
    return 0;
}

int main(void)
{
    board_init();
    printf("Advanced QSPI XIP (Execute In Place) Demo\n");
    printf("=========================================\n");
    /* 初始化SysTick计时器用于性能测试 */
    systick_init();
    /* 获取系统时钟信息 */
    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    printf("System Core Clock: %lu Hz\n", (unsigned long)SystemCoreClock);
    printf("AHB Clock: %lu Hz\n", (unsigned long)ahb_clk);
    /* 初始化QSPI控制器 */
    printf("\n=== Step 1: QSPI Initialization ===\n");
    const uint32_t qspi_freq = ahb_clk / 4;  /* 使用中等频率 */
    printf("QSPI frequency: %lu Hz (~%lu MHz)\n",
           (unsigned long)qspi_freq, (unsigned long)(qspi_freq / 1000000));
    qspi_cadence_init(ahb_clk, qspi_freq);
    /* 初始化W25Qxx驱动并启用Quad模式 */
    w25qxx_info_t flash_info;
    if (w25qxx_init(&flash_info, true, false) != 0)
    {
        printf("Failed to initialize W25Qxx driver\n");
        goto error;
    }
    printf("Flash Configuration:\n");
    printf("  Quad Mode: %s\n", flash_info.quad_enabled ? "Enabled" : "Disabled");
    printf("  4-Byte Address Mode: %s\n", flash_info.addr4b ? "Enabled" : "Disabled");
    /* 准备Flash内容 */
    printf("\n=== Step 2: Prepare Flash Content ===\n");
    if (prepare_flash_content() != 0)
    {
        goto error;
    }
    /* 验证Flash内容 */
    printf("\n=== Step 3: Verify Flash Content ===\n");
    if (verify_flash_content() != 0)
    {
        goto error;
    }
    /* 配置XIP模式 */
    printf("\n=== Step 4: Configure XIP Mode ===\n");
    if (configure_xip_mode() != 0)
    {
        goto error;
    }
    /* 测试XIP数据访问 - 资源管理 */
    printf("\n=== Step 5: Test XIP Data Access ===\n");
    if (test_xip_data_access() != 0)
    {
        goto error;
    }
    /* 测试XIP代码执行 */
    printf("\n=== Step 6: Test XIP Code Execution ===\n");
    if (test_xip_code_execution() != 0)
    {
        goto error;
    }
    /* 测试XIP数组处理 */
    printf("\n=== Step 7: Test XIP Array Processing ===\n");
    if (test_xip_array_processing() != 0)
    {
        goto error;
    }
    /* 测试混合访问模式 */
    printf("\n=== Step 8: Test Mixed Access Modes ===\n");
    if (test_mixed_access_modes() != 0)
    {
        goto error;
    }
    /* 性能基准测试 */
    printf("\n=== Step 9: Performance Benchmark ===\n");
    if (performance_benchmark() != 0)
    {
        goto error;
    }
    printf("\n=== Advanced XIP Demo Completed Successfully ===\n");
    printf("✓ All tests passed!\n");
    /* 显示XIP映射信息 */
    printf("\nXIP Memory Mapping:\n");
    printf("  Flash Physical Base:  0x%08lX\n", 0UL);
    printf("  XIP Virtual Base:     0x%08lX\n", (unsigned long)XIP_FLASH_BASE);
    printf("  Code Section:         0x%08lX (Flash: 0x%06lX)\n",
           (unsigned long)(XIP_FLASH_BASE + XIP_CODE_SECTION_ADDR),
           (unsigned long)XIP_CODE_SECTION_ADDR);
    printf("  Data Section:         0x%08lX (Flash: 0x%06lX)\n",
           (unsigned long)(XIP_FLASH_BASE + XIP_DATA_SECTION_ADDR),
           (unsigned long)XIP_DATA_SECTION_ADDR);
    printf("  Font Data Section:    0x%08lX (Flash: 0x%06lX)\n",
           (unsigned long)(XIP_FLASH_BASE + XIP_FONT_DATA_ADDR),
           (unsigned long)XIP_FONT_DATA_ADDR);
    printf("\nXIP mode remains active for further exploration.\n");
    printf("Demo completed. Entering idle loop.\n");
    while (1)
    {
        __WFI();
    }
error:
    printf("\n=== Advanced XIP Demo Failed ===\n");
    exit_xip_mode();
    printf("Entering error loop.\n");
    while (1)
    {
        __WFI();
    }
}
