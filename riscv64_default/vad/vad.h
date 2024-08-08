#ifndef __VAD_H__
#define __VAD_H__
#include <stdint.h>

typedef enum {SPEECH = 1, SILENCE = 0} OSTATE; /*语音(包括噪音)还是静音*/

typedef struct {
  uint16_t FRAME_LEN; /*帧长*/
  uint16_t SHIFT_LEN; /*帧移*/
  uint8_t  RSHIFT;    /*量化右移*/
  uint8_t* TABLE;     /*查表*/
} VAD; // VAD全局参数

typedef struct {
  uint8_t first_frame; /*是否是第一帧*/
  uint8_t Emax;              /*最大幅值*/
  uint8_t Emin;              /*最小幅值*/
  uint32_t delta;            /*波动因子*/
  uint8_t pre_state;         /*前状态*/
  uint8_t cur_state;         /*当前状态*/
  uint8_t active_count;      /*有效计数*/
  uint8_t inactive_count;    /*无效计数*/
  uint8_t confidence_count;  /*置信长度*/
} Context; // 上下文

/* 创建Context结构体 */
void init_cxt(Context** cxt);

/* 销毁Context结构体 */
void free_cxt(Context* cxt);

/* 创建VAD结构体 */
void init_vad(VAD** vad);

/* 销毁VAD结构体 */
void free_vad(VAD* vad);

/*分帧*/
void framing(int16_t* data, uint32_t data_len, int16_t** output, uint32_t* rows, uint32_t* cols, VAD* vad);

/* 程序主入口 */
OSTATE dynamic_energy_vad(int16_t* data, uint16_t data_len, Context* cxt, VAD* vad, uint8_t* rmse_o, uint8_t* threshold_o);
#endif