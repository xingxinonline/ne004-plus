#ifndef __LAYER_CNN_TYPES_H__
#define __LAYER_CNN_TYPES_H__
#include <stdint.h>
#ifndef BaseFloat
#define BaseFloat float
#endif

#ifndef MINUINT8
#define MINUINT8  0
#endif

#ifndef MAXINT8
#define MAXINT8   127
#endif

#ifndef MAXUINT8
#define MAXUINT8  255
#endif

#ifndef MININT8
#define MININT8   -128
#endif

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
#endif

#ifndef QALIAS
#define q7_t int8_t
#define q15_t int16_t
#define q31_t int32_t
#define q63_t int64_t
#define u7_t uint8_t
#define u15_t uint16_t
#define u31_t uint32_t
#endif


typedef struct {
  char words[35][15];
} TDICT;

struct arm_nn_double {
  uint32_t low;
  int32_t high;
};

union arm_nn_long_long {
  int64_t long_long;
  struct arm_nn_double word;
};

/** CMSIS-NN object used for the function context. */
typedef struct {
  void *buf;    /**< Pointer to a buffer needed for the optimization */
  int32_t size; /**< Buffer size */
} cmsis_nn_context;

/** CMSIS-NN object to contain the width and height of a tile */
typedef struct {
  int32_t w; /**< Width */
  int32_t h; /**< Height */
} cmsis_nn_tile;

/** CMSIS-NN object for the per-channel quantization parameters */
typedef struct {
  int8_t *shift;      /**< Shift values */
} cmsis_nn_per_channel_quant_params;

/** CMSIS-NN object for the quantized Relu activation */
typedef struct {
  int32_t min; /**< Min value used to clamp the result */
  int32_t max; /**< Max value used to clamp the result */
} cmsis_nn_activation;

/** CMSIS-NN object to contain the dimensions of the tensors */
typedef struct {
  int32_t n; /**< Generic dimension to contain either the batch size or output channels.
                     Please refer to the function documentation for more information */
  int32_t h; /**< Height */
  int32_t w; /**< Width */
  int32_t c; /**< Input channels */
} cmsis_nn_dims;

/** CMSIS-NN object for the convolution layer parameters */
typedef struct {
  int32_t input_offset;  /**< Zero value for the input tensor */
  int32_t output_offset; /**< Zero value for the output tensor */
  cmsis_nn_tile stride;
  cmsis_nn_tile padding;
  cmsis_nn_tile dilation;
  cmsis_nn_activation activation;
} cmsis_nn_conv_params;

/** CMSIS-NN object for Depthwise convolution layer parameters */
typedef struct {
  int32_t input_offset;  /**< Zero value for the input tensor */
  int32_t output_offset; /**< Zero value for the output tensor */
  int32_t ch_mult;       /**< Channel Multiplier. ch_mult * in_ch = out_ch */
  cmsis_nn_tile stride;
  cmsis_nn_tile padding;
  cmsis_nn_tile dilation;
  cmsis_nn_activation activation;
} cmsis_nn_dw_conv_params;

/**
 * @brief Error status returned by some functions in the library.
 */

typedef enum {
  ARM_MATH_SUCCESS                 =  0,        /**< No error */
  ARM_MATH_ARGUMENT_ERROR          = -1,        /**< One or more arguments are incorrect */
  ARM_MATH_LENGTH_ERROR            = -2,        /**< Length of data buffer is incorrect */
  ARM_MATH_SIZE_MISMATCH           = -3,        /**< Size of matrices is not compatible with the operation */
  ARM_MATH_NANINF                  = -4,        /**< Not-a-number (NaN) or infinity is generated */
  ARM_MATH_SINGULAR                = -5,        /**< Input matrix is singular and cannot be inverted */
  ARM_MATH_TEST_FAILURE            = -6,        /**< Test Failed */
  ARM_MATH_DECOMPOSITION_FAILURE   = -7         /**< Decomposition Failed */
} arm_status;

/*特征 每层的输入*/
typedef struct {
  char*   feature_buffer;
  uint8_t feature_type; // 0->uint8 1->int8
} IOFeature;

/*特征 CNN中间缓存*/
typedef struct {
  char*   cnn_buffer;
  uint8_t buffer_type; // 0->uint16 1->int16
} CNNBuffer;

/**
 *
 * @brief 卷积层
 *
 */
typedef struct CNN2D TCNN2D;
/*conv2d卷积层*/
struct CNN2D {
  int8_t layer_relu;            /*RELU*/
  char   layer_name[25];        /*名字*/
  char   layer_type[10];        /*类型*/
  char   layer_input_type[10];  /*输入数据类型*/
  char   layer_output_type[10]; /*输出数据类型*/

  uint16_t layer_groups;          /*deepwise cnn*/
  cmsis_nn_context ctx;
  cmsis_nn_conv_params conv_params;
  cmsis_nn_dw_conv_params dw_conv_params;
  cmsis_nn_per_channel_quant_params quant_params;
  cmsis_nn_dims input_dims;
  cmsis_nn_dims filter_dims;
  cmsis_nn_dims bias_dims;
  cmsis_nn_dims output_dims;

  int32_t* layer_bias;        /*cnn bias*/
  int8_t*  layer_weight;      /*cnn weight (Cout, H, W, Cin)*/
  int8_t*  layer_weight_shift;      /*cnn shift*/

  void (*forward)(IOFeature*, IOFeature*, TCNN2D*); /*推理函数*/
};


/**
 * @brief 全连接层
 *
 */
typedef struct LINEAR TLINEAR;
/*Linear层*/
struct LINEAR {
  char layer_name[25];        /*名字*/
  char layer_type[10];        /*类型*/
  char layer_input_type[10];  /*输入数据类型*/
  char layer_output_type[10]; /*输出数据类型*/

  uint16_t layer_input_shape[3];  /*linear input shape (H, W, C)*/
  uint16_t layer_output_shape[3]; /*linear output shape (H, W, C)*/

  int32_t* layer_bias;        /*linear bias*/
  int8_t*  layer_weight;      /*linear weight*/
  int8_t*  layer_weight_shift;      /*linear offset*/

  void (*forward)(IOFeature*, IOFeature*, TLINEAR*); /*推理函数*/
};

/**
 *
 * @brief 输入层,BaseFloat类型数据需要经过输入层量化
 *
 */
/*Input 层*/
typedef struct INPUT TINPUT;
struct INPUT {
  char      layer_name[25];        /*名字*/
  char      layer_type[10];        /*类型*/
  char      layer_output_type[10]; /*输出数据类型*/
  uint16_t  layer_input_shape[3];  /*input input shape [H, W, C]*/
  uint16_t  layer_output_shape[3]; /*input output shape [H, W, C]*/
  BaseFloat layer_output_scale;    /*输出变化因子*/

  void (*forward)(BaseFloat*, IOFeature*, TINPUT*); /*推理函数*/
};

/* Input Decode*/
void layer_input_forward(BaseFloat* input, IOFeature* ofeature, TINPUT* tinput);

/* Linear Decode */
void layer_linear_forward(IOFeature* ifeature, IOFeature* ofeature, TLINEAR* linear);

/* CNN Decode*/
void layer_conv_forward(IOFeature* ifeature, IOFeature* ofeature, TCNN2D* tcnn2d);
#endif
