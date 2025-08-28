#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

/* printf 已在 board_init 中完成串口与重定向初始化 */

/* 简易性能计时器：优先用 DWT，失败回退到 SysTick（核心时钟源） */
static bool g_use_dwt = false;
static uint32_t g_systick_reload = 0;

static void perf_timer_init(uint32_t cpu_hz)
{
    (void)cpu_hz;
    /* 尝试启用 DWT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* 尝试解锁 DWT（若实现了 LAR） */
    volatile uint32_t *DWT_LAR = (volatile uint32_t *)0xE0001FB0u;
    *DWT_LAR = 0xC5ACCE55u;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint32_t c0 = DWT->CYCCNT;
    for (volatile uint32_t i = 0; i < 1000u; ++i) __NOP();
    uint32_t c1 = DWT->CYCCNT;
    g_use_dwt = (c1 != c0);
    if (!g_use_dwt)
    {
        /* 配置 SysTick 为核心时钟、无中断、最大重装值，作为自由运行计数器 */
        SysTick->CTRL = 0; /* 先关 */
        SysTick->LOAD = 0xFFFFFFu;
        SysTick->VAL  = 0u;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; /* 不开中断 */
        g_systick_reload = SysTick->LOAD + 1u; /* 计数范围 */
    }
}

static inline uint32_t perf_now32(void)
{
    return g_use_dwt ? DWT->CYCCNT : SysTick->VAL; /* 注意：SysTick 向下计数 */
}

static uint32_t perf_cycles_since(uint32_t start)
{
    if (g_use_dwt)
    {
        uint32_t now = DWT->CYCCNT;
        return now - start; /* 自然溢出处理 */
    }
    /* SysTick: 向下计数，单次测量假定 < 一个重装周期（~87ms@192MHz） */
    uint32_t now = SysTick->VAL;
    if (start >= now) return start - now; /* 未溢出期间 */
    return (start + g_systick_reload) - now; /* 发生一次回绕 */
}

static void print_bw(const char *tag, uint32_t bytes, uint32_t cycles, uint32_t cpu_hz)
{
    if (cycles == 0u) cycles = 1u;
    /* 以 MB/s*100 打印，避免浮点 */
    uint64_t mbps_x100 = ((uint64_t)bytes * (uint64_t)cpu_hz * 100ull) / ((uint64_t)cycles * 1000000ull);
    unsigned whole = (unsigned)(mbps_x100 / 100ull);
    unsigned frac  = (unsigned)(mbps_x100 % 100ull);
    printf("%s: %lu bytes in %lu cycles -> %u.%02u MB/s\n",
           tag, (unsigned long)bytes, (unsigned long)cycles, whole, frac);
}

#define BW_TEST_SIZE   (16u * 1024u)  /* 16KB */

static const char *decode_flash_name(const uint8_t id[3], uint32_t *size_bytes)
{
    if (size_bytes) *size_bytes = 0;
    if (!id) return "Unknown";
    /* Winbond JEDEC: EFh 40h xxh, where xx is capacity code = log2(size in bytes) */
    if (id[0] == 0xEF && id[1] == 0x40)
    {
        uint32_t sz = 0u;
        if (id[2] >= 20 && id[2] <= 32) sz = 1u << id[2]; /* safe for <= 2^31 */
        if (size_bytes) *size_bytes = sz;
        switch (id[2])
        {
        case 0x14:
            return "Winbond W25Q80";  /* 1 MiB */
        case 0x15:
            return "Winbond W25Q16";  /* 2 MiB */
        case 0x16:
            return "Winbond W25Q32";  /* 4 MiB */
        case 0x17:
            return "Winbond W25Q64";  /* 8 MiB */
        case 0x18:
            return "Winbond W25Q128"; /* 16 MiB */
        case 0x19:
            return "Winbond W25Q256"; /* 32 MiB */
        default:
            return "Winbond W25Q series";
        }
    }
    return "Unknown";
}

