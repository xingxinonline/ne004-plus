/**
 * @file rbl_simple.c
 * @brief 极简RBL - 不依赖printf，直接操作硬件
 * @version 1.0
 * @date 2025-09-01
 */

#include "rbl_hal.h"
#include "rbl_qspi.h"
#include "rbl_sbl.h"
#include "s300.h"
#include "rcc.h"
#include "core_cm4.h"

// 系统时钟频率（启动默认24MHz，PLL后更新为目标频率）
uint32_t SystemCoreClock = 24000000;

/* 将字符转换为大写（仅ASCII字母） */
static inline char rbl_upper_char(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
}

/* 向缓冲追加一个字符（已转大写），维护滚动窗口 */
static void rbl_rolling_append(char *buf, int *len, size_t cap, char c) __attribute__((unused));
static void rbl_rolling_append(char *buf, int *len, size_t cap, char c) {
    if (!buf || !len || cap == 0) return;
    c = rbl_upper_char(c);
    if (*len < (int)cap - 1) {
        buf[*len] = c;
        (*len)++;
    } else {
        for (int i = 1; i < (int)cap - 1; ++i) {
            buf[i - 1] = buf[i];
        }
        buf[(int)cap - 2] = c;
    }
    buf[*len] = '\0';
}

/* 简单子串匹配（buf中查找token，均为大写） */
static int rbl_rolling_contains(const char *buf, int len, const char *token) __attribute__((unused));
static int rbl_rolling_contains(const char *buf, int len, const char *token) {
    if (!buf || !token || !*token || len <= 0) return 0;
    for (int i = 0; i < len; ++i) {
        int j = 0;
        while (token[j] && (i + j) < len && buf[i + j] == token[j]) {
            ++j;
        }
        if (!token[j]) return 1;
    }
    return 0;
}

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

    // 初始化SysTick用于精确心跳定时（1ms间隔）
    // SysTick时钟源为AHB时钟（192MHz），1ms需要192000个ticks
    // 但SysTick LOAD最大值为2^24-1=16777215，所以设置最大值
    SysTick->LOAD = 16777215UL;  // 最大值，约87ms
    SysTick->VAL = 0UL;          // 清零当前值
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // 启用SysTick，不启用中断

    // 发送启动信息
    RBL_LOG("\r\n==== S300 RBL Minimal v1.0 ====\r\n");
    RBL_LOG("Hello from SRAM RBL!\r\n");
    RBL_LOG("Build: " __DATE__ " " __TIME__ "\r\n");
    RBL_LOG("================================\r\n\r\n");

    // Phase 2: 初始化 QSPI 并读取 JEDEC ID (使用系统时钟的1/2作为SCLK)
    RBL_LOG("[RBL] Starting Phase 2: QSPI initialization...\r\n");
    uint32_t ahb_clk = SystemCoreClock;  // e.g. 192MHz
    uint32_t safe_freq = ahb_clk / 2;    // QSPI SCLK = SYSCLK/2（带宽与稳定折中）
    RBL_LOG("[RBL] About to call rbl_qspi_init()...\r\n");
    rbl_qspi_init(ahb_clk, safe_freq);
    RBL_LOG("[RBL] rbl_qspi_init() completed\r\n");
    
    // 读取JEDEC ID验证QSPI通信
    uint8_t id[3] = {0};
    if (rbl_qspi_read_jedec_id(id) == 0)
    {
        RBL_LOG("[RBL] QSPI JEDEC read ok\r\n");

        // Phase 2 验收：一页读写校验测试
        // 注意：0x10000(64KB) 处是 SBL 存放区域，不能用来做擦写测试
        // 改用更高的安全地址 0x40000 (256KB) 以避免覆盖系统镜像
        if (rbl_qspi_test_page_rw(0x40000) == 0)
        {
            RBL_LOG("[RBL] Phase 2 validation PASSED!\r\n");
            
            // XIP模式测试
            if (rbl_configure_xip_mode() == 0)
            {
                if (rbl_test_xip_fetch() == 0)
                {
                    RBL_LOG("[RBL] XIP fetch test PASSED\r\n");
                }
                else
                {
                    RBL_LOG("[RBL] XIP fetch test FAILED\r\n");
                }

                if (rbl_exit_xip_mode() != 0)
                {
                    RBL_LOG("[RBL] Warning: failed to exit XIP mode cleanly\r\n");
                }
            }
            else
            {
                RBL_LOG("[RBL] Configuring XIP mode failed\r\n");
            }

            // Phase 3: SBL 完整性检查与跳转
            RBL_LOG("\r\n[RBL] Starting Phase 3: SBL validation...\r\n");
            if (rbl_sbl_validate() == 0)
            {
                RBL_LOG("[RBL] Phase 3 validation PASSED!\r\n");

                // 尝试加载并跳转到SBL
                rbl_sbl_load_and_jump();

                // 如果到达这里，说明跳转失败
                RBL_LOG("[RBL] SBL jump failed, system reset...\r\n");
                RBL_LOG("[RBL] System reset DISABLED - continuing in main loop\r\n");
                // rbl_delay_cycles(1000000);  /* 延时1秒 */
                // NVIC_SystemReset();  /* 重启系统 */
            }
            else
            {
                RBL_LOG("[RBL] Phase 3 validation FAILED!\r\n");
                RBL_LOG("[RBL] SBL invalid, system reset...\r\n");
                RBL_LOG("[RBL] System reset DISABLED - continuing in main loop\r\n");
                // rbl_delay_cycles(1000000);  /* 延时1秒 */
                // NVIC_SystemReset();  /* 重启系统 */
            }
        }
        else
        {
            RBL_LOG("[RBL] Phase 2 validation FAILED!\r\n");
            RBL_LOG("[RBL] QSPI test failed, system reset...\r\n");
            RBL_LOG("[RBL] System reset DISABLED - continuing in main loop\r\n");
            // rbl_delay_cycles(1000000);  /* 延时1秒 */
            // NVIC_SystemReset();  /* 重启系统 */
        }
    }
    else
    {
        RBL_LOG("[RBL] QSPI JEDEC read failed\r\n");
        RBL_LOG("[RBL] QSPI init failed, system reset...\r\n");
        RBL_LOG("[RBL] System reset DISABLED - continuing in main loop\r\n");
        // rbl_delay_cycles(1000000);  /* 延时1秒 */
        // NVIC_SystemReset();  /* 重启系统 */
    }
    uint32_t counter = 0;
    uint32_t last_systick = SysTick->VAL;
    uint32_t systick_rollover = 0;
    uint32_t last_heartbeat_time = 0;

    // 主循环
    while (1)
    {
        // 更新SysTick计数（处理翻转）
        uint32_t current_systick = SysTick->VAL;
        if (current_systick > last_systick) {
            // SysTick翻转了
            systick_rollover++;
        }
        last_systick = current_systick;

        // 计算经过的时间（单位：SysTick ticks）
        // SysTick LOAD = 16777215, 所以每次翻转代表16777215个ticks
        uint32_t current_time = systick_rollover * 16777215UL + (16777215UL - current_systick);

        // 每隔1秒发送心跳（假设SysTick时钟为192MHz，192000000 ticks = 1秒）
        if (current_time - last_heartbeat_time >= 192000000UL)
        {
            last_heartbeat_time = current_time;
            counter++;
            RBL_LOG("[RBL] Heartbeat ");
            // 简单的数字转字符串 (只显示低4位)
            char num = '0' + (counter % 10);
            rbl_uart_write(&num, 1);
            RBL_LOG("\r\n");
        }

        // 简单延时，避免CPU占用过高
        rbl_delay_cycles(1000);
    }
    return 0;
}