/*
 * This header file includes all of the fix point signal processing library
 * (SPL) function descriptions and declarations. For specific function calls,
 * see bottom of file.
 */

#ifndef COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_
#define COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_

#include <stdint.h>
#include <string.h>

#ifdef __has_attribute
    #if __has_attribute(no_sanitize)
        #define RTC_NO_SANITIZE(what) __attribute__((no_sanitize(what)))
    #endif
#endif
#ifndef RTC_NO_SANITIZE
    #define RTC_NO_SANITIZE(what)
#endif

// Macros specific for the fixed point implementation
#define WEBRTC_SPL_WORD16_MAX 32767
#define WEBRTC_SPL_WORD16_MIN -32768
#define WEBRTC_SPL_WORD32_MAX (int32_t)0x7fffffff
#define WEBRTC_SPL_WORD32_MIN (int32_t)0x80000000
#define WEBRTC_SPL_MAX_LPC_ORDER 14
#define WEBRTC_SPL_MIN(A, B) (A < B ? A : B)  // Get min value
#define WEBRTC_SPL_MAX(A, B) (A > B ? A : B)  // Get max value
#define WEBRTC_SPL_ABS_W16(a) (((int16_t)a >= 0) ? ((int16_t)a) : -((int16_t)a))
#define WEBRTC_SPL_ABS_W32(a) (((int32_t)a >= 0) ? ((int32_t)a) : -((int32_t)a))
#define WEBRTC_SPL_MUL(a, b) ((int32_t)((int32_t)(a) * (int32_t)(b)))
#define WEBRTC_SPL_UMUL(a, b) ((uint32_t)((uint32_t)(a) * (uint32_t)(b)))
#define WEBRTC_SPL_UMUL_32_16(a, b) ((uint32_t)((uint32_t)(a) * (uint16_t)(b)))
#define WEBRTC_SPL_MUL_16_U16(a, b) ((int32_t)(int16_t)(a) * (uint16_t)(b))
#define WEBRTC_SPL_MUL_16_16_RSFT(a, b, c) (WEBRTC_SPL_MUL_16_16(a, b) >> (c))
#define WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(a, b, c) \
  ((WEBRTC_SPL_MUL_16_16(a, b) + ((int32_t)(((int32_t)1) << ((c)-1)))) >> (c))

// C + the 32 most significant bits of A * B
#define WEBRTC_SPL_SCALEDIFF32(A, B, C) \
  (C + (B >> 16) * A + (((uint32_t)(B & 0x0000FFFF) * A) >> 16))

#define WEBRTC_SPL_SAT(a, b, c) (b > a ? a : b < c ? c : b)

// Shifting with negative numbers allowed
// Positive means left shift
#define WEBRTC_SPL_SHIFT_W32(x, c) ((c) >= 0 ? (x) * (1 << (c)) : (x) >> -(c))
#define WEBRTC_SPL_LSHIFT_W32(x, c) ((x) << (c))
#define WEBRTC_SPL_RSHIFT_U32(x, c) ((uint32_t)(x) >> (c))
#define WEBRTC_SPL_RAND(a) ((int16_t)((((int16_t)a * 18816) >> 7) & 0x00007fff))
#define WEBRTC_SPL_MEMCPY_W16(v1, v2, length) \
  memcpy(v1, v2, (length) * sizeof(int16_t))

int16_t WebRtcSpl_GetScalingSquare(int16_t *in_vector, size_t in_vector_length,
                                   size_t times);

// Copy and set operations. Implementation in copy_set_operations.c.
// Descriptions at bottom of file.
void WebRtcSpl_MemSetW16(int16_t *vector, int16_t set_value,
                         size_t vector_length);
void WebRtcSpl_MemSetW32(int32_t *vector, int32_t set_value,
                         size_t vector_length);
void WebRtcSpl_MemCpyReversedOrder(int16_t *out_vector, int16_t *in_vector,
                                   size_t vector_length);
void WebRtcSpl_CopyFromEndW16(const int16_t *in_vector, size_t in_vector_length,
                              size_t samples, int16_t *out_vector);
void WebRtcSpl_ZerosArrayW16(int16_t *vector, size_t vector_length);
void WebRtcSpl_ZerosArrayW32(int32_t *vector, size_t vector_length);
// End: Copy and set operations.

// Randomization functions. Implementations collected in
// randomization_functions.c and descriptions at bottom of this file.
int16_t WebRtcSpl_RandU(uint32_t *seed);
int16_t WebRtcSpl_RandN(uint32_t *seed);
int16_t WebRtcSpl_RandUArray(int16_t *vector, int16_t vector_length,
                             uint32_t *seed);
// End: Randomization functions.

// Math functions
int32_t WebRtcSpl_Sqrt(int32_t value);

// Divisions. Implementations collected in division_operations.c and
// descriptions at bottom of this file.
uint32_t WebRtcSpl_DivU32U16(uint32_t num, uint16_t den);
int32_t WebRtcSpl_DivW32W16(int32_t num, int16_t den);
int16_t WebRtcSpl_DivW32W16ResW16(int32_t num, int16_t den);
int32_t WebRtcSpl_DivResultInQ31(int32_t num, int32_t den);
int32_t WebRtcSpl_DivW32HiLow(int32_t num, int16_t den_hi, int16_t den_low);
// End: Divisions.

int32_t WebRtcSpl_Energy(int16_t *vector, size_t vector_length,
                         int *scale_factor);

// End: Filter operations.

/*******************************************************************
 * resample_by_2.c
 *
 * Includes down and up sampling by a factor of two.
 *
 ******************************************************************/
void WebRtcSpl_DownsampleBy2(const int16_t *in, size_t len, int16_t *out,
                             int32_t *filtState);

