// Clean, full DSP test suite
#include "s300.h"
#include "s300_bsp.h"
#include "s300_uart.h"
#include <stdint.h>

static void u32_to_hex(uint32_t v, char *out)
{
    static const char hc[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) out[7 - i] = hc[(v >> (i * 4)) & 0xF];
    out[8] = 0;
}

static void print_hex32(const char *prefix, uint32_t v)
{
    char h[9];
    u32_to_hex(v, h);
    if (prefix) S300_UART_PutStringI(BOARD_UART_DEBUG_ID, prefix);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
}

static inline int32_t ref_smlad(int32_t a, int32_t b, int32_t acc)
{
    int16_t a_lo = (int16_t)(a & 0xFFFF);
    int16_t a_hi = (int16_t)((uint32_t)a >> 16);
    int16_t b_lo = (int16_t)(b & 0xFFFF);
    int16_t b_hi = (int16_t)((uint32_t)b >> 16);
    return (int32_t)a_lo * b_lo + (int32_t)a_hi * b_hi + acc;
}

// Helpers
static inline int32_t sat_s32(int64_t v)
{
    if (v > 0x7FFFFFFFLL) return 0x7FFFFFFF;
    if (v < (int64_t)0x80000000LL) return (int32_t)0x80000000;
    return (int32_t)v;
}
static inline int16_t sat_s16(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}
static inline int8_t sat_s8(int32_t v)
{
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}
static inline uint32_t pack16(int16_t lo, int16_t hi)
{
    return ((uint32_t)(uint16_t)lo) | (((uint32_t)(uint16_t)hi) << 16);
}
static inline int16_t hi16(int32_t x)
{
    return (int16_t)((uint32_t)x >> 16);
}
static inline int16_t lo16(int32_t x)
{
    return (int16_t)(x & 0xFFFF);
}
static inline int8_t  byte0(int32_t x)
{
    return (int8_t)(x & 0xFF);
}
static inline int8_t  byte1(int32_t x)
{
    return (int8_t)((x >> 8) & 0xFF);
}
static inline int8_t  byte2(int32_t x)
{
    return (int8_t)((x >> 16) & 0xFF);
}
static inline int8_t  byte3(int32_t x)
{
    return (int8_t)((x >> 24) & 0xFF);
}

static int s_pass = 0, s_fail = 0;
static void assert_eq(const char *name, uint32_t got, uint32_t exp)
{
    char h[9];
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, name);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ": got=0x");
    u32_to_hex(got, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", exp=0x");
    u32_to_hex(exp, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    if (got == exp)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, " PASS\n");
        ++s_pass;
    }
    else
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, " FAIL\n");
        ++s_fail;
    }
}

