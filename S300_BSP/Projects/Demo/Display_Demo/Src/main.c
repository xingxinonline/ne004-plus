#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "video.h"
#include "lvgl.h"
#include "uart.h"
#include "uart_s300.h"
/* Camera deps */
#include "gpio.h"
#include "i2c_soft.h"
#include "ov5640.h"
#include "mailbox.h"
// extern const lv_image_dsc_t img_demo; // demo image asset
// extern const lv_image_dsc_t img_yanqiu; // 30x38 eye image asset
// extern const lv_image_dsc_t img_yankuang; // 74x85 eye socket background
extern const lv_image_dsc_t img_yanbai_rotated_cw; // 95x74 eye socket background (pre-rotated asset, no runtime rotation)
extern const lv_image_dsc_t img_yanbai1_rotated_cw; // 95x74 eye socket background (pre-rotated asset, no runtime rotation)
extern const lv_image_dsc_t img_yanzhu_small_rotated_cw; // 56x45 eyeball image (pre-rotated asset, no runtime rotation)
/* Blink (close eyes) GIF assets generated via LVGLImage.py */
extern const lv_image_dsc_t biyan_final_rotated_cw_first9;
extern const lv_image_dsc_t biyan1_final_rotated_cw_first9;

/* --- Tuning knobs --- */
#ifndef EYE_PER_PX_MS
#define EYE_PER_PX_MS 30u   /* per-pixel animation time (ms) for both X/Y axes */
#endif
#ifndef EYE_ANIM_LINEAR
#define EYE_ANIM_LINEAR 0    /* 0: ease-in-out (default), 1: linear path for constant speed */
#endif
#ifndef EYE_RATE_LIMIT_PX_PER_S
#define EYE_RATE_LIMIT_PX_PER_S 360  /* max filtered mid change per second; 0 to disable */
#endif
#ifndef EYE_SMOOTH_NUM
#define EYE_SMOOTH_NUM 1    /* EMA numerator (alpha = NUM/DEN) */
#endif
#ifndef EYE_SMOOTH_DEN
#define EYE_SMOOTH_DEN 2    /* EMA denominator; 1/2 => 0.5 smoothing */
#endif
/* Minimal movement to trigger a new animation (in mid coords, pixels) */
#ifndef EYE_MIN_MOVE_PX
#define EYE_MIN_MOVE_PX 2
#endif
/* Throttle how often we print movement logs */
#ifndef MOVE_LOG_MIN_INTERVAL_MS
#define MOVE_LOG_MIN_INTERVAL_MS 120u
#endif
/* Throttle how often we print invalid-face logs */
#ifndef INVALID_LOG_INTERVAL_MS
#define INVALID_LOG_INTERVAL_MS 300u
#endif
/* Timeout to return to neutral when no valid face is detected */
#ifndef FACE_LOST_TIMEOUT_MS
#define FACE_LOST_TIMEOUT_MS 1500u
#endif
/* Throttle IDLE return logs */
#ifndef IDLE_RETURN_LOG_MIN_INTERVAL_MS
#define IDLE_RETURN_LOG_MIN_INTERVAL_MS 1000u
#endif

/* Debug switch for eyes/blink recovery flow */
#ifndef EYES_DEBUG
#define EYES_DEBUG 0
#endif

/* Compact state dump to help trace the order and coordinates when creating/restoring eyes */
static void eyes_debug_dump(const char *tag);

/* Animation: set y coordinate */
static void anim_set_y(void * obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t*)obj, v);
}

/* Animation: set x coordinate */
static void anim_set_x(void * obj, int32_t v)
{
    lv_obj_set_x((lv_obj_t*)obj, v);
}

/* legacy oscillating animation (kept for reference) */
static __attribute__((unused)) void start_ud_anim(lv_obj_t * obj, int32_t base_y, int32_t amp, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, base_y - amp, base_y + amp);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, t_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* One-shot vertical animation: from -> to, no playback, no repeat */
static void start_ud_anim_one_shot(lv_obj_t * obj, int32_t from_y, int32_t to_y, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_y, to_y);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, 0);
    lv_anim_set_repeat_count(&a, 0);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_y);
    #if EYE_ANIM_LINEAR
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    #else
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    #endif
    lv_anim_start(&a);
}

/* Retarget in-flight one-shot animation: cancel current and start towards new target from current Y */
static void retarget_ud_anim_one_shot(lv_obj_t * obj, int32_t to_y, uint32_t t_ms, uint32_t delay_ms)
{
    /* Delete any existing animation on Y for this object */
    lv_anim_delete(obj, anim_set_y);
    /* Start from the current displayed Y to ensure seamless retarget */
    int32_t from_y = lv_obj_get_y(obj);
    start_ud_anim_one_shot(obj, from_y, to_y, t_ms, delay_ms);
}

/* Retarget one-shot animation on X axis */
static void retarget_lr_anim_one_shot(lv_obj_t * obj, int32_t to_x, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_delete(obj, anim_set_x);
    int32_t from_x = lv_obj_get_x(obj);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_x, to_x);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, 0);
    lv_anim_set_repeat_count(&a, 0);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_x);
    #if EYE_ANIM_LINEAR
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    #else
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    #endif
    lv_anim_start(&a);
}

/* ---------------- Eyes globals and helpers ---------------- */
static lv_obj_t * g_eye_top = NULL;
static lv_obj_t * g_eye_bot = NULL;
static lv_obj_t * g_bg_top  = NULL;
static lv_obj_t * g_bg_bot  = NULL;
static lv_obj_t * g_blink_top = NULL;
static lv_obj_t * g_blink_bot = NULL;
static int        g_blink_active = 0; /* 1 when blink GIF is displayed */
/* Create-eyes hidden once: when set, next eyes_ui_create() will spawn eyes hidden to avoid a frame at (0,0) */
static int        g_spawn_hidden_once = 0;
static int32_t    g_eye_h = 16;
static int32_t    g_eye_spacing = 28;         /* distance between eye centers (vertical) */
static uint32_t   g_anim_time_ms = 2000;      /* default single-trip duration */
/* Visible geometry (no runtime rotation): equals actual image w/h */
static int32_t    g_eye_vis_h = 16;           /* eye visible height */
static int32_t    g_socket_vis_h = 16;        /* socket visible height */
static int32_t    g_eye_vis_w = 16;           /* eye visible width  */
static int32_t    g_socket_vis_w = 16;        /* socket visible width */

/* Last commanded mid point (to restore after blink); initialized lazily */
static int32_t    g_mid_x_last = 0;
static int32_t    g_mid_y_last = 0;
static int        g_mid_last_inited = 0;

/* --- Eye/Socket geometry constants (no runtime rotation) --- */
static const int32_t C_EYE_W = 38;
static const int32_t C_EYE_H = 30;
static const int32_t C_BG_W  = 95;  /* background image (eye white + eyelid on left) */
static const int32_t C_BG_H  = 74;
/* Effective socket horizontal range: measured right->left 1..83 px within the BG image */
static const int32_t C_SOCKET_W_EFF = 83;
/* Socket sub-rect center within the BG image: aligns with eyeball center */
static const int32_t C_SOCKET_CENTER_IN_IMG = 52;

