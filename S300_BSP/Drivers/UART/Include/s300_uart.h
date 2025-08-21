#ifndef S300_UART_H
#define S300_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART polling APIs */
void S300_UART_Init_115200(uint32_t idx);
void S300_UART_PutCharI(uint32_t idx, char c);
void S300_UART_PutStringI(uint32_t idx, const char *s);

#ifdef __cplusplus
}
#endif

#endif /* S300_UART_H */
