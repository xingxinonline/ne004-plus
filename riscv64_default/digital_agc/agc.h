#ifndef DIGITAL_AGC_H_
#define DIGITAL_AGC_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define AGC_MEAN_SHIFT    8   // 256 samples @ 16kHz

typedef enum
{
  AGC_HOLD,
  AGC_EXP,
  AGC_COM
}agc_state_t;

typedef struct agc
{
  uint32_t  target;        //3bit range:[-3db, -24db], step:3db
  uint32_t  noise_gate;    //3bit range:[-30db:-72db], step:6db
  uint8_t   noise_gate_en;
  uint8_t   noise_flag;

  uint8_t   max_gain;        //8bit range:[24db:-72db], step:0.375db
  uint8_t   def_gain;
  uint8_t   cur_gain;
  uint8_t   buf_gain;
  int16_t   last_sample;

  uint16_t  frame_num;
  uint8_t   attack;         //3bit range:[32ms:4096ms] 32ms*2^(0:7)
  uint8_t   decay;         //3bit range:[32ms:4096ms] 32ms*2^(0:7)
  uint8_t   att_ctr;
  uint8_t   dec_ctr;

  uint32_t  mean;
  uint16_t  mean_ctr;
  uint16_t  env;
  uint16_t  env_ctr;

  agc_state_t state;
} agc_t;

extern agc_t l_agc_inst;

void ag_set_th(agc_t *agc, uint8_t target, uint8_t gate, uint8_t gate_en);

void agc_set_gain(agc_t *agc, uint8_t max_gain, uint8_t def_gain);

void agc_set_time(agc_t *agc, uint32_t sr, uint8_t frame_time,
                                 uint8_t att_time, uint8_t dec_time);

int16_t agc_process(agc_t *agc, int16_t pcm_data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DIGITAL_AGC_H_
