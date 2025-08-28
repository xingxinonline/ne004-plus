#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

/* printf 已在 board_init 中完成串口与重定向初始化 */

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
    printf("QSPI W25Q128 demo start\n");
    /* QSPI ref clock from AHB; use current AHB/system clock */
    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    qspi_cadence_init(ahb_clk, 24 * 1000 * 1000); /* 24MHz SCLK */
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
    qspi_dump_regs("before erase");
    /* Test address */
    const uint32_t addr = 0x000000;
    uint8_t tx[256];
    for (uint32_t i = 0; i < sizeof tx; i++) tx[i] = (uint8_t)(i ^ 0xA5);
    printf("Erase 4K @0x%06lX...\n", (unsigned long)addr);
    if (qspi_erase_4k(addr) != 0)
    {
        printf("Erase failed\n");
        while (1) __WFI();
    }
    if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
        printf("After erase: SR1=%02X SR2=%02X SR3=%02X\n", sr1, sr2, sr3);
    qspi_dump_regs("before program");
    printf("Program 256B page...\n");
    if (qspi_page_program(addr, tx, sizeof tx) != 0)
    {
        printf("Program failed\n");
        while (1) __WFI();
    }
    if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
        printf("After program: SR1=%02X SR2=%02X SR3=%02X\n", sr1, sr2, sr3);
    qspi_dump_regs("before read");
    uint8_t rx[256] = {0};
    if (qspi_read(addr, rx, sizeof rx) != 0)
    {
        printf("Read back failed\n");
        while (1) __WFI();
    }
    int ok = memcmp(tx, rx, sizeof tx) == 0;
    printf("Verify %s\n", ok ? "OK" : "FAIL");
    while (1)
    {
        __WFI();
    }
}
