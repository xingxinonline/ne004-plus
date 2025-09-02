/**
 * @file rbl_simple.c
 * @brief 极简RBL - 不依赖printf，直接操作硬件
 * @version 1.0
 * @date 2025-09-01
 */

#include "rbl_hal.h"
#include "rbl_qspi.h"
#include "rbl_sbl.h"
#include "rbl_download.h"
#include "s300.h"
#include "rcc.h"

// 系统时钟频率（启动默认24MHz，PLL后更新为目标频率）
uint32_t SystemCoreClock = 24000000;

// 系统初始化函数 (startup.s需要的)
void SystemInit(void)
{
    // 基础初始化，保持在HSE 24MHz，PLL稍后由主程序配置
    SystemCoreClock = 24000000;
}

// 移除内联 UART 与延时，改用 HAL 抽象

// 主函数
int main(void)
{
    // 基本初始化
    // 注意：RBL运行在SRAM1，向量表也在SRAM1的开始位置
    SCB->VTOR = 0x20000000;  // 向量表在SRAM1开始
    __DSB();
    __ISB();  // 添加指令同步屏障
    
    // 先做最基本的延时测试，确保CPU工作正常
    for (volatile int i = 0; i < 100000; i++) {
        __NOP();
    }

    // 配置PLL至192MHz，并切换系统时钟到PLL
    // 目标：SYSCLK = 192MHz，HSE=24MHz
    // 选取参数：refdiv=2, fbdiv=64, frac=0, postdiv1=2, postdiv2=2
    // 粗略计算：Fvco=(24/2)*64=768MHz；Fout=Fvco/(2*2*2)=48MHz?（注意驱动内部又除2）
    // 根据 rcc.c 的实现，最终 system 时钟使用 CM4 PLL，并在 get_clock 有一次除2路径。
    // 参数：refdiv=1, fbdiv=64, postdiv1=2, postdiv2=2 -> 24*64/(2*2*2) = 192MHz
    if (rcc_init_cortex_m4_pll(1, 64, 0, 2, 2) == 0)
    {
        SystemCoreClock = rcc_get_clock(RCC_CLOCK_SYSTEM);
    }
    
    // 初始化UART（动态基于当前APB1时钟计算波特率）
    rbl_uart_init();
    
    // 再做一次延时，确保UART初始化后稳定
    for (volatile int i = 0; i < 100000; i++) {
        __NOP();
    }
    // 发送启动信息
    RBL_LOG("\r\n==== S300 RBL Minimal v1.0 ====\r\n");
    RBL_LOG("Hello from SRAM RBL!\r\n");
    RBL_LOG("Build: " __DATE__ " " __TIME__ "\r\n");
    RBL_LOG("================================\r\n\r\n");

    // Phase 2: 初始化 QSPI 并读取 JEDEC ID (使用系统时钟的1/4作为SCLK)
    RBL_LOG("[RBL] Starting Phase 2: QSPI initialization...\r\n");
    uint32_t ahb_clk = SystemCoreClock;  // e.g. 192MHz
    uint32_t safe_freq = ahb_clk / 4;    // QSPI SCLK = SYSCLK/4（带宽与稳定折中）
    RBL_LOG("[RBL] About to call rbl_qspi_init()...\r\n");
    rbl_qspi_init(ahb_clk, safe_freq);
    RBL_LOG("[RBL] rbl_qspi_init() completed\r\n");
    uint8_t id[3] = {0};
    if (rbl_qspi_read_jedec_id(id) == 0) {
        RBL_LOG("[RBL] QSPI JEDEC read ok\r\n");
        
    // Phase 2 验收：一页读写校验测试
    // 注意：0x10000(64KB) 处是 SBL 存放区域，不能用来做擦写测试
    // 改用更高的安全地址 0x40000 (256KB) 以避免覆盖系统镜像
    if (rbl_qspi_test_page_rw(0x40000) == 0) {
            RBL_LOG("[RBL] Phase 2 validation PASSED!\r\n");
            
            // Phase 3: SBL 完整性检查与跳转
            RBL_LOG("\r\n[RBL] Starting Phase 3: SBL validation...\r\n");
            if (rbl_sbl_validate() == 0)
            {
                RBL_LOG("[RBL] Phase 3 validation PASSED!\r\n");
                
                // 尝试加载并跳转到SBL
                rbl_sbl_load_and_jump();
                
                // 如果到达这里，说明跳转失败
                RBL_LOG("[RBL] SBL jump failed, entering download mode...\r\n");
            }
            else
            {
                RBL_LOG("[RBL] Phase 3 validation FAILED!\r\n");
                RBL_LOG("[RBL] SBL invalid, entering download mode...\r\n");
                
                /* Phase 4: 进入下载模式 */
                RBL_LOG("[RBL] === Phase 4: Download Mode ===\r\n");
                rbl_download_init();
                if (rbl_download_start()) {
                    RBL_LOG("[RBL] Download mode started successfully\r\n");
                    
                    /* 下载主循环 */
                    download_state_t download_state;
                    do {
                        download_state = rbl_download_process();
                        rbl_delay_cycles(1000);  /* 短暂延时 */
                    } while (download_state == DOWNLOAD_STATE_WAITING || 
                             download_state == DOWNLOAD_STATE_RECEIVING);
                    
                    if (download_state == DOWNLOAD_STATE_COMPLETED) {
                        RBL_LOG("[RBL] Download completed, restarting system...\r\n");
                        rbl_delay_cycles(1000000);  /* 延时1秒 */
                        NVIC_SystemReset();  /* 重启系统 */
                    } else {
                        RBL_LOG("[RBL] Download failed or timeout\r\n");
                    }
                } else {
                    RBL_LOG("[RBL] Failed to start download mode\r\n");
                }
            }
        } else {
            RBL_LOG("[RBL] Phase 2 validation FAILED!\r\n");
        }
    } else {
        RBL_LOG("[RBL] QSPI JEDEC read failed\r\n");
    }
    uint32_t counter = 0;
    // 主循环
    while (1)
    {
        counter++;
        // 每隔一段时间发送心跳
        if (counter % 1000000 == 0)
        {
            RBL_LOG("[RBL] Heartbeat ");
            // 简单的数字转字符串 (只显示低4位)
            char num = '0' + ((counter / 1000000) % 10);
            rbl_uart_write(&num, 1);
            RBL_LOG("\r\n");
        }
        // 简单延时
        rbl_delay_cycles(100);
    }
    return 0;
}
