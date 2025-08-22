#include "s300.h"
#include "s300_bsp.h"
#include "s300_uart.h"
#include "flash_if.h"
#include <string.h>

#define TEST_ADDR   0x08F00000u  /* W25Q128(16MB) 映射末端附近，尽量不干扰BL/APP */

static void u32_to_hex(uint32_t v, char *out)
{
    static const char hc[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i)
    {
        out[7 - i] = hc[(v >> (i * 4)) & 0xF];
    }
    out[8] = 0;
}

static void u8_to_hex(uint8_t v, char *out)
{
    static const char hc[] = "0123456789ABCDEF";
    out[0] = hc[(v >> 4) & 0xF];
    out[1] = hc[v & 0xF];
    out[2] = 0;
}

static void hexdump(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    char out[16];
    char b2[3];
    char sp[2] = { ' ', 0 };
    for (uint32_t i = 0; i < len; i += 16)
    {
        u32_to_hex(addr + i, out);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, out);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ": ");
        for (uint32_t j = 0; j < 16 && (i + j) < len; ++j)
        {
            u8_to_hex(buf[i + j], b2);
            S300_UART_PutStringI(BOARD_UART_DEBUG_ID, b2);
            S300_UART_PutStringI(BOARD_UART_DEBUG_ID, sp);
        }
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    }
}

int main(void)
{
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    S300_SysTick_Init();
    flash_if_init();
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Flash Test Start\n");
    uint8_t wbuf[256];
    for (uint32_t i = 0; i < sizeof(wbuf); ++i) wbuf[i] = (uint8_t)(i ^ 0xA5);
    /* 擦除 4KB 扇区 */
    if (flash_if_erase(TEST_ADDR, 4096) != 0)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Erase failed\n");
        while (1);
    }
    /* 写入一页 256B */
    if (flash_if_write(TEST_ADDR, wbuf, sizeof(wbuf)) != 0)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Write failed\n");
        while (1);
    }
    /* 读取并校验 */
    uint8_t rbuf[256];
    if (flash_if_read(TEST_ADDR, rbuf, sizeof(rbuf)) != 0)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Read failed\n");
        while (1);
    }
    if (memcmp(wbuf, rbuf, sizeof(wbuf)) == 0)
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Compare OK\n");
    }
    else
    {
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Compare BAD\n");
        hexdump(TEST_ADDR, rbuf, sizeof(rbuf));
    }
    /* CRC 验证（对测试区域的 256B 做 CRC）*/
    uint32_t crc = flash_if_crc32(TEST_ADDR, sizeof(wbuf));
    char haddr[9], hcrc[9];
    u32_to_hex(TEST_ADDR, haddr);
    u32_to_hex(crc, hcrc);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "CRC32(0x");
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, haddr);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ", 256) = 0x");
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, hcrc);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "\n");
    while (1)
    {
        S300_DelayMs(1000);
        S300_UART_PutStringI(BOARD_UART_DEBUG_ID, ".\n");
    }
}