static void dsp_full_suite(void)
{
    // Header
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_Test: CPUID=0x");
    print_hex32(NULL, SCB->CPUID);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
#if defined(__ARM_FEATURE_DSP)
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_Test: __ARM_FEATURE_DSP defined\n");
#else
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_Test: __ARM_FEATURE_DSP NOT defined\n");
#endif
    // Samples
    int32_t A = 0x00030002; // hi=3, lo=2
    int32_t B = 0x00050004; // hi=5, lo=4
    int32_t ACC = 7;
    // Dual MAC family
    assert_eq("DSP_Test: SMLAD",  __SMLAD((uint32_t)A, (uint32_t)B, (uint32_t)ACC), (uint32_t)ref_smlad(A, B, ACC));
    {
        uint32_t exp = (uint32_t)((int32_t)lo16(A) * hi16(B) + (int32_t)hi16(A) * lo16(B) + ACC);
        assert_eq("DSP_Test: SMLADX", __SMLADX((uint32_t)A, (uint32_t)B, (uint32_t)ACC), exp);
    }
    assert_eq("DSP_Test: SMUAD",  __SMUAD((uint32_t)A, (uint32_t)B), (uint32_t)ref_smlad(A, B, 0));
    {
        uint32_t exp = (uint32_t)((int32_t)lo16(A) * hi16(B) + (int32_t)hi16(A) * lo16(B));
        assert_eq("DSP_Test: SMUADX", __SMUADX((uint32_t)A, (uint32_t)B), exp);
    }
    {
        uint32_t exp = (uint32_t)((int32_t)lo16(A) * lo16(B) - (int32_t)hi16(A) * hi16(B));
        assert_eq("DSP_Test: SMUSD", __SMUSD((uint32_t)A, (uint32_t)B), exp);
        exp = (uint32_t)((int32_t)lo16(A) * hi16(B) - (int32_t)hi16(A) * lo16(B));
        assert_eq("DSP_Test: SMUSDX", __SMUSDX((uint32_t)A, (uint32_t)B), exp);
    }
    {
        uint32_t exp = (uint32_t)((int32_t)lo16(A) * lo16(B) - (int32_t)hi16(A) * hi16(B) + ACC);
        assert_eq("DSP_Test: SMLSD", __SMLSD((uint32_t)A, (uint32_t)B, (uint32_t)ACC), exp);
        exp = (uint32_t)((int32_t)lo16(A) * hi16(B) - (int32_t)hi16(A) * lo16(B) + ACC);
        assert_eq("DSP_Test: SMLSDX", __SMLSDX((uint32_t)A, (uint32_t)B, (uint32_t)ACC), exp);
    }
    {
        // 64-bit accumulate
        uint64_t acc64 = 5;
        uint64_t got64 = __SMLALD((uint32_t)A, (uint32_t)B, acc64);
        int64_t  exp64 = (int64_t)ref_smlad(A, B, 0) + (int64_t)acc64;
        assert_eq("DSP_Test: SMLALD.lo", (uint32_t)(got64 & 0xFFFFFFFFu), (uint32_t)exp64);
        assert_eq("DSP_Test: SMLALD.hi", (uint32_t)(got64 >> 32), (uint32_t)((uint64_t)exp64 >> 32));
    }
    // Scalar saturating add/sub
    assert_eq("DSP_Test: QADD", (uint32_t)__QADD(0x7FFFFFF0, 0x00000100), (uint32_t)sat_s32(0x7FFFFFF0LL + 0x100));
    assert_eq("DSP_Test: QSUB", (uint32_t)__QSUB(0x80000010, 0x00000100), (uint32_t)sat_s32(0x80000010LL - 0x100));
    // Parallel halfword and byte ops
    {
        int32_t op1 = 0x7FFF8001; // hi=0x7FFF, lo=0x8001(-32767)
        int32_t op2 = 0x00018000; // hi=0x0001, lo=0x8000(-32768)
        uint32_t exp = pack16((int16_t)(lo16(op1) + lo16(op2)), (int16_t)(hi16(op1) + hi16(op2)));
        assert_eq("DSP_Test: SADD16", __SADD16((uint32_t)op1, (uint32_t)op2), exp);
        exp = pack16(sat_s16((int32_t)lo16(op1) + lo16(op2)), sat_s16((int32_t)hi16(op1) + hi16(op2)));
        assert_eq("DSP_Test: QADD16", __QADD16((uint32_t)op1, (uint32_t)op2), exp);
        exp = pack16(sat_s16((int32_t)lo16(op1) - lo16(op2)), sat_s16((int32_t)hi16(op1) - hi16(op2)));
        assert_eq("DSP_Test: QSUB16", __QSUB16((uint32_t)op1, (uint32_t)op2), exp);
    }
    {
        int32_t op1 = 0x7F800180; // bytes: 80 01 80 7F
        int32_t op2 = 0x01800180; // bytes: 80 01 80 01
        uint32_t exp = (uint32_t)(uint8_t)((int8_t)(byte0(op1) + byte0(op2)))
                       | ((uint32_t)(uint8_t)((int8_t)(byte1(op1) + byte1(op2))) << 8)
                       | ((uint32_t)(uint8_t)((int8_t)(byte2(op1) + byte2(op2))) << 16)
                       | ((uint32_t)(uint8_t)((int8_t)(byte3(op1) + byte3(op2))) << 24);
        assert_eq("DSP_Test: SADD8", __SADD8((uint32_t)op1, (uint32_t)op2), exp);
        exp = (uint32_t)(uint8_t)sat_s8((int32_t)byte0(op1) + byte0(op2))
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte1(op1) + byte1(op2)) << 8)
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte2(op1) + byte2(op2)) << 16)
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte3(op1) + byte3(op2)) << 24);
        assert_eq("DSP_Test: QADD8", __QADD8((uint32_t)op1, (uint32_t)op2), exp);
        exp = (uint32_t)(uint8_t)sat_s8((int32_t)byte0(op1) - byte0(op2))
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte1(op1) - byte1(op2)) << 8)
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte2(op1) - byte2(op2)) << 16)
              | ((uint32_t)(uint8_t)sat_s8((int32_t)byte3(op1) - byte3(op2)) << 24);
        assert_eq("DSP_Test: QSUB8", __QSUB8((uint32_t)op1, (uint32_t)op2), exp);
    }
    {
        int32_t op1 = 0x00030002, op2 = 0x00050004;
        // SASX: top = op1[31:16] + op2[15:0], bottom = op1[15:0] - op2[31:16]
        uint32_t exp = pack16((int16_t)(lo16(op1) - hi16(op2)), (int16_t)(hi16(op1) + lo16(op2)));
        assert_eq("DSP_Test: SASX", __SASX((uint32_t)op1, (uint32_t)op2), exp);
        // SSAX: top = op1[31:16] - op2[15:0], bottom = op1[15:0] + op2[31:16]
        exp = pack16((int16_t)(hi16(op2) + lo16(op1)), (int16_t)(hi16(op1) - lo16(op2)));
        assert_eq("DSP_Test: SSAX", __SSAX((uint32_t)op1, (uint32_t)op2), exp);
    }
    // Packing helpers
    {
        uint32_t a = 0xAAAA5555, b = 0x12345678;
        int sh = 1;
        uint32_t exp = (a & 0x0000FFFFu) | (((b << sh) & 0xFFFF0000u));
        assert_eq("DSP_Test: PKHBT", __PKHBT(a, b, sh), exp);
        // PKHTB: top = a[31:16], bottom = (b ASR sh)[15:0]
        int32_t b_asr = ((int32_t)b) >> sh;
        exp = (a & 0xFFFF0000u) | ((uint32_t)b_asr & 0x0000FFFFu);
        assert_eq("DSP_Test: PKHTB", __PKHTB(a, b, sh), exp);
    }
    // Saturate
    {
        int32_t v = 0x12345678;
        int32_t got = __SSAT(v, 12); // [-2048, 2047]
        int32_t exp = v;
        if (v > ((1 << 11) - 1)) exp = ((1 << 11) - 1);
        else if (v < - (1 << 11))  exp = - (1 << 11);
        assert_eq("DSP_Test: SSAT", (uint32_t)got, (uint32_t)exp);
        uint32_t gotu = __USAT(v, 10); // [0,1023]
        uint32_t expu = (v < 0) ? 0u : (v > 1023 ? 1023u : (uint32_t)v);
        assert_eq("DSP_Test: USAT", gotu, expu);
    }
    // USAD8
    {
        uint32_t op1 = 0x01020304, op2 = 0x04030201;
        uint32_t got = __USAD8(op1, op2);
        uint32_t exp = (uint32_t)(
                           (uint32_t)((byte0(op1) > byte0(op2)) ? (byte0(op1) - byte0(op2)) : (byte0(op2) - byte0(op1))) +
                           (uint32_t)((byte1(op1) > byte1(op2)) ? (byte1(op1) - byte1(op2)) : (byte1(op2) - byte1(op1))) +
                           (uint32_t)((byte2(op1) > byte2(op2)) ? (byte2(op1) - byte2(op2)) : (byte2(op2) - byte2(op1))) +
                           (uint32_t)((byte3(op1) > byte3(op2)) ? (byte3(op1) - byte3(op2)) : (byte3(op2) - byte3(op1)))
                       );
        assert_eq("DSP_Test: USAD8", got, exp);
    }
    // Performance: repeat SMLAD
    const int N = 10000;
    volatile uint32_t sink = 0;
    uint32_t t0 = S300_SysTick_Millis();
    for (int i = 0; i < N; ++i) sink += __SMLAD((uint32_t)A, (uint32_t)B, (uint32_t)ACC);
    uint32_t dt_ms = S300_SysTick_Millis() - t0;
    char h[9];
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_Test: N=0x");
    u32_to_hex((uint32_t)N, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", dt_ms=");
    u32_to_hex(dt_ms, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", est_cycles=");
    uint32_t est_cycles = (SystemCoreClock / 1000u) * dt_ms;
    u32_to_hex(est_cycles, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    // Summary
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP_Test: summary pass=");
    u32_to_hex((uint32_t)s_pass, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", fail=");
    u32_to_hex((uint32_t)s_fail, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, h);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
}

int main(void)
{
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    S300_SysTick_Init();
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "DSP Test Start\n");
    dsp_full_suite();
    while (1)
    {
        S300_DelayMs(1000);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ".\n");
    }
}