/* Now that globals are declared, provide the debug dump implementation */
static void eyes_debug_dump(const char *tag)
{
#if EYES_DEBUG
    printf("[S300][EYES][%s] blink_active=%d top=%p bot=%p bg_top=%p bg_bot=%p\r\n",
           (tag?tag:"?"), g_blink_active, (void*)g_eye_top, (void*)g_eye_bot, (void*)g_bg_top, (void*)g_bg_bot);
    if (g_eye_top) {
        printf("  top: pos=(%ld,%ld) hidden=%d\r\n",
               (long)lv_obj_get_x(g_eye_top), (long)lv_obj_get_y(g_eye_top),
               lv_obj_has_flag(g_eye_top, LV_OBJ_FLAG_HIDDEN) ? 1 : 0);
    }
    if (g_eye_bot) {
        printf("  bot: pos=(%ld,%ld) hidden=%d\r\n",
               (long)lv_obj_get_x(g_eye_bot), (long)lv_obj_get_y(g_eye_bot),
               lv_obj_has_flag(g_eye_bot, LV_OBJ_FLAG_HIDDEN) ? 1 : 0);
    }
    if (g_blink_top) {
        printf("  gif_top: pos=(%ld,%ld)\r\n",
               (long)lv_obj_get_x(g_blink_top), (long)lv_obj_get_y(g_blink_top));
    }
    if (g_blink_bot) {
        printf("  gif_bot: pos=(%ld,%ld)\r\n",
               (long)lv_obj_get_x(g_blink_bot), (long)lv_obj_get_y(g_blink_bot));
    }
    printf("  last_mid=(%ld,%ld) inited=%d spacing=%ld vis(w,h)=(%ld,%ld)\r\n",
           (long)g_mid_x_last, (long)g_mid_y_last, g_mid_last_inited,
           (long)g_eye_spacing, (long)g_eye_vis_w, (long)g_eye_vis_h);
#else
    (void)tag;
#endif
}

/* One-shot timer: reveal eyes after positions are applied */
static void eyes_delayed_reveal_cb(lv_timer_t *tm)
{
    (void)tm;
    if (g_eye_top) lv_obj_clear_flag(g_eye_top, LV_OBJ_FLAG_HIDDEN);
    if (g_eye_bot) lv_obj_clear_flag(g_eye_bot, LV_OBJ_FLAG_HIDDEN);
    eyes_debug_dump("restore.reveal");
}

/* Forward declare limit helpers before use in timer callback */
static inline void eyes_mid_limits(int32_t *min_out, int32_t *max_out);
static inline void eyes_mid_limits_x(int32_t *min_out, int32_t *max_out);

/* Apply eyes restore (run inside LVGL timer context to avoid race with refresh) */
static void eyes_restore_apply_cb(lv_timer_t *tm)
{
    (void)tm;
    if (!(g_eye_top && g_eye_bot)) {
        return;
    }
    int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
    int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);
    int32_t mid_x = g_mid_last_inited ? g_mid_x_last : (min_x + max_x) / 2;
    int32_t mid_y = g_mid_last_inited ? g_mid_y_last : (min_y + max_y) / 2;
    if (mid_x < min_x) mid_x = min_x;
    if (mid_x > max_x) mid_x = max_x;
    if (mid_y < min_y) mid_y = min_y;
    if (mid_y > max_y) mid_y = max_y;

    const int32_t half_h = g_eye_h / 2;
    const int32_t half_w = g_eye_vis_w / 2;
    int32_t to_y_top = (mid_y - g_eye_spacing) - half_h;
    int32_t to_y_bot = (mid_y + g_eye_spacing) - half_h;
    int32_t to_x = mid_x - half_w;

    /* Ensure clean state, keep hidden while positioning */
    lv_anim_delete(g_eye_top, anim_set_y);
    lv_anim_delete(g_eye_top, anim_set_x);
    lv_anim_delete(g_eye_bot, anim_set_y);
    lv_anim_delete(g_eye_bot, anim_set_x);
    lv_obj_add_flag(g_eye_top, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_eye_bot, LV_OBJ_FLAG_HIDDEN);

    /* Apply positions explicitly */
    lv_obj_set_x(g_eye_top, to_x); lv_obj_set_y(g_eye_top, to_y_top);
    lv_obj_set_x(g_eye_bot, to_x); lv_obj_set_y(g_eye_bot, to_y_bot);

    printf("[S300][EYES][restore.apply] mid=(%ld,%ld) -> top(%ld,%ld) bot(%ld,%ld)\r\n",
           (long)mid_x, (long)mid_y, (long)to_x, (long)to_y_top, (long)to_x, (long)to_y_bot);
    eyes_debug_dump("restore.apply.hidden");

    /* Reveal now in timer context */
    if (g_eye_top) lv_obj_clear_flag(g_eye_top, LV_OBJ_FLAG_HIDDEN);
    if (g_eye_bot) lv_obj_clear_flag(g_eye_bot, LV_OBJ_FLAG_HIDDEN);
    eyes_debug_dump("restore.reveal");

    /* one-shot */
    lv_timer_del(tm);
}