void WebRtcSpl_UpsampleBy2(const int16_t *in, size_t len, int16_t *out,
                           int32_t *filtState);

/*******************************************************************
 * dot_product_with_scale.c
 *
 * Calculates the dot product between two (int16_t) vectors.
 *
 ******************************************************************/
//
// Input:
//      - vector1       : Vector 1
//      - vector2       : Vector 2
//      - vector_length : Number of samples used in the dot product
//      - scaling       : The number of right bit shifts to apply on each term
//                        during calculation to avoid overflow, i.e., the
//                        output will be in Q(-|scaling|)
//
// Return value         : The dot product in Q(-scaling)
int32_t WebRtcSpl_DotProductWithScale(const int16_t *vector1,
                                      const int16_t *vector2, size_t length,
                                      int scaling);

/*******************************************************************
 * spl_sqrt.c
 *
 *
 ******************************************************************/
extern const int8_t kWebRtcSpl_CountLeadingZeros32_Table[64];

// Don't call this directly except in tests!
static __inline int WebRtcSpl_CountLeadingZeros32_NotBuiltin(uint32_t n)
{
    // Normalize n by rounding up to the nearest number that is a sequence of 0
    // bits followed by a sequence of 1 bits. This number has the same number of
    // leading zeros as the original n. There are exactly 33 such values.
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    // Multiply the modified n with a constant selected (by exhaustive search)
    // such that each of the 33 possible values of n give a product whose 6 most
    // significant bits are unique. Then look up the answer in the table.
    return kWebRtcSpl_CountLeadingZeros32_Table[(n * 0x8c0b2891) >> 26];
}

// Don't call this directly except in tests!
static __inline int WebRtcSpl_CountLeadingZeros64_NotBuiltin(uint64_t n)
{
    const int leading_zeros = n >> 32 == 0 ? 32 : 0;
    return leading_zeros + WebRtcSpl_CountLeadingZeros32_NotBuiltin(
               (uint32_t)(n >> (32 - leading_zeros)));
}

// Returns the number of leading zero bits in the argument.
static __inline int WebRtcSpl_CountLeadingZeros32(uint32_t n)
{
#ifdef __GNUC__
    return n == 0 ? 32 : __builtin_clz(n);
#else
    return WebRtcSpl_CountLeadingZeros32_NotBuiltin(n);
#endif
}

// Returns the number of leading zero bits in the argument.
static __inline int WebRtcSpl_CountLeadingZeros64(uint64_t n)
{
#ifdef __GNUC__
    return n == 0 ? 64 : __builtin_clzll(n);
#else
    return WebRtcSpl_CountLeadingZeros64_NotBuiltin(n);
#endif
}

static __inline int16_t WebRtcSpl_SatW32ToW16(int32_t value32)
{
    int16_t out16 = (int16_t)value32;
    if (value32 > 32767)
        out16 = 32767;
    else if (value32 < -32768)
        out16 = -32768;
    return out16;
}

static __inline int32_t WebRtcSpl_AddSatW32(int32_t a, int32_t b)
{
    // Do the addition in unsigned numbers, since signed overflow is undefined
    // behavior.
    const int32_t sum = (int32_t)((uint32_t)a + (uint32_t)b);
    // a + b can't overflow if a and b have different signs. If they have the
    // same sign, a + b also has the same sign iff it didn't overflow.
    if ((a < 0) == (b < 0) && (a < 0) != (sum < 0))
    {
        // The direction of the overflow is obvious from the sign of a + b.
        return sum < 0 ? INT32_MAX : INT32_MIN;
    }
    return sum;
}

static __inline int32_t WebRtcSpl_SubSatW32(int32_t a, int32_t b)
{
    // Do the subtraction in unsigned numbers, since signed overflow is undefined
    // behavior.
    const int32_t diff = (int32_t)((uint32_t)a - (uint32_t)b);
    // a - b can't overflow if a and b have the same sign. If they have different
    // signs, a - b has the same sign as a iff it didn't overflow.
    if ((a < 0) != (b < 0) && (a < 0) != (diff < 0))
    {
        // The direction of the overflow is obvious from the sign of a - b.
        return diff < 0 ? INT32_MAX : INT32_MIN;
    }
    return diff;
}

static __inline int16_t WebRtcSpl_AddSatW16(int16_t a, int16_t b)
{
    return WebRtcSpl_SatW32ToW16((int32_t)a + (int32_t)b);
}

static __inline int16_t WebRtcSpl_SubSatW16(int16_t var1, int16_t var2)
{
    return WebRtcSpl_SatW32ToW16((int32_t)var1 - (int32_t)var2);
}

static __inline int16_t WebRtcSpl_GetSizeInBits(uint32_t n)
{
    return 32 - WebRtcSpl_CountLeadingZeros32(n);
}

// Return the number of steps a can be left-shifted without overflow,
// or 0 if a == 0.
static __inline int16_t WebRtcSpl_NormW32(int32_t a)
{
    return a == 0 ? 0 : WebRtcSpl_CountLeadingZeros32(a < 0 ? ~a : a) - 1;
}

// Return the number of steps a can be left-shifted without overflow,
// or 0 if a == 0.
static __inline int16_t WebRtcSpl_NormU32(uint32_t a)
{
    return a == 0 ? 0 : WebRtcSpl_CountLeadingZeros32(a);
}

// Return the number of steps a can be left-shifted without overflow,
// or 0 if a == 0.
static __inline int16_t WebRtcSpl_NormW16(int16_t a)
{
    const int32_t a32 = a;
    return a == 0 ? 0 : WebRtcSpl_CountLeadingZeros32(a < 0 ? ~a32 : a32) - 17;
}

static __inline int32_t WebRtc_MulAccumW16(int16_t a, int16_t b, int32_t c)
{
    return (a * b + c);
}

#endif  // COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_