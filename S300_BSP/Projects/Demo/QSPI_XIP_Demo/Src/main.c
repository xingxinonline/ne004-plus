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
#define XIP_TEST_CODE_ADDR      0x100000UL      /* 测试代码存储在Flash的1MB偏移处 */
#define XIP_TEST_DATA_ADDR      0x110000UL      /* 测试数据存储在Flash的1.1MB偏移处 */

/* 测试函数类型定义 */
typedef int (*xip_test_func_t)(int a, int b);

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

/* 测试数据 */
static const uint32_t test_data[] =
{
    0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x87654321
};

/* 正确的测试函数机器码 (ARM Thumb) */
static const uint8_t correct_test_function_code[] =
{
    0x08, 0x44,     /* add r0, r0, r1    ; a + b */
    0x2A, 0x30,     /* adds r0, #42      ; + 42 */
    0x70, 0x47      /* bx lr             ; 返回 */
};

/* 配置XIP模式 */
static int configure_xip_mode(void)
{
    printf("Configuring XIP mode...\n");
    /* 首先配置Quad读取模式(1-4-4)，与SPL保持一致 */
    qspi_configure_quad_io_read(true, true);
    /* 启用直接访问模式 (XIP) 和在下个READ时进入XIP模式 */
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg |= (CQSPI_CFG_DIRECT | CQSPI_CFG_XIP_NEXT);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    printf("XIP mode configured\n");
    return 0;
}

/* 退出XIP模式 */
static int exit_xip_mode(void)
{
    printf("Exiting XIP mode...\n");
    /* 禁用直接访问模式 */
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_DIRECT | CQSPI_CFG_XIP_NEXT);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    /* 清除模式位配置 */
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    rd &= ~(1u << CQSPI_RD_MODE_EN_LSB);
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
    REG32(g_qspi.reg, CQSPI_REG_MODE_BIT) = 0;
    /* 恢复到标准读取模式 */
    qspi_configure_quad_io_read(false, false);
    printf("XIP mode exited\n");
    return 0;
}

/* 准备Flash中的测试内容 */
static int prepare_flash_content(void)
{
    printf("Preparing Flash content for XIP test...\n");
    /* 确保退出XIP模式以便写入 */
    exit_xip_mode();
    /* 擦除测试区域 */
    printf("  Erasing 64KB block @0x%06lX for code\n", (unsigned long)XIP_TEST_CODE_ADDR);
    if (qspi_erase_64k(XIP_TEST_CODE_ADDR) != 0)
    {
        printf("  Failed to erase code block\n");
        return -1;
    }
    printf("  Erasing 64KB block @0x%06lX for data\n", (unsigned long)XIP_TEST_DATA_ADDR);
    if (qspi_erase_64k(XIP_TEST_DATA_ADDR) != 0)
    {
        printf("  Failed to erase data block\n");
        return -1;
    }
    /* 写入测试函数代码 */
    printf("  Programming test function (%zu bytes)\n", sizeof(correct_test_function_code));
    if (qspi_page_program(XIP_TEST_CODE_ADDR, correct_test_function_code,
                          sizeof(correct_test_function_code)) != 0)
    {
        printf("  Failed to program test function\n");
        return -1;
    }
    /* 写入测试数据 */
    printf("  Programming test data (%zu bytes)\n", sizeof(test_data));
    if (qspi_page_program(XIP_TEST_DATA_ADDR, test_data, sizeof(test_data)) != 0)
    {
        printf("  Failed to program test data\n");
        return -1;
    }
    printf("Flash content prepared successfully\n");
    return 0;
}

