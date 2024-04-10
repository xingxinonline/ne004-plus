#ifndef __LAYER_WEIGHTS_H__
#define __LAYER_WEIGHTS_H__
#include "layer_types.h"

/*precision: 96.67*/

extern TINPUT layer_0;

extern TCNN2D layer_1;

extern TCNN2D layer_2;

extern TCNN2D layer_3;

extern TCNN2D layer_4;

extern TCNN2D layer_5;

extern TCNN2D layer_6;

extern TCNN2D layer_7;

extern TCNN2D layer_8;

extern TCNN2D layer_9;

extern TCNN2D layer_10;

extern TCNN2D layer_11;

extern TCNN2D layer_12;

extern TLINEAR layer_13;

extern IOFeature ifeature;
extern IOFeature ofeature;
/*权重初始化*/
void layer_weight_init();
/*权重销毁*/
void layer_weight_destroy();
#endif
