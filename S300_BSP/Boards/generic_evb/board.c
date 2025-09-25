#include "board.h"
#include "rcc.h"

void board_clock_init(void)
{
    /* 切到 CM4 PLL：与参考配置一致（192MHz） */
    (void)init_cortex_m4_pll(6, 768, 0, 4, 2);
    /* 可选：保持 APB0/APB1 分频为 0（不分频），确保 APB=SYS */
    // set_apb_clock_div(0, 0);
    // set_apb_clock_div(1, 0);
    SystemCoreClockUpdate();
}

void board_init(void)
{
    board_clock_init();
}