/* Create eye backgrounds and eyeballs; installs globals and visible geometry */
static void eyes_ui_create(void)
{
    if (g_eye_top || g_eye_bot || g_bg_top || g_bg_bot) return; /* already created */
    eyes_debug_dump("eyes_ui_create.begin");
    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t center_x = screen_w / 2;
    const int32_t center_y = screen_h / 2;

    /* base positions */
    const int32_t base_x    = center_x - C_EYE_W / 2;
    const int32_t base_x_bg = center_x - C_SOCKET_CENTER_IN_IMG;
    int32_t base_y_bg_top = (center_y - g_eye_spacing) - C_BG_H / 2;
    int32_t base_y_bg_bot = (center_y + g_eye_spacing) - C_BG_H / 2;
    int32_t base_y_top    = (center_y - g_eye_spacing) - C_EYE_H / 2;
    int32_t base_y_bot    = (center_y + g_eye_spacing) - C_EYE_H / 2;

    /* backgrounds first (behind) */
    g_bg_top = lv_image_create(lv_screen_active());
    lv_image_set_src(g_bg_top, &img_yanbai_rotated_cw);
    lv_image_set_pivot(g_bg_top, C_BG_W / 2, C_BG_H / 2);
    lv_obj_set_pos(g_bg_top, base_x_bg, base_y_bg_top);

    g_bg_bot = lv_image_create(lv_screen_active());
    lv_image_set_src(g_bg_bot, &img_yanbai1_rotated_cw);
    lv_image_set_pivot(g_bg_bot, C_BG_W / 2, C_BG_H / 2);
    lv_obj_set_pos(g_bg_bot, base_x_bg, base_y_bg_bot);

    /* eyes */
    g_eye_top = lv_image_create(lv_screen_active());
    lv_image_set_src(g_eye_top, &img_yanzhu_small_rotated_cw);
    lv_image_set_pivot(g_eye_top, C_EYE_W / 2, C_EYE_H / 2);
    if (g_spawn_hidden_once) lv_obj_add_flag(g_eye_top, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(g_eye_top, base_x, base_y_top);

    g_eye_bot = lv_image_create(lv_screen_active());
    lv_image_set_src(g_eye_bot, &img_yanzhu_small_rotated_cw);
    lv_image_set_pivot(g_eye_bot, C_EYE_W / 2, C_EYE_H / 2);
    if (g_spawn_hidden_once) lv_obj_add_flag(g_eye_bot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(g_eye_bot, base_x, base_y_bot);

    /* export geometry */
    g_eye_h = C_EYE_H;
    g_eye_vis_h = C_EYE_H;
    g_socket_vis_h = C_BG_H;
    g_eye_vis_w = C_EYE_W;
    g_socket_vis_w = C_SOCKET_W_EFF;

    /* Clear one-shot spawn-hidden flag after creation */
    if (g_spawn_hidden_once) g_spawn_hidden_once = 0;

    /* Safety: if bottom eye somehow stayed at (0,0), force position to base to avoid visible origin frame */
    if (g_eye_bot) {
        int32_t bx = lv_obj_get_x(g_eye_bot);
        int32_t by = lv_obj_get_y(g_eye_bot);
        if (bx == 0 && by == 0) {
            lv_obj_set_x(g_eye_bot, base_x);
            lv_obj_set_y(g_eye_bot, base_y_bot);
        }
    }

    eyes_debug_dump("eyes_ui_create.after");
}

static void eyes_ui_destroy(void)
{
    eyes_debug_dump("eyes_ui_destroy.begin");
    if (g_eye_top) {
        /* Ensure no in-flight animations survive and accidentally target a future object at the same address */
        lv_anim_delete(g_eye_top, anim_set_y);
        lv_anim_delete(g_eye_top, anim_set_x);
        lv_obj_del(g_eye_top); g_eye_top = NULL;
    }
    if (g_eye_bot) {
        lv_anim_delete(g_eye_bot, anim_set_y);
        lv_anim_delete(g_eye_bot, anim_set_x);
        lv_obj_del(g_eye_bot); g_eye_bot = NULL;
    }
    if (g_bg_top)  { lv_obj_del(g_bg_top);  g_bg_top  = NULL; }
    if (g_bg_bot)  { lv_obj_del(g_bg_bot);  g_bg_bot  = NULL; }
    eyes_debug_dump("eyes_ui_destroy.after");
}

/* Update spacing between eyes (in pixels) and relayout backgrounds/eyeballs immediately. */
static void eyes_set_spacing(int32_t spacing)
{
    if (spacing < 0) spacing = 0;
    g_eye_spacing = spacing;

    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t center_x = screen_w / 2;
    const int32_t center_y = screen_h / 2;
    const int32_t base_x_bg = center_x - C_SOCKET_CENTER_IN_IMG;

    /* Relayout backgrounds */
    if (g_bg_top && g_bg_bot) {
        int32_t base_y_bg_top = (center_y - g_eye_spacing) - C_BG_H / 2;
        int32_t base_y_bg_bot = (center_y + g_eye_spacing) - C_BG_H / 2;
        lv_obj_set_pos(g_bg_top, base_x_bg, base_y_bg_top);
        lv_obj_set_pos(g_bg_bot, base_x_bg, base_y_bg_bot);
    }

    /* Relayout eyeballs (keep last mid if available, else center) */
    if (g_eye_top && g_eye_bot) {
        int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
        int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);
        int32_t mid_x = g_mid_last_inited ? g_mid_x_last : (min_x + max_x) / 2;
        int32_t mid_y = g_mid_last_inited ? g_mid_y_last : (min_y + max_y) / 2;
        if (mid_x < min_x) mid_x = min_x;
        if (mid_x > max_x) mid_x = max_x;
        if (mid_y < min_y) mid_y = min_y;
        if (mid_y > max_y) mid_y = max_y;
        const int32_t half_h = g_eye_h / 2;
        const int32_t half_w = g_eye_vis_w / 2;
        int32_t to_y_top = (mid_y - g_eye_spacing) - half_h;
        int32_t to_y_bot = (mid_y + g_eye_spacing) - half_h;
        int32_t to_x = mid_x - half_w;
        /* cancel animations to avoid conflict */
        lv_anim_delete(g_eye_top, anim_set_y); lv_anim_delete(g_eye_top, anim_set_x);
        lv_anim_delete(g_eye_bot, anim_set_y); lv_anim_delete(g_eye_bot, anim_set_x);
        /* apply */
        lv_obj_set_pos(g_eye_top, to_x, to_y_top);
        lv_obj_set_pos(g_eye_bot, to_x, to_y_bot);
        /* maintain last mids */
        g_mid_x_last = mid_x; g_mid_y_last = mid_y; g_mid_last_inited = 1;
    }

    eyes_debug_dump("spacing.update");
}

/* Show one blink GIF per eye region (until next face detection) */
static void blink_show(void)
{
    if (g_blink_active) return;
    eyes_debug_dump("blink_show.begin");
    /* remove eyes to save memory */
    eyes_ui_destroy();

    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t center_x = screen_w / 2;
    const int32_t center_y = screen_h / 2;
    const int32_t base_x_bg = center_x - C_SOCKET_CENTER_IN_IMG;
    int32_t base_y_bg_top = (center_y - g_eye_spacing) - C_BG_H / 2;
    int32_t base_y_bg_bot = (center_y + g_eye_spacing) - C_BG_H / 2;

    /* create GIFs aligned to backgrounds */
    g_blink_top = lv_gif_create(lv_screen_active());
    lv_gif_set_color_format(g_blink_top, LV_COLOR_FORMAT_RGB565);
    lv_gif_set_src(g_blink_top, &biyan_final_rotated_cw_first9);
    /* Play once and pause on last frame */
    lv_gif_set_loop_count(g_blink_top, 1);
    lv_obj_set_pos(g_blink_top, base_x_bg, base_y_bg_top);

    g_blink_bot = lv_gif_create(lv_screen_active());
    lv_gif_set_color_format(g_blink_bot, LV_COLOR_FORMAT_RGB565);
    lv_gif_set_src(g_blink_bot, &biyan1_final_rotated_cw_first9);
    /* Play once and pause on last frame */
    lv_gif_set_loop_count(g_blink_bot, 1);
    lv_obj_set_pos(g_blink_bot, base_x_bg, base_y_bg_bot);

    g_blink_active = 1;
    eyes_debug_dump("blink_show.after");
}

static void blink_hide_and_restore(void)
{
    if (!g_blink_active) return;
    eyes_debug_dump("restore.begin");
    if (g_blink_top) { lv_obj_del(g_blink_top); g_blink_top = NULL; }
    if (g_blink_bot) { lv_obj_del(g_blink_bot); g_blink_bot = NULL; }
    g_blink_active = 0;
    /* restore eyes */
    /* Spawn eyes hidden to avoid any transient draw at default pos */
    g_spawn_hidden_once = 1;
    eyes_ui_create();
    eyes_debug_dump("restore.afterCreate");
    /* Defer actual coordinate application to LVGL timer context */
    (void)lv_timer_create(eyes_restore_apply_cb, 1, NULL);
}

static inline void eyes_mid_limits(int32_t *min_out, int32_t *max_out)
{
        /* Constrain mid_y so that each eye stays inside its socket (no rotation).
             Socket centers are fixed at center_y ± g_eye_spacing. If socket visible height is S
             and eye visible height is E, then allowed mid_y range is:
                 mid ∈ [center_y - (S-E)/2, center_y + (S-E)/2]
             Additionally clamp to screen to be safe. */
        const int32_t screen_h = DISP_IMAGE_HEIGHT;
        const int32_t center_y = screen_h / 2;
        int32_t S = g_socket_vis_h; /* socket visible height */
        int32_t E = g_eye_vis_h;    /* eye visible height    */
        if (S < E) S = E; /* if socket smaller than eye, fall back to eye height */
        int32_t margin = (S - E) / 2; /* how far mid can deviate from center */
        int32_t mid_min = center_y - margin;
        int32_t mid_max = center_y + margin;
        /* Screen safety clamp (eye must remain fully on-screen as well) */
        int32_t half_eye = g_eye_h / 2; /* use object pivot-based placement */
        int32_t scr_min = g_eye_spacing + half_eye;
        int32_t scr_max = screen_h - (g_eye_spacing + half_eye);
        if (mid_min < scr_min) mid_min = scr_min;
        if (mid_max > scr_max) mid_max = scr_max;
        if (mid_min > mid_max) { mid_min = mid_max = center_y; }
        if (min_out) *min_out = mid_min;
        if (max_out) *max_out = mid_max;
}

/* Horizontal limits for eye mid X (shared by both eyes, socket centers at center_x) */
static inline void eyes_mid_limits_x(int32_t *min_out, int32_t *max_out)
{
    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t center_x = screen_w / 2;
    int32_t S = g_socket_vis_w; /* socket visible width */
    int32_t E = g_eye_vis_w;    /* eye visible width    */
    if (S < E) S = E;
    int32_t margin = (S - E) / 2; /* how far mid can deviate from center */
    int32_t mid_min = center_x - margin;
    int32_t mid_max = center_x + margin;
    /* Screen safety clamp */
    int32_t half_eye_w = g_eye_vis_w / 2;
    int32_t scr_min = half_eye_w;
    int32_t scr_max = screen_w - half_eye_w;
    if (mid_min < scr_min) mid_min = scr_min;
    if (mid_max > scr_max) mid_max = scr_max;
    if (mid_min > mid_max) { mid_min = mid_max = center_x; }
    if (min_out) *min_out = mid_min;
    if (max_out) *max_out = mid_max;
}

static int32_t eyes_goto_mid_y(int32_t mid_y)
{
    if (!g_eye_top || !g_eye_bot) return mid_y;
    /* Clamp midpoint so that both eyes remain fully visible */
    const int32_t half = g_eye_h / 2;
    int32_t mid_min, mid_max;
    eyes_mid_limits(&mid_min, &mid_max);
    if (mid_y < mid_min) mid_y = mid_min;
    if (mid_y > mid_max) mid_y = mid_max;

    int32_t top_center_y = mid_y - g_eye_spacing;
    int32_t bot_center_y = mid_y + g_eye_spacing;
    int32_t to_y_top = top_center_y - half;  /* object top-left y */
    int32_t to_y_bot = bot_center_y - half;

    /* 动画时长：按像素位移计，可调 */
    const uint32_t per_px_ms = EYE_PER_PX_MS;
    int32_t cur_top_y = lv_obj_get_y(g_eye_top);
    int32_t cur_bot_y = lv_obj_get_y(g_eye_bot);
    uint32_t dy_top = (cur_top_y > to_y_top) ? (uint32_t)(cur_top_y - to_y_top) : (uint32_t)(to_y_top - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > to_y_bot) ? (uint32_t)(cur_bot_y - to_y_bot) : (uint32_t)(to_y_bot - cur_bot_y);
    uint32_t dy_max = (dy_top > dy_bot) ? dy_top : dy_bot;
    uint32_t t_ms = dy_max * per_px_ms;

    if (t_ms == 0u) {
        /* 无位移，直接设置Y */
        lv_obj_set_y(g_eye_top, to_y_top);
    } else {
        retarget_ud_anim_one_shot(g_eye_top, to_y_top, t_ms, 0);
    }
    if (t_ms == 0u) {
        lv_obj_set_y(g_eye_bot, to_y_bot);
    } else {
        retarget_ud_anim_one_shot(g_eye_bot, to_y_bot, t_ms, 0);
    }

    /* Update last mid Y (keep X as-is) */
    if (!g_mid_last_inited)
    {
        int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
        g_mid_x_last = (min_x + max_x) / 2; /* default center X if first-time */
        g_mid_last_inited = 1;
    }
    g_mid_y_last = mid_y;

    return mid_y;
}

/* Move both eyes to target mid point (mid_x, mid_y) with clamping and XY animation */
static void eyes_goto_mid_xy(int32_t mid_x, int32_t mid_y)
{
    if (!g_eye_top || !g_eye_bot) return;

    /* Clamp mid points */
    int32_t mid_min_y, mid_max_y; eyes_mid_limits(&mid_min_y, &mid_max_y);
    if (mid_y < mid_min_y) { mid_y = mid_min_y; }
    if (mid_y > mid_max_y) { mid_y = mid_max_y; }
    int32_t mid_min_x, mid_max_x; eyes_mid_limits_x(&mid_min_x, &mid_max_x);
    if (mid_x < mid_min_x) { mid_x = mid_min_x; }
    if (mid_x > mid_max_x) { mid_x = mid_max_x; }

    const int32_t half_h = g_eye_h / 2;
    const int32_t half_w = g_eye_vis_w / 2; /* object width */

    int32_t top_center_y = mid_y - g_eye_spacing;
    int32_t bot_center_y = mid_y + g_eye_spacing;
    int32_t to_y_top = top_center_y - half_h;  /* object top-left y */
    int32_t to_y_bot = bot_center_y - half_h;
    int32_t to_x = mid_x - half_w;             /* object top-left x (both eyes share the same x) */

    const uint32_t per_px_ms = EYE_PER_PX_MS;
    /* current positions */
    int32_t cur_top_y = lv_obj_get_y(g_eye_top);
    int32_t cur_bot_y = lv_obj_get_y(g_eye_bot);
    int32_t cur_top_x = lv_obj_get_x(g_eye_top);
    int32_t cur_bot_x = lv_obj_get_x(g_eye_bot);

    uint32_t dy_top = (cur_top_y > to_y_top) ? (uint32_t)(cur_top_y - to_y_top) : (uint32_t)(to_y_top - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > to_y_bot) ? (uint32_t)(cur_bot_y - to_y_bot) : (uint32_t)(to_y_bot - cur_bot_y);
    uint32_t dx_top = (cur_top_x > to_x) ? (uint32_t)(cur_top_x - to_x) : (uint32_t)(to_x - cur_top_x);
    uint32_t dx_bot = (cur_bot_x > to_x) ? (uint32_t)(cur_bot_x - to_x) : (uint32_t)(to_x - cur_bot_x);

    uint32_t dy_max = (dy_top > dy_bot) ? dy_top : dy_bot;
    uint32_t dx_max = (dx_top > dx_bot) ? dx_top : dx_bot;
    uint32_t t_y = dy_max * per_px_ms;
    uint32_t t_x = dx_max * per_px_ms;

    /* Animate Y */
    if (t_y == 0u) lv_obj_set_y(g_eye_top, to_y_top); else retarget_ud_anim_one_shot(g_eye_top, to_y_top, t_y, 0);
    if (t_y == 0u) lv_obj_set_y(g_eye_bot, to_y_bot); else retarget_ud_anim_one_shot(g_eye_bot, to_y_bot, t_y, 0);
    /* Animate X */
    if (t_x == 0u) lv_obj_set_x(g_eye_top, to_x); else retarget_lr_anim_one_shot(g_eye_top, to_x, t_x, 0);
    if (t_x == 0u) lv_obj_set_x(g_eye_bot, to_x); else retarget_lr_anim_one_shot(g_eye_bot, to_x, t_x, 0);

    /* Update last mid XY */
    g_mid_x_last = mid_x;
    g_mid_y_last = mid_y;
    g_mid_last_inited = 1;
}

// 1ms 节拍计时
static volatile uint32_t g_tick_ms = 0;

/* 将“移动到指定中点并打印详情”的逻辑封装为接口，供串口与邮箱两处共用 */
static void eyes_move_to_mid_and_log(int32_t target_mid_y, const char *src_tag)
{
    int32_t min_mid, max_mid;
    eyes_mid_limits(&min_mid, &max_mid);

    /* 执行动画重定向，返回实际生效的中点（被范围裁剪后） */
    int32_t eff = eyes_goto_mid_y(target_mid_y);

    /* 计算目标 top/bot Y 与动画估计时长（与 eyes_goto_mid_y 逻辑保持一致） */
    int32_t half = g_eye_h / 2;
    int32_t top_y = (eff - g_eye_spacing) - half;
    int32_t bot_y = (eff + g_eye_spacing) - half;
    const uint32_t per_px_ms = EYE_PER_PX_MS;
    int32_t cur_top_y = g_eye_top ? lv_obj_get_y(g_eye_top) : top_y;
    int32_t cur_bot_y = g_eye_bot ? lv_obj_get_y(g_eye_bot) : bot_y;
    uint32_t dy_top = (cur_top_y > top_y) ? (uint32_t)(cur_top_y - top_y) : (uint32_t)(top_y - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > bot_y) ? (uint32_t)(cur_bot_y - bot_y) : (uint32_t)(bot_y - cur_bot_y);
    uint32_t t_top_ms = dy_top * per_px_ms;
    uint32_t t_bot_ms = dy_bot * per_px_ms;

    printf("[S300][MOVE:%s] goto %ld => mid=%ld (range %ld..%ld), top_y=%ld (%lums), bot_y=%ld (%lums)\r\n",
           (src_tag ? src_tag : "?"),
           (long)target_mid_y, (long)eff, (long)min_mid, (long)max_mid,
           (long)top_y, (unsigned long)t_top_ms, (long)bot_y, (unsigned long)t_bot_ms);
}

/* New helper: move to (mid_x, mid_y) and log */
static void eyes_move_to_xy_and_log(int32_t target_mid_x, int32_t target_mid_y, const char *src_tag)
{
    int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
    int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);

    /* Before move, fetch current for time estimation */
    int32_t half_h = g_eye_h / 2;
    int32_t half_w = g_eye_vis_w / 2;
    int32_t top_to_y = (target_mid_y - g_eye_spacing) - half_h;
    int32_t bot_to_y = (target_mid_y + g_eye_spacing) - half_h;
    int32_t to_x = target_mid_x - half_w;
    const uint32_t per_px_ms = EYE_PER_PX_MS;
    int32_t cur_top_y = g_eye_top ? lv_obj_get_y(g_eye_top) : top_to_y;
    int32_t cur_bot_y = g_eye_bot ? lv_obj_get_y(g_eye_bot) : bot_to_y;
    int32_t cur_top_x = g_eye_top ? lv_obj_get_x(g_eye_top) : to_x;
    int32_t cur_bot_x = g_eye_bot ? lv_obj_get_x(g_eye_bot) : to_x;
    uint32_t dy_top = (cur_top_y > top_to_y) ? (uint32_t)(cur_top_y - top_to_y) : (uint32_t)(top_to_y - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > bot_to_y) ? (uint32_t)(cur_bot_y - bot_to_y) : (uint32_t)(bot_to_y - cur_bot_y);
    uint32_t dx_top = (cur_top_x > to_x) ? (uint32_t)(cur_top_x - to_x) : (uint32_t)(to_x - cur_top_x);
    uint32_t dx_bot = (cur_bot_x > to_x) ? (uint32_t)(cur_bot_x - to_x) : (uint32_t)(to_x - cur_bot_x);
    /* Independent axis animations; estimate per-eye by taking max of axis durations */
    uint32_t t_top_y = dy_top * per_px_ms;
    uint32_t t_top_x = dx_top * per_px_ms;
    uint32_t t_bot_y = dy_bot * per_px_ms;
    uint32_t t_bot_x = dx_bot * per_px_ms;
    uint32_t t_top_ms = (t_top_y > t_top_x) ? t_top_y : t_top_x;
    uint32_t t_bot_ms = (t_bot_y > t_bot_x) ? t_bot_y : t_bot_x;

    eyes_goto_mid_xy(target_mid_x, target_mid_y);

    printf("[S300][MOVE:%s] goto (%ld,%ld) midX[%ld..%ld] midY[%ld..%ld], est top=%lums bot=%lums\r\n",
           (src_tag ? src_tag : "?"), (long)target_mid_x, (long)target_mid_y,
           (long)min_x, (long)max_x, (long)min_y, (long)max_y,
           (unsigned long)t_top_ms, (unsigned long)t_bot_ms);
}

