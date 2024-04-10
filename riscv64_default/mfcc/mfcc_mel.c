#ifdef MAC
#include <stdlib.h>
#include <string.h>
#elif RISCV
#include "malloc.h"
#elif ARM_MATH_DSP
#include <stdlib.h>
#include <string.h>
#endif
// #include "mfcc_fft.h"
#include "mfcc_mel.h"
#include "mfcc_tools.h"
#include "mfcc_win.h"
extern int16_t FRAME_LEN;
extern int16_t STAGE;
extern int64_t* wn_sin;
extern int64_t* wn_cos;
extern inline BaseFloat MelScale(BaseFloat freq);
extern inline BaseFloat InverseMelScale(BaseFloat mel_freq);
#define BIN_NUM  448

void MfccOptInit(TMFCC* mfcc) {
  TMFCCOPT* mfcc_opt = mfcc->mfcc_opt;
  mfcc_opt->num_ceps = 10;
  mfcc_opt->cepstral_lifter = 22.0;
  mfcc_opt->num_bins = 10;
  mfcc_opt->samp_freq = 16000.0;
  mfcc_opt->frame_length_ms = 32.0;
  mfcc_opt->frame_shift_ms = 20.0;
  mfcc_opt->preemph_coeff = round(0.97 * 256);
  mfcc_opt->snip_edges = 1;
}

void MelBankCompute(TMFCC* mfcc, int32_t* power_spectrum,
                    int32_t* mel_energies_out, int32_t* mel_energy_out_len) {
  for (int32_t i = 0; i < mfcc->mel_bank->bin_len; i++) {
    int16_t offset = mfcc->mel_bank->keys[i];
    uint8_t sizes = mfcc->mel_bank->sizes[i];
    int32_t energy = InnerProduct_Int32_ShiftRight(
        mfcc->mel_bank->bins + offset, power_spectrum + offset, sizes, 10);
    mel_energies_out[i] = energy;
  }
  *mel_energy_out_len = mfcc->mel_bank->bin_len;
}

