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
    if (qspi_read_id(id, sizeof(id)) == 0)
    {
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
    }
    else
    {
        printf("Failed to read JEDEC ID\n");
        return;
    }
    
    /* 读取Device ID */
    uint8_t dev_id = 0;
    if (qspi_read_device_id(&dev_id) == 0)
        printf("Device ID (ABh): %02X\n", dev_id);
    
    /* 读取Manufacturer/Device ID */
    uint8_t mfg_id = 0, mfg_dev_id = 0;
    if (qspi_read_manufacturer_device_id(&mfg_id, &mfg_dev_id) == 0)
        printf("Mfg/Dev ID (90h): %02X %02X\n", mfg_id, mfg_dev_id);
    
    /* 读取Unique ID */
    uint8_t uid[8] = {0};
    if (qspi_read_unique_id(uid, 8) == 0)
    {
        printf("Unique ID: ");
        for (int i = 0; i < 8; i++) printf("%02X", uid[i]);
        printf("\n");
    }
    
    /* 读取SFDP信息 */
    printf("SFDP Information:\n");
    uint8_t sfdp_header[8] = {0};
    if (qspi_read_sfdp(0, sfdp_header, 8) == 0)
    {
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
    }
    
    /* 读取状态寄存器 */
    uint8_t sr1 = 0, sr2 = 0, sr3 = 0;
    if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
    {
        printf("Status Registers:\n");
        printf("  SR1: %02X (BUSY:%u WEL:%u BP:%u TB:%u SEC:%u SRP0:%u)\n", sr1,
               (sr1 >> 0) & 1, (sr1 >> 1) & 1, (sr1 >> 2) & 7, (sr1 >> 5) & 1, (sr1 >> 6) & 1, (sr1 >> 7) & 1);
        printf("  SR2: %02X (SRP1:%u QE:%u LB:%u CMP:%u SUS:%u)\n", sr2,
               (sr2 >> 0) & 1, (sr2 >> 1) & 1, (sr2 >> 3) & 7, (sr2 >> 6) & 1, (sr2 >> 7) & 1);
        printf("  SR3: %02X (ADP:%u ADS:%u DRV:%u Hold/RST:%u)\n", sr3,
               (sr3 >> 1) & 1, (sr3 >> 0) & 1, (sr3 >> 5) & 3, (sr3 >> 7) & 1);
    }
}

