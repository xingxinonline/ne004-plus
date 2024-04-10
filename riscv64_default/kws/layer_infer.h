#ifndef __LAYER_INFER_H__
#define __LAYER_INFER_H__
#include "layer_types.h"

/**
 * @brief 推理 算法对外入口
 *
 */

/*推理初始化*/
void inference_init(void);

/*推理运算*/
void inference_start(BaseFloat* input);

/*推理彻底结束 销毁资源*/
void inference_stop(void);
#endif