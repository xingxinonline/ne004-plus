#ifdef MAC
    #include <stdlib.h>
    #include <string.h>
#elif RISCV
    #include "malloc.h"
#elif ARM_MATH_DSP
    #include <stdlib.h>
    #include <string.h>
#endif
// #include <math.h>
#include <stdio.h>

#include "mfcc_tools.h"
#include "mfcc_type.h"
#include "mfcc_win.h"
#ifdef RISCV
    #define RAND_MAX 2147483647
#endif
extern inline int32_t WindowSize(TMFCCOPT *opt);
extern inline int32_t WindowShift(TMFCCOPT *opt);

static uint16_t window_tab_512[512] =
{
    0,     5,     18,    36,    60,    87,    119,   155,   195,   238,   285,
    335,   389,   445,   505,   568,   633,   702,   773,   848,   925,   1004,
    1086,  1171,  1258,  1348,  1440,  1535,  1632,  1731,  1833,  1936,  2042,
    2151,  2261,  2374,  2488,  2605,  2723,  2844,  2967,  3091,  3217,  3346,
    3476,  3608,  3741,  3877,  4014,  4153,  4293,  4436,  4579,  4725,  4872,
    5020,  5170,  5322,  5475,  5629,  5785,  5942,  6100,  6260,  6421,  6583,
    6747,  6912,  7077,  7245,  7413,  7582,  7753,  7924,  8096,  8270,  8444,
    8620,  8796,  8973,  9152,  9331,  9510,  9691,  9872,  10054, 10237, 10421,
    10605, 10790, 10975, 11161, 11348, 11535, 11722, 11910, 12099, 12288, 12477,
    12667, 12857, 13048, 13239, 13430, 13621, 13813, 14004, 14196, 14388, 14581,
    14773, 14965, 15158, 15350, 15543, 15736, 15928, 16120, 16313, 16505, 16697,
    16889, 17081, 17272, 17463, 17654, 17845, 18036, 18226, 18415, 18605, 18794,
    18982, 19170, 19358, 19545, 19731, 19917, 20103, 20288, 20472, 20656, 20839,
    21021, 21203, 21383, 21563, 21743, 21921, 22099, 22276, 22452, 22627, 22802,
    22975, 23147, 23319, 23489, 23659, 23827, 23995, 24161, 24326, 24490, 24653,
    24815, 24976, 25135, 25294, 25451, 25607, 25761, 25915, 26067, 26218, 26367,
    26515, 26662, 26807, 26951, 27093, 27234, 27374, 27512, 27649, 27784, 27918,
    28050, 28180, 28309, 28437, 28562, 28687, 28809, 28930, 29049, 29167, 29283,
    29397, 29510, 29620, 29729, 29837, 29942, 30046, 30148, 30248, 30347, 30443,
    30538, 30631, 30722, 30811, 30898, 30984, 31067, 31149, 31229, 31306, 31382,
    31456, 31528, 31598, 31666, 31732, 31796, 31858, 31918, 31976, 32032, 32086,
    32138, 32188, 32236, 32282, 32326, 32368, 32407, 32445, 32481, 32514, 32546,
    32575, 32602, 32627, 32651, 32672, 32690, 32707, 32722, 32735, 32745, 32754,
    32760, 32764, 32766, 32766, 32764, 32760, 32754, 32745, 32735, 32722, 32707,
    32690, 32672, 32651, 32627, 32602, 32575, 32546, 32514, 32481, 32445, 32407,
    32368, 32326, 32282, 32236, 32188, 32138, 32086, 32032, 31976, 31918, 31858,
    31796, 31732, 31666, 31598, 31528, 31456, 31382, 31306, 31229, 31149, 31067,
    30984, 30898, 30811, 30722, 30631, 30538, 30443, 30347, 30248, 30148, 30046,
    29942, 29837, 29729, 29620, 29510, 29397, 29283, 29167, 29049, 28930, 28809,
    28687, 28562, 28437, 28309, 28180, 28050, 27918, 27784, 27649, 27512, 27374,
    27234, 27093, 26951, 26807, 26662, 26515, 26367, 26218, 26067, 25915, 25761,
    25607, 25451, 25294, 25135, 24976, 24815, 24653, 24490, 24326, 24161, 23995,
    23827, 23659, 23489, 23319, 23147, 22975, 22802, 22627, 22452, 22276, 22099,
    21921, 21743, 21563, 21383, 21203, 21021, 20839, 20656, 20472, 20288, 20103,
    19917, 19731, 19545, 19358, 19170, 18982, 18794, 18605, 18415, 18226, 18036,
    17845, 17654, 17463, 17272, 17081, 16889, 16697, 16505, 16313, 16120, 15928,
    15736, 15543, 15350, 15158, 14965, 14773, 14581, 14388, 14196, 14004, 13813,
    13621, 13430, 13239, 13048, 12857, 12667, 12477, 12288, 12099, 11910, 11722,
    11535, 11348, 11161, 10975, 10790, 10605, 10421, 10237, 10054, 9872,  9691,
    9510,  9331,  9152,  8973,  8796,  8620,  8444,  8270,  8096,  7924,  7753,
    7582,  7413,  7245,  7077,  6912,  6747,  6583,  6421,  6260,  6100,  5942,
    5785,  5629,  5475,  5322,  5170,  5020,  4872,  4725,  4579,  4436,  4293,
    4153,  4014,  3877,  3741,  3608,  3476,  3346,  3217,  3091,  2967,  2844,
    2723,  2605,  2488,  2374,  2261,  2151,  2042,  1936,  1833,  1731,  1632,
    1535,  1440,  1348,  1258,  1171,  1086,  1004,  925,   848,   773,   702,
    633,   568,   505,   445,   389,   335,   285,   238,   195,   155,   119,
    87,    60,    36,    18,    5,     0
};
int32_t FirstSampleOfFrame(int16_t frame, TMFCCOPT *opts)
{
    int32_t frame_shift = WindowShift(opts);
    if (opts->snip_edges)
    {
        return frame * frame_shift;
    }
    else
    {
        int32_t midpoint_of_frame = frame_shift * frame + frame_shift / 2,
                beginning_of_frame = midpoint_of_frame - WindowSize(opts) / 2;
        return beginning_of_frame;
    }
}

