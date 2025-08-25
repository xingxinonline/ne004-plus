/* S300 GPIO implementation using real registers from docs */
#include "s300_gpio.h"

typedef struct {
    volatile uint32_t *base; /* register base */
    uint8_t ports;           /* number of ports implemented (1..4) */
    uint8_t widthA;          /* bits in A (e.g. 32) */
    uint8_t widthB;          /* bits in B (e.g. 16) */
    uint8_t widthC;          /* bits in C (e.g. 8) */
    uint8_t widthD;          /* bits in D (e.g. 8) */
} gpio_desc_t;

static inline volatile uint32_t *gpio_base_ptr(s300_gpio_ctrl_t ctrl)
{
    return (volatile uint32_t *)(ctrl == S300_GPIO_CTRL_M4 ? GPIO_BASE : AON_GPIO_BASE);
}

static void gpio_describe(s300_gpio_ctrl_t ctrl, gpio_desc_t *out)
{
    volatile uint32_t *base = gpio_base_ptr(ctrl);
    out->base = base;
    /* Read config regs if available; otherwise use defaults per docs */
    uint32_t cfg1 = *(volatile uint32_t *)((uintptr_t)base + S300_GPIO_OFF_CFG1);
    uint32_t cfg2 = *(volatile uint32_t *)((uintptr_t)base + S300_GPIO_OFF_CFG2);
    uint32_t num_ports = (cfg1 >> 2) & 0x3u; /* 0:1port,1:2ports,2:3,3:4 */
    out->ports = (uint8_t)(num_ports + 1u);
    out->widthA = ((cfg2 & 0x1Fu) + 1u);          /* ENCODED_ID_PWIDTH_A */
    out->widthB = (((cfg2 >> 5) & 0x1Fu) + 1u);
    out->widthC = (((cfg2 >> 10) & 0x1Fu) + 1u);
    out->widthD = (((cfg2 >> 15) & 0x1Fu) + 1u);
}

/* Map global io (0..47) to port index (0=A..3=D) and bit mask */
static int map_io_to_portbit(const gpio_desc_t *d, uint32_t io, uint32_t *port_idx, uint32_t *bit_mask)
{
    if (io >= 48u) return -1;
    uint32_t rem = io;
    uint32_t widths[4] = { d->widthA, d->widthB, d->widthC, d->widthD };
    uint32_t acc = 0;
    for (uint32_t p = 0; p < d->ports && p < 4u; ++p)
    {
        uint32_t w = widths[p];
        if (rem < w)
        {
            *port_idx = p;
            *bit_mask = (1u << rem);
            return 0;
        }
        rem -= w;
        acc += w;
    }
    return -1;
}

static inline volatile uint32_t *reg_port_dr(const gpio_desc_t *d, uint32_t port)
{
    return (volatile uint32_t *)((uintptr_t)d->base + S300_GPIO_OFF_PORT_DR + port * 12u);
}
static inline volatile uint32_t *reg_port_ddr(const gpio_desc_t *d, uint32_t port)
{
    return (volatile uint32_t *)((uintptr_t)d->base + S300_GPIO_OFF_PORT_DDR + port * 12u);
}
static inline volatile uint32_t *reg_port_ctl(const gpio_desc_t *d, uint32_t port)
{
    return (volatile uint32_t *)((uintptr_t)d->base + S300_GPIO_OFF_PORT_CTL + port * 12u);
}
static inline volatile uint32_t *reg_ext_port(const gpio_desc_t *d, uint32_t port)
{
    return (volatile uint32_t *)((uintptr_t)d->base + S300_GPIO_OFF_EXT_PORTA + port * 4u);
}

int S300_GPIO_SetDirection(s300_gpio_ctrl_t ctrl, uint32_t io, s300_gpio_dir_t dir)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    uint32_t port, bit; if (map_io_to_portbit(&desc, io, &port, &bit)) return -1;
    /* Set software control for the port bit (if single ctl register exists) */
    volatile uint32_t *ctl = reg_port_ctl(&desc, port);
    *ctl &= ~bit; /* 0 = software control */
    volatile uint32_t *ddr = reg_port_ddr(&desc, port);
    if (dir == S300_GPIO_OUTPUT) *ddr |= bit; else *ddr &= ~bit;
    return 0;
}

int S300_GPIO_Write(s300_gpio_ctrl_t ctrl, uint32_t io, uint32_t val)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    uint32_t port, bit; if (map_io_to_portbit(&desc, io, &port, &bit)) return -1;
    volatile uint32_t *dr = reg_port_dr(&desc, port);
    if (val) *dr |= bit; else *dr &= ~bit;
    return 0;
}

