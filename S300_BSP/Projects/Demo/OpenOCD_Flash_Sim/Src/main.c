#include "s300.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "board.h"
#include "qspi_cadence.h"
#include "w25qxx.h"
#include <stdlib.h>

/*
 * OpenOCD-like Flash Ops Demo
 *
 * This demo simulates typical OpenOCD flash operations on a W25Qxx NOR:
 *  - probe (JEDEC ID + SR1-3)
 *  - unlock all protections
 *  - region erase (4KB sectors) over [addr, addr+len)
 *  - page program (256B) in a loop
 *  - verify (byte-wise + CRC32)
 *  - timings with 1us systick
 *
 * It does NOT enter XIP; it uses STIG commands via the W25Qxx driver, to
 * better reflect how OpenOCD drivers operate during flashing.
 */

#define SYSTICK_BASE 0xE000E010u

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} systick_t;

#define SYSTICK ((systick_t *)SYSTICK_BASE)

static uint32_t systick_reload;      // reload value (LOAD)
static uint32_t cycles_per_us;       // CPU cycles per microsecond

// Configure SysTick as a free-running cycle counter (no interrupt),
// with maximum reload; compute cycles_per_us from CPU clock.
static void systick_init(uint32_t cpu_hz)
{
    cycles_per_us = cpu_hz / 1000000u;
    if (cycles_per_us == 0u) cycles_per_us = 1u;
    systick_reload = 0xFFFFFFu; // 24-bit counter max
    SYSTICK->LOAD  = systick_reload; // counts down from LOAD to 0
    SYSTICK->VAL   = 0u;             // any write clears to LOAD
    SYSTICK->CTRL  = 0x5u;           // enable, use processor clock, no interrupt
}

static inline uint32_t systick_get_ticks(void)
{
    return SYSTICK->VAL; // counts down at CPU clock
}

// Return elapsed microseconds between two VAL snapshots, handling wrap.
static inline uint32_t systick_elapsed_us(uint32_t start_val, uint32_t end_val)
{
    // When counting down: elapsed_cycles = (start_val - end_val) mod (reload+1)
    uint32_t reload_plus_1 = systick_reload + 1u;
    uint32_t elapsed_cycles = (start_val >= end_val)
                            ? (start_val - end_val)
                            : (start_val + reload_plus_1 - end_val);
    // Convert cycles -> us with rounding: us = (cycles + cycles_per_us-1)/cycles_per_us
    uint32_t us = (elapsed_cycles + (cycles_per_us - 1u)) / cycles_per_us;
    return us;
}

static inline float speed_kb_per_s(uint32_t bytes, uint32_t us)
{
    if (us == 0u) return 0.0f;
    float kb = (float)bytes / 1024.0f;
    float secs = (float)us / 1000000.0f;
    return kb / secs; // KB/s
}

// ---------------------- UART CLI (minimal) ----------------------

typedef struct {
    uint32_t base;
    uint32_t length;
    uint32_t chunk_kb;   // program/verify chunk size in KB
    uint8_t  xip_dummy;
    uint8_t  xip_mode_bits;
    bool     verbose;
    bool     auto_xip_restore; // auto re-enter XIP after CLI ops
    bool     auto_xip_restore_eraseonly; // restore XIP after erase_region only
    bool     quiet;            // reduce per-chunk prints
} sim_cfg_t;

static sim_cfg_t g_cfg = {
    .base = 0x00100000u,
    .length = 8u * 1024u,
    .chunk_kb = 4u,
    .xip_dummy = 4u,
    .xip_mode_bits = 0x20u,
    .verbose = false,
    .auto_xip_restore = true,
    .auto_xip_restore_eraseonly = true,
    .quiet = false,
};

// ---------------------- Stats ----------------------

typedef struct {
    // erase
    uint32_t cnt_e4k, cnt_e32k, cnt_e64k;
    uint64_t us_e4k_sum, us_e32k_sum, us_e64k_sum;
    uint32_t us_e4k_min, us_e4k_max;
    uint32_t us_e32k_min, us_e32k_max;
    uint32_t us_e64k_min, us_e64k_max;
    // program page
    uint32_t cnt_page;
    uint64_t us_page_sum;
    uint32_t us_page_min, us_page_max;
    // chunk program
    uint32_t cnt_pchunk;
    uint64_t us_pchunk_sum;
    uint32_t us_pchunk_min, us_pchunk_max;
    // verify chunk
    uint32_t cnt_vchunk;
    uint64_t us_vchunk_sum;
    uint32_t us_vchunk_min, us_vchunk_max;
} sim_stats_t;

static sim_stats_t g_stats;
static uint32_t g_flash_size = 0u;
static w25qxx_info_t g_info; // store probed JEDEC info for CLI queries

static inline void stats_init(sim_stats_t *s)
{
    memset(s, 0, sizeof(*s));
    s->us_e4k_min = s->us_e32k_min = s->us_e64k_min = 0xFFFFFFFFu;
    s->us_page_min = s->us_pchunk_min = s->us_vchunk_min = 0xFFFFFFFFu;
}

static inline void stats_update(uint32_t *minp, uint32_t *maxp, uint64_t *sump, uint32_t us)
{
    if (us < *minp) *minp = us;
    if (us > *maxp) *maxp = us;
    *sump += us;
}

static void stats_print(const sim_stats_t *s)
{
    printf("\r\n[SIM][STATS] Erase 4K: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_e4k, (unsigned long long)s->us_e4k_sum,
        (unsigned long)(s->cnt_e4k? s->us_e4k_min:0), (unsigned long)s->us_e4k_max,
        (unsigned long)(s->cnt_e4k? (uint32_t)(s->us_e4k_sum / s->cnt_e4k):0));
    printf("[SIM][STATS] Erase 32K: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_e32k, (unsigned long long)s->us_e32k_sum,
        (unsigned long)(s->cnt_e32k? s->us_e32k_min:0), (unsigned long)s->us_e32k_max,
        (unsigned long)(s->cnt_e32k? (uint32_t)(s->us_e32k_sum / s->cnt_e32k):0));
    printf("[SIM][STATS] Erase 64K: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_e64k, (unsigned long long)s->us_e64k_sum,
        (unsigned long)(s->cnt_e64k? s->us_e64k_min:0), (unsigned long)s->us_e64k_max,
        (unsigned long)(s->cnt_e64k? (uint32_t)(s->us_e64k_sum / s->cnt_e64k):0));
    printf("[SIM][STATS] Page Prog: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_page, (unsigned long long)s->us_page_sum,
        (unsigned long)(s->cnt_page? s->us_page_min:0), (unsigned long)s->us_page_max,
        (unsigned long)(s->cnt_page? (uint32_t)(s->us_page_sum / s->cnt_page):0));
    printf("[SIM][STATS] ProgChunk: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_pchunk, (unsigned long long)s->us_pchunk_sum,
        (unsigned long)(s->cnt_pchunk? s->us_pchunk_min:0), (unsigned long)s->us_pchunk_max,
        (unsigned long)(s->cnt_pchunk? (uint32_t)(s->us_pchunk_sum / s->cnt_pchunk):0));
    printf("[SIM][STATS] VerfChunk: cnt=%lu sum=%llu us min=%lu max=%lu avg=%lu us\r\n",
        (unsigned long)s->cnt_vchunk, (unsigned long long)s->us_vchunk_sum,
        (unsigned long)(s->cnt_vchunk? s->us_vchunk_min:0), (unsigned long)s->us_vchunk_max,
        (unsigned long)(s->cnt_vchunk? (uint32_t)(s->us_vchunk_sum / s->cnt_vchunk):0));
}