/* 验证Flash内容 */
static int verify_flash_content(void)
{
    printf("Verifying Flash content...\n");
    /* 读回并验证函数代码 */
    uint8_t read_code[sizeof(correct_test_function_code)];
    if (qspi_read(XIP_TEST_CODE_ADDR, read_code, sizeof(read_code)) != 0)
    {
        printf("  Failed to read back function code\n");
        return -1;
    }
    if (memcmp(correct_test_function_code, read_code, sizeof(correct_test_function_code)) != 0)
    {
        printf("  Function code verification failed\n");
        printf("  Expected: ");
        for (size_t i = 0; i < sizeof(correct_test_function_code); i++)
        {
            printf("%02X ", correct_test_function_code[i]);
        }
        printf("\n  Got:      ");
        for (size_t i = 0; i < sizeof(read_code); i++)
        {
            printf("%02X ", read_code[i]);
        }
        printf("\n");
        return -1;
    }
    /* 读回并验证数据 */
    uint32_t read_data[sizeof(test_data) / sizeof(test_data[0])];
    if (qspi_read(XIP_TEST_DATA_ADDR, read_data, sizeof(read_data)) != 0)
    {
        printf("  Failed to read back test data\n");
        return -1;
    }
    if (memcmp(test_data, read_data, sizeof(test_data)) != 0)
    {
        printf("  Test data verification failed\n");
        return -1;
    }
    printf("Flash content verified successfully\n");
    return 0;
}

/* 测试XIP数据访问 */
static int test_xip_data_access(void)
{
    printf("Testing XIP data access...\n");
    /* 打印地址和基址信息 */
    printf("  XIP_FLASH_BASE: 0x%08lX\n", (unsigned long)XIP_FLASH_BASE);
    printf("  XIP_TEST_DATA_ADDR: 0x%08lX\n", (unsigned long)XIP_TEST_DATA_ADDR);
    printf("  Full XIP address: 0x%08lX\n", (unsigned long)(XIP_FLASH_BASE + XIP_TEST_DATA_ADDR));
    /* 通过XIP地址访问数据 */
    volatile uint32_t *xip_data = (volatile uint32_t *)(XIP_FLASH_BASE + XIP_TEST_DATA_ADDR);
    /* 先检查原始字节数据 */
    volatile uint8_t *xip_bytes = (volatile uint8_t *)(XIP_FLASH_BASE + XIP_TEST_DATA_ADDR);
    printf("  Raw bytes at XIP address:\n");
    for (int i = 0; i < 16; i++)
    {
        printf("    [%02d]: 0x%02X\n", i, xip_bytes[i]);
    }
    printf("  Reading data through XIP:\n");
    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        uint32_t val = xip_data[i];
        printf("    [%zu]: 0x%08lX (expected: 0x%08lX) %s\n",
               i, (unsigned long)val, (unsigned long)test_data[i],
               (val == test_data[i]) ? "OK" : "FAIL");
        if (val != test_data[i])
        {
            printf("  XIP data access test failed\n");
            return -1;
        }
    }
    printf("XIP data access test passed\n");
    return 0;
}

/* 测试XIP代码执行 */
static int test_xip_code_execution(void)
{
    printf("Testing XIP code execution...\n");
    /* 计算XIP函数地址 (需要设置Thumb位) */
    uint32_t xip_func_addr = XIP_FLASH_BASE + XIP_TEST_CODE_ADDR;
    xip_func_addr |= 1;  /* 设置Thumb位 */
    xip_test_func_t xip_func = (xip_test_func_t)xip_func_addr;
    printf("  XIP function address: 0x%08lX\n", (unsigned long)xip_func_addr);
    /* 测试函数调用 */
    int test_cases[][3] =
    {
        {5, 10, 57},    /* 5 + 10 + 42 = 57 */
        {0, 0, 42},     /* 0 + 0 + 42 = 42 */
        {-10, 15, 47},  /* -10 + 15 + 42 = 47 */
        {100, 200, 342} /* 100 + 200 + 42 = 342 */
    };
    printf("  Testing function calls:\n");
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++)
    {
        int a = test_cases[i][0];
        int b = test_cases[i][1];
        int expected = test_cases[i][2];
        printf("    Calling xip_func(%d, %d)...", a, b);
        /* 调用XIP中的函数 */
        int result = xip_func(a, b);
        printf(" result=%d, expected=%d %s\n",
               result, expected, (result == expected) ? "OK" : "FAIL");
        if (result != expected)
        {
            printf("  XIP code execution test failed\n");
            return -1;
        }
    }
    printf("XIP code execution test passed\n");
    return 0;
}

