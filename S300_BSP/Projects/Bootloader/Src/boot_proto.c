#include "s300_uart.h"
#include "s300.h"
#include "s300_bsp.h"
#include "flash_if.h"
#include <string.h>

/* Minimalistic ASCII protocol for demo; replace with XMODEM/SLIP later if needed */
static void bl_tx(uint32_t uart, const char *s)
{
    S300_UART_PutStringI(uart, s);
}

static int bl_rx_line(uint32_t uart, char *buf, int maxlen, uint32_t timeout_ms)
{
    int n = 0;
    uint32_t start = S300_SysTick_Millis();
    while (n < maxlen - 1)
    {
        uint8_t ch;
        int r = S300_UART_TryRead(uart, &ch);
        if (r == 1)
        {
            if (ch == '\r') continue;
            if (ch == '\n') break;
            buf[n++] = (char)ch;
        }
        else if (timeout_ms && (uint32_t)(S300_SysTick_Millis() - start) >= timeout_ms) break;
    }
    buf[n] = 0;
    return n;
}

static int hex2u32(const char *s, uint32_t *v)
{
    uint32_t x = 0;
    int i = 0;
    char c;
    while ((c = s[i++]))
    {
        x <<= 4;
        if (c >= '0' && c <= '9') x |= c - '0';
        else if (c >= 'a' && c <= 'f') x |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') x |= c - 'A' + 10;
        else return -1;
    }
    *v = x;
    return 0;
}

static void cmd_help(uint32_t u)
{
    bl_tx(u, "cmd: ping|info|erase <addr> <len>|write <addr> <len>|verify <addr> <len> <crc>|boot\n");
}

int bl_proto_loop(uint32_t uart_idx)
{
    char line[128];
    for (;;)
    {
        bl_tx(uart_idx, "> ");
        int n = bl_rx_line(uart_idx, line, sizeof(line), 0);
        if (n <= 0) continue;
        if (!strcmp(line, "ping"))
        {
            bl_tx(uart_idx, "pong\n");
        }
        else if (!strcmp(line, "info"))
        {
            bl_tx(uart_idx, "S300 BL v0.1; APP @0x08010000\n");
        }
        else if (!strncmp(line, "erase ", 7))
        {
            /* erase <addr_hex> <len_hex> */
            char *p = line + 6;
            while (*p == ' ') p++;
            char *sp = strchr(p, ' ');
            if (!sp)
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            *sp = 0;
            const char *saddr = p;
            const char *slen = sp + 1;
            uint32_t addr, len;
            if (hex2u32(saddr, &addr) || hex2u32(slen, &len))
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            if (flash_if_erase(addr, len) == 0) bl_tx(uart_idx, "OK\n");
            else bl_tx(uart_idx, "ERR\n");
        }
        else if (!strncmp(line, "write ", 7))
        {
            /* write <addr_hex> <len_hex>, then receive raw bytes len and program */
            char *p = line + 6;
            while (*p == ' ') p++;
            char *sp = strchr(p, ' ');
            if (!sp)
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            *sp = 0;
            const char *saddr = p;
            const char *slen = sp + 1;
            uint32_t addr, len;
            if (hex2u32(saddr, &addr) || hex2u32(slen, &len))
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            if (len == 0 || len > 4096)
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            static uint8_t buf[4096];
            bl_tx(uart_idx, "READY\n");
            /* receive exact len bytes */
            uint32_t got = 0;
            uint32_t start = S300_SysTick_Millis();
            while (got < len)
            {
                uint8_t ch;
                int r = S300_UART_TryRead(uart_idx, &ch);
                if (r == 1)
                {
                    buf[got++] = ch;
                    start = S300_SysTick_Millis();
                }
                else if ((uint32_t)(S300_SysTick_Millis() - start) > 1000u) break;
            }
            if (got != len)
            {
                bl_tx(uart_idx, "TO\n");
                continue;
            }
            int rc = flash_if_write(addr, buf, len);
            if (rc == 0) bl_tx(uart_idx, "OK\n");
            else bl_tx(uart_idx, "ERR\n");
        }
        else if (!strncmp(line, "verify ", 8))
        {
            /* verify <addr_hex> <len_hex> <crc_hex> */
            char *p = line + 7;
            while (*p == ' ') p++;
            char *p2 = strchr(p, ' ');
            if (!p2)
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            *p2 = 0;
            const char *saddr = p;
            p = p2 + 1;
            char *p3 = strchr(p, ' ');
            if (!p3)
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            *p3 = 0;
            const char *slen = p;
            const char *scrc = p3 + 1;
            uint32_t addr, len, crc;
            if (hex2u32(saddr, &addr) || hex2u32(slen, &len) || hex2u32(scrc, &crc))
            {
                bl_tx(uart_idx, "ERR\n");
                continue;
            }
            uint32_t c = flash_if_crc32(addr, len);
            if (c == crc) bl_tx(uart_idx, "OK\n");
            else bl_tx(uart_idx, "BAD\n");
        }
        else if (!strcmp(line, "boot"))
        {
            bl_tx(uart_idx, "BOOT\n");
            return 0;
        }
        else if (!strcmp(line, "help"))
        {
            cmd_help(uart_idx);
        }
        else
        {
            bl_tx(uart_idx, "?\n");
        }
    }
}