/* FaceRect from DSP shared memory (base + offset) */
typedef struct FaceRect_ {
    float score;
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
    float lm[10];
} FaceRect;

#ifndef DSP_FACE_BASE_ADDR
#define DSP_FACE_BASE_ADDR 0x44800000u
#endif

/* Face coordinates space (can be overridden if DSP uses different space, e.g., 1280x720) */
#ifndef FACE_COORD_SPACE_W
#define FACE_COORD_SPACE_W DISP_IMAGE_WIDTH
#endif
#ifndef FACE_COORD_SPACE_H
#define FACE_COORD_SPACE_H DISP_IMAGE_HEIGHT
#endif

/* Require continuous face presence before considering it valid (debounce) */
#ifndef FACE_PRESENCE_CONFIRM_MS
#define FACE_PRESENCE_CONFIRM_MS 200u
#endif
/* Optional: verbose logs for presence/confirm state */
#ifndef FACE_DEBUG_CONFIRM
#define FACE_DEBUG_CONFIRM 0
#endif

void SysTick_Handler(void)
{
    g_tick_ms++;
    /* feed LVGL tick (1ms) */
    lv_tick_inc(1);
}

static inline uint32_t millis(void)
{
    return g_tick_ms;
}

/* Count display flushes for FPS measurement */
static volatile uint32_t g_flush_cnt = 0;

