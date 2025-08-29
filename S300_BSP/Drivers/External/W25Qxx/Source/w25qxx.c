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

/* 扩展 ID 读取功能 */
int w25qxx_read_device_id(uint8_t *dev_id)
{
    return qspi_read_device_id(dev_id);
}

int w25qxx_read_manufacturer_device_id(uint8_t *mfg_id, uint8_t *dev_id)
{
    return qspi_read_manufacturer_device_id(mfg_id, dev_id);
}

int w25qxx_read_unique_id(uint8_t *uid, uint32_t len)
{
    return qspi_read_unique_id(uid, len);
}

int w25qxx_read_sfdp(uint32_t addr, uint8_t *buf, uint32_t len)
{
    return qspi_read_sfdp(addr, buf, len);
}

/* 扩展写入功能 */
int w25qxx_write_page_quad(uint32_t addr, const void *buf, uint32_t len)
{
    if (len > 256u) len = 256u;
    return qspi_page_program_quad(addr, buf, len);
}

/* 扩展擦除功能 */
int w25qxx_erase_32k(uint32_t addr)
{
    return qspi_erase_32k(addr);
}

/* 高级功能 */
int w25qxx_software_reset(void)
{
    return qspi_software_reset();
}

int w25qxx_configure_read_mode(int mode)
{
    switch (mode)
    {
    case 0: /* 单线模式 */
        qspi_configure_quad_read(false);
        return 0;
    case 1: /* 四线输出模式 (6Bh) */
        qspi_configure_quad_read(true);
        return 0;
    case 2: /* 四线 I/O 模式 (EBh) */
        qspi_configure_quad_io_read(true);
        return 0;
    default:
        return -1; /* 不支持的模式 */
    }
}