static int uart_getline(char *buf, int maxlen)
{
    int n = 0;
    while (n < maxlen - 1) {
        uint16_t ch = uart_read(UART_IDX3, UARTTYPE_STD_SERIAL); // blocking
        char c = (char)(ch & 0xFF);
        if (c == '\r') continue;
        if (c == '\n') break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

static int uart_read_exact(uint8_t *buf, uint32_t len)
{
    uint32_t got = 0;
    while (got < len) {
        uint16_t d = uart_read(UART_IDX3, UARTTYPE_STD_SERIAL);
        buf[got++] = (uint8_t)(d & 0xFF);
    }
    return 0;
}

// On error: exit XIP, print status, try unlock, retry once (caller provides op)
static void flash_recovery_note(void)
{
    uint8_t s1=0,s2=0,s3=0; qspi_read_status(&s1,&s2,&s3);
    printf("[SIM][RECOVER] SR1=0x%02X SR2=0x%02X SR3=0x%02X -> unlock all\r\n", s1,s2,s3);
    qspi_unlock_all(); qspi_read_status(&s1,&s2,&s3);
    printf("[SIM][RECOVER] after unlock SR1=0x%02X SR2=0x%02X SR3=0x%02X\r\n", s1,s2,s3);
}

static int try_erase4k(uint32_t a)
{
    int rc = w25qxx_erase_4k(a);
    if (rc != 0) { w25qxx_exit_xip(); flash_recovery_note(); rc = w25qxx_erase_4k(a); }
    return rc;
}
static int try_erase32k(uint32_t a)
{
    int rc = w25qxx_erase_32k(a);
    if (rc != 0) { w25qxx_exit_xip(); flash_recovery_note(); rc = w25qxx_erase_32k(a); }
    return rc;
}
static int try_erase64k(uint32_t a)
{
    int rc = w25qxx_erase_64k(a);
    if (rc != 0) { w25qxx_exit_xip(); flash_recovery_note(); rc = w25qxx_erase_64k(a); }
    return rc;
}
static int try_prog_page(uint32_t a, const void *p, uint32_t n)
{
    int rc = w25qxx_write_page(a, p, n);
    if (rc != 0) { w25qxx_exit_xip(); flash_recovery_note(); rc = w25qxx_write_page(a, p, n); }
    return rc;
}
static int try_read(uint32_t a, void *p, uint32_t n)
{
    int rc = w25qxx_read(a, p, n);
    if (rc != 0) { w25qxx_exit_xip(); flash_recovery_note(); rc = w25qxx_read(a, p, n); }
    return rc;
}

static void print_help(void)
{
    printf("\r\nCommands:\r\n");
    printf("  help                                 - show this help\r\n");
    printf("  cfg base 0xADDR len BYTES            - set base & length\r\n");
    printf("  cfg chunk KB                         - set program chunk size in KB (default 4)\r\n");
    printf("  cfg verbose 0|1                      - toggle verbose per-page prints\r\n");
    printf("  cfg quiet 0|1                        - suppress per-chunk logs (default 0)\r\n");
    printf("  cfg xiprestore 0|1                   - auto re-enter XIP after CLI ops (default 1)\r\n");
    printf("  cfg xiprestore eraseonly 0|1         - restore XIP after erase_region only (default 1)\r\n");
    printf("  xip set dummy N mode 0xMM            - set XIP params (1-4-4 mode bits)\r\n");
    printf("  xip enter                            - enter XIP with current params\r\n");
    printf("  xip exit                             - exit XIP\r\n");
    printf("  run [BYTES]                           - erase+program+verify; with BYTES uses generated pattern; no-arg prints plan and auto-restores XIP\r\n");
    printf("  erase_plan  0xADDR SIZE               - preview erase plan (64K -> 32K -> 4K)\r\n");
    printf("  erase_region 0xADDR SIZE              - mixed 64K/32K/4K erase\r\n");
    printf("  write_image  0xADDR SIZE              - host sends SIZE raw bytes after READY\r\n");
    printf("  verify_image 0xADDR SIZE              - compute CRC32 over flash region\r\n");
    printf("  flash_image  0xADDR SIZE              - erase + receive + program + verify, final report\r\n");
    printf("  stats                                 - print operation statistics\r\n");
    printf("  stats reset                           - reset statistics\r\n");
    printf("  run_1mb | run1m                       - synthetic 1MiB image: mixed erase + generated program/verify\r\n");
    printf("  version | id                          - print firmware version and JEDEC/flash info\r\n");
}

// ---------------------- Flash ops helpers ----------------------


static void print_status_all(const char *tag)
{
    uint8_t s1=0, s2=0, s3=0;
    if (qspi_read_status(&s1, &s2, &s3) == 0) {
        unsigned busy = (s1 & 0x01u) ? 1u : 0u;
        unsigned wel  = (s1 & 0x02u) ? 1u : 0u;
        unsigned qe   = (s2 & 0x02u) ? 1u : 0u;
        printf("%s SR1=0x%02X (BUSY=%u WEL=%u) SR2=0x%02X (QE=%u) SR3=0x%02X\r\n",
               tag ? tag : "[SIM]", s1, busy, wel, s2, qe, s3);
    } else {
        printf("%s read status failed\r\n", tag ? tag : "[SIM]");
    }
}

/* simple CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, xorout 0xFFFFFFFF) */
static uint32_t crc32_le(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (unsigned b = 0; b < 8; ++b) {
            uint32_t m = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & m);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

//
// Timed operations with total time accumulation
//




// Mixed erase strategy: cover [addr, addr+len) by erasing
// from start4k=floor(addr,4K) to end4k=ceil(end,4K). Prefer 64K aligned blocks.
static int erase_region_mixed(uint32_t addr, uint32_t len, uint32_t *total_us_out)
{
    const uint32_t S4K = 0x1000u;
    const uint32_t S64K = 0x10000u;
    const uint32_t S32K = 0x8000u;
    if (len == 0u) { if (total_us_out) *total_us_out = 0; return 0; }
    uint32_t start4k = addr & ~(S4K - 1u);
    uint32_t end4k = (addr + len + (S4K - 1u)) & ~(S4K - 1u);
    uint32_t cur = start4k;
    uint32_t total_us = 0u;
    unsigned n4 = 0, n32 = 0, n64 = 0;

    while (cur < end4k) {
        if (((cur % S64K) == 0u) && ((end4k - cur) >= S64K)) {
            printf("[SIM] Erase 64K @0x%08lX... ", (unsigned long)cur);
            uint32_t t0 = systick_get_ticks();
            int rc = try_erase64k(cur);
            uint32_t t1 = systick_get_ticks();
            uint32_t us = systick_elapsed_us(t0, t1);
            if (rc != 0) { printf("FAIL (rc=%d)\r\n", rc); return rc ? rc : -10; }
            printf("OK (%lu us)\r\n", (unsigned long)us);
            total_us += us; cur += S64K; ++n64;
            g_stats.cnt_e64k++; stats_update(&g_stats.us_e64k_min, &g_stats.us_e64k_max, &g_stats.us_e64k_sum, us);
        } else if (((cur % S32K) == 0u) && ((end4k - cur) >= S32K)) {
            printf("[SIM] Erase 32K @0x%08lX... ", (unsigned long)cur);
            uint32_t t0 = systick_get_ticks();
            int rc = try_erase32k(cur);
            uint32_t t1 = systick_get_ticks();
            uint32_t us = systick_elapsed_us(t0, t1);
            if (rc != 0) { printf("FAIL (rc=%d)\r\n", rc); return rc ? rc : -12; }
            printf("OK (%lu us)\r\n", (unsigned long)us);
            total_us += us; cur += S32K; ++n32;
            g_stats.cnt_e32k++; stats_update(&g_stats.us_e32k_min, &g_stats.us_e32k_max, &g_stats.us_e32k_sum, us);
        } else {
            printf("[SIM] Erase 4K  @0x%08lX... ", (unsigned long)cur);
            uint32_t t0 = systick_get_ticks();
            int rc = try_erase4k(cur);
            uint32_t t1 = systick_get_ticks();
            uint32_t us = systick_elapsed_us(t0, t1);
            if (rc != 0) { printf("FAIL (rc=%d)\r\n", rc); return rc ? rc : -11; }
            printf("OK (%lu us)\r\n", (unsigned long)us);
            total_us += us; cur += S4K; ++n4;
            g_stats.cnt_e4k++; stats_update(&g_stats.us_e4k_min, &g_stats.us_e4k_max, &g_stats.us_e4k_sum, us);
        }
    }
    printf("[SIM] Erased: %u x64K + %u x32K + %u x4K (covers 0x%08lX..0x%08lX)\r\n",
           n64, n32, n4, (unsigned long)start4k, (unsigned long)end4k);
    if (total_us_out) *total_us_out = total_us;
    return 0;
}

// Preview erase plan with ordering visualization: 64K blocks first, then 32K, then 4K
static void erase_plan_preview(uint32_t addr, uint32_t len)
{
    const uint32_t S4K = 0x1000u;
    const uint32_t S64K = 0x10000u;
    const uint32_t S32K = 0x8000u;
    if (len == 0u) { printf("[SIM][PLAN] empty length\r\n"); return; }
    uint32_t start4k = addr & ~(S4K - 1u);
    uint32_t end4k = (addr + len + (S4K - 1u)) & ~(S4K - 1u);
    uint32_t first64 = (start4k + (S64K - 1u)) & ~(S64K - 1u);
    if (first64 > end4k) first64 = end4k;
    uint32_t last64_end = end4k & ~(S64K - 1u); // end of last full 64K-aligned window
    if (last64_end < first64) last64_end = first64;

    unsigned c64 = 0, c32 = 0, c4 = 0;
    // Count middle 64K blocks
    for (uint32_t a = first64; a + S64K <= end4k; a += S64K) ++c64;
    // Head section: [start4k, first64)
    for (uint32_t a = start4k; a < first64; ) {
        if (((a % S32K) == 0u) && (first64 - a) >= S32K) { ++c32; a += S32K; }
        else { ++c4; a += S4K; }
    }
    // Tail section: [last64_end, end4k)
    for (uint32_t a = last64_end; a < end4k; ) {
        if (((a % S32K) == 0u) && (end4k - a) >= S32K) { ++c32; a += S32K; }
        else { ++c4; a += S4K; }
    }

    printf("[SIM][PLAN] region 0x%08lX..0x%08lX (len=%lu)\r\n",
           (unsigned long)start4k, (unsigned long)end4k, (unsigned long)(end4k - start4k));
    printf("[SIM][PLAN] 64K blocks: %u\r\n", c64);
    // Print up to 16 addresses per type to avoid log flood
    unsigned shown = 0;
    for (uint32_t a = first64; a + S64K <= end4k; a += S64K) {
        if (shown++ == 0) printf("[SIM][PLAN]   64K @");
        if (shown <= 16) printf(" 0x%08lX", (unsigned long)a);
        else if (shown == 17) printf(" ...");
    }
    if (shown) printf("\r\n");

    // Recompute and show 32K addresses (head + tail)
    shown = 0;
    for (uint32_t a = start4k; a < first64; ) {
        if (((a % S32K) == 0u) && (first64 - a) >= S32K) {
            if (shown++ == 0) printf("[SIM][PLAN]   32K @");
            if (shown <= 16) printf(" 0x%08lX", (unsigned long)a);
            else if (shown == 17) printf(" ...");
            a += S32K;
        } else { a += S4K; }
    }
    for (uint32_t a = last64_end; a < end4k; ) {
        if (((a % S32K) == 0u) && (end4k - a) >= S32K) {
            if (shown++ == 0) printf("[SIM][PLAN]   32K @");
            if (shown <= 16) printf(" 0x%08lX", (unsigned long)a);
            else if (shown == 17) printf(" ...");
            a += S32K;
        } else { a += S4K; }
    }
    if (shown) printf("\r\n");

    // 4K addresses (head + tail)
    shown = 0;
    for (uint32_t a = start4k; a < first64; ) {
        if (!(((a % S32K) == 0u) && (first64 - a) >= S32K)) {
            if (shown++ == 0) printf("[SIM][PLAN]   4K  @");
            if (shown <= 16) printf(" 0x%08lX", (unsigned long)a);
            else if (shown == 17) printf(" ...");
            a += S4K;
        } else { a += S32K; }
    }
    for (uint32_t a = last64_end; a < end4k; ) {
        if (!(((a % S32K) == 0u) && (end4k - a) >= S32K)) {
            if (shown++ == 0) printf("[SIM][PLAN]   4K  @");
            if (shown <= 16) printf(" 0x%08lX", (unsigned long)a);
            else if (shown == 17) printf(" ...");
            a += S4K;
        } else { a += S32K; }
    }
    if (shown) printf("\r\n");
    printf("[SIM][PLAN] Exec order: 64K -> 32K -> 4K (visualization)\r\n");
}

// Chunked program using page writes within chunk; reduced prints.
static int program_region_chunked(uint32_t addr, const uint8_t *buf, uint32_t len,
                                  uint32_t chunk_bytes, uint32_t *total_us_out)
{
    const uint32_t PAGE = 256u;
    uint32_t off = 0u;
    uint32_t total_us = 0u;
    while (off < len) {
        uint32_t a = addr + off;
        uint32_t chunk = (len - off > chunk_bytes) ? chunk_bytes : (len - off);
        uint32_t cstart = systick_get_ticks();
        uint32_t inner_us = 0u;
        uint32_t inner_off = 0u;
        while (inner_off < chunk) {
            uint32_t pa = a + inner_off;
            uint32_t page_off = pa & (PAGE - 1u);
            uint32_t wr = PAGE - page_off;
            if (wr > (chunk - inner_off)) wr = chunk - inner_off;
            uint32_t t0 = systick_get_ticks();
            int rc = try_prog_page(pa, &buf[off + inner_off], wr);
            uint32_t t1 = systick_get_ticks();
            uint32_t us = systick_elapsed_us(t0, t1);
            if (rc != 0) return -20;
            inner_us += us;
            g_stats.cnt_page++; stats_update(&g_stats.us_page_min, &g_stats.us_page_max, &g_stats.us_page_sum, us);
            inner_off += wr;
        }
        uint32_t cend = systick_get_ticks();
        uint32_t chunk_us = systick_elapsed_us(cstart, cend);
        total_us += inner_us; // account actual program time
        g_stats.cnt_pchunk++; stats_update(&g_stats.us_pchunk_min, &g_stats.us_pchunk_max, &g_stats.us_pchunk_sum, chunk_us);
        if (!g_cfg.quiet) {
            printf("[SIM] Prog chunk %5luB @0x%08lX: time %lu us\r\n",
                   (unsigned long)chunk, (unsigned long)a, (unsigned long)chunk_us);
        }
        off += chunk;
    }
    if (total_us_out) *total_us_out = total_us;
    return 0;
}

static int verify_region_chunked(uint32_t addr, const uint8_t *buf, uint32_t len,
                                 uint32_t chunk_bytes, uint32_t *total_us_out)
{
    uint8_t tmp[256];
    uint32_t off = 0u;
    uint32_t total_us = 0u;
    uint32_t crc_w = crc32_le(buf, len);
    uint32_t crc_r = 0xFFFFFFFFu;
    while (off < len) {
        uint32_t chunk = (len - off > chunk_bytes) ? chunk_bytes : (len - off);
        uint32_t cstart = systick_get_ticks();
        uint32_t inner_off = 0u;
        while (inner_off < chunk) {
            uint32_t rsz = (chunk - inner_off > sizeof(tmp)) ? sizeof(tmp) : (chunk - inner_off);
            int rc = try_read(addr + off + inner_off, tmp, rsz);
            if (rc != 0) return -21;
            for (uint32_t i = 0; i < rsz; ++i) {
                crc_r ^= tmp[i];
                for (unsigned b = 0; b < 8; ++b) {
                    uint32_t m = -(crc_r & 1u);
                    crc_r = (crc_r >> 1) ^ (0xEDB88320u & m);
                }
                if (tmp[i] != buf[off + inner_off + i]) {
                    printf("[SIM] MISMATCH @0x%08lX: W=%02X R=%02X\r\n",
                           (unsigned long)(addr + off + inner_off + i), buf[off + inner_off + i], tmp[i]);
                    return -22;
                }
            }
            inner_off += rsz;
        }
     uint32_t cend = systick_get_ticks();
     uint32_t chunk_us = systick_elapsed_us(cstart, cend);
     total_us += chunk_us;
     g_stats.cnt_vchunk++; stats_update(&g_stats.us_vchunk_min, &g_stats.us_vchunk_max, &g_stats.us_vchunk_sum, chunk_us);
     if (!g_cfg.quiet) {
         printf("[SIM] Verify chunk %5luB @0x%08lX: time %lu us\r\n",
             (unsigned long)chunk, (unsigned long)(addr + off), (unsigned long)chunk_us);
     }
        off += chunk;
    }
    crc_r ^= 0xFFFFFFFFu;
    if (crc_w != crc_r) {
        printf("[SIM] CRC mismatch: W=%08lX R=%08lX\r\n",
               (unsigned long)crc_w, (unsigned long)crc_r);
        return -23;
    }
    if (total_us_out) *total_us_out = total_us;
    return 0;
}

// Generate deterministic byte pattern used for synthetic images
static inline uint8_t gen_pattern_byte(uint32_t base, uint32_t rel_index)
{
    return (uint8_t)((rel_index ^ 0x5Au) + (base >> 12));
}

static int program_region_generated(uint32_t base, uint32_t len, uint32_t chunk_bytes, uint32_t *total_us_out)
{
    const uint32_t PAGE = 256u;
    uint32_t off = 0u;
    uint32_t total_us = 0u;
    if (chunk_bytes == 0u) chunk_bytes = 4096u;
    // allocate one chunk buffer
    uint8_t *buf = (uint8_t *)malloc(chunk_bytes);
    if (!buf) return -30;
    while (off < len) {
        uint32_t a = base + off;
        uint32_t chunk = (len - off > chunk_bytes) ? chunk_bytes : (len - off);
        // fill pattern for this chunk
        for (uint32_t i = 0; i < chunk; ++i) buf[i] = gen_pattern_byte(base, off + i);
        uint32_t cstart = systick_get_ticks();
        uint32_t inner_off = 0u; uint32_t inner_us = 0u;
        while (inner_off < chunk) {
            uint32_t pa = a + inner_off;
            uint32_t page_off = pa & (PAGE - 1u);
            uint32_t wr = PAGE - page_off;
            if (wr > (chunk - inner_off)) wr = chunk - inner_off;
            uint32_t t0 = systick_get_ticks();
            int rc = try_prog_page(pa, &buf[inner_off], wr);
            uint32_t t1 = systick_get_ticks();
            uint32_t us = systick_elapsed_us(t0, t1);
            if (rc != 0) { free(buf); return -31; }
            inner_us += us; g_stats.cnt_page++; stats_update(&g_stats.us_page_min, &g_stats.us_page_max, &g_stats.us_page_sum, us);
            inner_off += wr;
        }
        uint32_t cend = systick_get_ticks();
        uint32_t chunk_us = systick_elapsed_us(cstart, cend);
        total_us += inner_us;
     g_stats.cnt_pchunk++; stats_update(&g_stats.us_pchunk_min, &g_stats.us_pchunk_max, &g_stats.us_pchunk_sum, chunk_us);
     if (!g_cfg.quiet) {
         printf("[SIM] Prog chunk %5luB @0x%08lX: time %lu us\r\n",
             (unsigned long)chunk, (unsigned long)a, (unsigned long)chunk_us);
     }
        off += chunk;
    }
    free(buf);
    if (total_us_out) *total_us_out = total_us;
    return 0;
}

static int verify_region_generated(uint32_t base, uint32_t len, uint32_t chunk_bytes, uint32_t *total_us_out, uint32_t *crc_out)
{
    if (chunk_bytes == 0u) chunk_bytes = 4096u;
    uint8_t *buf = (uint8_t *)malloc(chunk_bytes);
    if (!buf) return -32;
    uint32_t off = 0u; uint32_t total_us = 0u; uint32_t crc = 0xFFFFFFFFu;
    while (off < len) {
        uint32_t a = base + off;
        uint32_t chunk = (len - off > chunk_bytes) ? chunk_bytes : (len - off);
        uint32_t cstart = systick_get_ticks();
        int rc = try_read(a, buf, chunk);
        uint32_t cend = systick_get_ticks();
        uint32_t chunk_us = systick_elapsed_us(cstart, cend);
        total_us += chunk_us;
        g_stats.cnt_vchunk++; stats_update(&g_stats.us_vchunk_min, &g_stats.us_vchunk_max, &g_stats.us_vchunk_sum, chunk_us);
        if (rc != 0) { free(buf); return -33; }
        for (uint32_t i = 0; i < chunk; ++i) {
            uint8_t expect = gen_pattern_byte(base, off + i);
            if (buf[i] != expect) {
                printf("[SIM] MISMATCH @0x%08lX: W=%02X R=%02X\r\n", (unsigned long)(a + i), expect, buf[i]);
                free(buf); return -34;
            }
            crc ^= buf[i];
            for (unsigned b = 0; b < 8; ++b) { uint32_t m = -(crc & 1u); crc = (crc >> 1) ^ (0xEDB88320u & m); }
        }
     if (!g_cfg.quiet) {
         printf("[SIM] Verify chunk %5luB @0x%08lX: time %lu us\r\n",
             (unsigned long)chunk, (unsigned long)a, (unsigned long)chunk_us);
     }
        off += chunk;
    }
    free(buf);
    crc ^= 0xFFFFFFFFu;
    if (total_us_out) *total_us_out = total_us;
    if (crc_out) *crc_out = crc;
    return 0;
}

// Compute CRC32 over a flash region by reading in chunks
static int crc_region(uint32_t addr, uint32_t len, uint32_t chunk_bytes,
                      uint32_t *total_us_out, uint32_t *crc_out)
{
    if (chunk_bytes == 0u) chunk_bytes = 4096u;
    uint8_t *buf = (uint8_t *)malloc(chunk_bytes);
    if (!buf) return -40;
    uint32_t off = 0u, total_us = 0u; uint32_t crc = 0xFFFFFFFFu;
    while (off < len) {
        uint32_t a = addr + off;
        uint32_t chunk = (len - off > chunk_bytes) ? chunk_bytes : (len - off);
        uint32_t t0 = systick_get_ticks();
        int rc = try_read(a, buf, chunk);
        uint32_t t1 = systick_get_ticks();
        uint32_t us = systick_elapsed_us(t0, t1);
        total_us += us;
        if (rc != 0) { free(buf); return -41; }
        for (uint32_t i = 0; i < chunk; ++i) {
            crc ^= buf[i];
            for (unsigned b = 0; b < 8; ++b) { uint32_t m = -(crc & 1u); crc = (crc >> 1) ^ (0xEDB88320u & m); }
        }
        off += chunk;
    }
    free(buf);
    crc ^= 0xFFFFFFFFu;
    if (total_us_out) *total_us_out = total_us;
    if (crc_out) *crc_out = crc;
    return 0;
}

int main(void)
{
    board_init();

    uint32_t ahb_hz  = rcc_get_clock(RCC_CLOCK_AHB);
    uint32_t sclk_hz = ahb_hz / 2u; // conservative; tune per board/timing

    // SysTick: free-running at CPU clock, convert cycles -> microseconds
    systick_init(ahb_hz);

    printf("\r\n[SIM] OpenOCD-like Flash Ops demo start. AHB=%lu Hz, QSPI=%lu Hz\r\n",
           (unsigned long)ahb_hz, (unsigned long)sclk_hz);

    qspi_set_verbose(false);
    qspi_cadence_init(ahb_hz, sclk_hz);

    w25qxx_info_t info;
    // Default: prefer 3-byte addr for <=16MiB, 4-byte for larger parts
    bool force_qe = true;      // enable QE if supported
    bool use_4b   = false;     // auto-adjust after probe

    if (w25qxx_init(&info, force_qe, use_4b) != 0) {
        printf("[SIM] w25qxx_init failed\r\n");
        return -1;
    }
    g_flash_size = info.size_bytes;
    g_info = info;

    // Driver may auto-enable 4B when size > 16MiB or if want_4byte_addr=true in init.
    // No explicit toggle here since API is not exposed; info.addr4b reflects active mode.

    (void)qspi_unlock_all();

    printf("[SIM] JEDEC: manuf=0x%02X type=0x%02X cap=0x%02X size=%lu\r\n",
           info.manuf_id, info.memory_type, info.capacity, (unsigned long)info.size_bytes);
    printf("[SIM] QE=%u 4B=%u\r\n", info.quad_enabled ? 1u : 0u, info.addr4b ? 1u : 0u);
    print_status_all("[SIM] Status(before):");

    // Choose region similar to OpenOCD flashing flow
    // Config (defaults can be changed via CLI later)
    const bool restore_xip = true;       // re-enter XIP after operations if true

    // Generate a test image (pattern) crossing page boundaries
    static uint8_t image[8u * 1024u];
    for (uint32_t i = 0; i < sizeof(image); ++i) {
        image[i] = (uint8_t)((i ^ 0x5Au) + (g_cfg.base >> 12));
    }

    stats_init(&g_stats);

    // Erase region (mixed 64K/32K/4K)
    uint32_t erase_us = 0u;
    int rc = erase_region_mixed(g_cfg.base, g_cfg.length, &erase_us);
    if (rc != 0) {
        printf("[SIM] Erase failed rc=%d\r\n", rc);
        return rc;
    }
    printf("[SIM] Erase total: %lu us, speed: %.2f KB/s\r\n",
        (unsigned long)erase_us, speed_kb_per_s(g_cfg.length, erase_us));

    // Program region (page loop)
    uint32_t prog_us = 0u;
    rc = program_region_chunked(g_cfg.base, image, g_cfg.length, g_cfg.chunk_kb * 1024u, &prog_us);
    if (rc != 0) {
        printf("[SIM] Program failed rc=%d\r\n", rc);
        return rc;
    }
    printf("[SIM] Program total: %lu us, speed: %.2f KB/s\r\n",
        (unsigned long)prog_us, speed_kb_per_s(g_cfg.length, prog_us));

    // Verify
    uint32_t ver_us = 0u;
    rc = verify_region_chunked(g_cfg.base, image, g_cfg.length, g_cfg.chunk_kb * 1024u, &ver_us);
    if (rc != 0) {
        printf("[SIM] Verify failed rc=%d\r\n", rc);
        return rc;
    }
    printf("[SIM] Verify total: %lu us, speed: %.2f KB/s\r\n",
        (unsigned long)ver_us, speed_kb_per_s(g_cfg.length, ver_us));

    stats_print(&g_stats);

    print_status_all("[SIM] Status(after):");

    // Optionally restore XIP mode for normal runtime
    if (restore_xip) {
        // For W25Q128: often use 3-byte addr, dummy=4, mode_bits=0x20 (aligns to earlier demos)
        if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
            volatile const uint8_t *xip = (volatile const uint8_t *)g_qspi.ahb;
            uint8_t peek[8];
            for (unsigned i = 0; i < sizeof(peek); ++i) peek[i] = xip[g_cfg.base + i];
            printf("[SIM] XIP restored, peek: ");
            for (unsigned i = 0; i < sizeof(peek); ++i) printf("%02X ", peek[i]);
            printf("\r\n");
        } else {
            printf("[SIM] Restore XIP failed.\r\n");
        }
    }

    printf("[SIM] Demo DONE.\r\n");
    // Enter CLI loop for parameterized runs
    print_help();
    char line[128];
    while (1) {
        printf("\r\n> ");
        int n = uart_getline(line, sizeof(line));
        if (n <= 0) continue;
        // trim basic
        if (strcmp(line, "help") == 0) { print_help(); continue; }
        if (strncmp(line, "cfg base", 8) == 0) {
            unsigned long b=0, l=0; unsigned matched=0;
            matched = (unsigned)sscanf(line, "cfg base %lx len %lu", &b, &l);
            if (matched >= 2) {
                if (l == 0) { printf("[SIM] cfg: len must be > 0\r\n"); continue; }
                g_cfg.base = (uint32_t)b; g_cfg.length = (uint32_t)l;
                if ((g_cfg.base & 0xFFFu) != 0) {
                    printf("[SIM] cfg: base not 4K-aligned, erase will extend to 4K boundaries\r\n");
                }
                if (g_flash_size && (g_cfg.base + g_cfg.length > g_flash_size)) {
                    printf("[SIM] cfg: warning: region exceeds flash size=%lu\r\n", (unsigned long)g_flash_size);
                }
                printf("[SIM] cfg: base=0x%08lX len=%lu\r\n", (unsigned long)g_cfg.base, (unsigned long)g_cfg.length);
            }
            else printf("[SIM] usage: cfg base 0xADDR len BYTES\r\n");
            continue;
        }
        if (strncmp(line, "cfg chunk", 9) == 0) {
            unsigned long kb=0; if (sscanf(line, "cfg chunk %lu", &kb) == 1 && kb>=1 && kb<=128) { g_cfg.chunk_kb = (uint32_t)kb; }
            printf("[SIM] cfg: chunk=%lu KB\r\n", (unsigned long)g_cfg.chunk_kb); continue;
        }
        if (strncmp(line, "cfg verbose", 11) == 0) {
            int v=0; if (sscanf(line, "cfg verbose %d", &v) == 1) g_cfg.verbose = (v!=0);
            printf("[SIM] cfg: verbose=%d\r\n", g_cfg.verbose?1:0); continue;
        }
        if (strncmp(line, "cfg quiet", 10) == 0) {
            int v=0; if (sscanf(line, "cfg quiet %d", &v) == 1) g_cfg.quiet = (v!=0);
            printf("[SIM] cfg: quiet=%d\r\n", g_cfg.quiet?1:0); continue;
        }
        if (strncmp(line, "cfg xiprestore", 14) == 0) {
            int v=0;
            if (sscanf(line, "cfg xiprestore eraseonly %d", &v) == 1) {
                g_cfg.auto_xip_restore_eraseonly = (v!=0);
                printf("[SIM] cfg: xiprestore.eraseonly=%d\r\n", g_cfg.auto_xip_restore_eraseonly?1:0);
            } else if (sscanf(line, "cfg xiprestore %d", &v) == 1) {
                g_cfg.auto_xip_restore = (v!=0);
                printf("[SIM] cfg: xiprestore=%d\r\n", g_cfg.auto_xip_restore?1:0);
            } else {
                printf("[SIM] usage: cfg xiprestore 0|1 | cfg xiprestore eraseonly 0|1\r\n");
            }
            continue;
        }
        if (strncmp(line, "xip set", 7) == 0) {
            unsigned d=0; unsigned m=0; if (sscanf(line, "xip set dummy %u mode %x", &d, &m) == 2) { g_cfg.xip_dummy=(uint8_t)d; g_cfg.xip_mode_bits=(uint8_t)m; printf("[SIM] xip: dummy=%u mode=0x%02X\r\n", g_cfg.xip_dummy, g_cfg.xip_mode_bits); }
            else printf("[SIM] usage: xip set dummy N mode 0xMM\r\n");
            continue;
        }
        if (strcmp(line, "xip enter") == 0) {
            if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                volatile const uint8_t *xip = (volatile const uint8_t *)g_qspi.ahb;
                uint8_t peek[8]; for (unsigned i=0;i<sizeof(peek);++i) peek[i] = xip[g_cfg.base+i];
                printf("[SIM] XIP entered, peek: "); for (unsigned i=0;i<sizeof(peek);++i) printf("%02X ", peek[i]); printf("\r\n");
            } else printf("[SIM] XIP enter failed\r\n");
            continue;
        }
        if (strcmp(line, "xip exit") == 0) { w25qxx_exit_xip(); printf("[SIM] XIP exited\r\n"); continue; }
        if (strcmp(line, "run_1mb") == 0 || strcmp(line, "run1m") == 0) {
            uint32_t size = 1024u * 1024u; uint32_t addr = g_cfg.base;
            if (g_flash_size && (addr + size > g_flash_size)) { printf("[SIM] out of range: flash size=%lu\r\n", (unsigned long)g_flash_size); continue; }
            printf("[SIM] RUN 1MiB: base=0x%08lX len=%lu chunk=%luKB\r\n", (unsigned long)addr, (unsigned long)size, (unsigned long)g_cfg.chunk_kb);
            stats_init(&g_stats);
            uint32_t e_us=0, p_us=0, v_us=0, crc=0;
            erase_plan_preview(addr, size);
            int rc1 = erase_region_mixed(addr, size, &e_us);
            if (rc1==0) rc1 = program_region_generated(addr, size, g_cfg.chunk_kb*1024u, &p_us);
            if (rc1==0) rc1 = verify_region_generated(addr, size, g_cfg.chunk_kb*1024u, &v_us, &crc);
            printf("[SIM] 1MiB: erase %lu us (%.2f KB/s), prog %lu us (%.2f KB/s), verify %lu us (%.2f KB/s), CRC=%08lX\r\n",
                   (unsigned long)e_us, speed_kb_per_s(size, e_us),
                   (unsigned long)p_us, speed_kb_per_s(size, p_us),
                   (unsigned long)v_us, speed_kb_per_s(size, v_us), (unsigned long)crc);
            stats_print(&g_stats);
            if (g_cfg.auto_xip_restore) {
                if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                    printf("[SIM] XIP restored (cfg)\r\n");
                } else {
                    printf("[SIM] XIP restore failed (cfg)\r\n");
                }
            }
            continue;
        }
        if (strncmp(line, "erase_plan", 10) == 0) {
            unsigned long b=0,l=0; if (sscanf(line, "erase_plan %lx %lu", &b, &l) == 2) {
                erase_plan_preview((uint32_t)b, (uint32_t)l);
            } else printf("[SIM] usage: erase_plan 0xADDR SIZE\r\n");
            continue;
        }
        if (strncmp(line, "erase_region", 12) == 0) {
            unsigned long b=0,l=0; if (sscanf(line, "erase_region %lx %lu", &b, &l) == 2) {
                stats_init(&g_stats);
                erase_plan_preview((uint32_t)b, (uint32_t)l);
                uint32_t e_us=0; int rc1 = erase_region_mixed((uint32_t)b, (uint32_t)l, &e_us);
                if (rc1 != 0) {
                    printf("[SIM] ERASE failed rc=%d\r\n", rc1);
                } else {
                    printf("[SIM] ERASE: total %lu us (%.2f KB/s)\r\n", (unsigned long)e_us, speed_kb_per_s((uint32_t)l, e_us));
                    stats_print(&g_stats);
                    if (g_cfg.auto_xip_restore_eraseonly) {
                        if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                            printf("[SIM] XIP restored (cfg)\r\n");
                        } else {
                            printf("[SIM] XIP restore failed (cfg)\r\n");
                        }
                    }
                }
            } else printf("[SIM] usage: erase_region 0xADDR SIZE\r\n");
            continue;
        }
        if (strncmp(line, "write_image", 11) == 0) {
            unsigned long b=0,l=0; if (sscanf(line, "write_image %lx %lu", &b, &l) == 2) {
                printf("[SIM] write_image: addr=0x%08lX size=%lu\r\n", b, l);
                // Stream receive and program in chunks
                uint32_t addr = (uint32_t)b; uint32_t remaining = (uint32_t)l;
                if (g_flash_size && (addr + remaining > g_flash_size)) { printf("ERROR out of range: flash size=%lu\r\n", (unsigned long)g_flash_size); continue; }
                uint32_t chunk = g_cfg.chunk_kb * 1024u; if (chunk == 0) chunk = 1024u;
                printf("READY\r\n");
                stats_init(&g_stats);
                uint8_t *buf = (uint8_t *)malloc(chunk);
                if (!buf) { printf("[SIM] alloc failed\r\n"); continue; }
                uint32_t total_prog_us = 0u; uint32_t total_bytes = 0u;
                while (remaining > 0) {
                    uint32_t this_chunk = (remaining > chunk) ? chunk : remaining;
                    uart_read_exact(buf, this_chunk);
                    uint32_t pu=0; int rc1 = program_region_chunked(addr, buf, this_chunk, chunk, &pu);
                    if (rc1 != 0) { printf("[SIM] program failed rc=%d\r\n", rc1); break; }
                    total_prog_us += pu; total_bytes += this_chunk; addr += this_chunk; remaining -= this_chunk;
                }
                free(buf);
                printf("[SIM] WRITE: bytes=%lu time=%lu us speed=%.2f KB/s\r\n", (unsigned long)total_bytes, (unsigned long)total_prog_us, speed_kb_per_s(total_bytes, total_prog_us));
                stats_print(&g_stats);
                if (g_cfg.auto_xip_restore) {
                    if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                        printf("[SIM] XIP restored (cfg)\r\n");
                    } else {
                        printf("[SIM] XIP restore failed (cfg)\r\n");
                    }
                }
                printf("DONE\r\n");
            } else printf("[SIM] usage: write_image 0xADDR SIZE\r\n");
            continue;
        }
        if (strncmp(line, "verify_image", 12) == 0) {
            unsigned long b=0,l=0; if (sscanf(line, "verify_image %lx %lu", &b, &l) == 2) {
                uint32_t addr=(uint32_t)b; uint32_t size=(uint32_t)l;
                printf("[SIM] verify_image: addr=0x%08lX size=%lu\r\n", (unsigned long)addr, (unsigned long)size);
                if (g_flash_size && (addr + size > g_flash_size)) { printf("ERROR out of range: flash size=%lu\r\n", (unsigned long)g_flash_size); continue; }
                stats_init(&g_stats);
                uint8_t tmp[256]; uint32_t off=0; uint32_t total_us=0; uint32_t crc=0xFFFFFFFFu;
                while (off < size) {
                    uint32_t rsz = (size - off > sizeof(tmp)) ? sizeof(tmp) : (size - off);
                    uint32_t t0 = systick_get_ticks();
                    int rc1 = try_read(addr + off, tmp, rsz);
                    uint32_t t1 = systick_get_ticks();
                    uint32_t us = systick_elapsed_us(t0, t1);
                    total_us += us; g_stats.cnt_vchunk++; stats_update(&g_stats.us_vchunk_min, &g_stats.us_vchunk_max, &g_stats.us_vchunk_sum, us);
                    if (rc1 != 0) { printf("[SIM] read failed rc=%d\r\n", rc1); break; }
                    for (uint32_t i=0;i<rsz;++i) {
                        crc ^= tmp[i];
                        for (unsigned b8=0;b8<8;++b8) { uint32_t m=-(crc & 1u); crc=(crc>>1)^(0xEDB88320u & m);} }
                    off += rsz;
                }
                crc ^= 0xFFFFFFFFu;
                printf("[SIM] VERIFY: size=%lu time=%lu us speed=%.2f KB/s CRC32=%08lX\r\n",
                       (unsigned long)size, (unsigned long)total_us, speed_kb_per_s(size, total_us), (unsigned long)crc);
                stats_print(&g_stats);
                if (g_cfg.auto_xip_restore) {
                    if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                        printf("[SIM] XIP restored (cfg)\r\n");
                    } else {
                        printf("[SIM] XIP restore failed (cfg)\r\n");
                    }
                }
                printf("DONE\r\n");
            } else printf("[SIM] usage: verify_image 0xADDR SIZE\r\n");
            continue;
        }
        if (strncmp(line, "flash_image", 11) == 0) {
            unsigned long b=0,l=0; if (sscanf(line, "flash_image %lx %lu", &b, &l) == 2) {
                uint32_t addr = (uint32_t)b; uint32_t size = (uint32_t)l;
                if (g_flash_size && (addr + size > g_flash_size)) { printf("[SIM] out of range: flash size=%lu\r\n", (unsigned long)g_flash_size); continue; }
                printf("[SIM] flash_image: addr=0x%08lX size=%lu chunk=%luKB\r\n", (unsigned long)addr, (unsigned long)size, (unsigned long)g_cfg.chunk_kb);
                erase_plan_preview(addr, size);
                stats_init(&g_stats);
                // 1) Erase
                uint32_t e_us=0, p_us_total=0, v_us=0; uint32_t crc_w = 0xFFFFFFFFu, crc_r = 0u;
                int rc1 = erase_region_mixed(addr, size, &e_us);
                if (rc1 != 0) { printf("[SIM] erase failed rc=%d\r\n", rc1); continue; }
                // 2) Receive and program
                uint32_t remaining = size; uint32_t cur = addr; uint32_t chunk = g_cfg.chunk_kb * 1024u; if (chunk == 0) chunk = 4096u;
                uint8_t *buf = (uint8_t *)malloc(chunk);
                if (!buf) { printf("[SIM] alloc failed\r\n"); continue; }
                printf("READY\r\n");
                while (remaining > 0) {
                    uint32_t this_chunk = (remaining > chunk) ? chunk : remaining;
                    uart_read_exact(buf, this_chunk);
                    // update write-side CRC
                    for (uint32_t i = 0; i < this_chunk; ++i) {
                        crc_w ^= buf[i];
                        for (unsigned b8 = 0; b8 < 8; ++b8) { uint32_t m = -(crc_w & 1u); crc_w = (crc_w >> 1) ^ (0xEDB88320u & m); }
                    }
                    uint32_t pu=0; int prc = program_region_chunked(cur, buf, this_chunk, chunk, &pu);
                    if (prc != 0) { printf("[SIM] program failed rc=%d\r\n", prc); free(buf); break; }
                    p_us_total += pu; cur += this_chunk; remaining -= this_chunk;
                }
                free(buf);
                crc_w ^= 0xFFFFFFFFu;
                if (remaining != 0) { printf("[SIM] FLASH aborted.\r\n"); continue; }
                // 3) Verify by CRC of flash region
                int vrc = crc_region(addr, size, chunk, &v_us, &crc_r);
                if (vrc != 0) { printf("[SIM] verify(read) failed rc=%d\r\n", vrc); continue; }
                printf("[SIM] FLASH: erase %lu us (%.2f KB/s), prog %lu us (%.2f KB/s), verify %lu us (%.2f KB/s)\r\n",
                       (unsigned long)e_us, speed_kb_per_s(size, e_us),
                       (unsigned long)p_us_total, speed_kb_per_s(size, p_us_total),
                       (unsigned long)v_us, speed_kb_per_s(size, v_us));
                printf("[SIM] CRC: host=%08lX flash=%08lX %s\r\n",
                       (unsigned long)crc_w, (unsigned long)crc_r, (crc_w==crc_r)?"MATCH":"MISMATCH");
                uint32_t total_us_all = e_us + p_us_total + v_us;
                printf("[SIM] OVERALL: total=%lu us overall speed=%.2f KB/s\r\n", (unsigned long)total_us_all, speed_kb_per_s(size, total_us_all));
                stats_print(&g_stats);
                if (g_cfg.auto_xip_restore) {
                    if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                        printf("[SIM] XIP restored (cfg)\r\n");
                    } else {
                        printf("[SIM] XIP restore failed (cfg)\r\n");
                    }
                }
                printf("%s\r\n", (crc_w==crc_r)?"DONE":"ERROR");
            } else printf("[SIM] usage: flash_image 0xADDR SIZE\r\n");
            continue;
        }
        if (strcmp(line, "stats") == 0) { stats_print(&g_stats); continue; }
        if (strcmp(line, "stats reset") == 0) { stats_init(&g_stats); printf("OK\r\n"); continue; }
        if (strcmp(line, "version") == 0 || strcmp(line, "id") == 0) {
            printf("[SIM] FW 0.1.0; JEDEC: manuf=0x%02X type=0x%02X cap=0x%02X size=%lu; QE=%u 4B=%u\r\n",
                   g_info.manuf_id, g_info.memory_type, g_info.capacity, (unsigned long)g_info.size_bytes,
                   g_info.quad_enabled?1u:0u, g_info.addr4b?1u:0u);
            continue;
        }
        if (strncmp(line, "run ", 4) == 0) {
            unsigned long l=0; if (sscanf(line, "run %lu", &l) == 1) {
                uint32_t size = (uint32_t)l; uint32_t addr = g_cfg.base;
                if (size == 0u) { printf("[SIM] size must be > 0\r\n"); continue; }
                if (g_flash_size && (addr + size > g_flash_size)) { printf("[SIM] out of range: flash size=%lu\r\n", (unsigned long)g_flash_size); continue; }
                printf("[SIM] RUN GEN: base=0x%08lX len=%lu chunk=%luKB\r\n", (unsigned long)addr, (unsigned long)size, (unsigned long)g_cfg.chunk_kb);
                stats_init(&g_stats);
                erase_plan_preview(addr, size);
                uint32_t e_us=0, p_us=0, v_us=0, crc=0;
                int rc1 = erase_region_mixed(addr, size, &e_us);
                if (rc1==0) rc1 = program_region_generated(addr, size, g_cfg.chunk_kb*1024u, &p_us);
                if (rc1==0) rc1 = verify_region_generated(addr, size, g_cfg.chunk_kb*1024u, &v_us, &crc);
                printf("[SIM] RUN: erase %lu us (%.2f KB/s), prog %lu us (%.2f KB/s), verify %lu us (%.2f KB/s), CRC=%08lX\r\n",
                       (unsigned long)e_us, speed_kb_per_s(size, e_us),
                       (unsigned long)p_us, speed_kb_per_s(size, p_us),
                       (unsigned long)v_us, speed_kb_per_s(size, v_us), (unsigned long)crc);
                uint32_t total_us_all = e_us + p_us + v_us;
                printf("[SIM] OVERALL: total=%lu us overall speed=%.2f KB/s\r\n", (unsigned long)total_us_all, speed_kb_per_s(size, total_us_all));
                stats_print(&g_stats);
                if (g_cfg.auto_xip_restore) {
                    if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                        printf("[SIM] XIP restored (cfg)\r\n");
                    } else {
                        printf("[SIM] XIP restore failed (cfg)\r\n");
                    }
                }
                continue;
            }
        }
        if (strcmp(line, "run") == 0) {
            // regenerate image
            static uint8_t image2[16u * 1024u];
            uint32_t img_len = (g_cfg.length <= sizeof(image2)) ? g_cfg.length : (uint32_t)sizeof(image2);
            for (uint32_t i=0;i<img_len;++i) image2[i] = (uint8_t)((i ^ 0x5Au) + (g_cfg.base >> 12));
            if (g_flash_size && (g_cfg.base + img_len > g_flash_size)) { printf("[SIM] out of range: base=0x%08lX len=%lu flash=%lu\r\n", (unsigned long)g_cfg.base, (unsigned long)img_len, (unsigned long)g_flash_size); continue; }
            printf("[SIM] RUN: base=0x%08lX len=%lu chunk=%luKB\r\n", (unsigned long)g_cfg.base, (unsigned long)img_len, (unsigned long)g_cfg.chunk_kb);
            stats_init(&g_stats);
            erase_plan_preview(g_cfg.base, img_len);
            uint32_t e_us=0,p_us=0,v_us=0;
            int rc1 = erase_region_mixed(g_cfg.base, img_len, &e_us);
            if (rc1==0) rc1 = program_region_chunked(g_cfg.base, image2, img_len, g_cfg.chunk_kb*1024u, &p_us);
            if (rc1==0) rc1 = verify_region_chunked(g_cfg.base, image2, img_len, g_cfg.chunk_kb*1024u, &v_us);
            printf("[SIM] RUN: erase %lu us (%.2f KB/s), prog %lu us (%.2f KB/s), verify %lu us (%.2f KB/s)\r\n",
                   (unsigned long)e_us, speed_kb_per_s(img_len, e_us),
                   (unsigned long)p_us, speed_kb_per_s(img_len, p_us),
                   (unsigned long)v_us, speed_kb_per_s(img_len, v_us));
            uint32_t total_us_all = e_us + p_us + v_us;
            printf("[SIM] OVERALL: total=%lu us overall speed=%.2f KB/s\r\n", (unsigned long)total_us_all, speed_kb_per_s(img_len, total_us_all));
            stats_print(&g_stats);
            if (g_cfg.auto_xip_restore) {
                if (w25qxx_enter_xip_144(g_cfg.xip_dummy, g_cfg.xip_mode_bits) == 0) {
                    printf("[SIM] XIP restored (cfg)\r\n");
                } else {
                    printf("[SIM] XIP restore failed (cfg)\r\n");
                }
            }
            continue;
        }
        printf("[SIM] unknown command: %s\r\n", line);
    }
}