/* FPS UI removed to save space */

// /* Periodic LVGL memory usage logger (every ~2s) */
// static void mem_log_timer_cb(lv_timer_t *t)
// {
//     (void)t;
//     lv_mem_monitor_t mon;
//     lv_mem_monitor(&mon);
//     size_t free_b = (size_t)mon.free_size;
//     size_t used_b = (size_t)LV_MEM_SIZE;
//     if (used_b > free_b) used_b -= free_b; else used_b = 0;
//     unsigned long used_pct = (unsigned long)((used_b * 100UL) / (size_t)LV_MEM_SIZE);
//     printf("[S300][LVGL][MEM] used=%luKB free=%luKB used=%lu%%\r\n",
//            (unsigned long)(used_b / 1024UL),
//            (unsigned long)(free_b / 1024UL),
//            used_pct);
// }

/* ---------------- UART echo (RX polling on debug UART) ---------------- */
#ifndef UART_DEBUG_IDX
#define UART_DEBUG_IDX BOARD_UART_DEBUG_IDX
#endif

static inline S300_UART_TypeDef * dbg_uart_dev(void)
{
    switch (UART_DEBUG_IDX)
    {
    case 0: return UART0;
    case 1: return UART1;
    case 2: return UART2;
    default: return UART3;
    }
}