TMELBANK* CreateMelBanks(TMFCC* mfcc) {
  int32_t num_bins = mfcc->mfcc_opt->num_bins;

  uint16_t bins[BIN_NUM] = {
      35,  129, 220, 307, 391, 472, 473, 397, 324, 253, 184, 118, 53,  38,  114,
      187, 258, 327, 393, 458, 502, 441, 381, 323, 266, 211, 157, 104, 53,  2,
      9,   70,  130, 188, 245, 300, 354, 407, 458, 509, 465, 417, 370, 324, 278,
      234, 190, 148, 106, 65,  24,  46,  94,  141, 187, 233, 277, 321, 363, 405,
      446, 487, 496, 457, 419, 381, 344, 308, 272, 236, 201, 167, 133, 100, 67,
      35,  3,   15,  54,  92,  130, 167, 203, 239, 275, 310, 344, 378, 411, 444,
      476, 508, 484, 453, 422, 392, 362, 333, 304, 275, 247, 219, 191, 164, 137,
      110, 84,  58,  32,  7,   27,  58,  89,  119, 149, 178, 207, 236, 264, 292,
      320, 347, 374, 401, 427, 453, 479, 504, 494, 469, 444, 420, 396, 372, 349,
      325, 302, 279, 257, 234, 212, 190, 168, 147, 125, 104, 83,  63,  42,  22,
      1,   17,  42,  67,  91,  115, 139, 162, 186, 209, 232, 254, 277, 299, 321,
      343, 364, 386, 407, 428, 448, 469, 489, 510, 493, 474, 454, 434, 415, 396,
      377, 358, 339, 321, 302, 284, 266, 248, 230, 213, 195, 178, 160, 143, 126,
      109, 93,  76,  60,  43,  27,  11,  18,  37,  57,  77,  96,  115, 134, 153,
      172, 190, 209, 227, 245, 263, 281, 298, 316, 333, 351, 368, 385, 402, 418,
      435, 451, 468, 484, 500, 507, 491, 475, 459, 444, 428, 413, 397, 382, 367,
      352, 337, 323, 308, 293, 279, 264, 250, 236, 222, 208, 194, 180, 166, 152,
      139, 125, 112, 98,  85,  72,  59,  46,  33,  20,  7,   4,   20,  36,  52,
      67,  83,  98,  114, 129, 144, 159, 174, 188, 203, 218, 232, 247, 261, 275,
      289, 303, 317, 331, 345, 359, 372, 386, 399, 413, 426, 439, 452, 465, 478,
      491, 504, 506, 493, 481, 468, 456, 443, 431, 419, 406, 394, 382, 370, 358,
      346, 334, 323, 311, 299, 288, 276, 265, 253, 242, 231, 219, 208, 197, 186,
      175, 164, 153, 142, 131, 120, 110, 99,  88,  78,  67,  57,  46,  36,  26,
      16,  5,   5,   18,  30,  43,  55,  68,  80,  92,  105, 117, 129, 141, 153,
      165, 177, 188, 200, 212, 223, 235, 246, 258, 269, 280, 292, 303, 314, 325,
      336, 347, 358, 369, 380, 391, 401, 412, 423, 433, 444, 454, 465, 475, 485,
      495, 506, 507, 497, 487, 477, 467, 457, 447, 437, 427, 417, 408, 398, 388,
      379, 369, 360, 350, 341, 331, 322, 312, 303, 294, 285, 275, 266, 257, 248,
      239, 230, 221, 212, 203, 194, 186, 177, 168, 159, 151, 142, 133, 125, 116,
      108, 99,  91,  82,  74,  65,  57,  49,  40,  32,  24,  16,  8};
  uint8_t sizes[10] = {13, 17, 21, 26, 33, 41, 51, 64, 81, 101};
  uint16_t keys[10] = {1, 7, 14, 24, 35, 50, 68, 91, 119, 155};

#ifdef MAC
  TMELBANK* mel_bank = (TMELBANK*)malloc(sizeof(TMELBANK));
  mel_bank->bins = (uint16_t*)malloc(sizeof(uint16_t) * BIN_NUM);
  mel_bank->sizes = (uint8_t*)malloc(sizeof(uint8_t) * 10);
  mel_bank->keys = (uint16_t*)malloc(sizeof(uint16_t) * 10);
  mel_bank->bin_len = num_bins;

  memcpy(mel_bank->bins, bins, sizeof(uint16_t) * BIN_NUM);
  memcpy(mel_bank->sizes, sizes, sizeof(uint8_t) * 10);
  memcpy(mel_bank->keys, keys, sizeof(uint16_t) * 10);
#elif RISCV
  TMELBANK* mel_bank = (TMELBANK*)mymalloc(0, sizeof(TMELBANK));
  mel_bank->bins = (uint16_t*)mymalloc(0, sizeof(uint16_t) * BIN_NUM);
  mel_bank->sizes = (uint8_t*)mymalloc(0, sizeof(uint8_t) * 10);
  mel_bank->keys = (uint16_t*)mymalloc(0, sizeof(uint16_t) * 10);
  mel_bank->bin_len = num_bins;

  mymemcpy(mel_bank->bins, (void*)bins, sizeof(uint16_t) * BIN_NUM);
  mymemcpy(mel_bank->sizes, (void*)sizes, sizeof(uint8_t) * 10);
  mymemcpy(mel_bank->keys, (void*)keys, sizeof(uint16_t) * 10);
#elif ARM_MATH_DSP
  TMELBANK* mel_bank = (TMELBANK*)malloc(sizeof(TMELBANK));
  mel_bank->bins = (uint16_t*)malloc(sizeof(uint16_t) * BIN_NUM);
  mel_bank->sizes = (uint8_t*)malloc(sizeof(uint8_t) * 10);
  mel_bank->keys = (uint16_t*)malloc(sizeof(uint16_t) * 10);
  mel_bank->bin_len = num_bins;

  memcpy(mel_bank->bins, bins, sizeof(uint16_t) * BIN_NUM);
  memcpy(mel_bank->sizes, sizes, sizeof(uint8_t) * 10);
  memcpy(mel_bank->keys, keys, sizeof(uint16_t) * 10);
#endif
  return mel_bank;
}
#include <stdio.h>
void MfccMelInit(TMFCC* mfcc) {
  int8_t dct_mat_arry[100] = {
      40, 40,  40,  40,  40,  40,  40,  40,  40,  40,  56,  51,  40,  25,  8,
      -8, -25, -40, -51, -56, 54,  33,  0,   -33, -54, -54, -33, 0,   33,  54,
      51, 8,   -40, -56, -25, 25,  56,  40,  -8,  -51, 46,  -17, -57, -17, 46,
      46, -17, -57, -17, 46,  40,  -40, -40, 40,  40,  -40, -40, 40,  40,  -40,
      33, -54, 0,   54,  -33, -33, 54,  0,   -54, 33,  25,  -56, 40,  8,   -51,
      51, -8,  -40, 56,  -25, 17,  -46, 57,  -46, 17,  17,  -46, 57,  -46, 17,
      8,  -25, 40,  -51, 56,  -56, 51,  -40, 25,  -8};
  int16_t lifter_coeffs_arr[10] = {256,  656,  1049, 1425, 1778,
                                   2100, 2384, 2624, 2817, 2957};
  int32_t num_bins = mfcc->mfcc_opt->num_bins;
  int32_t num_ceps = mfcc->mfcc_opt->num_ceps;
  mfcc->mel_len = num_bins;
  mfcc->lifter_len = num_ceps;
  mfcc->dct_row_len = num_ceps;
  mfcc->dct_col_len = num_bins;
  mfcc->mel_bank = CreateMelBanks(mfcc);
  FRAME_LEN = PaddedWindowSize(WindowSize(mfcc->mfcc_opt));
//   printf("frame_len:%d\n", FRAME_LEN);
//   STAGE = GetStage(FRAME_LEN);
//   FFTInit();
//   CreateWinTable(1, wn_sin);
//   CreateWinTable(0, wn_cos);
#ifdef MAC
  mfcc->fft_out = (int32_t*)malloc(sizeof(int32_t) * (FRAME_LEN * 2));
  mfcc->wavform = (int32_t*)malloc(sizeof(int32_t) * (FRAME_LEN));
  mfcc->dct_weights = (int8_t*)malloc(sizeof(int8_t) * num_ceps * num_bins);
  mfcc->mel_energies = (int32_t*)malloc(sizeof(int32_t) * num_bins);
  mfcc->lifter_coeffs = (int16_t*)malloc(sizeof(int16_t) * num_ceps);
  mfcc->window = (int16_t*)malloc(sizeof(int16_t) * FRAME_LEN);
  mfcc->window_len = WindowSize(mfcc->mfcc_opt);
  memset(mfcc->dct_weights, 0, sizeof(int8_t) * num_ceps * num_bins);
  memcpy(mfcc->dct_weights, dct_mat_arry, sizeof(int8_t) * num_ceps * num_bins);
  memset(mfcc->mel_energies, 0, sizeof(int32_t) * num_bins);
  memcpy(mfcc->lifter_coeffs, lifter_coeffs_arr, sizeof(int16_t) * 10);
#elif RISCV
  mfcc->fft_out = (int32_t*)mymalloc(0, sizeof(int32_t) * (FRAME_LEN * 2));
  mfcc->wavform = (int32_t*)mymalloc(0, sizeof(int32_t) * (FRAME_LEN));
  mfcc->dct_weights =
      (int8_t*)mymalloc(0, sizeof(int8_t) * num_ceps * num_bins);
  mfcc->mel_energies = (int32_t*)mymalloc(0, sizeof(int32_t) * num_bins);
  mfcc->lifter_coeffs = (int16_t*)mymalloc(0, sizeof(int16_t) * num_ceps);
  mfcc->window = (int16_t*)mymalloc(0, sizeof(int16_t) * FRAME_LEN);
  mfcc->window_len = WindowSize(mfcc->mfcc_opt);
  mymemset(mfcc->dct_weights, 0, sizeof(int8_t) * num_ceps * num_bins);
  mymemcpy(mfcc->dct_weights, dct_mat_arry,
           sizeof(int8_t) * num_ceps * num_bins);
  mymemset(mfcc->mel_energies, 0, sizeof(int32_t) * num_bins);
  mymemcpy(mfcc->lifter_coeffs, lifter_coeffs_arr, sizeof(int16_t) * 10);
#elif ARM_MATH_DSP
  mfcc->fft_out = (int32_t*)malloc(sizeof(int32_t) * (FRAME_LEN * 2));
  mfcc->wavform = (int32_t*)malloc(sizeof(int32_t) * (FRAME_LEN));
  mfcc->dct_weights = (int8_t*)malloc(sizeof(int8_t) * num_ceps * num_bins);
  mfcc->mel_energies = (int32_t*)malloc(sizeof(int32_t) * num_bins);
  mfcc->lifter_coeffs = (int16_t*)malloc(sizeof(int16_t) * num_ceps);
  mfcc->window = (int16_t*)malloc(sizeof(int16_t) * FRAME_LEN);
  mfcc->window_len = WindowSize(mfcc->mfcc_opt);
  memset(mfcc->dct_weights, 0, sizeof(int8_t) * num_ceps * num_bins);
  memcpy(mfcc->dct_weights, dct_mat_arry, sizeof(int8_t) * num_ceps * num_bins);
  memset(mfcc->mel_energies, 0, sizeof(int32_t) * num_bins);
  memcpy(mfcc->lifter_coeffs, lifter_coeffs_arr, sizeof(int16_t) * 10);
#endif
}