int S300_GPIO_Read(s300_gpio_ctrl_t ctrl, uint32_t io, uint32_t *val_out)
{
    if (!val_out) return -1;
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    uint32_t port, bit; if (map_io_to_portbit(&desc, io, &port, &bit)) return -1;
    uint32_t v = *reg_ext_port(&desc, port);
    *val_out = (v & bit) ? 1u : 0u;
    return 0;
}

int S300_GPIO_Toggle(s300_gpio_ctrl_t ctrl, uint32_t io)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    uint32_t port, bit; if (map_io_to_portbit(&desc, io, &port, &bit)) return -1;
    volatile uint32_t *dr = reg_port_dr(&desc, port);
    *dr ^= bit;
    return 0;
}

int S300_GPIO_ConfigPull(uint32_t io, s300_gpio_pull_t pull)
{
    /* Per IO Mux docs: Data0..2 @ IO_MUX_BASE + 0x00/0x04/0x08 control GPIO0..47 pulls.
       Even bits are used: bit[0] for GPIO0, bit[2] for GPIO1, ... 1=PU, 0=PD.
       No explicit 'none' state documented; return -2 for NONE to signal not supported. */
    if (pull == S300_GPIO_PULL_NONE) return -2;
    uint32_t group = io / 16u;      /* 0:0..15, 1:16..31, 2:32..47 */
    uint32_t idx   = io % 16u;
    volatile uint32_t *data = (volatile uint32_t *)(IO_MUX_BASE + (group * 4u)); /* Data0/1/2 */
    uint32_t shift = (idx * 2u);    /* even bit positions */
    uint32_t v = *data;
    if (pull == S300_GPIO_PULL_UP) v |= (1u << shift); else v &= ~(1u << shift);
    *data = v;
    return 0;
}

int S300_GPIO_DebounceEnable(s300_gpio_ctrl_t ctrl, uint32_t io, uint8_t enable)
{
    /* Only Port A supports interrupts/debounce; ensure io is within Port A width */
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    if (io >= desc.widthA) return -1;
    volatile uint32_t *deb = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_DEBOUNCE);
    uint32_t bit = (1u << io);
    uint32_t v = *deb;
    if (enable) v |= bit; else v &= ~bit;
    *deb = v;
    return 0;
}

int S300_GPIO_IntConfigure(s300_gpio_ctrl_t ctrl, uint32_t io, s300_gpio_int_type_t type, uint8_t enable)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    if (io >= desc.widthA) return -1; /* Only Port A */
    volatile uint32_t *inten   = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_INTEN);
    volatile uint32_t *intmask = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_INTMASK);
    volatile uint32_t *inttype = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_INTTYPE);
    volatile uint32_t *intpol  = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_INTPOLAR);
    uint32_t bit = (1u << io);
    /* type: edge/level */
    if ((type & 0x10u) != 0) *inttype |= bit; else *inttype &= ~bit; /* 1=edge,0=level */
    if ((type & 0x01u) != 0) *intpol  |= bit; else *intpol  &= ~bit; /* 1=high/rise,0=low/fall */
    if (enable) *inten |= bit; else *inten &= ~bit;
    /* default: unmask */
    *intmask &= ~bit;
    return 0;
}

int S300_GPIO_IntMask(s300_gpio_ctrl_t ctrl, uint32_t io, uint8_t mask)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    if (io >= desc.widthA) return -1; /* Only Port A */
    volatile uint32_t *intmask = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_INTMASK);
    uint32_t bit = (1u << io);
    if (mask) *intmask |= bit; else *intmask &= ~bit;
    return 0;
}

int S300_GPIO_IntClear(s300_gpio_ctrl_t ctrl, uint32_t io)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    if (io >= desc.widthA) return -1; /* Only Port A */
    volatile uint32_t *eoi = (volatile uint32_t *)((uintptr_t)desc.base + S300_GPIO_OFF_PORTA_EOI);
    *eoi = (1u << io);
    return 0;
}

uint32_t S300_GPIO_IntStatus(s300_gpio_ctrl_t ctrl, uint8_t raw)
{
    gpio_desc_t desc; gpio_describe(ctrl, &desc);
    volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)desc.base + (raw ? S300_GPIO_OFF_RAWSTAT : S300_GPIO_OFF_INTSTATUS));
    return *reg;
}