static void uart_echo_poll(void)
{
    S300_UART_TypeDef *U = dbg_uart_dev();
    /* Simple line buffer for commands */
    static char     s_buf[64];
    static uint8_t  s_len = 0;
    /* USR bit3: RFNE (RX FIFO Not Empty). Drain all pending bytes and echo back. */
    while (U->USR & 0x8u)
    {
        uint8_t ch = (uint8_t)U->RBR_THR_DLL; /* read one byte */
        /* echo */
        write_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, ch);
        /* accumulate */
        if (ch == '\r' || ch == '\n')
        {
            if (s_len > 0)
            {
                s_buf[s_len] = '\0';
                /* parse command */
                const char *p = s_buf;
                /* skip leading spaces */
                while (*p == ' ' || *p == '\t') p++;
                if ((p[0]=='g'||p[0]=='G') && (p[1]=='o'||p[1]=='O') && (p[2]=='t'||p[2]=='T') && (p[3]=='o'||p[3]=='O'))
                {
                    p += 4;
                    while (*p == ' ' || *p == '\t') p++;
                    int32_t neg = 0, val = 0, got = 0;
                    if (*p == '+') { p++; }
                    else if (*p == '-') { neg = 1; p++; }
                    while (*p >= '0' && *p <= '9') { val = val*10 + (*p - '0'); p++; got = 1; }
                    if (got)
                    {
                        if (neg) val = -val;
                        eyes_move_to_mid_and_log(val, "UART");
                    }
                    else
                    {
                        printf("[S300][CMD] usage: goto <y_mid>\r\n");
                    }
                }
                s_len = 0; /* reset buffer */
            }
            /* CR->LF for nicer terminals */
            if (ch == '\r') write_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, '\n');
        }
        else if (ch == 0x08 || ch == 0x7F)
        {
            /* handle backspace */
            if (s_len > 0) s_len--;
        }
            else if ((size_t)s_len + 1U < sizeof(s_buf))
        {
            s_buf[s_len++] = (char)ch;
        }
        else
        {
            /* overflow: reset */
            s_len = 0;
        }
    }
}

static void fill_buffer(volatile uint16_t *frame,
                        volatile uint8_t *alpha,
                        size_t pixel_count,
                        uint16_t color,
                        uint8_t alpha_value)
{
    for (size_t i = 0; i < pixel_count; ++i)
    {
        frame[i] = color;
        alpha[i] = alpha_value;
    }
}

/* ---------------- LVGL display binding ---------------- */
static volatile uint16_t* s_f0;
static volatile uint16_t* s_f1;
static volatile uint8_t*  s_a0;
static volatile uint8_t*  s_a1;
static const uint32_t REG_F0 = (DSP_VIDEO_SS_BASE + 0x50u);
static const uint32_t REG_F1 = (DSP_VIDEO_SS_BASE + 0x54u);

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* FULL frame double-buffering: px_map points to the active draw buffer (s_f0 or s_f1) */
    (void)area;

    // /* Present exactly one buffer as ready */
    // REG32(REG_F0) = 0u;
    // REG32(REG_F1) = 0u;

    uintptr_t p = (uintptr_t)px_map;
    if (p == (uintptr_t)s_f0)
    {
        REG32(REG_F0) = 1u;
        while ((REG32(REG_F0) & 0x1u) != 0u) { /* wait until accepted */ }
    }
    else if (p == (uintptr_t)s_f1)
    {
        REG32(REG_F1) = 1u;
        while ((REG32(REG_F1) & 0x1u) != 0u) { /* wait until accepted */ }
    }
    // else
    // {
    //     /* Unexpected pointer: as a fallback copy to f0 and present */
    //     const int32_t w = area->x2 - area->x1 + 1;
    //     const int32_t h = area->y2 - area->y1 + 1;
    //     for (int32_t y = 0; y < h; ++y)
    //     {
    //         memcpy((void*)&s_f0[(area->y1 + y) * DISP_IMAGE_WIDTH + area->x1],
    //                (const void*)&((const uint16_t*)px_map)[y * w],
    //                (size_t)w * sizeof(uint16_t));
    //     }
    //     REG32(REG_F0) = 1u;
    // }

    /* one frame flushed */
    g_flush_cnt++;
    lv_display_flush_ready(disp);
}

/* ---------------- OV5640 camera bring-up (before video init) ---------------- */
/* Assume board wiring: OV5640 reset/powerdown pins on GPIOA15 and GPIOA6 respectively. */
#ifndef CAM_RST_PIN
#define CAM_RST_PIN  15u  /* GPIOA15 */
#endif
#ifndef CAM_PWDN_PIN
#define CAM_PWDN_PIN 6u   /* GPIOA6  */
#endif

static void cam_gpio_init(void)
{
    /* Enable GPIO clock and configure pins as output, pull-up */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    gpio_set_function(GPIOA, CAM_RST_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_RST_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_RST_PIN, 1);
    gpio_set_function(GPIOA, CAM_PWDN_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_PWDN_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_PWDN_PIN, 1);
}

static void cam_power_on_sequence(void)
{
    /* PWDN high, RST low -> delay -> PWDN low -> delay -> RST high -> delay */
    gpio_set_data(GPIOA, CAM_RST_PIN, 0);
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 1);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 0);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_RST_PIN, 1);
    for (volatile uint32_t i = 0; i < 2400000u; i++) __asm volatile("nop");
}

/* Initialize OV5640 via software I2C (I2C1: GPIOA0/A1) to YUYV 720p */
static int ov5640_preinit(void)
{
    i2c_soft_t i2c1;
    /* Moderate bus speed for robustness */
    int ret = i2c_soft_init_default_idx(&i2c1, 1, 50000);
    if (ret) {
        printf("[S300][DisplayDemo][CAM] i2c init fail %d\r\n", ret);
        return ret;
    }
    cam_gpio_init();
    cam_power_on_sequence();
    (void)i2c_soft_bus_recover(&i2c1);

    /* Probe slave address 0x3C/0x3D */
    uint8_t saddr = 0x3C;
    int p3c = i2c_soft_probe(&i2c1, 0x3C);
    int p3d = i2c_soft_probe(&i2c1, 0x3D);
    if (p3c != 0 && p3d == 0) saddr = 0x3D;

    /* Read chip ID for log */
    uint8_t idh = 0, idl = 0;
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Au, true, &idh, 1);
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Bu, true, &idl, 1);
    printf("[S300][DisplayDemo][CAM] OV5640 ID: 0x%02X 0x%02X (addr=0x%02X)\r\n", idh, idl, saddr);
    int lr = ov5640_set_light(&i2c1, saddr, true);
    printf("Enable light: %s\n", lr == 0 ? "OK" : "FAIL");
    /* 简短预览一段时间后自动关闭，避免常亮 */
    for (volatile uint32_t i = 0; i < 4800000u; ++i) __asm volatile("nop");
    int lf = ov5640_set_light(&i2c1, saddr, false);
    printf("Disable light: %s\n", lf == 0 ? "OK" : "FAIL");
    ret = ov5640_init(&i2c1, saddr, OV5640_FMT_YUV422_YUYV);
    printf("[S300][DisplayDemo][CAM] ov5640_init ret=%d\r\n", ret);
    return ret;
}


