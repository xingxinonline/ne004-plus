/*** 
 * @Author       : xingxinonline
 * @Date         : 2024-08-22 14:51:12
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-23 19:25:28
 * @FilePath     : \\ne004-plus\\cortexm4_default\\main.cpp
 * @Description  : 
 * @
 * @Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include <cstdio>  // 用于使用 printf 函数

#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/examples/hello_world/models/hello_world_int8_model_data.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace
{
using HelloWorldOpResolver = tflite::MicroMutableOpResolver<1>;

TfLiteStatus RegisterOps(HelloWorldOpResolver &op_resolver)
{
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    return kTfLiteOk;
}
}  // namespace

// 使用 extern "C" 来确保正确链接 C 函数
extern "C" {
    int hal_init(void);
}

void debug_log_printf(const char* s) {
    printf("%s", s);  // 假设使用标准输出接口，这里可以替换为其他输出方式
}

TfLiteStatus LoadQuantModelAndPerformInference()
{
    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    MicroPrintf("LoadQuantModelAndPerformInference start");
    const tflite::Model *model =
        ::tflite::GetModel(g_hello_world_int8_model_data);
    MicroPrintf("LoadQuantModelAndPerformInference GetModel");
    TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);
    MicroPrintf("LoadQuantModelAndPerformInference TFLITE_CHECK_EQ");
    HelloWorldOpResolver op_resolver;
    TF_LITE_ENSURE_STATUS(RegisterOps(op_resolver));
    MicroPrintf("LoadQuantModelAndPerformInference RegisterOps");
    // Arena size just a round number. The exact arena usage can be determined
    // using the RecordingMicroInterpreter.
    constexpr int kTensorArenaSize = 3000;
    uint8_t tensor_arena[kTensorArenaSize];
    tflite::MicroInterpreter interpreter(model, op_resolver, tensor_arena,
                                         kTensorArenaSize);
    MicroPrintf("LoadQuantModelAndPerformInference MicroInterpreter");
    TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());
    MicroPrintf("LoadQuantModelAndPerformInference AllocateTensors");
    TfLiteTensor *input = interpreter.input(0);
    TFLITE_CHECK_NE(input, nullptr);
    TfLiteTensor *output = interpreter.output(0);
    TFLITE_CHECK_NE(output, nullptr);
    float output_scale = output->params.scale;
    int output_zero_point = output->params.zero_point;
    // Check if the predicted output is within a small range of the
    // expected output
    float epsilon = 0.05;
    constexpr int kNumTestValues = 4;
    float golden_inputs_float[kNumTestValues] = {0.77, 1.57, 2.3, 3.14};
    // The int8 values are calculated using the following formula
    // (golden_inputs_float[i] / input->params.scale + input->params.scale)
    int8_t golden_inputs_int8[kNumTestValues] = {-96, -63, -34, 0};
    for (int i = 0; i < kNumTestValues; ++i)
    {
        MicroPrintf("LoadQuantModelAndPerformInference for kNumTestValues");
        input->data.int8[0] = golden_inputs_int8[i];
        TF_LITE_ENSURE_STATUS(interpreter.Invoke());
        float y_pred = (output->data.int8[0] - output_zero_point) * output_scale;
        TFLITE_CHECK_LE(abs((float)sin(golden_inputs_float[i]) - y_pred), epsilon);
    }
    MicroPrintf("LoadQuantModelAndPerformInference end");
    return kTfLiteOk;
}

int main() {
    // 调用外部的C函数（如有必要）
    // start_riscv(42);  // 示例调用，42 是传递的参数
    hal_init();
    // 输出 "Hello, World" 到标准输出（可能被重定向到 UART）
    printf("Hello, World!\r\n");

    // 注册调试日志回调
    RegisterDebugLogCallback(debug_log_printf);


    // 打印初始消息
    MicroPrintf("System initialized successfully.");

    TF_LITE_ENSURE_STATUS(LoadQuantModelAndPerformInference());

    MicroPrintf("~~~ALL TESTS PASSED~~~\n");

    while (1) {
        // 主循环，在嵌入式系统中通常是一个无限循环
    }

    return 0;
}
