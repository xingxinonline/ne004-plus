#ifdef MAC
    #include <stdlib.h>
    #include <string.h>
#elif RISCV
    #include "malloc.h"
#elif ARM_MATH_DSP
    #include <stdlib.h>
    #include <string.h>
#endif
#include "mfcc.h"
#include "mfcc_core.h"
// #include "mfcc_fft.h"
#include "mfcc_win.h"

TMFCC *MfccInit(void)
{
    TMFCC *mfcc = NULL;
#ifdef MAC
    mfcc = (TMFCC *)malloc(sizeof(TMFCC));
    mfcc->mfcc_opt = (TMFCCOPT *)malloc(sizeof(TMFCCOPT));
#elif RISCV
    mfcc = (TMFCC *)mymalloc(0, sizeof(TMFCC));
    mfcc->mfcc_opt = (TMFCCOPT *)mymalloc(0, sizeof(TMFCCOPT));
#elif ARM_MATH_DSP
    mfcc = (TMFCC *)malloc(sizeof(TMFCC));
    mfcc->mfcc_opt = (TMFCCOPT *)malloc(sizeof(TMFCCOPT));
#endif
    /*mfcc option init*/
    MfccOptInit(mfcc);
    /*mfcc window init*/
    MfccWindowInit(mfcc);
    /*mfcc dct init*/
    MfccMelInit(mfcc);
    return mfcc;
}
#include <stdio.h>
void MfccStart(TMFCC *mfcc, int16_t *wave, uint32_t wav_len, BaseFloat **output,
               int16_t *row_out, int16_t *col_out)
{
    // printf("wave_len:%d\n", wav_len);
    int16_t rows_out = NumFrames((int32_t)wav_len, mfcc);
    int16_t cols_out = mfcc->mfcc_opt->num_ceps;
    // printf("rows_out:%hd\n", rows_out);
    // printf("cols_out:%hd\n", cols_out);
    if (*output == NULL)
    {
#ifdef MAC
        (*output) = (BaseFloat *)malloc(sizeof(BaseFloat) * rows_out * cols_out);
#endif
    }
    *row_out = rows_out;
    *col_out = cols_out;
#ifdef MAC
    memset(mfcc->window, 0, sizeof(int16_t) * mfcc->window_len);
#elif RISCV
    mymemset(mfcc->window, 0, sizeof(int16_t) * mfcc->window_len);
#elif ARM_MATH_DSP
    memset(mfcc->window, 0, sizeof(int16_t) * mfcc->window_len);
#endif
    for (int16_t r = 0; r < rows_out; ++r)
    {
        ExtractWindow(wave, wav_len, r, mfcc);  // 32, 16
        BaseFloat *output_row = (*output) + r * cols_out;
        ComputeMfccCore(mfcc, output_row);
    }
}

void MfccDestroy(TMFCC *mfcc)
{
    if (mfcc != NULL)
    {
        if (mfcc->mel_bank != NULL)
        {
            if (mfcc->mel_bank->bins != NULL)
            {
#ifdef MAC
                free(mfcc->mel_bank->bins);
#elif RISCV
                myfree(0, mfcc->mel_bank->bins);
#elif ARM_MATH_DSP
                free(mfcc->mel_bank->bins);
#endif
            }
            if (mfcc->mel_bank->keys != NULL)
            {
#ifdef MAC
                free(mfcc->mel_bank->keys);
#elif RISCV
                myfree(0, mfcc->mel_bank->keys);
#elif ARM_MATH_DSP
                free(mfcc->mel_bank->keys);
#endif
            }
            if (mfcc->mel_bank->sizes != NULL)
            {
#ifdef MAC
                free(mfcc->mel_bank->sizes);
#elif RISCV
                myfree(0, mfcc->mel_bank->sizes);
#elif ARM_MATH_DSP
                free(mfcc->mel_bank->sizes);
#endif
            }
#ifdef MAC
            free(mfcc->mel_bank);
#elif RISCV
            myfree(0, mfcc->mel_bank);
#elif ARM_MATH_DSP
            free(mfcc->mel_bank);
#endif
        }
        if (mfcc->window != NULL)
        {
#ifdef MAC
            free(mfcc->window);
#elif RISCV
            myfree(0, mfcc->window);
#elif ARM_MATH_DSP
            free(mfcc->window);
#endif
        }
        if (mfcc->mfcc_opt != NULL)
        {
#ifdef MAC
            free(mfcc->mfcc_opt);
#elif RISCV
            myfree(0, mfcc->mfcc_opt);
#elif ARM_MATH_DSP
            free(mfcc->mfcc_opt);
#endif
        }
        if (mfcc->dct_weights != NULL)
        {
#ifdef MAC
            free(mfcc->dct_weights);
#elif RISCV
            myfree(0, mfcc->dct_weights);
#elif ARM_MATH_DSP
            free(mfcc->dct_weights);
#endif
        }
        if (mfcc->window_weights != NULL)
        {
#ifdef MAC
            free(mfcc->window_weights);
#elif RISCV
            myfree(0, mfcc->window_weights);
#elif ARM_MATH_DSP
            free(mfcc->window_weights);
#endif
        }
        if (mfcc->mel_energies != NULL)
        {
#ifdef MAC
            free(mfcc->mel_energies);
#elif RISCV
            myfree(0, mfcc->mel_energies);
#elif ARM_MATH_DSP
            free(mfcc->mel_energies);
#endif
        }
        if (mfcc->lifter_coeffs != NULL)
        {
#ifdef MAC
            free(mfcc->lifter_coeffs);
#elif RISCV
            myfree(0, mfcc->lifter_coeffs);
#elif ARM_MATH_DSP
            free(mfcc->lifter_coeffs);
#endif
        }
        if (mfcc->fft_out != NULL)
        {
#ifdef MAC
            free(mfcc->fft_out);
#elif RISCV
            myfree(0, mfcc->fft_out);
#elif ARM_MATH_DSP
            free(mfcc->fft_out);
#endif
        }
        if (mfcc->wavform != NULL)
        {
#ifdef MAC
            free(mfcc->wavform);
#elif RISCV
            myfree(0, mfcc->wavform);
#elif ARM_MATH_DSP
            free(mfcc->wavform);
#endif
        }
#ifdef MAC
        free(mfcc);
#elif RISCV
        myfree(0, mfcc);
#elif ARM_MATH_DSP
        free(mfcc);
#endif
    }
}
