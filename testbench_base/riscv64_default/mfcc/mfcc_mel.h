#ifndef __MFCC_MEL_H__
#define __MFCC_MEL_H__
#include <stdint.h>

#include "mfcc_type.h"

/*mel bank*/
typedef struct {
  uint16_t* bins;
  uint8_t* sizes;
  uint16_t* keys;
  int32_t bin_len;
} TMELBANK;

typedef struct {
  int8_t snip_edges;  // If True, end effects will be handled by outputting only
                      // frames that completely fit in the file, and the number
                      // of frames depends on the frame_length. If False, the
                      // number of frames depends only on the frame_shift, and
                      // we reflect the data at the ends. (Default: True)
  int16_t num_ceps;   // Number of cepstra in MFCC computation
  int16_t num_bins;   // e.g. 25; number of triangular bins
  uint8_t preemph_coeff;  // Coefficient for use in signal preemphasis (Default:
                          // 0.97 and only 0.97)
  BaseFloat cepstral_lifter;  //  Constant that controls scaling of MFCCs
                              //  (Default: 22.0)
  BaseFloat samp_freq;        // Waveform data sample frequency (must match the
                        // waveform file, if specified there) (Default: 16000.0)
  BaseFloat frame_shift_ms;   // Frame shift in milliseconds (Default: 10.0)
  BaseFloat frame_length_ms;  // Frame length in milliseconds (Default: 25.0)
  char* window_type;          // e.g. Hamming window
} TMFCCOPT;

typedef struct {
  TMFCCOPT* mfcc_opt; /*配置*/
  int16_t* window;
  int16_t window_len;       /*窗口长度*/
  uint16_t* window_weights; /*窗口函数加权向量*/
  int8_t* dct_weights;      /*离散余弦变换映射矩阵*/
  int16_t dct_row_len;      /*上述矩阵的行数*/
  int16_t dct_col_len;      /*上述矩阵的列数*/
  int32_t* mel_energies;    /*梅尔能量 定点*/
  int32_t mel_len;          /*上述向量的长度*/
  int16_t* lifter_coeffs;   /*MFCC Scale*/
  int16_t lifter_len;       /*上述向量的长度*/
  int32_t* wavform;         /* Wav Magnitude pre malloc */
  int32_t* fft_out;         /*FFT output pre malloc*/
  TMELBANK* mel_bank;
} TMFCC;

/*初始化MelBanks*/
void MfccMelInit(TMFCC* mfcc);

/*MFCC 配置初始化*/
void MfccOptInit(TMFCC* mfcc);

/*计算MelBank*/
void MelBankCompute(TMFCC* mfcc, int32_t* power_spectrum,
                    int32_t* mel_energies_out, int32_t* mel_energy_out_len);
#endif