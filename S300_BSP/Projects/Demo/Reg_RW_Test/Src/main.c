#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "s300.h"
#include "rcc.h"
#include "gpio.h"

typedef uint32_t UINT32;

#define DEBUG_BASE    			(M4_SLV_RAM0_END - 0x3F) //RAM8K

#define DEBUG_ARG0				(DEBUG_BASE + 0x0000 ) //
#define DEBUG_ARG1				(DEBUG_BASE + 0x0004 ) //
#define DEBUG_ARG2				(DEBUG_BASE + 0x0008 ) //
#define DEBUG_ARG3				(DEBUG_BASE + 0x000C ) //

#define DEBUG_ST				(DEBUG_BASE + 0x0010 ) // 测试状态；OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_ISRS				(DEBUG_BASE + 0x0014 ) // 中断次数;目前程序中预设10次
#define DEBUG_CODE				(DEBUG_BASE + 0x0018 ) // 程序运行结束 OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_FLOW				(DEBUG_BASE + 0x001C ) //

//c语言调用
#define DEBUG_ARG0_c			(*((volatile UINT32*)(DEBUG_ARG0 ))) //
#define DEBUG_ARG1_c			(*((volatile UINT32*)(DEBUG_ARG1 ))) //
#define DEBUG_ARG2_c			(*((volatile UINT32*)(DEBUG_ARG2 ))) //
#define DEBUG_ARG3_c			(*((volatile UINT32*)(DEBUG_ARG3 ))) //

#define DEBUG_ST_c				(*((volatile UINT32*)(DEBUG_ST ))) // 测试状态；OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_ISRS_c			(*((volatile UINT32*)(DEBUG_ISRS ))) // 中断次数
#define DEBUG_CODE_c			(*((volatile UINT32*)(DEBUG_CODE ))) // 程序运行结束 OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_FLOW_c			(*((volatile UINT32*)(DEBUG_FLOW ))) //

#define TEST_REG_COUNT  5u

typedef struct {
    const char *name;
    volatile uint32_t *reg;
    uint32_t test_value;
    uint32_t expected_mask;
} reg_test_t;

static const reg_test_t reg_tests[TEST_REG_COUNT] = {
    {"GPIO_SWPORTA_DR",    &GPIO->SWPORTA_DR,    0x55555555u, 0xFFFFFFFFu},
    {"GPIO_SWPORTA_DDR",   &GPIO->SWPORTA_DDR,   0xAAAAAAAAu, 0xFFFFFFFFu},
    {"GPIO_SWPORTA_CTL",   &GPIO->SWPORTA_CTL,   0x55555555u, 0xFFFFFFFFu},
    {"RCC_CM4_AHB_CLK_EN", &RCC->CM4_AHB_CLK_EN, 0x00000001u, 0xFFFFFFFFu},
    {"RCC_CM4_APB0_CLK_EN",&RCC->CM4_APB0_CLK_EN,0x00000100u, 0xFFFFFFFFu}
};

static void update_reg_test_status(bool passed, uint32_t test_index)
{
    // Store test results in debug variables
    if (test_index < 4) {
        volatile uint32_t *debug_arg = (volatile uint32_t *)(DEBUG_BASE + test_index * 4);
        *debug_arg = passed ? 0xAAAAAAAAu : 0xEEEEEEEEu;
    }

    // Update flow counter
    DEBUG_FLOW_c = test_index + 1;
}

static int run_register_rw_tests(void)
{
    int failed_count = 0;

    // Initialize debug status
    DEBUG_ST_c = 0xBBBBBBBBu; // Testing in progress

    for (uint32_t i = 0; i < TEST_REG_COUNT; ++i)
    {
        const reg_test_t *test = &reg_tests[i];

        // Read original value
        uint32_t original_value = *test->reg;

        // Write test value
        *test->reg = test->test_value;

        // Read back
        uint32_t read_value = *test->reg;

        // Check if write was successful (considering mask)
        uint32_t expected_value = test->test_value & test->expected_mask;
        uint32_t actual_value = read_value & test->expected_mask;

        bool passed = (expected_value == actual_value);

        update_reg_test_status(passed, i);

        // Restore original value
        *test->reg = original_value;

        if (!passed) {
            ++failed_count;
        }
    }

    // Set final test status
    DEBUG_ST_c = (failed_count == 0) ? 0xAAAAAAAAu : 0xEEEEEEEEu;

    return failed_count == 0 ? 0 : -1;
}

int main(void)
{
    // Initialize debug variables
    DEBUG_CODE_c = 0xCCCCCCCCu; // Program starting
    DEBUG_ISRS_c = 0u;
    DEBUG_FLOW_c = 0u;

    // Run register tests
    int test_result = run_register_rw_tests();

    // Set final program status
    DEBUG_CODE_c = (test_result == 0) ? 0xAAAAAAAAu : 0xEEEEEEEEu;

    // Enter idle loop
    while (1) {
        __WFI();
    }
}