#include "s300.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

/* SysTick 计时：使用 24-bit 自由运行计数，转换为 us */
#define SYSTICK_BASE 0xE000E010u
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} systick_t;
#define SYSTICK ((systick_t *)SYSTICK_BASE)
static uint32_t s_reload = 0;
static uint32_t s_cycles_per_us = 1;
static void systick_init_free_running(uint32_t cpu_hz)
{
    s_cycles_per_us = cpu_hz / 1000000u;
    if (s_cycles_per_us == 0u) s_cycles_per_us = 1u;
    s_reload = 0xFFFFFFu; /* 24-bit 最大值 */
    SYSTICK->LOAD = s_reload;
    SYSTICK->VAL  = 0u;       /* 写入清零为 LOAD */
    SYSTICK->CTRL = 0x5u;     /* 使能，处理器时钟，无中断 */
}
static inline uint32_t systick_ticks(void) { return SYSTICK->VAL; }
static inline uint32_t ticks_elapsed_us(uint32_t start, uint32_t end)
{
    uint32_t period = s_reload + 1u;
    uint32_t elapsed_cycles = (start >= end) ? (start - end) : (start + period - end);
    return (elapsed_cycles + (s_cycles_per_us - 1u)) / s_cycles_per_us;
}

static void print_hex(const char *tag, const uint8_t *buf, unsigned len)
{
    if (!tag) tag = "";
    printf("%s", tag);
    for (unsigned i = 0; i < len; ++i) printf(" %02X", buf[i]);
    printf("\r\n");
}

static int test_rw_once(uint32_t test_addr, uint8_t salt)
{
    enum { PAGE = 256 };
    uint8_t w[PAGE], r[PAGE];
    for (unsigned i = 0; i < PAGE; ++i) w[i] = (uint8_t)(i ^ 0xA5u ^ salt);

    if (w25qxx_erase_4k(test_addr & ~0xFFFu) != 0) {
        printf("  erase 4K @0x%08lX failed\r\n", (unsigned long)(test_addr & ~0xFFFu));
        return -1;
    }
    if (w25qxx_write_page(test_addr, w, PAGE) != 0) {
        printf("  program 256B @0x%08lX failed\r\n", (unsigned long)test_addr);
        return -2;
    }
    memset(r, 0, sizeof(r));
    if (w25qxx_read(test_addr, r, PAGE) != 0) {
        printf("  read 256B @0x%08lX failed\r\n", (unsigned long)test_addr);
        return -3;
    }
    if (memcmp(w, r, PAGE) != 0) {
        printf("  verify failed\r\n");
        print_hex("  W:", w, 32);
        print_hex("  R:", r, 32);
        return -4;
    }
    return 0;
}

static int run_one_freq(uint32_t sclk_hz, uint32_t test_addr)
{
    uint32_t ahb_hz = rcc_get_clock(RCC_CLOCK_AHB);
    systick_init_free_running(ahb_hz);
    printf("[REQ_CLK=%lu Hz] AHB=%lu Hz\r\n", (unsigned long)sclk_hz, (unsigned long)ahb_hz);

    qspi_set_verbose(false);
    qspi_cadence_init(ahb_hz, sclk_hz);
    uint32_t act_clk = qspi_get_actual_sclk_hz();
    uint32_t baud_raw = qspi_get_baud_raw();
    printf("  ACT_CLK=%lu Hz (BAUD=0x%lX -> ref/(2*(%lu+1)))\r\n",
           (unsigned long)act_clk, (unsigned long)baud_raw, (unsigned long)baud_raw);

    /* 读 ID */
    w25qxx_info_t info;
    int rc = w25qxx_init(&info, true, false);
    if (rc != 0) {
        printf("  w25qxx_init failed (%d)\r\n", rc);
        return -10;
    }
    printf("  JEDEC ID: %02X %02X %02X  size=%lu bytes  QE=%u  4B=%u\r\n",
           info.manuf_id, info.memory_type, info.capacity, (unsigned long)info.size_bytes,
           info.quad_enabled ? 1u : 0u, info.addr4b ? 1u : 0u);

    /* 判定 ID 基本正确（宽松校验：厂商=0xEF；容量码在 0x14..0x26） */
    bool id_ok = (info.manuf_id == 0xEFu) && (info.capacity >= 0x14u) && (info.capacity <= 0x26u);
    if (!id_ok) {
        printf("  ID unexpected -> FAIL\r\n");
        return -11;
    }

    (void)qspi_unlock_all();
    (void)qspi_unlock_all();

    /* 读取并打印 SFDP 头部（16字节） */
    {
        uint8_t sfdp[16] = {0};
        if (w25qxx_read_sfdp(0u, sfdp, sizeof(sfdp)) == 0) {
            bool sig_ok = (sfdp[0]=='S' && sfdp[1]=='F' && sfdp[2]=='D' && sfdp[3]=='P');
            printf("  SFDP[0..15]:");
            for (unsigned i=0;i<sizeof(sfdp);++i) printf(" %02X", sfdp[i]);
            printf("  %s\r\n", sig_ok?"(SIG OK)":"(SIG NG)");
        } else {
            printf("  SFDP read failed\r\n");
        }
    }

    /* 写读校验 */
    uint8_t salt = (uint8_t)((sclk_hz / 1000000u) & 0xFFu);
    uint32_t t0 = systick_ticks();
    rc = test_rw_once(test_addr, salt);
    uint32_t t1 = systick_ticks();
    if (rc != 0) {
        printf("  R/W test FAILED (%d)\r\n", rc);
        return -12;
    }
    printf("  R/W test OK  time=%lu us\r\n", (unsigned long)ticks_elapsed_us(t0, t1));
    return 0;
}

