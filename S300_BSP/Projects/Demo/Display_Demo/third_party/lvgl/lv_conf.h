#ifndef LV_CONF_H
#define LV_CONF_H

/* Minimal LVGL v9 config for bare-metal RGB565 framebuffer without OS */

#define LV_USE_OS             LV_OS_NONE
#define LV_TICK_CUSTOM        1
#define LV_TICK_CUSTOM_INCLUDE "s300.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (0) /* we call lv_tick_inc in SysTick */

/* Display */
#define LV_COLOR_DEPTH        16
#define LV_COLOR_16_SWAP      0
#define LV_COLOR_SCREEN_TRANSP 0

/* No GPU */
#define LV_USE_DRAW_SW        1
#define LV_DRAW_SW_COMPLEX    1

/* Features */
#define LV_USE_LOG            1
#define LV_LOG_LEVEL          LV_LOG_LEVEL_TRACE

/* Default display resolution (can be overridden at runtime) */
#define LV_HOR_RES_MAX        128
#define LV_VER_RES_MAX        160

/* Fonts: enable 12px Montserrat and set as default */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_12

/* Memory: use static internal heap (optional). We'll provide buffers from BSP. */
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_BUILTIN
#define LV_MEM_SIZE           (16U * 1024U)

/* Input devices off for now */
#define LV_USE_INDEV          0

/* Widgets on (defaults OK) */
#define LV_USE_WIDGETS        1

#endif /* LV_CONF_H */
