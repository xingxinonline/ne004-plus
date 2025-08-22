#include "s300.h"
#include "board.h"
#include "s300_bsp.h"
#include "s300_uart.h"
#include "flash_if.h"

#define BL_FLASH_BASE      (0x08000000u)
#define APP_FLASH_BASE     (0x08010000u) /* app offset after bootloader */
#define APP_FLASH_MAX_SIZE (0x00100000u) /* 1MB for example */

/* Image header */
typedef struct
{
    uint32_t magic;      /* 'S3IM' */
    uint32_t version;    /* semver or monotonic */
    uint32_t img_size;   /* payload size */
    uint32_t load_addr;  /* usually 0x08010000 for XIP or 0x20000000 for RAM */
    uint32_t entry;      /* entry address */
    uint32_t crc32;      /* CRC of payload */
} bl_image_header_t;

#define IMG_MAGIC 0x4D493353u /* 'S3IM' little-endian */

/* CRC32 (poly 0x04C11DB7, initial 0xFFFFFFFF, reflect in/out false) */
static uint32_t bl_crc32(const uint8_t *p, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i)
    {
        crc ^= ((uint32_t)p[i]) << 24;
        for (uint32_t b = 0; b < 8; ++b)
        {
            if (crc & 0x80000000u) crc = (crc << 1) ^ 0x04C11DB7u;
            else crc <<= 1;
        }
    }
    return crc;
}

static int bl_app_valid(uint32_t base)
{
    /* basic checks: MSP points to RAM1, Reset vector within FLASH/ROM/RAM */
    uint32_t msp = *(uint32_t *)(base + 0);
    uint32_t reset = *(uint32_t *)(base + 4);
    if (msp < 0x20000000u || msp > 0x20060000u) return 0;
    if ((reset & 1u) == 0u) return 0; /* thumb */
    return 1;
}

static void bl_jump_to_app(uint32_t base)
{
    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    /* set vector table to app */
    SCB->VTOR = base;
    uint32_t msp = *(uint32_t *)(base + 0);
    uint32_t reset = *(uint32_t *)(base + 4);
    __set_MSP(msp);
    void (*app_reset)(void) = (void (*)(void))reset;
    app_reset();
}

/* Protocol stubs */
extern int bl_proto_loop(uint32_t uart_idx);

int main(void)
{
    /* Make sure vector table points to FLASH for bootloader */
    SCB->VTOR = BL_FLASH_BASE;
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    S300_SysTick_Init();
    flash_if_init();
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "S300 Bootloader\n");
    /* Enter app if valid and no host intervention */
    uint32_t t0 = S300_SysTick_Millis();
    while ((uint32_t)(S300_SysTick_Millis() - t0) < 500u)
    {
        /* If a byte arrives within 500ms, stay in bootloader */
        if (S300_UART_RxReady(BOARD_UART_DEBUG_ID))
        {
            S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Host detected, enter BL\n");
            goto bootloader;
        }
    }
    if (bl_app_valid(APP_FLASH_BASE))
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Jump app...\n");
        bl_jump_to_app(APP_FLASH_BASE);
    }
bootloader:
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "BL> ");
    bl_proto_loop(BOARD_UART_DEBUG_ID);
    /* If protocol loop returns, reset */
    NVIC_SystemReset();
    for (;;);
}
