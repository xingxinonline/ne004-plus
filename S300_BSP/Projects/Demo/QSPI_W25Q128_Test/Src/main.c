#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "w25qxx.h"

#define TEST_PAGE_SIZE          W25QXX_PAGE_SIZE
#define TEST_SUBSECTOR_SIZE     W25QXX_SUBSECTOR_SIZE

static const uint8_t g_xip_exec_stub_code[] = {
    0x5A, 0x20,
    0x70, 0x47
};

static const uint32_t g_xip_exec_expected_value = 0x5Au;

typedef enum {
    TIMER_SOURCE_DWT = 0,
    TIMER_SOURCE_SYSTICK = 1
} timer_source_t;

static w25qxx_device_t g_flash;
static timer_source_t g_timer_source = TIMER_SOURCE_DWT;
static uint32_t g_timer_frequency_hz = 0u;
static uint32_t g_systick_load_value = 0u;
static uint32_t g_systick_reload_ticks = 0u;
static volatile uint32_t g_systick_overflow_count = 0u;

static void cycle_counter_init(void)
{
    bool dwt_available = false;

#if defined(DWT) && defined(CoreDebug)
    if ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0u)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0u;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        uint32_t before = DWT->CYCCNT;
        for (volatile int i = 0; i < 64; i++)
        {
            __NOP();
        }
        uint32_t after = DWT->CYCCNT;
        if ((after - before) > 0u)
        {
            dwt_available = true;
        }
        else
        {
            DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
        }
    }
#endif

    if (dwt_available)
    {
        g_timer_source = TIMER_SOURCE_DWT;
        g_timer_frequency_hz = SystemCoreClock;
        printf("Timing source: DWT cycle counter\n");
        return;
    }

    g_timer_source = TIMER_SOURCE_SYSTICK;

    SysTick->CTRL = 0u;
    SysTick->LOAD = 0xFFFFFFu;
    SysTick->VAL = 0u;
    g_systick_load_value = SysTick->LOAD;
    g_systick_reload_ticks = g_systick_load_value + 1u;
    g_systick_overflow_count = 0u;

    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
    g_timer_frequency_hz = SystemCoreClock;

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    __enable_irq();

    printf("Timing source: SysTick (HCLK)\n");
}

static uint64_t get_time_ticks(void)
{
    if (g_timer_source == TIMER_SOURCE_DWT)
    {
        return (uint64_t)DWT->CYCCNT;
    }

    uint32_t ov = g_systick_overflow_count;
    uint32_t val = SysTick->VAL;
    uint32_t ctrl = SysTick->CTRL;

    if ((ctrl & SysTick_CTRL_COUNTFLAG_Msk) != 0u)
    {
        ov++;
        val = SysTick->VAL;
    }

    uint32_t ov2 = g_systick_overflow_count;
    if (ov2 != ov)
    {
        ov = ov2;
        val = SysTick->VAL;
    }

    uint64_t base_ticks = (uint64_t)ov * (uint64_t)g_systick_reload_ticks;
    uint32_t elapsed_in_cycle = g_systick_load_value - val;
    return base_ticks + (uint64_t)elapsed_in_cycle;
}

static uint32_t ticks_to_us(uint64_t ticks)
{
    if (g_timer_frequency_hz == 0u)
    {
        return 0u;
    }
    uint64_t temp = ticks * 1000000ULL;
    temp /= (uint64_t)g_timer_frequency_hz;
    return (uint32_t)temp;
}

static uint32_t ticks_to_ms(uint64_t ticks)
{
    if (g_timer_frequency_hz == 0u)
    {
        return 0u;
    }
    uint64_t temp = ticks * 1000ULL;
    temp /= (uint64_t)g_timer_frequency_hz;
    return (uint32_t)temp;
}

void SysTick_Handler(void)
{
    if (g_timer_source == TIMER_SOURCE_SYSTICK)
    {
        g_systick_overflow_count++;
    }
}

static inline uint8_t throughput_pattern(uint32_t offset)
{
    uint8_t low = (uint8_t)(offset & 0xFFu);
    uint8_t mid = (uint8_t)((offset >> 8) & 0xFFu);
    uint8_t high = (uint8_t)((offset >> 16) & 0xFFu);
    uint8_t value = (uint8_t)(0x5Au ^ low);
    value = (uint8_t)(value + (uint8_t)(0x33u ^ mid));
    value = (uint8_t)(value ^ high);
    return value;
}