/* 简单的读写功能测试 */
static bool functional_test(uint32_t test_addr, uint32_t freq_hz)
{
    printf("Functional test @%lu Hz...\n", (unsigned long)freq_hz);
    
    /* 擦除测试扇区 */
    printf("  Erasing 4KB sector @0x%06lX\n", (unsigned long)test_addr);
    if (qspi_erase_4k(test_addr) != 0)
    {
        printf("  Erase failed\n");
        return false;
    }
    
    /* 准备测试数据 */
    uint8_t tx_data[256];
    for (uint32_t i = 0; i < sizeof(tx_data); i++)
        tx_data[i] = (uint8_t)(i ^ (freq_hz >> 16) ^ 0xA5);
    
    /* 写入数据 */
    printf("  Programming 256 bytes\n");
    if (qspi_page_program(test_addr, tx_data, sizeof(tx_data)) != 0)
    {
        printf("  Program failed\n");
        return false;
    }
    
    /* 读回数据 */
    uint8_t rx_data[256] = {0};
    printf("  Reading back 256 bytes\n");
    if (qspi_read(test_addr, rx_data, sizeof(rx_data)) != 0)
    {
        printf("  Read failed\n");
        return false;
    }
    
    /* 验证数据 */
    if (memcmp(tx_data, rx_data, sizeof(tx_data)) == 0)
    {
        printf("  Data verification: PASS\n");
        return true;
    }
    else
    {
        printf("  Data verification: FAIL\n");
        /* 显示第一个不匹配的位置 */
        for (unsigned i = 0; i < sizeof(tx_data); i++)
        {
            if (tx_data[i] != rx_data[i])
            {
                printf("    First mismatch @%u: expected %02X, got %02X\n", 
                       i, tx_data[i], rx_data[i]);
                break;
            }
        }
        return false;
    }
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
    
    qspi_cadence_init(ahb_clk, detect_freq);
    qspi_dump_regs("after low frequency init");
    
    /* 获取详细Flash信息 */
    get_flash_info();
    
    /* 初始化W25Qxx驱动以启用高级功能 */
    w25qxx_info_t flash_info;
    if (w25qxx_init(&flash_info, true, false) == 0)  /* 尝试启用Quad模式 */
    {
        printf("\nFlash configuration:\n");
        printf("  Quad Enable: %s\n", flash_info.quad_enabled ? "YES" : "NO");
        printf("  4-Byte Address: %s\n", flash_info.addr4b ? "YES" : "NO");
    }
    
    /* 解锁所有保护区域 */
    qspi_unlock_all();
    
    /* 第二步：从低频开始逐步提升频率做功能测试 */
    printf("\n=== Step 2: Progressive Frequency Functional Tests ===\n");
    
    const uint32_t test_frequencies[] = {
        ahb_clk / 32,  /* ~6MHz @192MHz AHB */
        ahb_clk / 16,  /* ~12MHz */
        ahb_clk / 8,   /* ~24MHz */
        ahb_clk / 4,   /* ~48MHz */
        ahb_clk / 2    /* ~96MHz */
    };
    
    const char* freq_names[] = {
        "AHB/32", "AHB/16", "AHB/8", "AHB/4", "AHB/2"
    };
    
    uint32_t max_working_freq = 0;
    bool all_passed = true;
    
    for (unsigned i = 0; i < sizeof(test_frequencies)/sizeof(test_frequencies[0]); i++)
    {
        uint32_t freq = test_frequencies[i];
        printf("\n--- Test %u: %s (~%lu MHz) ---\n", 
               i + 1, freq_names[i], (unsigned long)(freq / 1000000));
        
        /* 重新初始化QSPI控制器 */
        qspi_cadence_init(ahb_clk, freq);
        
        /* 重新配置Flash（频率变化后可能需要重新配置） */
        (void)w25qxx_init(&flash_info, true, false);
        
        /* 使用不同地址避免重复擦写同一扇区 */
        uint32_t test_addr = 0x10000 + (i * 0x1000);  /* 从64KB开始，每个频率4KB间隔 */
        
        if (functional_test(test_addr, freq))
        {
            printf("Frequency %s: PASS\n", freq_names[i]);
            max_working_freq = freq;
        }
        else
        {
            printf("Frequency %s: FAIL - stopping frequency progression\n", freq_names[i]);
            all_passed = false;
            break;
        }
    }
    
    if (max_working_freq > 0)
    {
        printf("\nMax working frequency: %lu Hz (~%lu MHz)\n", 
               (unsigned long)max_working_freq, 
               (unsigned long)(max_working_freq / 1000000));
    }
    
    /* 测试软件复位功能 */
    printf("\n=== Additional Tests ===\n");
    printf("Testing software reset...\n");
    if (qspi_software_reset() == 0)
    {
        printf("Software reset: OK\n");
        /* 复位后重新初始化 */
        qspi_cadence_init(ahb_clk, max_working_freq > 0 ? max_working_freq : detect_freq);
        (void)w25qxx_init(&flash_info, true, false);
    }
    else
    {
        printf("Software reset: FAILED\n");
    }
    
    /* 最终状态报告 */
    printf("\n=== Test Summary ===\n");
    if (all_passed)
    {
        printf("All frequency tests: PASSED\n");
    }
    else
    {
        printf("Some frequency tests: FAILED\n");
    }
    printf("Maximum working frequency: %lu Hz (~%lu MHz)\n", 
           (unsigned long)max_working_freq, 
           (unsigned long)(max_working_freq / 1000000));
    
    printf("\nTest completed. Entering idle loop.\n");
    while (1)
    {
        __WFI();
    }
}