int main(void)
{
    board_init();
    printf("QSPI W25Q128 AHB div test start\n");
    /* QSPI ref clock from AHB; use current AHB/system clock */
    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    printf("AHB clock: %lu Hz\n", (unsigned long)ahb_clk);
    perf_timer_init(SystemCoreClock);

    const uint32_t divisors[] = {4u, 6u, 8u};
    for (unsigned t = 0; t < sizeof(divisors)/sizeof(divisors[0]); ++t)
    {
        uint32_t div = divisors[t];
        uint32_t target_sclk = ahb_clk / div;
        printf("\n=== Test #%u: AHB/%lu -> target SCLK ~ %lu Hz ===\n", t + 1u, (unsigned long)div, (unsigned long)target_sclk);

        qspi_cadence_init(ahb_clk, target_sclk);
        qspi_dump_regs("after init");

        uint8_t id[3] = {0};
        if (qspi_read_id(id, sizeof id) == 0)
        {
            uint32_t sz = 0;
            const char *name = decode_flash_name(id, &sz);
            unsigned mib = (sz >> 20);
            if (mib)
                printf("RDID: %02X %02X %02X -> %s (%u MiB)\n", id[0], id[1], id[2], name, mib);
            else
                printf("RDID: %02X %02X %02X -> %s\n", id[0], id[1], id[2], name);
        }
        else
        {
            printf("RDID failed\n");
        }

        uint8_t sr1 = 0, sr2 = 0, sr3 = 0;
        if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
            printf("SR1=%02X SR2=%02X SR3=%02X\n", sr1, sr2, sr3);

        /* 也可以通过更高层的 w25qxx_init 试探配置 QE/4B（可选） */
        w25qxx_info_t info;
        (void)w25qxx_init(&info, false, false);

        /* 尝试解锁所有区域以防止写保护 */
        (void)qspi_unlock_all();

        /* 每个分频使用不同的 4K 扇区，避免重复磨损 */
        const uint32_t addr = (t * 0x1000u);
        uint8_t tx[256];
        for (uint32_t i = 0; i < sizeof tx; i++) tx[i] = (uint8_t)(i ^ (uint8_t)div ^ 0xA5u);

        printf("Erase 4K @0x%06lX...\n", (unsigned long)addr);
        if (qspi_erase_4k(addr) != 0)
        {
            printf("Erase failed (div=%lu)\n", (unsigned long)div);
            continue; /* 下一轮分频 */
        }
        if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
            printf("After erase: SR1=%02X SR2=%02X SR3=%02X\n", sr1, sr2, sr3);

        qspi_dump_regs("before program");
        printf("Program 256B page...\n");
        if (qspi_page_program(addr, tx, sizeof tx) != 0)
        {
            printf("Program failed (div=%lu)\n", (unsigned long)div);
            continue;
        }
        if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
            printf("After program: SR1=%02X SR2=%02X SR3=%02X\n", sr1, sr2, sr3);

        qspi_dump_regs("before read");
        uint8_t rx[256] = {0};
        if (qspi_read(addr, rx, sizeof rx) != 0)
        {
            printf("Read back failed (div=%lu)\n", (unsigned long)div);
            continue;
        }
        int ok = memcmp(tx, rx, sizeof tx) == 0;
        printf("Verify %s at AHB/%lu\n", ok ? "OK" : "FAIL", (unsigned long)div);
        if (!ok)
        {
            /* 打印首个不一致位置，便于定位 */
            for (unsigned i = 0; i < sizeof tx; ++i)
            {
                if (tx[i] != rx[i])
                {
                    printf("Mismatch @%u: tx=%02X rx=%02X\n", i, tx[i], rx[i]);
                    break;
                }
            }
            continue;
        }

        /* 带宽测试：读 BW 与写 BW（间接/退化在驱动内处理） */
        enum { BW_ADDR_BASE = 0x010000u }; /* 避免与功能性测试同扇区重叠 */
        uint32_t bw_addr = BW_ADDR_BASE + (t * 0x4000u); /* 每轮 16KB 对齐 */
        uint8_t bw_wr[BW_TEST_SIZE];
        uint8_t bw_rd[BW_TEST_SIZE];
        for (uint32_t i = 0; i < BW_TEST_SIZE; ++i) bw_wr[i] = (uint8_t)(i * 7u + 3u);
        /* 先擦除覆盖范围（4K 对齐，16KB 共 4 个扇区） */
        for (uint32_t off = 0; off < BW_TEST_SIZE; off += 0x1000u)
        {
            if (qspi_erase_4k(bw_addr + off) != 0)
            {
                printf("BW erase fail @0x%06lX (div=%lu)\n", (unsigned long)(bw_addr + off), (unsigned long)div);
                break;
            }
        }
    /* 写带宽（按 256B 页）。为避免 UART 干扰计时，暂时关闭驱动内部 verbose。 */
    qspi_set_verbose(false);
        /* 写带宽（按 256B 页）：逐页计时并累加，避免计时器回绕影响 */
        uint64_t wr_cycles_total = 0;
        for (uint32_t off = 0; off < BW_TEST_SIZE; off += 256u)
        {
            uint32_t c0 = perf_now32();
            if (qspi_page_program(bw_addr + off, &bw_wr[off], 256u) != 0)
            {
                printf("BW program fail @0x%06lX (div=%lu)\n", (unsigned long)(bw_addr + off), (unsigned long)div);
                break;
            }
            uint32_t c1 = perf_now32();
            wr_cycles_total += (uint64_t)perf_cycles_since(c0);
        }
        print_bw("Write BW", BW_TEST_SIZE, (uint32_t)(wr_cycles_total & 0xFFFFFFFFu), SystemCoreClock);

        /* 读带宽 */
    uint32_t c0 = perf_now32();
        if (qspi_read(bw_addr, bw_rd, BW_TEST_SIZE) != 0)
        {
            printf("BW read fail (div=%lu)\n", (unsigned long)div);
            qspi_set_verbose(true);
            continue;
        }
    uint32_t c1 = perf_now32();
    print_bw("Read  BW", BW_TEST_SIZE, perf_cycles_since(c0), SystemCoreClock);
        if (memcmp(bw_wr, bw_rd, BW_TEST_SIZE) != 0)
        {
            printf("BW data mismatch (div=%lu)\n", (unsigned long)div);
        }
        qspi_set_verbose(true);
    }

    printf("\nAll divider tests done.\n");
    while (1)
    {
        __WFI();
    }
}