static int measure_subsector_erase_time(w25qxx_device_t *flash, uint32_t address, uint32_t *duration_us)
{
    if (!flash)
    {
        return -1;
    }

    uint32_t aligned_address = address & ~(TEST_SUBSECTOR_SIZE - 1u);
    uint64_t start_ticks = get_time_ticks();
    int rc = w25qxx_erase_subsector(flash, aligned_address);
    uint64_t elapsed_ticks = get_time_ticks() - start_ticks;

    if (duration_us != NULL)
    {
        *duration_us = ticks_to_us(elapsed_ticks);
    }

    if (rc == 0)
    {
        printf("STIG subsector erase completed in %lu us (%lu ms)\n",
               (unsigned long)ticks_to_us(elapsed_ticks),
               (unsigned long)ticks_to_ms(elapsed_ticks));
    }

    return rc;
}

static int dac_write_pattern_range(w25qxx_device_t *flash, uint32_t address, uint32_t length)
{
    if (!flash || length == 0u)
    {
        return -1;
    }

    uint8_t page_buffer[TEST_PAGE_SIZE];
    uint32_t remaining = length;
    uint32_t curr_addr = address;

    while (remaining > 0u)
    {
        uint32_t chunk = (remaining > TEST_PAGE_SIZE) ? TEST_PAGE_SIZE : remaining;
        for (uint32_t i = 0; i < chunk; i++)
        {
            uint32_t global_offset = (curr_addr + i) - address;
            page_buffer[i] = throughput_pattern(global_offset);
        }
        if (w25qxx_direct_write(flash, curr_addr, page_buffer, chunk) != 0)
        {
            return -1;
        }
        curr_addr += chunk;
        remaining -= chunk;
    }

    return 0;
}

static int dac_write_pattern_range_timed(w25qxx_device_t *flash, uint32_t address, uint32_t length, uint64_t *total_ticks)
{
    if (!flash || length == 0u || !total_ticks)
    {
        return -1;
    }

    uint8_t page_buffer[TEST_PAGE_SIZE];
    uint32_t remaining = length;
    uint32_t curr_addr = address;
    *total_ticks = 0ULL;

    if (w25qxx_direct_mode_begin(flash) != 0)
    {
        return -1;
    }

    volatile uint8_t *flash_ptr_base = w25qxx_direct_base(flash);
    if (!flash_ptr_base)
    {
        w25qxx_direct_mode_end(flash);
        return -1;
    }

    while (remaining > 0u)
    {
        uint32_t offset_in_page = curr_addr & (TEST_PAGE_SIZE - 1u);
        uint32_t chunk = TEST_PAGE_SIZE - offset_in_page;
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        for (uint32_t i = 0; i < chunk; i++)
        {
            uint32_t global_offset = (curr_addr + i) - address;
            page_buffer[i] = throughput_pattern(global_offset);
        }

        if (w25qxx_write_enable(flash) != 0)
        {
            w25qxx_direct_mode_end(flash);
            return -1;
        }

        uint64_t t0 = get_time_ticks();
        volatile uint8_t *flash_ptr = flash_ptr_base + curr_addr;
        for (uint32_t i = 0; i < chunk; i++)
        {
            flash_ptr[i] = page_buffer[i];
        }
        if (w25qxx_wait_busy_clear(flash, 200u) != 0)
        {
            w25qxx_direct_mode_end(flash);
            return -1;
        }
        uint64_t t1 = get_time_ticks();
        *total_ticks += (t1 - t0);

        curr_addr += chunk;
        remaining -= chunk;
    }

    return w25qxx_direct_mode_end(flash);
}

