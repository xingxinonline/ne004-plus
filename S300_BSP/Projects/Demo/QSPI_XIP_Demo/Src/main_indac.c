#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

#define TEST_ADDR       0x10000   /* 8KB偏移，避开头部 */
#define TEST_BUF_SIZE   32       /* 1KB 测试 */

static volatile uint32_t systick_ms = 0;
void SysTick_Handler(void) { systick_ms++; }
static void systick_init(void) { systick_ms = 0; SysTick_Config(SystemCoreClock / 1000); }
static uint32_t now_ms(void) { return systick_ms; }

static int do_indac_rw_demo(void)
{
    uint8_t tx[TEST_BUF_SIZE];
    uint8_t rx[TEST_BUF_SIZE];
    for (uint32_t i = 0; i < TEST_BUF_SIZE; ++i) tx[i] = (uint8_t)(i ^ 0x5Au);
    memset(rx, 0, sizeof(rx));

    // printf("Erasing 4K sector @0x%06lX...\n", (unsigned long)TEST_ADDR);
    // if (qspi_erase_4k(TEST_ADDR) != 0) { printf("  erase failed\n"); return -1; }

    // /* 使用INDAC写（页大小对齐由调用者保证，这里长度不跨页的示例） */
    // printf("Programming %u bytes with INDAC...\n", TEST_BUF_SIZE);
    // if (qspi_indac_write_pp(TEST_ADDR, tx, TEST_BUF_SIZE) != 0) {
    //     printf("  indac write failed\n");
    //     return -1;
    // }

    /* 使用INDAC读回 */
    printf("Reading back %u bytes with INDAC...\n", TEST_BUF_SIZE);
    if (qspi_indac_read_fast(TEST_ADDR, rx, TEST_BUF_SIZE) != 0) {
        printf("  indac read failed\n");
        return -1;
    }

    if (memcmp(tx, rx, TEST_BUF_SIZE) != 0) {
        printf("Verify FAIL\n");
        for (uint32_t i = 0; i < 16; ++i) printf("  TX[%02lu]=%02X RX[%02lu]=%02X\n", (unsigned long)i, tx[i], (unsigned long)i, rx[i]);
        return -1;
    }
    printf("Verify OK\n");

    /* 简单带宽测试：多次INDAC读 */
    const uint32_t loops = 64; /* 64 * 1KB = 64KB */
    uint32_t start = now_ms();
    for (uint32_t i = 0; i < loops; ++i) {
        if (qspi_indac_read_fast(TEST_ADDR, rx, TEST_BUF_SIZE) != 0) return -1;
    }
    uint32_t dur_ms = now_ms() - start;
    uint32_t bytes = loops * TEST_BUF_SIZE;
    printf("INDAC read throughput: %lu bytes in %lu ms => %lu KB/s\n",
           (unsigned long)bytes, (unsigned long)dur_ms,
           (unsigned long)((bytes / 1024u) * (dur_ms ? 1000u / dur_ms : 0u)));
    return 0;
}

int main(void)
{
    board_init();
    printf("QSPI INDAC (Indirect) Demo\n");
    printf("===========================\n");
    systick_init();

    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    const uint32_t qspi_freq = ahb_clk / 2; /* 中等频率 */
    printf("AHB=%lu Hz, QSPI=%lu Hz\n", (unsigned long)ahb_clk, (unsigned long)qspi_freq);

    qspi_cadence_init(ahb_clk, qspi_freq);

    w25qxx_info_t info;
    if (w25qxx_init(&info, true, false) != 0) {
        printf("Flash init failed\n");
        goto fail;
    }
    printf("Flash Quad=%s, 4B=%s\n", info.quad_enabled?"YES":"NO", info.addr4b?"YES":"NO");

    /* 推荐设置读/写分区为近似1:1 */
    qspi_set_sram_partition(CQSPI_SRAM_TOTAL_LOCATIONS/2);

    if (do_indac_rw_demo() != 0) goto fail;

    printf("\nINDAC demo completed.\n");
    while (1) { __WFI(); }
fail:
    printf("\nINDAC demo failed.\n");
    while (1) { __WFI(); }
}
