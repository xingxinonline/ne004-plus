#include "rbl_qspi.h"
#include "qspi_cadence.h"
#include "rbl_hal.h"
#include "rcc.h"  // 时钟控制
#include <stdbool.h>  // bool 类型

// 保护区：防止擦写系统镜像（Header+RBL+预留到SBL以前）
#ifndef RBL_FLASH_PROTECT_START
#define RBL_FLASH_PROTECT_START   0x00000u
#endif
#ifndef RBL_FLASH_PROTECT_END
#define RBL_FLASH_PROTECT_END     0x20000u  // 128KB内保留（覆盖Header+RBL和余量）
#endif

void rbl_qspi_init(uint32_t ref_clk_hz, uint32_t sclk_hz)
{
    RBL_LOG("[RBL] QSPI init step 1: enabling clocks...\r\n");
    // 启用QSPI时钟：需要同时启用APB0和AHB时钟
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
    
    RBL_LOG("[RBL] QSPI init step 2: waiting for clock stability...\r\n");
    // 短暂延时让时钟稳定
    for (volatile int i = 0; i < 1000; i++) {}
    
    RBL_LOG("[RBL] QSPI init step 3: calling qspi_cadence_init...\r\n");
    qspi_set_verbose(false);
    qspi_cadence_init(ref_clk_hz, sclk_hz);
    
    RBL_LOG("[RBL] QSPI init step 4: dumping registers...\r\n");
    // 添加调试信息 - 在LOG开关控制下 (暂时注释掉以绕过卡死问题)
    #if 0 // RBL_LOG_ENABLE
    qspi_dump_regs("after QSPI init");
    #endif
    RBL_LOG("[RBL] QSPI init step 4: registers dump skipped\r\n");
    
    RBL_LOG("[RBL] QSPI init step 5: force exit XIP mode...\r\n");
    // 强制退出XIP模式 - 这是关键！从Flash启动后可能还处于XIP状态
    // 发送多个FFh来确保Flash退出任何特殊模式
    for (int i = 0; i < 8; i++) {
        qspi_software_reset();
        // 额外延时确保复位生效
        for (volatile int j = 0; j < 1000; j++) {}
    }
    
    RBL_LOG("[RBL] QSPI init step 6: final delay...\r\n");
    // 更长的延时让Flash完全准备就绪
    for (volatile int i = 0; i < 50000; i++) {}
    
    RBL_LOG("[RBL] QSPI init done\r\n");
}

int rbl_qspi_read_jedec_id(uint8_t id[3])
{
    if (!id) return -1;
    int rc = qspi_read_id(id, 3);
    if (rc == 0)
    {
        char buf[64];
        // 简易十六进制打印，避免 printf：每字节两位十六进制
        static const char hex[] = "0123456789ABCDEF";
        buf[0] = '['; buf[1] = 'R'; buf[2] = 'B'; buf[3] = 'L'; buf[4] = ']'; buf[5] = ' ';
        buf[6] = 'I'; buf[7] = 'D'; buf[8] = ':'; buf[9] = ' ';
        int pos = 10;
        for (int i = 0; i < 3; ++i)
        {
            buf[pos++] = '0'; buf[pos++] = 'x';
            buf[pos++] = hex[(id[i] >> 4) & 0xF];
            buf[pos++] = hex[(id[i] >> 0) & 0xF];
            if (i != 2) buf[pos++] = ' ';
        }
        buf[pos++] = '\r'; buf[pos++] = '\n';
        rbl_uart_write(buf, (size_t)pos);
    }
    return rc;
}

// === Phase 2 扩展: 基本读写擦除功能 ===

int rbl_qspi_wait_ready(uint32_t timeout_ms)
{
    // 简化：固定超时循环，不依赖定时器
    uint32_t loops = timeout_ms * 1000; // 粗略估算
    for (uint32_t i = 0; i < loops; i++)
    {
        uint8_t sr1, sr2, sr3;
        if (qspi_read_status(&sr1, &sr2, &sr3) == 0)
        {
            if ((sr1 & 0x01) == 0) // BUSY位为0表示就绪
                return 0;
        }
        // 简单延时
        for (volatile int j = 0; j < 100; j++) {}
    }
    return -1; // 超时
}

int rbl_qspi_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    if (!data || len == 0) return -1;
    return qspi_read(addr, data, len);
}

int rbl_qspi_page_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (!data || len == 0 || len > 256) return -1;
    
    // 等待Flash就绪
    if (rbl_qspi_wait_ready(100) != 0) return -1;
    
    // 页编程
    int rc = qspi_page_program(addr, data, len);
    if (rc != 0) return rc;
    
    // 等待编程完成
    return rbl_qspi_wait_ready(100);
}

int rbl_qspi_erase_4k(uint32_t addr)
{
    // 等待Flash就绪
    if (rbl_qspi_wait_ready(100) != 0) return -1;
    
    // 4KB扇区擦除
    int rc = qspi_erase_4k(addr);
    if (rc != 0) return rc;
    
    // 等待擦除完成（擦除时间较长）
    return rbl_qspi_wait_ready(5000); // 5秒超时
}

// === 便捷的一页读写校验功能 ===

int rbl_qspi_test_page_rw(uint32_t test_addr)
{
    RBL_LOG("[RBL] Flash R/W test starting...\r\n");

    // 防护：避免擦写落入受保护区间
    if ((test_addr & ~0xFFFu) < RBL_FLASH_PROTECT_END)
    {
        RBL_LOG("[RBL] Test address 0x%08X in protected region, skip test.\r\n", test_addr);
        return 0; // 视为通过，避免破坏
    }
    
    // 准备测试数据（256字节）
    uint8_t write_data[256];
    uint8_t read_data[256];
    
    // 生成测试模式
    for (int i = 0; i < 256; i++)
    {
        write_data[i] = (uint8_t)(i ^ 0xA5 ^ (test_addr >> 8));
    }
    
    // 1. 擦除4KB扇区
    RBL_LOG("[RBL] Erasing 4KB sector...\r\n");
    if (rbl_qspi_erase_4k(test_addr & ~0xFFF) != 0) // 对齐到4KB边界
    {
        RBL_LOG("[RBL] Erase failed!\r\n");
        return -1;
    }
    
    // 2. 写入一页数据
    RBL_LOG("[RBL] Programming 256 bytes...\r\n");
    if (rbl_qspi_page_program(test_addr, write_data, 256) != 0)
    {
        RBL_LOG("[RBL] Program failed!\r\n");
        return -1;
    }
    
    // 3. 读回数据
    RBL_LOG("[RBL] Reading back 256 bytes...\r\n");
    if (rbl_qspi_read(test_addr, read_data, 256) != 0)
    {
        RBL_LOG("[RBL] Read failed!\r\n");
        return -1;
    }
    
    // 4. 校验数据
    RBL_LOG("[RBL] Verifying data...\r\n");
    for (int i = 0; i < 256; i++)
    {
        if (write_data[i] != read_data[i])
        {
            RBL_LOG("[RBL] Verify failed!\r\n");
            return -1;
        }
    }
    
    RBL_LOG("[RBL] Flash R/W test PASSED!\r\n");
    return 0;
}
