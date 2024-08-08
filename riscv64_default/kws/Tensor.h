#ifndef __TENSOR_H__
#define __TENSOR_H__
#include <stdint.h>

//typedef union {
//  uint8_t* udata_;
//  int8_t*  qdata_;
//  float*   fdata_;
//} UData;

typedef struct {
  void    *data_;
  int32_t data_len_;       // ���ݴ�Ŷ���
  int16_t data_shape_[3];  // ����ά��
  int32_t capacity_;       // Tensor������С
} Tensor;

/*Create Tensor*/
void CreateTensorFromData(Tensor* ts, void* input, int16_t h, int16_t w, int16_t c);
/*Set Tensor Value*/
void SetTensorHWC(Tensor* ts, int16_t h, int16_t w, int16_t c);
///*Free Tensor Space*/
//void FreeTensor(Tensor** tensor);

///*����Tensor*/
//void TensorSwap(Tensor* a, Tensor* b);
#endif
