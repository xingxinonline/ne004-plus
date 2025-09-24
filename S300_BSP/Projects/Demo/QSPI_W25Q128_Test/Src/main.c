#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

/* printf 已在 board_init 中完成串口与重定向初始化 */

/* 测试配置 */
#define FUNCTIONAL_TEST_SIZE   (4096u)       /* 功能测试4KB */

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

/* 获取详细的Flash信息 */
static void get_flash_info(void)
{
    printf("\n=== Flash Information ===\n");
    
    /* 读取JEDEC ID */
    uint8_t id[3] = {0};
    uint32_t sz = 0;
        const char *name = decode_flash_name(id, &sz);
        printf("JEDEC ID: %02X %02X %02X -> %s", id[0], id[1], id[2], name);
        if (sz > 0)
        {
            unsigned mib = (sz >> 20);
            printf(" (%u MiB)\n", mib);
        }
        else
        {
            printf("\n");
        }
    
    /* 读取Device ID */
    uint8_t dev_id = 0;
    
        printf("Device ID (ABh): %02X\n", dev_id);
    
    /* 读取Manufacturer/Device ID */
    uint8_t mfg_id = 0, mfg_dev_id = 0;
    
        printf("Mfg/Dev ID (90h): %02X %02X\n", mfg_id, mfg_dev_id);
    
    /* 读取Unique ID */
    uint8_t uid[8] = {0};
    printf("Unique ID: ");
        for (int i = 0; i < 8; i++) printf("%02X", uid[i]);
        printf("\n");
    
    /* 读取SFDP信息 */
    printf("SFDP Information:\n");
    uint8_t sfdp_header[8] = {0};
    printf("  SFDP Header: ");
    for (int i = 0; i < 8; i++) printf("%02X ", sfdp_header[i]);
    printf("\n");
    
    /* 检查SFDP签名 */
    if (sfdp_header[0] == 0x53 && sfdp_header[1] == 0x46 && 
        sfdp_header[2] == 0x44 && sfdp_header[3] == 0x50)
    {
        printf("  SFDP Signature: Valid\n");
        printf("  SFDP Version: %u.%u\n", sfdp_header[5], sfdp_header[4]);
        printf("  Number of Parameter Headers: %u\n", sfdp_header[6] + 1);
    }
    else
    {
        printf("  SFDP Signature: Invalid\n");
    }
    
    /* 读取状态寄存器 */
    uint8_t sr1 = 0, sr2 = 0, sr3 = 0;
    printf("Status Registers:\n");
        printf("  SR1: %02X (BUSY:%u WEL:%u BP:%u TB:%u SEC:%u SRP0:%u)\n", sr1,
               (sr1 >> 0) & 1, (sr1 >> 1) & 1, (sr1 >> 2) & 7, (sr1 >> 5) & 1, (sr1 >> 6) & 1, (sr1 >> 7) & 1);
        printf("  SR2: %02X (SRP1:%u QE:%u LB:%u CMP:%u SUS:%u)\n", sr2,
               (sr2 >> 0) & 1, (sr2 >> 1) & 1, (sr2 >> 3) & 7, (sr2 >> 6) & 1, (sr2 >> 7) & 1);
        printf("  SR3: %02X (ADP:%u ADS:%u DRV:%u Hold/RST:%u)\n", sr3,
               (sr3 >> 1) & 1, (sr3 >> 0) & 1, (sr3 >> 5) & 3, (sr3 >> 7) & 1);
}



int main(void)
{
    board_init();
    printf("QSPI W25Q128 Progressive Frequency Test\n");
    
    /* 获取AHB时钟 */
    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0) ahb_clk = SystemCoreClock;
    printf("AHB clock: %lu Hz\n", (unsigned long)ahb_clk);
    
    /* 第一步：使用最低频率获取详细Flash信息 */
    printf("\n=== Step 1: Flash Detection @Low Frequency ===\n");
    const uint32_t detect_freq = ahb_clk / 32;  /* 最低分频：AHB/32 */
    printf("Detection frequency: %lu Hz\n", (unsigned long)detect_freq);

    
    /* 获取详细Flash信息 */
    get_flash_info();
    
    
    
    printf("\nTest completed. Entering idle loop.\n");
    while (1)
    {
        __WFI();
    }
}
