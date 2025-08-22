#ifndef S300_SYSTICK_H
#define S300_SYSTICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void S300_SysTick_Init(void);
uint32_t S300_SysTick_Millis(void);
void S300_DelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* S300_SYSTICK_H */