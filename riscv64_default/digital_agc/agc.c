#include <stdint.h>
#include <stdio.h>

#include "agc.h"

agc_t l_agc_inst;

#define AGC_ADJUST_STEP   4   // 4*0.375 = 1.5db

// 0b00000000 = 24db
// 0b00000001 = 23.625db
//     :         :
//     :         :
// 0b01000000 = 0db
//     :         :
// 0b11111110 = -71.625db
// 0b11111111 = mute

// [0db:-0.375db:-5.625dB]
static uint16_t agc_gain_table[16] =
{
  32768,31383,30057,28787,27571,26406,25290,24221,
  23198,22218,21279,20380,19519,18694,17904,17147
};
// [-3db:-3db:-24db] Q15
static uint16_t target_level_tab[8] =
{
  23198, 16423, 11627, 8231, 5827, 4125, 2920, 2068
};
// [-30db:-6db:-72db] Q15
static uint16_t noise_gete_tab[8] =
{
  1036, 519, 260, 131, 65, 33, 16, 8
};

void ag_set_th(agc_t *agc, uint8_t target, uint8_t gate, uint8_t gate_en)
{
  uint8_t bits = 0;

  bits = target & 0x07;
  agc->target = target_level_tab[bits];
  bits = gate & 0x07;
  agc->noise_gate = noise_gete_tab[bits];

  agc->noise_gate_en = gate_en;
  agc->mean = 0;
  agc->mean_ctr = (uint16_t)1 << AGC_MEAN_SHIFT;
  agc->noise_flag = 0;

  agc->state = AGC_HOLD;
}

void agc_set_gain(agc_t *agc, uint8_t max_gain, uint8_t def_gain)
{
  agc->max_gain = max_gain;
  agc->def_gain = def_gain;
  agc->cur_gain = def_gain;
  agc->buf_gain = def_gain;
}

void agc_set_time(agc_t *agc, uint32_t sr, uint8_t frame_time,
                                 uint8_t att_time, uint8_t dec_time)
{
  uint8_t bits = 0;

  agc->frame_num = frame_time * sr / 1000; // samples

  bits = att_time & 0x07;
  agc->attack = (uint8_t)1 << bits;

  bits = dec_time & 0x07;
  agc->decay = (uint8_t)1 << bits;

  agc->env_ctr = agc->frame_num;
}

static void agc_detect_noise(agc_t *agc, int16_t pcm_data)
{
  int16_t val;

  val = pcm_data;
  if(val < 0)
      val = - val;
  agc->mean += val;

  agc->mean_ctr--;
  if(agc->mean_ctr == 0)
  {
    agc->mean = agc->mean >> AGC_MEAN_SHIFT;
    if(agc->mean< agc->noise_gate)
    {
      agc->noise_flag = 1;
    }
    else
    {
      agc->noise_flag = 0;
    }
    agc->mean = 0;
    agc->mean_ctr = (uint16_t)1 << AGC_MEAN_SHIFT;
  }
}

static void agc_calc_gain(agc_t *agc, int16_t pcm_data)
{
  int16_t val;

  val = pcm_data;
  if(val < 0)
      val = - val;
  if(agc->env < val)
    agc->env = val;

  agc->env_ctr--;
  if(agc->env_ctr == 0)
  {
    if(agc->env > agc->target)
    {
      if(agc->state == AGC_COM)
      {
        agc->att_ctr--;
        if(agc->att_ctr == 0)
        {
          agc->cur_gain = agc->cur_gain + AGC_ADJUST_STEP;
          agc->att_ctr = agc->attack;
        }
      }
      else
      {
        agc->state = AGC_COM;
        agc->cur_gain = agc->cur_gain + AGC_ADJUST_STEP;
        agc->att_ctr = agc->attack;
      }
    }
    else
    {
      if(agc->env > (agc->noise_gate << (4 - ((agc->max_gain >> 4)&0x0F))))
      {
        if(agc->state == AGC_EXP)
        {
          agc->dec_ctr--;
          if(agc->dec_ctr == 0)
          {
              agc->cur_gain = agc->cur_gain - AGC_ADJUST_STEP;
              agc->dec_ctr = agc->decay;
          }
        }
        else
        {
          agc->state = AGC_EXP;
          agc->cur_gain = agc->cur_gain - AGC_ADJUST_STEP;
          agc->dec_ctr = agc->decay;
        }
      }
    }

    if(agc->cur_gain < agc->max_gain)
      agc->cur_gain = agc->max_gain;

    agc->env = 0;
    agc->env_ctr = agc->frame_num;
  }
}

static int16_t agc_apply_gain(agc_t *agc, int16_t data_in)
{
  uint16_t frac;
  uint8_t  sht;
  int32_t  result;
  uint8_t  gain;

  if(agc->buf_gain != agc->cur_gain)
  {
    if(agc->last_sample * data_in < 0)
      agc->buf_gain = agc->cur_gain;
  }
  agc->last_sample = data_in;

  gain = agc->buf_gain;
  if(gain == 0xff || (agc->noise_gate_en == 1 && agc->noise_flag == 1))
  {
    result = 0;
  }
  else
  {
    frac = agc_gain_table[gain & 0x0F];
    sht = (gain>>4) & 0x0F;
    result = data_in * frac;
    // 15: frac q15
    // sht - 4: 2^4 ~ 2^12, 24db ~ -72db
    result = result >> (15 + sht - 4);

    if(result > 32767)
        result = 32767;
    if(result < -32768)
        result = -32768;
  }
  return (int16_t)result;
}

int16_t agc_process(agc_t *agc, int16_t pcm_data)
{
  int16_t data_out;

  agc_detect_noise(agc, pcm_data);

  data_out = agc_apply_gain(agc, pcm_data);

  agc_calc_gain(agc, data_out);

  return data_out;
}

