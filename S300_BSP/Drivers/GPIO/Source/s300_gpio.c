#include "s300_gpio.h"

static int s300_gpio_check(uint32_t io)
{
    return (io < 48u) ? 0 : -1; /* IO0..47 */
}

int S300_GPIO_Init(uint32_t io, uint32_t mode)
{
    if (s300_gpio_check(io) != 0) return -1;
    /* Enable GPIO clock on APB1 bit8 (as used in board pinmux) */
    S300_RCC_EnableAPB1(1u << 8);
    S300_RCC_ReleaseAPB1Reset(1u << 8);
    /* Configure IO direction via hypothetical GPIO_DIR_REG and IO function as GPIO in IO_MATRIX
       Assume IO_MATRIX func=0 means GPIO; adjust if spec differs. */
    if (mode == S300_GPIO_OUTPUT)
    {
        GPIO_DIR_REG |= (1u << io);
    }
    else
    {
        GPIO_DIR_REG &= ~(1u << io);
    }
    /* Route as GPIO function0 */
    S300_IOMAT_SetIoFunc(io, 0u);
    return 0;
}

int S300_GPIO_Write(uint32_t io, uint32_t val)
{
    if (s300_gpio_check(io) != 0) return -1;
    if (val) GPIO_DATA_REG |= (1u << io);
    else     GPIO_DATA_REG &= ~(1u << io);
    return 0;
}

int S300_GPIO_Read(uint32_t io, uint32_t *val)
{
    if (s300_gpio_check(io) != 0 || val == 0) return -1;
    *val = (GPIO_DATA_REG >> io) & 1u;
    return 0;
}

int S300_GPIO_Toggle(uint32_t io)
{
    if (s300_gpio_check(io) != 0) return -1;
    GPIO_DATA_REG ^= (1u << io);
    return 0;
}

int S300_GPIO_ConfigPull(uint32_t io, uint32_t pull)
{
    /* Placeholder: Pulls typically configured in IO_MUX per-pad; without spec we no-op. */
    (void)io;
    (void)pull;
    return 0;
}