/* SRAM中的测试函数 */
static int sram_test_func(int a, int b)
{
    return a + b + 42;
}

/* 性能测试 */
static int performance_test(void)
{
    printf("Running XIP performance test...\n");
    /* 比较相同计算的性能：SRAM vs XIP */
    const int iterations = 10000;
    uint32_t xip_func_addr = XIP_FLASH_BASE + XIP_TEST_CODE_ADDR;
    xip_func_addr |= 1;  /* 设置Thumb位 */
    xip_test_func_t xip_func = (xip_test_func_t)xip_func_addr;
    /* 测试SRAM性能 */
    uint32_t start_time = get_systick_us();
    volatile int sram_result = 0;
    for (int i = 0; i < iterations; i++)
    {
        sram_result += sram_test_func(i, i + 1);
    }
    uint32_t sram_time = get_systick_us() - start_time;
    /* 测试XIP性能 */
    start_time = get_systick_us();
    volatile int xip_result = 0;
    for (int i = 0; i < iterations; i++)
    {
        xip_result += xip_func(i, i + 1);
    }
    uint32_t xip_time = get_systick_us() - start_time;
    printf("  Performance comparison (%d iterations):\n", iterations);
    printf("    SRAM: %lu us (result: %d)\n", (unsigned long)sram_time, sram_result);
    printf("    XIP:  %lu us (result: %d)\n", (unsigned long)xip_time, xip_result);
    if (sram_result == xip_result)
    {
        printf("    Results match: OK\n");
        float slowdown = (float)xip_time / (float)sram_time;
        printf("    XIP slowdown factor: %.2fx\n", slowdown);
    }
    else
    {
        printf("    Results mismatch: FAIL\n");
        return -1;
    }
    return 0;
}

int main(void)
{
    board_init();
    printf("QSPI XIP (Execute In Place) Demo\n");
    printf("================================\n");
    /* 初始化SysTick用于性能测试 */
    systick_init();
    /* 获取系统时钟信息 */
    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    printf("AHB clock: %lu Hz\n", (unsigned long)ahb_clk);
    /* 初始化QSPI控制器 */
    printf("\n=== Step 1: QSPI Initialization ===\n");
    const uint32_t qspi_freq = ahb_clk / 2;  /* 使用中等频率 */
    printf("QSPI frequency: %lu Hz\n", (unsigned long)qspi_freq);
    qspi_cadence_init(ahb_clk, qspi_freq);
    /* 初始化W25Qxx驱动并启用Quad模式 */
    w25qxx_info_t flash_info;
    if (w25qxx_init(&flash_info, true, false) != 0)
    {
        printf("Failed to initialize W25Qxx driver\n");
        goto error;
    }
    printf("Flash detected: Quad=%s, 4ByteAddr=%s\n",
           flash_info.quad_enabled ? "YES" : "NO",
           flash_info.addr4b ? "YES" : "NO");
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
    /* 测试XIP数据访问 */
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
    /* 性能测试 */
    printf("\n=== Step 7: Performance Test ===\n");
    if (performance_test() != 0)
    {
        goto error;
    }
    printf("\n=== XIP Demo Completed Successfully ===\n");
    printf("All tests passed!\n");
    /* 保持XIP模式活跃以供进一步测试 */
    printf("\nXIP mode remains active. Flash is mapped at 0x%08lX\n",
           (unsigned long)XIP_FLASH_BASE);
    printf("Test function at: 0x%08lX\n",
           (unsigned long)(XIP_FLASH_BASE + XIP_TEST_CODE_ADDR));
    printf("Test data at:     0x%08lX\n",
           (unsigned long)(XIP_FLASH_BASE + XIP_TEST_DATA_ADDR));
    printf("\nDemo completed. Entering idle loop.\n");
    while (1)
    {
        __WFI();
    }
error:
    printf("\n=== XIP Demo Failed ===\n");
    exit_xip_mode();
    printf("Entering error loop.\n");
    while (1)
    {
        __WFI();
    }
}
