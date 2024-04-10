#ifdef RISCV
    #include "malloc.h"
#else
    #include <stdlib.h>
    #include <string.h>
#endif
#include "vad.h"

void framing(int16_t *data, uint32_t data_len, int16_t **output, uint32_t *rows, uint32_t *cols, VAD *vad)
{
    uint32_t frame_nums = (data_len - vad->FRAME_LEN) / vad->SHIFT_LEN + 1;
    *rows = frame_nums, *cols = vad->FRAME_LEN;
    uint32_t start = 0;
#ifdef RISCV
    *output = (int16_t *) mymalloc(0, sizeof(int16_t) * (*rows) * (*cols));
#else
    *output = (int16_t *) malloc(sizeof(int16_t) * (*rows) * (*cols));
#endif
    for (uint32_t i = 0; i < (*rows); ++i)
    {
        for (uint32_t j = 0; j < (*cols); ++j)
        {
            (*output)[i * (*cols) + j] = data[start + j];
        }
        start = i * vad->SHIFT_LEN;
    }
}

void init_cxt(Context **cxt)
{
#ifdef RISCV
    *cxt = (Context *) mymalloc(0, sizeof(Context));
#else
    *cxt = (Context *) malloc(sizeof(Context));
#endif
    (*cxt)->first_frame = 1;
    (*cxt)->Emax = 0;
    (*cxt)->Emin = 0;
    (*cxt)->delta = 65536;
    (*cxt)->pre_state = SILENCE;
    (*cxt)->cur_state = SILENCE;
    (*cxt)->active_count = 0;
    (*cxt)->inactive_count = 0;
    (*cxt)->confidence_count = 5;
}

void free_cxt(Context *cxt)
{
#ifdef RISCV
    myfree(0, cxt);
#else
    free(cxt);
#endif
}

void init_vad(VAD **vad)
{
#ifdef RISCV
    *vad = (VAD *) mymalloc(0, sizeof(VAD));
#else
    *vad = (VAD *) malloc(sizeof(VAD));
#endif
    (*vad)->FRAME_LEN = 512;
    (*vad)->SHIFT_LEN = 256;
    (*vad)->RSHIFT = 12;
    static uint8_t table[256] = {255, 128, 85, 64, 51, 42, 36, 32, 28, 25, 23,
                                 21, 19, 18, 17, 16, 15, 14, 13, 12, 12, 11, 11,
                                 10, 10, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6,
                                 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4,
                                 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
                                 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
                                };
    (*vad)->TABLE = table;
}

void free_vad(VAD *vad)
{
#ifdef RISCV
    myfree(0, vad);
#else
    free(vad);
#endif
}

uint8_t compute_rmse(int16_t *data, uint16_t data_len, uint8_t rshift)
{
    uint32_t rmse = 0;
    for (uint16_t i = 0; i < data_len; ++i)
    {
        rmse += abs(data[i]);
    }
    rmse = rmse >> rshift; // 除以data_len
    rmse = rmse > 256 ? 255 : rmse;
    // printf("%u\n", rmse);
    return rmse;
}

OSTATE dynamic_energy_vad(int16_t *data, uint16_t data_len, Context *cxt, VAD *vad, uint8_t *rmse_o, uint8_t *threshold_o)
{
    uint8_t rmse = compute_rmse(data, data_len, vad->RSHIFT);
    *rmse_o = rmse;
    if (cxt->first_frame == 1)
    {
        cxt->Emax = rmse > 0 ? rmse : 200;
        cxt->Emin = rmse > 0 ? rmse : 3;
        cxt->first_frame = 0;
    }
    if (rmse > cxt->Emax)
    {
        cxt->Emax = rmse;
    }
    if (rmse < cxt->Emax >> 3)
    {
        cxt->Emax--;
    }
    if (rmse < cxt->Emin)
    {
        if (rmse == 0)   // 类似于一个简单的AGC
        {
            cxt->Emin = 3;
        }
        else
        {
            cxt->Emin = rmse;
        }
    }
    cxt->delta++;
    uint16_t labda = (cxt->Emax - cxt->Emin) * vad->TABLE[cxt->Emax];
    // printf("%u\t%u\n", labda, cxt->Emin);
    uint8_t threshold = ((256 - labda) * cxt->Emax + labda * cxt->Emin) >> 8;
    threshold = threshold > 20 ? threshold : 20;
    *threshold_o = threshold;
    if (rmse >> 2 >= threshold)
    {
        cxt->Emin <<= 1;
    }
    if (rmse > threshold)
    {
        if (cxt->active_count == cxt->confidence_count)
        {
            cxt->pre_state = cxt->cur_state;
            cxt->cur_state = 1;
            cxt->inactive_count = 0;
        }
        else
        {
            cxt->cur_state = cxt->pre_state;
            cxt->active_count++;
        }
    }
    else
    {
        if (cxt->inactive_count == cxt->confidence_count)
        {
            cxt->pre_state = cxt->cur_state;
            cxt->cur_state = 0;
            cxt->active_count = 0;
        }
        else
        {
            cxt->cur_state = cxt->pre_state;
            cxt->inactive_count++;
        }
    }
    cxt->Emin = cxt->Emin * cxt->delta >> 16;
    return cxt->cur_state;
}