void preemphasize(int16_t *waveform, int32_t wave_len, uint8_t preemph_coeff)
{
    for (int32_t i = wave_len - 1; i > 0; i--)
    {
        waveform[i] = waveform[i] - ((preemph_coeff * waveform[i - 1]) >> 8);
    }
    waveform[0] = waveform[0] - ((preemph_coeff * waveform[0]) >> 8);
}

void ProcessWindow(TMFCC *mfcc, int16_t *window, int16_t frame_length)
{
    int32_t means = Sum_Int16(window, frame_length) / frame_length;
    // printf("means: %d\tframe_length: %d\n", means, frame_length);
    Add_Int16(window, frame_length, -means);
    preemphasize(window, frame_length, mfcc->mfcc_opt->preemph_coeff);
    Multiply_Int16_ShiftRight(window, mfcc->window_weights, mfcc->window_len, 15);
}

void ExtractWindow(int16_t *wave, uint32_t wave_len, int16_t f, TMFCC *mfcc)
{
    int16_t sample_offset = 0;
    int16_t frame_length = WindowSize(mfcc->mfcc_opt);
    int16_t frame_length_padded =
        PaddedWindowSize(frame_length);  // 默认到补0到2的指数
    int32_t start_sample = FirstSampleOfFrame(f, mfcc->mfcc_opt);
    int16_t *window = mfcc->window;
    int32_t wave_start = start_sample - sample_offset;
    uint32_t wave_end = wave_start + frame_length;
    if (wave_start >= 0 && wave_end <= wave_len)
    {
        for (int16_t i = 0; i < frame_length; ++i)
        {
            window[i] = wave[wave_start + i];
        }
    }
    else
    {
        for (int32_t s = 0; s < frame_length; s++)
        {
            int32_t s_in_wave = s + wave_start;
            while (s_in_wave < 0 || s_in_wave >= (int32_t)wave_len)
            {
                if (s_in_wave < 0)
                {
                    s_in_wave = -s_in_wave - 1;
                }
                else
                {
                    s_in_wave = 2 * wave_len - 1 - s_in_wave;
                }
            }
            window[s] = wave[s_in_wave];
        }
    }
    if (frame_length_padded > frame_length)
    {
#ifdef MAC
        memset(window + frame_length, 0,
               sizeof(int16_t) * (frame_length_padded - frame_length));
#elif RISCV
        mymemset(window + frame_length, 0,
                 sizeof(int16_t) * (frame_length_padded - frame_length));
#elif ARM_MATH_DSP
        memset(window + frame_length, 0,
               sizeof(int16_t) * (frame_length_padded - frame_length));
#endif
    }
    ProcessWindow(mfcc, window, frame_length);
}
#include <stdio.h>
int16_t NumFrames(int32_t num_samples, TMFCC *mfcc)
{
    int32_t frame_shift = WindowShift(mfcc->mfcc_opt);
    int32_t frame_length = WindowSize(mfcc->mfcc_opt);
    // printf("frame_length: %d\n", frame_length);
    // printf("frame_shift: %d\n", frame_shift);
    if (mfcc->mfcc_opt->snip_edges)
    {
        if (num_samples < frame_length)
            return 0;
        else
            return (1 + ((num_samples - frame_length) / frame_shift));
    }
    else
    {
        int16_t num_frames = (num_samples + (frame_shift / 2)) / frame_shift;
        return num_frames;
    }
}

void MfccWindowInit(TMFCC *mfcc)
{
    TMFCCOPT *opts = mfcc->mfcc_opt;
    int32_t frame_length = PaddedWindowSize(WindowSize(opts));
#ifdef MAC
    mfcc->window_weights = (uint16_t *)malloc(sizeof(uint16_t) * frame_length);
#elif RISCV
    mfcc->window_weights =
        (uint16_t *)mymalloc(0, sizeof(uint16_t) * frame_length);
#elif ARM_MATH_DSP
    mfcc->window_weights = (uint16_t *)malloc(sizeof(uint16_t) * frame_length);
#endif
#ifdef MAC
    memcpy(mfcc->window_weights, window_tab_512, sizeof(uint16_t) * frame_length);
#elif RISCV
    mymemcpy(mfcc->window_weights, (void *)window_tab_512,
             sizeof(uint16_t) * frame_length);
#elif ARM_MATH_DSP
    memcpy(mfcc->window_weights, window_tab_512, sizeof(uint16_t) * frame_length);
#endif
}