static void monitor_mailbox_rx(void)
{
    /* 若 DSP->M4 有数据（即 CM4_MAILBOX_BASE 非空），读出并打印 */
    static uint32_t s_last_valid_face_ms = 0;  /* 上次收到有效人脸的时间戳 */
    static uint32_t s_last_idle_log_ms   = 0;  /* 空闲回中打印的节流 */
    static int      s_idle_active        = 0;  /* 已处于“空闲回中”状态 */
    /* 新增：人脸持续存在判定去抖（至少持续 FACE_PRESENCE_CONFIRM_MS 才算有效） */
    static int      s_face_present       = 0;  /* 当前周期检测到人脸（原始有效框） */
    static uint32_t s_face_present_since = 0;  /* 初次检测到人脸的时间戳 */
    static int      s_face_confirmed     = 0;  /* 已确认（超过阈值）的人脸存在 */
    /* 速率限制：记录上一次下发移动命令的时间 */
    static uint32_t s_last_cmd_ms        = 0;
    /* 移动命令历史：用于阈值和速率限制 */
    static int      s_have_last_cmd      = 0;
    static int32_t  s_last_cmd_mid_x     = 0;
    static int32_t  s_last_cmd_mid_y     = 0;
    static uint32_t s_last_move_log_ms   = 0;
    while (mailbox_sta_empty_flag_is(MAILBOX_BASE, 0) == 0)
    {
        uint32_t offset = read_mailbox(MAILBOX_BASE);
        uintptr_t addr = (uintptr_t)DSP_FACE_BASE_ADDR + (uintptr_t)offset;
        const FaceRect *fr = (const FaceRect*)addr;

        /* 读取人脸矩形中心，做边界检查与过滤无效数据 */
        int32_t x1 = fr->x1, y1 = fr->y1, x2 = fr->x2, y2 = fr->y2;
        if (x2 < x1) { int32_t t = x1; x1 = x2; x2 = t; }
        if (y2 < y1) { int32_t t = y1; y1 = y2; y2 = t; }

        bool valid = true;
        if (x1 < 0 || y1 < 0 || x2 > FACE_COORD_SPACE_W || y2 > FACE_COORD_SPACE_H) valid = false;
        if ((x2 - x1) <= 2 || (y2 - y1) <= 2) valid = false; /* 太小或无面积的框忽略 */
        /* 可选：score 阈值过滤
         * if (!(fr->score >= 0.0f && fr->score <= 1.0f) || fr->score < 0.10f) valid = false; */

        static uint32_t s_last_invalid_log_ms = 0;
        uint32_t now_ms = millis();
        if (!valid)
        {
            if ((uint32_t)(now_ms - s_last_invalid_log_ms) >= INVALID_LOG_INTERVAL_MS)
            {
                printf("RX[M4]: off=0x%08lx addr=%p invalid face=(%ld,%ld)-(%ld,%ld) skip\r\n",
                       (unsigned long)offset, (void*)addr,
                       (long)x1, (long)y1, (long)x2, (long)y2);
                s_last_invalid_log_ms = now_ms;
            }
            /* 无效帧：立即重置持续存在与确认状态，下一次需重新计时 */
            s_face_present = 0;
            s_face_confirmed = 0;
            #if FACE_DEBUG_CONFIRM
            printf("[S300][FACE] reset by invalid frame at %lums\r\n", (unsigned long)now_ms);
            #endif
            continue;
        }

        /* 有效框：开始/维持去抖计时，未达阈值前不算“确认” */
        if (!s_face_present) {
            s_face_present = 1;
            s_face_present_since = now_ms;
            #if FACE_DEBUG_CONFIRM
            printf("[S300][FACE] present start at %lums\r\n", (unsigned long)s_face_present_since);
            #endif
        }
        if (!s_face_confirmed) {
            uint32_t held = (uint32_t)(now_ms - s_face_present_since);
            if (held >= FACE_PRESENCE_CONFIRM_MS) {
                s_face_confirmed = 1;
                /* 刚确认：若处于眨眼态，隐藏GIF并恢复眼睛 */
                if (g_blink_active) {
                    blink_hide_and_restore();
                }
                #if FACE_DEBUG_CONFIRM
                printf("[S300][FACE] confirmed after %lums (start %lums -> now %lums)\r\n",
                       (unsigned long)held, (unsigned long)s_face_present_since, (unsigned long)now_ms);
                #endif
            }
        }

        int32_t cx_raw = x1 + (x2 - x1) / 2;
        int32_t cy_raw = y1 + (y2 - y1) / 2;

        /* 将原始坐标空间 (FACE_COORD_SPACE_W/H) 等比例映射到眼球中点允许范围 */
        int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
        int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);
        if (cx_raw < 0) { cx_raw = 0; }
        if (cx_raw > FACE_COORD_SPACE_W) { cx_raw = FACE_COORD_SPACE_W; }
        if (cy_raw < 0) { cy_raw = 0; }
        if (cy_raw > FACE_COORD_SPACE_H) { cy_raw = FACE_COORD_SPACE_H; }
        int32_t span_x = (max_x >= min_x) ? (max_x - min_x) : 0;
        int32_t span_y = (max_y >= min_y) ? (max_y - min_y) : 0;
        int32_t mid_x = min_x;
        int32_t mid_y = min_y;
        if (FACE_COORD_SPACE_W > 0 && span_x > 0)
            mid_x = min_x + (int32_t)(((int64_t)cx_raw * (int64_t)span_x) / (int64_t)FACE_COORD_SPACE_W);
        if (FACE_COORD_SPACE_H > 0 && span_y > 0)
            mid_y = min_y + (int32_t)(((int64_t)cy_raw * (int64_t)span_y) / (int64_t)FACE_COORD_SPACE_H);

        /* 简单EMA平滑，减少抖动 */
        static int s_ema_inited = 0;
        static int32_t s_mid_x = 0, s_mid_y = 0;
        if (!s_ema_inited)
        {
            s_mid_x = mid_x; s_mid_y = mid_y; s_ema_inited = 1;
        }
        else
        {
            s_mid_x = (int32_t)(((int64_t)s_mid_x * (EYE_SMOOTH_DEN - EYE_SMOOTH_NUM) + (int64_t)mid_x * EYE_SMOOTH_NUM) / EYE_SMOOTH_DEN);
            s_mid_y = (int32_t)(((int64_t)s_mid_y * (EYE_SMOOTH_DEN - EYE_SMOOTH_NUM) + (int64_t)mid_y * EYE_SMOOTH_NUM) / EYE_SMOOTH_DEN);
        }

        /* 速率限制（像素/秒）：限制每次命令相对上次命令的最大步长，减少突变导致的抖动 */
        int32_t lim_x = s_mid_x;
        int32_t lim_y = s_mid_y;
        if (EYE_RATE_LIMIT_PX_PER_S > 0 && s_have_last_cmd)
        {
            uint32_t dt_ms = (uint32_t)(now_ms - s_last_cmd_ms);
            /* 防止 0 除；dt=0 则不允许跨步 */
            uint32_t max_step = (dt_ms > 0u) ? (uint32_t)((EYE_RATE_LIMIT_PX_PER_S * (uint64_t)dt_ms) / 1000u) : 0u;
            /* 至少允许一步 1px，避免停滞 */
            if (max_step == 0u) max_step = 1u;
            int32_t dx_req = s_mid_x - s_last_cmd_mid_x;
            int32_t dy_req = s_mid_y - s_last_cmd_mid_y;
            if (dx_req > (int32_t)max_step) dx_req = (int32_t)max_step;
            if (dx_req < -(int32_t)max_step) dx_req = -(int32_t)max_step;
            if (dy_req > (int32_t)max_step) dy_req = (int32_t)max_step;
            if (dy_req < -(int32_t)max_step) dy_req = -(int32_t)max_step;
            lim_x = s_last_cmd_mid_x + dx_req;
            lim_y = s_last_cmd_mid_y + dy_req;
        }

    /* 仅在必要时移动与打印：小于阈值的微小变化不触发 */

    int32_t dx = s_have_last_cmd ? (lim_x - s_last_cmd_mid_x) : EYE_MIN_MOVE_PX;
    int32_t dy = s_have_last_cmd ? (lim_y - s_last_cmd_mid_y) : EYE_MIN_MOVE_PX;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        if (dx < EYE_MIN_MOVE_PX && dy < EYE_MIN_MOVE_PX)
        {
            /* 抑制抖动：不移动、不打印 */
            continue;
        }

        /* 未确认前：跳过移动与打印，仅进行平滑以减少确认瞬间的跳变 */
        if (!s_face_confirmed) {
            continue;
        }

        bool do_log = ((uint32_t)(now_ms - s_last_move_log_ms) >= MOVE_LOG_MIN_INTERVAL_MS);
        if (do_log)
        {
            printf("RX[M4]: off=0x%08lx addr=%p face=(%ld,%ld)-(%ld,%ld) center=(%ld,%ld) -> mid=(%ld,%ld) smoothed=(%ld,%ld)\r\n",
                   (unsigned long)offset, (void*)addr,
                   (long)x1, (long)y1, (long)x2, (long)y2, (long)cx_raw, (long)cy_raw, (long)mid_x, (long)mid_y, (long)s_mid_x, (long)s_mid_y);
            eyes_move_to_xy_and_log(lim_x, lim_y, "MBX");
            s_last_move_log_ms = now_ms;
        }
        else
        {
            /* 不打印，仅执行移动，避免日志刷屏影响性能 */
            eyes_goto_mid_xy(lim_x, lim_y);
        }
        /* 标记为“有有效人脸”，清除空闲状态 */
        s_last_valid_face_ms = now_ms;
        s_idle_active = 0;
        s_last_cmd_mid_x = lim_x;
        s_last_cmd_mid_y = lim_y;
        s_last_cmd_ms    = now_ms;
        s_have_last_cmd = 1;
    }

    /* 邮箱拉取结束后，若长时间无有效人脸，则回到居中（初始）状态 */
    uint32_t now2 = millis();
    if (s_last_valid_face_ms != 0u)
    {
        uint32_t dt = (uint32_t)(now2 - s_last_valid_face_ms);
        if (dt >= FACE_LOST_TIMEOUT_MS && !s_idle_active)
        {
            /* Show one blink action instead of returning to center */
            if ((uint32_t)(now2 - s_last_idle_log_ms) >= IDLE_RETURN_LOG_MIN_INTERVAL_MS)
            {
                printf("[S300][IDLE] no face %lums -> blink (show close-eye GIF)\r\n", (unsigned long)dt);
                s_last_idle_log_ms = now2;
            }
            blink_show();
            /* 长时间无确认人脸：复位确认/去抖状态，下一次需重新计时 */
            s_face_present = 0;
            s_face_confirmed = 0;
            #if FACE_DEBUG_CONFIRM
            printf("[S300][FACE] reset by idle at %lums\r\n", (unsigned long)now2);
            #endif
            s_idle_active = 1; /* 避免重复触发 */
        }
    }
}

