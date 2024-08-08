#include "Tensor.h"

void CreateTensorFromData(Tensor* ts, void* input, int16_t h, int16_t w, int16_t c) {
  int32_t input_len   = h * w * c;
  int32_t input_size  = input_len * sizeof(float);
  ts->data_          = input;
  ts->data_len_      = input_len;
  ts->capacity_      = input_size;
  ts->data_shape_[0] = h;
  ts->data_shape_[1] = w;
  ts->data_shape_[2] = c;
}

void SetTensorHWC(Tensor* ts, int16_t h, int16_t w, int16_t c) {
  int32_t input_len = h * w * c;
  ts->data_len_      = input_len;
  ts->data_shape_[0] = h;
  ts->data_shape_[1] = w;
  ts->data_shape_[2] = c;
}

//void TensorSwap(Tensor* a, Tensor* b) {
//  char*   tmp_data = (char*)a->data_.qdata_;
//  int16_t tmp_shape[3];
//  int32_t tmp_data_len_ = a->data_len_;
//  memcpy(tmp_shape, a->data_shape_, sizeof(int16_t) * 3);
//  int32_t tmp_cap = a->capacity_;
//  a->data_.qdata_ = b->data_.qdata_;
//  a->data_len_    = b->data_len_;
//  memcpy(a->data_shape_, b->data_shape_, sizeof(int16_t) * 3);
//  a->capacity_ = b->capacity_;
//
//  b->data_.qdata_ = (int8_t*)tmp_data;
//  b->data_len_    = tmp_data_len_;
//  memcpy(b->data_shape_, tmp_shape, sizeof(int16_t) * 3);
//  b->capacity_ = tmp_cap;
//}

//void FreeTensor(Tensor** tensor) {
//  if (*tensor != NULL) {
//    if ((*tensor)->data_.qdata_ != NULL) {
//      umm_free((void*)(*tensor)->data_.qdata_);
//    } else if ((*tensor)->data_.fdata_ != NULL) {
//      umm_free((void*)(*tensor)->data_.fdata_);
//    } else {
//      umm_free((void*)(*tensor)->data_.udata_);
//    }
//    umm_free(*tensor);
//    *tensor = NULL;
//  }
//}
