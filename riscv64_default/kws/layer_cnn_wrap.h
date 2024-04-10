#ifndef __LAYER_CNN_WRAP_H__
#define __LAYER_CNN_WRAP_H__
#include "layer_types.h"
/*The Top Level Interface To OutSide*/
void layer_conv_forward(IOFeature* ifeature, IOFeature* ofeature, TCNN2D* tcnn2d);
#endif