int main(void)
{
    // 板级初始化：时钟 + UART3，printf 可用
    board_init();
    printf("\r\n[S300][DisplayDemo] Booting...\r\n");

    // 启动 SysTick 为 1ms 节拍
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        printf("[S300][DisplayDemo][ERR] SysTick_Config failed!\r\n");
    }

    rcc_init_mm_pll(8, 400, 0, 3, 2); /* 100MHz */
    rcc_init_dsp_pll(8, 400, 0, 2, 1); /* 300MHz */

    // 在初始化视频前先初始化 OV5640（DVP 摄像头经软 I2C 配置到 YUYV）
    int cam_ret = ov5640_preinit();
    if (cam_ret != 0) {
        printf("[S300][DisplayDemo][WARN] OV5640 init failed (%d), continue to init video for display path only.\r\n", cam_ret);
    }

    // 初始化视频子系统（包含 ST77 SPI LCD 序列）
    printf("[S300][DisplayDemo] init video...\r\n");
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);
    // while (1)
    // {
    //     /* code */;
    // }
    
 
    init_mailbox(MAILBOX_BASE, 4, MAILBOX_IRQ_NONE);
    set_dsp_warm_reset(true);

    write_mailbox(MAILBOX_BASE, 0x5A5A5A5A);

    // /* 测试与DSP通信 */
    // while (1)
    // {
    //     /* UART echo (non-blocking) */
    //     uart_echo_poll();
    //     monitor_mailbox_rx();
    //     /* tiny sleep ~5ms to reduce busy loop */
    //     uint32_t t0 = millis();
    //     while ((uint32_t)(millis() - t0) < 5u) { /* spin */ }
    // }

    volatile uint16_t* f0 = (volatile uint16_t*)DISP_RFRAME0_ADDR;
    volatile uint16_t* f1 = (volatile uint16_t*)DISP_RFRAME1_ADDR;
    volatile uint8_t*  a0 = (volatile uint8_t*)DISP_RALPHA0_ADDR;
    volatile uint8_t*  a1 = (volatile uint8_t*)DISP_RALPHA1_ADDR;

    /* Cache to globals for flush callback */
    s_f0 = f0; s_f1 = f1; s_a0 = a0; s_a1 = a1;

    printf("[S300][DisplayDemo] frame0=%p frame1=%p alpha0=%p alpha1=%p\r\n", (void*)f0, (void*)f1, (void*)a0, (void*)a1);

    const size_t pixels = (size_t)DISP_IMAGE_WIDTH * (size_t)DISP_IMAGE_HEIGHT;

    /* Prepare initial frame buffers: white canvas so transparent parts show white, not video */
    fill_buffer(f0, a0, pixels, 0xFFFFu, 0xAAu); // white
    fill_buffer(f1, a1, pixels, 0xFFFFu, 0xAAu); // white

    /* Stop presenting during init */
    REG32(REG_F0) = 0u;
    REG32(REG_F1) = 0u;

    /* ---------------- LVGL init ---------------- */
    lv_init();

    /* Print LVGL version once after init */
    printf("[S300][DisplayDemo] LVGL version: %d.%d.%d (%s)\r\n",
        lv_version_major(), lv_version_minor(), lv_version_patch(), lv_version_info());

    lv_display_t * disp = lv_display_create(DISP_IMAGE_WIDTH, DISP_IMAGE_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    /* Provide full-frame double buffers mapped to HW frame buffers */
    lv_display_set_buffers(disp,
                           (void*)f0,
                           (void*)f1,
                           (uint32_t)(DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    eyes_debug_dump("boot.after_disp_create");

    /* Make screen background opaque white to ensure alpha areas reveal white canvas */
    /* 设置屏幕背景为不透明的 #ffc21e 颜色 */
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xffc21e), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* FPS label removed to save space */

    /* Periodically print LVGL memory usage */
    // (void)lv_timer_create(mem_log_timer_cb, 2000, NULL);

     /* Two eyes (no runtime rotation): symmetric around horizontal center line, vertical movement */
     g_eye_spacing = 38;      /* vertical distance between eye centers */
     g_anim_time_ms = 2000;
     /* Build UI */
     eyes_ui_create();
    eyes_debug_dump("boot.after_ui_create");

    // LV_IMAGE_DECLARE(zhengyandonghua_rotated_cw);
    // lv_obj_t * img;

    // img = lv_gif_create(lv_screen_active());
    // lv_gif_set_color_format(img, LV_COLOR_FORMAT_RGB565);
    // lv_gif_set_src(img, &zhengyandonghua_rotated_cw);

    // lv_obj_t * img1;
    // img1 = lv_gif_create(lv_screen_active());
    // lv_gif_set_color_format(img1, LV_COLOR_FORMAT_RGB565);
    // lv_gif_set_src(img1, &zhengyandonghua_rotated_cw);

    printf("[S300][DisplayDemo] LVGL started.\r\n");
    printf("[S300][DisplayDemo] UART echo enabled on debug UART (CR->CRLF).\r\n");
    printf("[S300][DisplayDemo] Command: goto <y_mid>  (move eyes midpoint vertically)\r\n");

    /* Main loop: run LVGL timers */
    while (1)
    {
        /* UART echo (non-blocking) */
        // uart_echo_poll();
        monitor_mailbox_rx();
        lv_timer_handler();
        /* tiny sleep ~5ms to reduce busy loop */
        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 5u) { /* spin */ }
    }
}
