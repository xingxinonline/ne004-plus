#ifndef S300_GPIO_H
#define S300_GPIO_H

#include <stdint.h>
#include "s300.h"
#include "s300_rcc.h"
#include "s300_iomux.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * S300 GPIO driver (M4 GPIO @0x40018000, optional AON GPIO @0x43014000)
 * - Ports: A..D with variable width reported by config regs (0x70/0x74)
 * - Only Port A supports interrupts
 * - Public API supports global IO index (0..47) mapped across ports
 */

/* Optional AON base (not defined in s300.h) */
#ifndef AON_GPIO_BASE
#define AON_GPIO_BASE (0x43014000UL)
#endif

typedef enum {
   S300_GPIO_CTRL_M4  = 0,   /* base = GPIO_BASE */
   S300_GPIO_CTRL_AON = 1    /* base = AON_GPIO_BASE */
} s300_gpio_ctrl_t;

typedef enum {
   S300_GPIO_INPUT  = 0u,
   S300_GPIO_OUTPUT = 1u
} s300_gpio_dir_t;

typedef enum {
   S300_GPIO_PULL_NONE = 0u,
   S300_GPIO_PULL_UP   = 1u,
   S300_GPIO_PULL_DOWN = 2u
} s300_gpio_pull_t;

typedef enum {
   S300_GPIO_INT_LEVEL_LOW  = 0x00,
   S300_GPIO_INT_LEVEL_HIGH = 0x01,
   S300_GPIO_INT_EDGE_FALL  = 0x10,
   S300_GPIO_INT_EDGE_RISE  = 0x11
} s300_gpio_int_type_t;

/* Low-level register offsets (per GPIO docs) */
enum {
   S300_GPIO_OFF_PORT_DR   = 0x00, /* +port*12 */
   S300_GPIO_OFF_PORT_DDR  = 0x04, /* +port*12 */
   S300_GPIO_OFF_PORT_CTL  = 0x08, /* +port*12 */
   S300_GPIO_OFF_INTEN     = 0x30,
   S300_GPIO_OFF_INTMASK   = 0x34,
   S300_GPIO_OFF_INTTYPE   = 0x38,
   S300_GPIO_OFF_INTPOLAR  = 0x3C,
   S300_GPIO_OFF_INTSTATUS = 0x40, /* RO */
   S300_GPIO_OFF_RAWSTAT   = 0x44, /* RO */
   S300_GPIO_OFF_DEBOUNCE  = 0x48,
   S300_GPIO_OFF_PORTA_EOI = 0x4C, /* WO */
   S300_GPIO_OFF_EXT_PORTA = 0x50, /* +port*4 for EXT ports */
   S300_GPIO_OFF_LS_SYNC   = 0x60,
   S300_GPIO_OFF_CFG2      = 0x70,
   S300_GPIO_OFF_CFG1      = 0x74,
};

/* Public API */

/* Route a pad to GPIO function in IO Matrix (function 0 by convention) */
static inline void S300_GPIO_RouteToGPIO(uint32_t io)
{
   S300_IOMAT_SetIoFunc(io, 0u);
}

/* Configure direction for a global IO on a controller */
int S300_GPIO_SetDirection(s300_gpio_ctrl_t ctrl, uint32_t io, s300_gpio_dir_t dir);

/* Atomic bit write/read/toggle using port DR/EXT */
int S300_GPIO_Write(s300_gpio_ctrl_t ctrl, uint32_t io, uint32_t val);
int S300_GPIO_Read(s300_gpio_ctrl_t ctrl, uint32_t io, uint32_t *val_out);
int S300_GPIO_Toggle(s300_gpio_ctrl_t ctrl, uint32_t io);

/* Optional pad pull config: return -ENOTSUP if not supported at SOC level here */
int S300_GPIO_ConfigPull(uint32_t io, s300_gpio_pull_t pull);

/* Debounce (Port A only) */
int S300_GPIO_DebounceEnable(s300_gpio_ctrl_t ctrl, uint32_t io, uint8_t enable);

/* Interrupts (Port A only): enable + type + mask control */
int S300_GPIO_IntConfigure(s300_gpio_ctrl_t ctrl, uint32_t io, s300_gpio_int_type_t type, uint8_t enable);
int S300_GPIO_IntMask(s300_gpio_ctrl_t ctrl, uint32_t io, uint8_t mask);
int S300_GPIO_IntClear(s300_gpio_ctrl_t ctrl, uint32_t io);
uint32_t S300_GPIO_IntStatus(s300_gpio_ctrl_t ctrl, uint8_t raw);

/* Clock helpers */
static inline void S300_GPIO_EnableClock(void)
{
   /* APB1 bit8 used for GPIO per board pinmux */
   S300_RCC_EnableAPB1(1u << 8);
   S300_RCC_ReleaseAPB1Reset(1u << 8);
}

/* Compatibility wrappers (default to M4 controller) */
static inline int S300_GPIO_Init(uint32_t io, uint32_t mode)
{
   S300_GPIO_EnableClock();
   S300_GPIO_RouteToGPIO(io);
   return S300_GPIO_SetDirection(S300_GPIO_CTRL_M4, io, (mode ? S300_GPIO_OUTPUT : S300_GPIO_INPUT));
}

static inline int S300_GPIO_WriteCompat(uint32_t io, uint32_t val)
{
   return S300_GPIO_Write(S300_GPIO_CTRL_M4, io, val);
}

static inline int S300_GPIO_ReadCompat(uint32_t io, uint32_t *val)
{
   return S300_GPIO_Read(S300_GPIO_CTRL_M4, io, val);
}

static inline int S300_GPIO_ToggleCompat(uint32_t io)
{
   return S300_GPIO_Toggle(S300_GPIO_CTRL_M4, io);
}

#ifdef __cplusplus
}
#endif

#endif /* S300_GPIO_H */
