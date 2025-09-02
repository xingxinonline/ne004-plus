#include "rbl_qspi.h"
#include "qspi_cadence.h"
#include "rbl_hal.h"

void rbl_qspi_init(uint32_t ref_clk_hz, uint32_t sclk_hz)
{
    qspi_set_verbose(false);
    qspi_cadence_init(ref_clk_hz, sclk_hz);
    RBL_LOG("[RBL] QSPI init done\r\n");
}

int rbl_qspi_read_jedec_id(uint8_t id[3])
{
    if (!id) return -1;
    int rc = qspi_read_id(id, 3);
    if (rc == 0)
    {
        char buf[64];
        // 简易十六进制打印，避免 printf：每字节两位十六进制
        static const char hex[] = "0123456789ABCDEF";
        buf[0] = '['; buf[1] = 'R'; buf[2] = 'B'; buf[3] = 'L'; buf[4] = ']'; buf[5] = ' ';
        buf[6] = 'I'; buf[7] = 'D'; buf[8] = ':'; buf[9] = ' ';
        int pos = 10;
        for (int i = 0; i < 3; ++i)
        {
            buf[pos++] = '0'; buf[pos++] = 'x';
            buf[pos++] = hex[(id[i] >> 4) & 0xF];
            buf[pos++] = hex[(id[i] >> 0) & 0xF];
            if (i != 2) buf[pos++] = ' ';
        }
        buf[pos++] = '\r'; buf[pos++] = '\n';
        rbl_uart_write(buf, (size_t)pos);
    }
    return rc;
}