static int dac_read_and_verify_range_timed(w25qxx_device_t *flash,
                                           uint32_t address,
                                           uint32_t length,
                                           uint64_t *total_ticks,
                                           uint32_t *mismatch_offset,
                                           uint8_t *expected_value,
                                           uint8_t *actual_value)
{
    if (!flash || length == 0u || !total_ticks)
    {
        return -1;
    }

    uint8_t page_buffer[TEST_PAGE_SIZE];
    uint32_t remaining = length;
    uint32_t curr_addr = address;
    uint32_t total_offset = 0u;
    *total_ticks = 0ULL;

    if (w25qxx_direct_mode_begin(flash) != 0)
    {
        return -1;
    }

    volatile const uint8_t *flash_ptr_base = w25qxx_direct_base(flash);
    if (!flash_ptr_base)
    {
        w25qxx_direct_mode_end(flash);
        return -1;
    }

    while (remaining > 0u)
    {
        uint32_t chunk = (remaining > TEST_PAGE_SIZE) ? TEST_PAGE_SIZE : remaining;

        uint64_t t0 = get_time_ticks();
        volatile const uint8_t *flash_ptr = flash_ptr_base + curr_addr;
        for (uint32_t i = 0; i < chunk; i++)
        {
            page_buffer[i] = flash_ptr[i];
        }
        uint64_t t1 = get_time_ticks();
        *total_ticks += (t1 - t0);

        for (uint32_t i = 0; i < chunk; i++)
        {
            uint8_t expected = throughput_pattern(total_offset + i);
            uint8_t actual = page_buffer[i];
            if (actual != expected)
            {
                if (mismatch_offset)
                {
                    *mismatch_offset = total_offset + i;
                }
                if (expected_value)
                {
                    *expected_value = expected;
                }
                if (actual_value)
                {
                    *actual_value = actual;
                }
                w25qxx_direct_mode_end(flash);
                return -1;
            }
        }

        curr_addr += chunk;
        total_offset += chunk;
        remaining -= chunk;
    }

    return w25qxx_direct_mode_end(flash);
}

static int test_dac_throughput_1mb(w25qxx_device_t *flash, const w25qxx_info_t *info)
{
    if (!flash || !info || info->size_bytes == 0u)
    {
        return -1;
    }

    const uint32_t test_size = 1024u * 1024u;
    const uint32_t block_size = 65536u;

    if (info->size_bytes < test_size)
    {
        printf("Flash size ( %lu bytes ) too small for 1MB DAC test\n",
               (unsigned long)info->size_bytes);
        return -1;
    }

    uint32_t region_start = info->size_bytes - test_size;
    region_start &= ~(block_size - 1u);
    uint32_t region_end = region_start + test_size;

    printf("Preparing 1MB DAC test region: 0x%08lX - 0x%08lX\n",
           (unsigned long)region_start,
           (unsigned long)(region_end - 1u));

    const uint32_t block_count = test_size / block_size;
    for (uint32_t i = 0; i < block_count; i++)
    {
        uint32_t block_address = region_start + (i * block_size);
        if (w25qxx_erase_subsector(flash, block_address) != 0)
        {
            printf("Failed to erase first subsector of block at 0x%08lX\n",
                   (unsigned long)block_address);
            return -1;
        }
        for (uint32_t subsector = TEST_SUBSECTOR_SIZE;
             subsector < block_size;
             subsector += TEST_SUBSECTOR_SIZE)
        {
            uint32_t subsector_addr = block_address + subsector;
            if (w25qxx_erase_subsector(flash, subsector_addr) != 0)
            {
                printf("Failed to erase subsector at 0x%08lX\n",
                       (unsigned long)subsector_addr);
                return -1;
            }
        }
    }

    printf("Erase for 1MB DAC test region completed\n");

    uint64_t write_ticks = 0ULL;
    if (dac_write_pattern_range_timed(flash, region_start, test_size, &write_ticks) != 0)
    {
        printf("DAC 1MB write failed\n");
        return -1;
    }

    uint64_t read_ticks = 0ULL;
    uint32_t mismatch_offset = 0u;
    uint8_t expected_value = 0u;
    uint8_t actual_value = 0u;
    int verify_result = dac_read_and_verify_range_timed(flash,
                                                        region_start,
                                                        test_size,
                                                        &read_ticks,
                                                        &mismatch_offset,
                                                        &expected_value,
                                                        &actual_value);

    if (write_ticks == 0ULL || read_ticks == 0ULL)
    {
        printf("Warning: measured 0 ticks; timer likely wrapped or not configured as expected. Results may be inaccurate.\n");
    }

    printf("DAC 1MB write time: %lu us (%lu ms)\n",
           (unsigned long)ticks_to_us(write_ticks),
           (unsigned long)ticks_to_ms(write_ticks));
    printf("DAC 1MB read time: %lu us (%lu ms)\n",
           (unsigned long)ticks_to_us(read_ticks),
           (unsigned long)ticks_to_ms(read_ticks));

    if (verify_result != 0)
    {
        printf("DAC 1MB verify failed at offset %lu (expected 0x%02X, actual 0x%02X)\n",
               (unsigned long)mismatch_offset,
               expected_value,
               actual_value);
        return -1;
    }

    uint32_t write_us = ticks_to_us(write_ticks);
    uint32_t read_us = ticks_to_us(read_ticks);
    if (write_us > 0u)
    {
        uint64_t throughput_write = ((uint64_t)test_size * 1000000ULL) / (uint64_t)write_us;
        printf("Approximate write throughput: %lu bytes/s\n", (unsigned long)throughput_write);
    }
    if (read_us > 0u)
    {
        uint64_t throughput_read = ((uint64_t)test_size * 1000000ULL) / (uint64_t)read_us;
        printf("Approximate read throughput: %lu bytes/s\n", (unsigned long)throughput_read);
    }

    printf("DAC 1MB throughput test completed successfully\n");
    return 0;
}

