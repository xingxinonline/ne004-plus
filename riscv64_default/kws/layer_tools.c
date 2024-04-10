#ifdef MAC
    #include <string.h>
    #include <stdlib.h>
#elif RISCV
    #include "malloc.h"
#elif ARM_MATH_DSP
    #include <string.h>
    #include <stdlib.h>
#endif
#include <stdio.h>
#include "layer_tools.h"
// static uint8_t table[13][12] = {{206, 180, 214, 170},
//     {196, 227, 186, 195, 208, 161, 190, 169},
//     {196, 227, 186, 195, 208, 161, 197, 201},
//     {203, 175, 195, 223, 196, 163, 202, 189},
//     {187, 189, 208, 209, 196, 163, 202, 189},
//     {181, 227, 193, 193, 198, 193, 196, 187},
//     {185, 216, 177, 213, 198, 193, 196, 187},
//     {181, 247, 180, 243, 210, 244, 193, 191},
//     {181, 247, 208, 161, 210, 244, 193, 191},
//     {191, 170, 202, 188, 187, 225, 210, 233},
//     {199, 208, 187, 187, 205, 168, 181, 192},
//     {180, 242, 191, 170, 202, 211, 198, 181},
//     {185, 216, 177, 213, 202, 211, 198, 181}
// };

BaseFloat clampfloat(BaseFloat d, BaseFloat min, BaseFloat max)
{
    const BaseFloat t = d < min ? min : d;
    return t > max ? max : t;
}

#ifndef RISCV
#include <stdio.h>
void ShowCNNLayerData(IOFeature *ofeature, TCNN2D *cnn)
{
    int16_t out_x = cnn->output_dims.h;
    int16_t out_y = cnn->output_dims.w;
    int16_t out_ch = cnn->output_dims.c;
    if (ofeature->feature_type == 1)
    {
        int8_t *data = (int8_t *)ofeature->feature_buffer;
        for (int32_t i = 0; i < out_x * out_y * out_ch; ++i)
        {
            // for (uint32_t i = 0; i < 3120; ++i) {
            if (i % out_x == 0)
            {
                int a = 0;
                while (a < 1000000)
                {
                    a++;
                }
            }
            printf("%d,", data[i]);
        }
    }
    else
    {
        uint8_t *data = (uint8_t *)ofeature->feature_buffer;
        for (int32_t i = 0; i < out_x * out_y * out_ch; ++i)
        {
            // for (uint32_t i = 0; i < 3120; ++i) {
            if (i % out_x == 0)
            {
                int a = 0;
                while (a < 1000000)
                {
                    a++;
                }
            }
            printf("%u,", data[i]);
        }
    }
}

void ShowLinearLayerData(IOFeature *ofeature, TLINEAR *linear)
{
    int16_t out_x = linear->layer_output_shape[0];
    int16_t out_y = linear->layer_output_shape[1];
    int16_t out_ch = linear->layer_output_shape[2];
    if (ofeature->feature_type == 1)
    {
        int8_t *data = (int8_t *)ofeature->feature_buffer;
        for (int32_t i = 0; i < out_x * out_y * out_ch; ++i)
        {
            printf("%d,", data[i]);
        }
    }
    else
    {
        uint8_t *data = (uint8_t *)ofeature->feature_buffer;
        for (int32_t i = 0; i < out_x * out_y * out_ch; ++i)
        {
            printf("%u,", data[i]);
        }
    }
}

void split_floats(char *input, const char *delimit, BaseFloat *out)
{
    char    *token = strtok(input, delimit);
    uint32_t sidx = 0;
    while (token)
    {
        out[sidx++] = atof(token);
        token = strtok(NULL, delimit);
    }
}

void show_word(int8_t idx)
{
    switch (idx)
    {
    case 0:
        printf("未知\n");
        break;
    case 1:
        printf("你好小京\n");
        break;
    case 2:
        printf("你好小派\n");
        break;
    case 3:
        printf("睡眠模式\n");
        break;
    case 4:
        printf("唤醒模式\n");
        break;
    case 5:
        printf("点亮屏幕\n");
        break;
    case 6:
        printf("关闭屏幕\n");
        break;
    case 7:
        printf("调大音量\n");
        break;
    case 8:
        printf("调小音量\n");
        break;
    case 9:
        printf("开始会议\n");
        break;
    case 10:
        printf("切换通道\n");
        break;
    case 11:
        printf("打开视频\n");
        break;
    case 12:
        printf("关闭视频\n");
        break;
    default:
        printf("Error check maxidx\n");
        break;
    }
}
#endif

extern void delayus(unsigned long dly);
extern int print_bcd(int8_t hex);
void print_word(int8_t idx)
{
    switch (idx)
    {
    case 0:
        // for (int8_t i = 0; i < 4; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[0][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("未知\n");
        break;
    case 1:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[1][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("你好小京\n");
        break;
    case 2:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[2][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("你好小派\n");
        break;
    case 3:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[3][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("睡眠模式\n");
        break;
    case 4:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[4][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("唤醒模式\n");
        break;
    case 5:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[5][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("点亮屏幕\n");
        break;
    case 6:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[6][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("关闭屏幕\n");
        break;
    case 7:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[7][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("调大音量\n");
        break;
    case 8:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[8][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("调小音量\n");
        break;
    case 9:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[9][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("开始会议\n");
        break;
    case 10:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[10][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("切换通道\n");
        break;
    case 11:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[11][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("打开视频\n");
        break;
    case 12:
        // for (int8_t i = 0; i < 8; ++i)
        // {
        //     (*(volatile uint8_t *)0x64000000UL) = table[12][i];
        //     delayus(10);
        // }
        // (*(volatile uint8_t *)0x64000000UL) = '\n';
        // delayus(10);
        printf("关闭视频\n");
        break;
    default:
        break;
    }
}


int8_t get_max_id(int8_t *input, int16_t dict_size, double threshold, double threshold02)
{
    int8_t    min_prob = -128;
    int16_t   max_idx = 0;
    BaseFloat max_probs = min_prob;
    for (uint16_t i = 0; i < dict_size; ++i)
    {
        if (i != 0 && input[i] > threshold02)
        {
            if (max_idx != 0)
            {
                max_probs = max_probs > input[i] ? max_probs : input[i];
                max_idx = max_probs > input[i] ? max_idx : i;
            }
            else
            {
                max_probs = input[i];
                max_idx = i;
            }
        }
        else
        {
            max_probs = max_probs > input[i] ? max_probs : input[i];
            max_idx = max_probs > input[i] ? max_idx : i;
        }
    }
    if (max_probs <= threshold)
    {
        max_idx = 0;
        max_probs = 0;
    }
    return max_idx;
}

#ifdef MAC
void read_inputs(const char *input_file, BaseFloat *output)
{
    FILE  *file = fopen(input_file, "r");
    size_t len = 0, read = 0;
    char  *line = NULL;
    while ((read = getline(&line, &len, file)) != -1)
    {
        split_floats(line, ",", output);
    }
    free(line);
    fclose(file);
}
#endif
