#ifndef __LAYER_TOOLS_H__
#define __LAYER_TOOLS_H__
#include "layer_types.h"

BaseFloat clampfloat(BaseFloat d, BaseFloat min, BaseFloat max);
#ifndef RISCV
void ShowLinearLayerData(IOFeature* ofeature, TLINEAR* linear);
void ShowCNNLayerData(IOFeature* ofeature, TCNN2D* cnn);
void show_word(int8_t idx);
#endif
int8_t get_max_id(int8_t* input, int16_t dict_size, double threshold, double threshold02);

#ifdef MAC
void read_inputs(const char* input_file, BaseFloat* output);
#endif
void print_word(int8_t idx);
#endif