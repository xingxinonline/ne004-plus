#include "rbl_qspi.h"
#include "qspi_cadence.h"
#include "rbl_hal.h"
#include "rcc.h"  // 时钟控制
#include <stdbool.h>  // bool 类型

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
