#include "w25qxx.h"
#include "qspi_cadence.h"
#include <string.h>

int w25qxx_init(w25qxx_info_t *info, bool want_quad, bool want_4byte_addr)
{
    if (!info) return -1;
    uint8_t id[3] = {0};
    if (qspi_read_id(id, 3) != 0) return -2;
    memset(info, 0, sizeof(*info));
    info->manuf_id = id[0];
    info->memory_type = id[1];
    info->capacity = id[2];
    info->page_size = 256u;
    info->sector_size = 4096u;
    info->block_size = 65536u;
    if (id[2] >= 0x14 && id[2] <= 0x26)
        info->size_bytes = (1u << id[2]);
    /* Quad Enable */
    if (want_quad)
    {
        (void)qspi_set_quad_enable(true);
        uint8_t sr2 = 0;
        (void)qspi_read_status(NULL, &sr2, NULL);
        info->quad_enabled = !!(sr2 & 0x02u);
        
        /* 如果成功启用QE，则配置控制器使用Quad读模式 */
        if (info->quad_enabled)
        {
            qspi_configure_quad_read(true);
        }
    }
    /* 4-byte addressing if size > 16MiB or forced */
    if (want_4byte_addr || info->size_bytes > (16u << 20))
    {
        (void)qspi_set_address_mode_4byte(true);
        info->addr4b = true;
    }
    return 0;
}

int w25qxx_read(uint32_t addr, void *buf, uint32_t len)
{
    return qspi_read(addr, buf, len);
}

int w25qxx_write_page(uint32_t addr, const void *buf, uint32_t len)
{
    if (len > 256u) len = 256u;
    return qspi_page_program(addr, buf, len);
}

int w25qxx_erase_4k(uint32_t addr)
{
    return qspi_erase_4k(addr);
}

int w25qxx_erase_64k(uint32_t addr)
{
    return qspi_erase_64k(addr);
}

int w25qxx_chip_erase(void)
{
    return qspi_chip_erase();
}