/* 强制将系统时钟切换到 HSE（不使用 CM4 PLL） */
static void switch_sysclk_to_hse(void)
{
    /* 低两位：00=?? 01=HSE 10=PLL（参考 rcc.c 实现） */
    uint32_t sel = RCC->CM4_SYS_CLK_SEL;
    sel &= ~0x3u;
    sel |= 1u; /* 选择 HSE */
    RCC->CM4_SYS_CLK_SEL = sel;
    /* 可选：关闭 PLL 以降低功耗（不强制）
       RCC->CM4_PLL_CTL2 |= (1u << 31); // PD
    */
    SystemCoreClockUpdate();
}

static void run_group_no_pll(void)
{
    printf("\r\n=== Group A: no PLL (HSE=24MHz), divisors 2..32 step 2 ===\r\n");
    /* 先切到 HSE，确保不使用 PLL；再初始化调试串口 */
    switch_sysclk_to_hse();
    board_debug_uart_init();

    const uint32_t test_addr = 0x00100000u; /* 避开可能的Boot区域 */
    uint32_t ref = rcc_get_clock(RCC_CLOCK_AHB); /* 预期约 24MHz */
    for (uint32_t div = 2; div <= 32; div += 2) {
        uint32_t req = ref / div;
        printf("[DIV=%lu] ", (unsigned long)div);
        int r = run_one_freq(req, test_addr);
        printf("  => %s\r\n", (r == 0) ? "PASS" : "FAIL");
    }
}

static void run_group_with_pll(void)
{
    printf("\r\n=== Group B: with PLL (AHB=192MHz), divisors 2..32 step 2 ===\r\n");
    /* 切到与板卡一致的 CM4 PLL 配置（约 192MHz） */
    (void)init_cortex_m4_pll(6, 768, 0, 4, 2);
    SystemCoreClockUpdate();
    /* 若未初始化串口，则此处可再次确保串口已启用 */
    board_debug_uart_init();

    const uint32_t test_addr = 0x00102000u; /* 与A组不同扇区，避免相互影响 */
    uint32_t ref = rcc_get_clock(RCC_CLOCK_AHB); /* 预期约 192MHz */
    for (uint32_t div = 2; div <= 32; div += 2) {
        uint32_t req = ref / div;
        printf("[DIV=%lu] ", (unsigned long)div);
        int r = run_one_freq(req, test_addr);
        printf("  => %s\r\n", (r == 0) ? "PASS" : "FAIL");
    }
}

int main(void)
{
    /* 注意：不要调用 board_init()，它会强制开启 PLL。
       我们需要分别测试未开PLL和已开PLL两组场景。 */

    /* UART 可能尚未初始化；先用不启用PLL组函数内部初始化串口 */
    run_group_no_pll();

    /* 然后启用 PLL 并跑另一组 */
    run_group_with_pll();

    printf("\r\n[DEMO] W25Qxx ClkSweep DONE.\r\n");
    while (1) { __WFI(); }
}