static int test_xip_mode_144(w25qxx_device_t *flash, const w25qxx_info_t *info, bool refresh_pattern)
{
    if (!flash || !info)
    {
        return -1;
    }

    if (info->type != W25QXX_FLASH_W25Q)
    {
        printf("Skipping XIP test: flash type %s not Winbond W25Q\n", info->type_name);
        return 0;
    }

    if (info->size_bytes < (2u * TEST_SUBSECTOR_SIZE))
    {
        printf("Skipping XIP test: flash density too small\n");
        return -1;
    }

    uint32_t subsector_base = info->size_bytes - (2u * TEST_SUBSECTOR_SIZE);
    subsector_base &= ~(TEST_SUBSECTOR_SIZE - 1u);
    uint32_t test_address = subsector_base;
    const uint32_t sample_len = TEST_PAGE_SIZE;
    const uint32_t exec_offset = sample_len;
    const uint32_t exec_address = test_address + exec_offset;

    uint8_t program_buffer[TEST_PAGE_SIZE];
    uint8_t baseline_buffer[TEST_PAGE_SIZE];
    uint8_t xip_buffer[TEST_PAGE_SIZE];
    uint8_t exit_buffer[TEST_PAGE_SIZE];
    uint8_t exec_verify[sizeof(g_xip_exec_stub_code)];

    for (uint32_t i = 0; i < sample_len; ++i)
    {
        program_buffer[i] = throughput_pattern(i);
    }

    printf("Starting XIP 1-4-4 dummy=4 test at 0x%08lX\n", (unsigned long)test_address);

    if (refresh_pattern)
    {
        if (w25qxx_erase_subsector(flash, subsector_base) != 0)
        {
            printf("XIP test: subsector erase failed\n");
            return -1;
        }

        if (dac_write_pattern_range(flash, test_address, sample_len) != 0)
        {
            printf("XIP test: pattern program failed\n");
            return -1;
        }

        if (w25qxx_direct_write(flash, exec_address, g_xip_exec_stub_code,
                                (uint32_t)sizeof(g_xip_exec_stub_code)) != 0)
        {
            printf("XIP test: execution stub program failed\n");
            return -1;
        }
    }
    else
    {
        printf("XIP test: reuse existing pattern (skip erase/program)\n");
    }

    if (w25qxx_direct_read(flash, exec_address, exec_verify,
                           (uint32_t)sizeof(exec_verify)) != 0)
    {
        printf("XIP test: execution stub readback failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < (uint32_t)sizeof(exec_verify); ++i)
    {
        if (exec_verify[i] != g_xip_exec_stub_code[i])
        {
            printf("XIP test: execution stub verify mismatch at byte %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, g_xip_exec_stub_code[i], exec_verify[i]);
            return -1;
        }
    }

    if (w25qxx_direct_read(flash, test_address, baseline_buffer, sample_len) != 0)
    {
        printf("XIP test: baseline read failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < sample_len; ++i)
    {
        if (baseline_buffer[i] != program_buffer[i])
        {
            printf("XIP test: baseline verify mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, program_buffer[i], baseline_buffer[i]);
            return -1;
        }
    }

    w25qxx_xip_state_t restore = {0};
    int ret = -1;

    if (w25qxx_enter_xip_144(flash, &restore) != 0)
    {
        printf("Failed to enter XIP 1-4-4 mode\n");
        goto exit_restore;
    }

    volatile const uint8_t *xip_ptr = w25qxx_direct_base(flash);
    if (!xip_ptr)
    {
        printf("XIP test: direct base unavailable\n");
        goto exit_restore;
    }

    xip_ptr += test_address;
    for (uint32_t i = 0; i < sample_len; ++i)
    {
        xip_buffer[i] = xip_ptr[i];
    }

    for (uint32_t i = 0; i < sample_len; ++i)
    {
        if (xip_buffer[i] != program_buffer[i])
        {
            printf("XIP test: XIP read mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, program_buffer[i], xip_buffer[i]);
            goto exit_restore;
        }
    }

    printf("XIP test: XIP read verified successfully\n");

    typedef uint32_t (*xip_exec_fn_t)(void);
    uintptr_t exec_ptr = (uintptr_t)w25qxx_direct_base(flash) + (uintptr_t)exec_address;
    xip_exec_fn_t exec_fn = (xip_exec_fn_t)(exec_ptr | (uintptr_t)1u);
    uint32_t exec_result = exec_fn();

    if (exec_result != g_xip_exec_expected_value)
    {
        printf("XIP test: instruction execution returned 0x%08lX (expected 0x%08lX)\n",
               (unsigned long)exec_result,
               (unsigned long)g_xip_exec_expected_value);
        goto exit_restore;
    }

    printf("XIP test: instruction fetch execution result 0x%08lX verified\n",
           (unsigned long)exec_result);

    ret = 0;

exit_restore:
    if (w25qxx_exit_xip(flash, &restore, test_address) != 0)
    {
        printf("Failed to exit XIP mode cleanly\n");
        ret = -1;
    }

    if (ret == 0)
    {
        if (w25qxx_direct_read(flash, test_address, exit_buffer, sample_len) != 0)
        {
            printf("XIP test: post-exit read failed\n");
            ret = -1;
        }
        else
        {
            for (uint32_t i = 0; i < sample_len; ++i)
            {
                if (exit_buffer[i] != program_buffer[i])
                {
                    printf("XIP test: post-exit verify mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                           (unsigned long)i, program_buffer[i], exit_buffer[i]);
                    ret = -1;
                    break;
                }
            }
        }
    }

    if (ret == 0)
    {
        printf("XIP 1-4-4 dummy=4 test completed successfully\n");
    }

    return ret;
}

static int perform_direct_access_test(w25qxx_device_t *flash)
{
    if (!flash)
    {
        return -1;
    }

    const w25qxx_info_t *info = w25qxx_get_info(flash);
    if (!info)
    {
        return -1;
    }

    uint32_t test_address = info->size_bytes - (3u * TEST_SUBSECTOR_SIZE);
    test_address &= ~(TEST_SUBSECTOR_SIZE - 1u);

    uint8_t write_buffer[TEST_PAGE_SIZE];
    uint8_t direct_read_buffer[TEST_PAGE_SIZE];

    for (uint32_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        write_buffer[i] = (uint8_t)(0xA5u ^ i);
    }

    if (w25qxx_erase_subsector(flash, test_address) != 0)
    {
        printf("DAC write failed: subsector erase failed\n");
        return -1;
    }

    printf("DAC write start\n");
    if (w25qxx_direct_write(flash, test_address, write_buffer, TEST_PAGE_SIZE) != 0)
    {
        printf("DAC write failed\n");
        return -1;
    }
    printf("DAC write success\n");

    printf("DAC read start\n");
    if (w25qxx_direct_read(flash, test_address, direct_read_buffer, TEST_PAGE_SIZE) != 0)
    {
        printf("DAC read failed\n");
        return -1;
    }
    printf("DAC read success\n");

    printf("DAC verify start\n");
    for (uint32_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        if (direct_read_buffer[i] != write_buffer[i])
        {
            printf("DAC verify failed: data mismatch at offset %lu\n", (unsigned long)i);
            return -1;
        }
    }
    printf("DAC verify success\n");

    return 0;
}

int main(void)
{
    printf("Initialization start\n");

    board_init();
    printf("Board initialization completed\n");

    cycle_counter_init();

    printf("Program start\n");
    printf("Clear status\n");
    printf("Clock initialization completed\n");

    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0u)
    {
        ahb_clk = SystemCoreClock;
    }
    printf("AHB clock frequency: %lu Hz\n", (unsigned long)ahb_clk);

    w25qxx_info_t flash_info = {0};
    w25qxx_bus_config_t bus_cfg = {
        .reg_base = QSPI_CFG_BASE,
        .ahb_base = M4_SLV_FLASH_BASE,
        .ref_clk_hz = ahb_clk,
        .trigger_address = M4_SLV_FLASH_BASE,
        .sram_partition = 0u
    };

    int init_result = w25qxx_init(&g_flash, &bus_cfg, 24000000u, true, true, &flash_info);
    if (init_result != 0)
    {
        printf("QSPI initialization failed\n");
    }
    else
    {
        printf("QSPI initialization success\n");
        printf("Flash info: Manufacturer ID=0x%02X, Device ID=0x%02X, Capacity Code=0x%02X, Type=%s, Size=%lu bytes\n",
               flash_info.manuf_id,
               flash_info.memory_type,
               flash_info.capacity,
               flash_info.type_name,
               (unsigned long)flash_info.size_bytes);
        printf("Flash ID read success\n");
    }

    int test_result = init_result;
    if (init_result == 0)
    {
        if (w25qxx_configure_clock(&g_flash, 24000000u) != 0)
        {
            printf("Clock configuration failed\n");
            test_result = -1;
        }
    }

    int erase_time_result = -1;
    int dac_throughput_result = -1;
    int xip_test_24mhz_result = -1;
    int xip_test_48mhz_result = -1;
    int xip_test_96mhz_result = -1;
    int dac_test_result = -1;

    if (test_result == 0)
    {
        if (w25qxx_disable_block_protect(&g_flash) != 0)
        {
            printf("Write protect disable failed before throughput test\n");
            test_result = -1;
        }
        else if (w25qxx_enable_quad_mode(&g_flash, true) != 0)
        {
            printf("Quad enable failed before throughput test\n");
            test_result = -1;
        }
    }

    if (test_result == 0)
    {
        uint32_t erase_time_us = 0u;
        uint32_t erase_address = flash_info.size_bytes - TEST_SUBSECTOR_SIZE;
        erase_time_result = measure_subsector_erase_time(&g_flash, erase_address, &erase_time_us);

        if (erase_time_result == 0)
        {
            printf("Measured erase time at address 0x%08lX\n", (unsigned long)erase_address);
            dac_throughput_result = test_dac_throughput_1mb(&g_flash, &flash_info);
            if (dac_throughput_result == 0)
            {
                xip_test_24mhz_result = test_xip_mode_144(&g_flash, &flash_info, true);

                if (xip_test_24mhz_result == 0)
                {
                    printf("Reconfiguring QSPI for 48MHz XIP validation\n");
                    if (w25qxx_configure_clock(&g_flash, 48000000u) != 0)
                    {
                        printf("Failed to configure QSPI clock to 48MHz for XIP test\n");
                        xip_test_48mhz_result = -1;
                    }
                    else
                    {
                        printf("Starting XIP test at 48MHz\n");
                        xip_test_48mhz_result = test_xip_mode_144(&g_flash, &flash_info, false);

                        if (xip_test_48mhz_result == 0)
                        {
                            printf("Reconfiguring QSPI for 96MHz XIP validation\n");
                            if (w25qxx_configure_clock(&g_flash, 96000000u) != 0)
                            {
                                printf("Failed to configure QSPI clock to 96MHz for XIP test\n");
                                xip_test_96mhz_result = -1;
                            }
                            else
                            {
                                printf("Starting XIP test at 96MHz\n");
                                xip_test_96mhz_result = test_xip_mode_144(&g_flash, &flash_info, false);
                            }
                        }
                    }

                    if (w25qxx_configure_clock(&g_flash, 24000000u) != 0)
                    {
                        printf("Warning: failed to restore QSPI clock to 24MHz after high-speed XIP tests\n");
                    }
                }
            }
        }
    }

    if (test_result == 0 && dac_throughput_result == 0 &&
        xip_test_24mhz_result == 0 && xip_test_48mhz_result == 0 &&
        xip_test_96mhz_result == 0)
    {
        dac_test_result = perform_direct_access_test(&g_flash);
    }

    printf("Program end normally\n");

    if (init_result != 0)
    {
        printf("Initialization failed\n");
    }
    else if (test_result != 0)
    {
        printf("ID read failed\n");
    }
    else if (erase_time_result != 0)
    {
        printf("Erase timing test failed\n");
    }
    else if (dac_throughput_result != 0)
    {
        printf("DAC throughput test failed\n");
    }
    else if (xip_test_24mhz_result != 0)
    {
        printf("XIP mode test at 24MHz failed\n");
    }
    else if (xip_test_48mhz_result != 0)
    {
        printf("XIP mode test at 48MHz failed\n");
    }
    else if (xip_test_96mhz_result != 0)
    {
        printf("XIP mode test at 96MHz failed\n");
    }
    else if (dac_test_result != 0)
    {
        printf("DAC test failed\n");
    }
    else
    {
        printf("All tests successful\n");
    }

    while (1)
    {
        __WFI();
    }
}
