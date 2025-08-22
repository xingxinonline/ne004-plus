#ifndef S300_GPIO_H
#define S300_GPIO_H

#include <stdint.h>
#include "s300.h"
#include "s300_rcc.h"
#include "s300_iomux.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal GPIO facade based on IO_MATRIX (mode) and a simple data register stub.
   NOTE: Real SoC may have dedicated GPIO DIR/DATA registers; here we assume IO_MATRIX
   controls direction and a hypothetical GPIO_DATA for value. Adjust when spec is available. */

/* Modes */
#define S300_GPIO_INPUT   0u
#define S300_GPIO_OUTPUT  1u

/* Pull config (placeholder, actual pulls may live in IO_MUX pad config) */
#define S300_GPIO_NOPULL  0u
#define S300_GPIO_PULLUP  1u
#define S300_GPIO_PULLDN  2u

/* Hypothetical GPIO data regs (for demo/placeholder) */
#ifndef GPIO_DATA_REG
#define GPIO_DATA_REG   (*(volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPIO_DIR_REG    (*(volatile uint32_t *)(GPIO_BASE + 0x04))
#endif

int S300_GPIO_Init(uint32_t io, uint32_t mode);
int S300_GPIO_Write(uint32_t io, uint32_t val);
int S300_GPIO_Read(uint32_t io, uint32_t *val);
int S300_GPIO_Toggle(uint32_t io);
int S300_GPIO_ConfigPull(uint32_t io, uint32_t pull);

#ifdef __cplusplus
}
#endif

#endif /* S300_GPIO_H */
