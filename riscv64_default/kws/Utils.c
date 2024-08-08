#include "Utils.h"

int8_t ClampQInt8(int32_t input) {
  if (input < -127) {
    return -127;
  } else if (input > 127) {
    return 127;
  } else {
    return input;
  }
}

uint8_t ClampUInt8(int32_t input) {
  if (input < 0) {
    return 0;
  } else if (input > 255) {
    return 255;
  } else {
    return input;
  }
}

int32_t RoundRShift(int32_t x, int32_t bits, int32_t bit) {
//  if(x > 0)
    return (x + bits) >> bit;
//  else
//    return (x - bits) >> bit;
}

int32_t FloatRound(float x) {
//  if(x > 0)
    return (int32_t)(x + 0.5);
//  else
//    return (int32_t)(x - 0.5);
}

