#include "w25qxx.h"
#include "rcc.h"
#include "s300_memmap.h"

#include <string.h>

extern uint32_t SystemCoreClock;

#ifndef W25QXX_DEFAULT_SCLK_HZ
#define W25QXX_DEFAULT_SCLK_HZ      (24000000u)
#endif

#ifndef W25QXX_DEFAULT_TIMEOUT_US
#define W25QXX_DEFAULT_TIMEOUT_US   (500000u)
#endif

#ifndef W25QXX_READ_TIMEOUT_US
#define W25QXX_READ_TIMEOUT_US      (10000u)
#endif

#define W25Q_CMD_WRITE_ENABLE       0x06u
#define W25Q_CMD_WRITE_DISABLE      0x04u
#define W25Q_CMD_READ_SR1           0x05u
#define W25Q_CMD_READ_SR2           0x35u
#define W25Q_CMD_WRITE_SR1_SR2      0x01u
#define W25Q_CMD_WRITE_SR2          0x31u
#define W25Q_CMD_READ_JEDEC_ID      0x9Fu
#define W25Q_CMD_READ_UNIQUE_ID     0x4Bu
#define W25Q_CMD_FAST_READ          0x0Bu
#define W25Q_CMD_QUAD_READ          0x6Bu
#define W25Q_CMD_QUAD_IO_READ       0xEBu
#define W25Q_CMD_LEGACY_READ        0x03u
#define W25Q_CMD_PAGE_PROGRAM       0x02u
#define W25Q_CMD_QUAD_PAGE_PROGRAM  0x32u
#define W25Q_CMD_SECTOR_ERASE_4K    0x20u
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8u
#define W25Q_CMD_CHIP_ERASE         0xC7u
#define W25Q_CMD_ENTER_4BYTE        0xB7u
#define W25Q_CMD_EXIT_4BYTE         0xE9u
#define W25Q_CMD_ENABLE_RESET       0x66u
#define W25Q_CMD_RESET_DEVICE       0x99u

#define W25Q_STATUS_BUSY_MASK       0x01u
#define W25Q_STATUS_WEL_MASK        0x02u
#define W25Q_STATUS_QE_MASK         0x02u

static inline volatile uint32_t *w25qxx_reg_ptr(const w25qxx_device_t *dev, uint32_t offset)
{
    return (volatile uint32_t *)(dev->controller.regs + offset);
}

static inline uint32_t w25qxx_reg_read(const w25qxx_device_t *dev, uint32_t offset)
{
    return *w25qxx_reg_ptr(dev, offset);
}

static inline void w25qxx_reg_write(const w25qxx_device_t *dev, uint32_t offset, uint32_t value)
{
    *w25qxx_reg_ptr(dev, offset) = value;
}

static uint32_t w25qxx_select_read_delay(uint32_t sclk_hz)
{
    if (sclk_hz <= 24000000u)
    {
        return 0u;
    }
    else if (sclk_hz <= 48000000u)
    {
        return 1u;
    }
    else if (sclk_hz <= 72000000u)
    {
        return 2u;
    }
    else if (sclk_hz <= 96000000u)
    {
        return 3u;
    }
    return 4u;
}

static int w25qxx_configure_capture(w25qxx_device_t *dev, uint32_t delay_cycles)
{
    if (!dev)
    {
        return -1;
    }
    uint32_t capture = w25qxx_reg_read(dev, CQSPI_REG_RD_DATA_CAPTURE);
    capture &= ~((CQSPI_RD_CAPTURE_DELAY_MASK << CQSPI_RD_CAPTURE_DELAY_LSB) | CQSPI_RD_CAPTURE_BYPASS);
    if (delay_cycles == 0u)
    {
        capture |= CQSPI_RD_CAPTURE_BYPASS;
    }
    else
    {
        capture |= ((delay_cycles & CQSPI_RD_CAPTURE_DELAY_MASK) << CQSPI_RD_CAPTURE_DELAY_LSB);
    }
    w25qxx_reg_write(dev, CQSPI_REG_RD_DATA_CAPTURE, capture);
    return cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us);
}

static uint32_t w25qxx_capacity_to_size(uint8_t capacity)
{
    if (capacity < 16u || capacity > 31u)
    {
        return 0u;
    }
    return (1u << capacity);
}

static w25qxx_flash_type_t w25qxx_identify_type(uint8_t manuf, uint8_t memory_type)
{
    switch (manuf)
    {
    case 0xEF:
        if (memory_type == 0x40u)
        {
            return W25QXX_FLASH_W25Q;
        }
        break;
    case 0x20:
        if (memory_type == 0xBAu)
        {
            return W25QXX_FLASH_N25Q;
        }
        break;
    case 0xC2:
        if (memory_type == 0x20u)
        {
            return W25QXX_FLASH_MX25L;
        }
        break;
    default:
        break;
    }
    return W25QXX_FLASH_UNKNOWN;
}

static const char *w25qxx_type_name(w25qxx_flash_type_t type)
{
    switch (type)
    {
    case W25QXX_FLASH_W25Q:
        return "Winbond W25Q";
    case W25QXX_FLASH_N25Q:
        return "Micron N25Q";
    case W25QXX_FLASH_MX25L:
        return "Macronix MX25L";
    default:
        return "Unknown";
    }
}

static int w25qxx_send_simple_cmd(w25qxx_device_t *dev, uint8_t opcode)
{
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = opcode;
    return cqspi_stig_execute(&dev->controller, &cmd, NULL, NULL);
}

static int w25qxx_read_status_generic(w25qxx_device_t *dev, uint8_t opcode, uint8_t *value)
{
    if (!value)
    {
        return -1;
    }
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = opcode;
    cmd.read_len = 1u;
    return cqspi_stig_execute(&dev->controller, &cmd, value, NULL);
}

static int w25qxx_configure_write_default(w25qxx_device_t *dev)
{
    cqspi_indirect_write_config_t cfg = {0};
    cfg.opcode = W25Q_CMD_PAGE_PROGRAM;
    cfg.addr_bytes = dev->info.addr4b ? 4u : 3u;
    cfg.instr_width = CQSPI_BUSWIDTH_1;
    cfg.addr_width = CQSPI_BUSWIDTH_1;
    cfg.data_width = CQSPI_BUSWIDTH_1;
    cfg.mode_enable = false;
    cfg.mode_bits = 0u;
    return cqspi_configure_indirect_write(&dev->controller, &cfg);
}

static int w25qxx_apply_read_config(w25qxx_device_t *dev, const cqspi_indirect_read_config_t *cfg)
{
    if (!dev || !cfg)
    {
        return -1;
    }
    if (cqspi_configure_indirect_read(&dev->controller, cfg) != 0)
    {
        return -1;
    }
    dev->cached_rd_instr = w25qxx_reg_read(dev, CQSPI_REG_RD_INSTR);
    uint32_t mode_reg = w25qxx_reg_read(dev, CQSPI_REG_MODE_BIT);
    dev->cached_mode_bits = mode_reg & CQSPI_MODE_BITS_MASK;
    dev->cached_read_config_valid = true;
    return 0;
}

static int w25qxx_read_unique_id(w25qxx_device_t *dev, uint8_t uid[8])
{
    if (!uid)
    {
        return -1;
    }
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_READ_UNIQUE_ID;
    cmd.dummy_cycles = 32u;
    cmd.read_len = 8u;
    return cqspi_stig_execute(&dev->controller, &cmd, uid, NULL);
}

int w25qxx_read_jedec_id(w25qxx_device_t *dev, uint8_t id[3])
{
    if (!dev || !id)
    {
        return -1;
    }
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_READ_JEDEC_ID;
    cmd.read_len = 3u;
    return cqspi_stig_execute(&dev->controller, &cmd, id, NULL);
}

static volatile uint8_t *w25qxx_default_ahb_base(void)
{
    return (volatile uint8_t *)M4_SLV_FLASH_BASE;
}

int w25qxx_init(w25qxx_device_t *dev,
                const w25qxx_bus_config_t *bus_cfg,
                uint32_t default_sclk_hz,
                bool want_quad,
                bool want_4byte_addr,
                w25qxx_info_t *out_info)
{
    if (!dev)
    {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    w25qxx_bus_config_t cfg_local = {0};
    if (!bus_cfg)
    {
        cfg_local.reg_base = QSPI_CFG_BASE;
        cfg_local.ahb_base = (uintptr_t)w25qxx_default_ahb_base();
        cfg_local.ref_clk_hz = 0u;
        cfg_local.trigger_address = M4_SLV_FLASH_BASE;
        cfg_local.sram_partition = CQSPI_SRAM_TOTAL_LOCATIONS / 2u;
        bus_cfg = &cfg_local;
    }

    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
    rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_QSPIFLASH, false);
    rcc_set_cortex_m4_ahb_reset(RCC_CM4_AHB_QSPIFLASH, false);

    uint32_t ref_clk = bus_cfg->ref_clk_hz;
    if (ref_clk == 0u)
    {
        ref_clk = rcc_get_clock(RCC_CLOCK_AHB);
        if (ref_clk == 0u)
        {
            ref_clk = SystemCoreClock;
        }
    }

    cqspi_config_t cqspi_cfg = {0};
    cqspi_cfg.reg_base = bus_cfg->reg_base ? bus_cfg->reg_base : QSPI_CFG_BASE;
    cqspi_cfg.ahb_base = bus_cfg->ahb_base ? bus_cfg->ahb_base : (uintptr_t)w25qxx_default_ahb_base();
    cqspi_cfg.ref_clk_hz = ref_clk;
    cqspi_cfg.trigger_address = bus_cfg->trigger_address ? bus_cfg->trigger_address : M4_SLV_FLASH_BASE;
    cqspi_cfg.sram_partition = bus_cfg->sram_partition ? bus_cfg->sram_partition : (CQSPI_SRAM_TOTAL_LOCATIONS / 2u);
    cqspi_cfg.fifo_width_bytes = 4u;
    cqspi_cfg.decode_cs = false;

    if (cqspi_init(&dev->controller, &cqspi_cfg) != 0)
    {
        return -1;
    }

    dev->controller.indirect_timeout_us = W25QXX_DEFAULT_TIMEOUT_US;
    dev->controller.read_timeout_us = W25QXX_READ_TIMEOUT_US;
    dev->cached_read_config_valid = false;
    dev->direct_mode_active = false;
    dev->current_read_mode = W25QXX_READMODE_FAST_1_1_1;

    dev->info.page_size = W25QXX_PAGE_SIZE;
    dev->info.subsector_size = W25QXX_SUBSECTOR_SIZE;
    dev->info.block_size = W25QXX_BLOCK_SIZE_64K;
    dev->info.type = W25QXX_FLASH_UNKNOWN;
    dev->info.type_name = w25qxx_type_name(dev->info.type);
    dev->info.addr4b = false;
    dev->info.quad_enabled = false;
    memset(dev->info.unique_id, 0, sizeof(dev->info.unique_id));

    uint32_t initial_sclk = default_sclk_hz ? default_sclk_hz : W25QXX_DEFAULT_SCLK_HZ;
    if (w25qxx_configure_clock(dev, initial_sclk) != 0)
    {
        return -1;
    }

    if (w25qxx_configure_write_default(dev) != 0)
    {
        return -1;
    }

    if (w25qxx_select_read_mode(dev, W25QXX_READMODE_FAST_1_1_1) != 0)
    {
        return -1;
    }

    uint8_t jedec_id[3] = {0};
    if (w25qxx_read_jedec_id(dev, jedec_id) != 0)
    {
        return -1;
    }

    dev->info.manuf_id = jedec_id[0];
    dev->info.memory_type = jedec_id[1];
    dev->info.capacity = jedec_id[2];
    dev->info.size_bytes = w25qxx_capacity_to_size(jedec_id[2]);
    dev->info.type = w25qxx_identify_type(jedec_id[0], jedec_id[1]);
    dev->info.type_name = w25qxx_type_name(dev->info.type);

    if (w25qxx_read_unique_id(dev, dev->info.unique_id) != 0)
    {
        memset(dev->info.unique_id, 0, sizeof(dev->info.unique_id));
    }

    uint8_t sr2 = 0u;
    if (w25qxx_read_status2(dev, &sr2) == 0)
    {
        dev->info.quad_enabled = (sr2 & W25Q_STATUS_QE_MASK) != 0u;
    }

    if (dev->info.size_bytes > (16u * 1024u * 1024u) && want_4byte_addr)
    {
        if (w25qxx_set_address_mode(dev, true) != 0)
        {
            return -1;
        }
    }

    if (want_quad)
    {
        if (w25qxx_enable_quad_mode(dev, true) != 0)
        {
            return -1;
        }
    }

    if (w25qxx_disable_block_protect(dev) != 0)
    {
        return -1;
    }

    if (out_info)
    {
        *out_info = dev->info;
    }

    return 0;
}

const w25qxx_info_t *w25qxx_get_info(const w25qxx_device_t *dev)
{
    return dev ? &dev->info : NULL;
}

cqspi_dev_t *w25qxx_get_controller(w25qxx_device_t *dev)
{
    return dev ? &dev->controller : NULL;
}

int w25qxx_configure_clock(w25qxx_device_t *dev, uint32_t target_hz)
{
    if (!dev || target_hz == 0u)
    {
        return -1;
    }
    if (cqspi_configure_clock(&dev->controller, target_hz) != 0)
    {
        return -1;
    }
    uint32_t delay_cycles = w25qxx_select_read_delay(dev->controller.current_sclk_hz);
    return w25qxx_configure_capture(dev, delay_cycles);
}

int w25qxx_select_read_mode(w25qxx_device_t *dev, w25qxx_read_mode_t mode)
{
    if (!dev)
    {
        return -1;
    }

    cqspi_indirect_read_config_t cfg = {0};
    cfg.addr_bytes = dev->info.addr4b ? 4u : 3u;
    cfg.mode_enable = false;
    cfg.mode_bits = 0u;

    switch (mode)
    {
    case W25QXX_READMODE_FAST_1_1_1:
        cfg.opcode = W25Q_CMD_FAST_READ;
        cfg.instr_width = CQSPI_BUSWIDTH_1;
        cfg.addr_width = CQSPI_BUSWIDTH_1;
        cfg.data_width = CQSPI_BUSWIDTH_1;
        cfg.dummy_cycles = 8u;
        break;
    case W25QXX_READMODE_FAST_1_1_4:
        if (!dev->info.quad_enabled)
        {
            return -1;
        }
        cfg.opcode = W25Q_CMD_QUAD_READ;
        cfg.instr_width = CQSPI_BUSWIDTH_1;
        cfg.addr_width = CQSPI_BUSWIDTH_1;
        cfg.data_width = CQSPI_BUSWIDTH_4;
        cfg.dummy_cycles = 8u;
        break;
    case W25QXX_READMODE_FAST_1_4_4:
        if (!dev->info.quad_enabled)
        {
            return -1;
        }
        cfg.opcode = W25Q_CMD_QUAD_IO_READ;
        cfg.instr_width = CQSPI_BUSWIDTH_1;
        cfg.addr_width = CQSPI_BUSWIDTH_4;
        cfg.data_width = CQSPI_BUSWIDTH_4;
        cfg.dummy_cycles = 6u;
        break;
    default:
        return -1;
    }

    if (w25qxx_apply_read_config(dev, &cfg) != 0)
    {
        return -1;
    }

    dev->current_read_mode = mode;
    return 0;
}

int w25qxx_write_enable(w25qxx_device_t *dev)
{
    if (!dev)
    {
        return -1;
    }
    if (w25qxx_send_simple_cmd(dev, W25Q_CMD_WRITE_ENABLE) != 0)
    {
        return -1;
    }
    uint8_t sr1 = 0u;
    if (w25qxx_read_status1(dev, &sr1) != 0)
    {
        return -1;
    }
    return (sr1 & W25Q_STATUS_WEL_MASK) ? 0 : -1;
}

int w25qxx_wait_busy_clear(w25qxx_device_t *dev, uint32_t timeout_ms)
{
    if (!dev)
    {
        return -1;
    }
    if (timeout_ms == 0u)
    {
        timeout_ms = W25QXX_DEFAULT_TIMEOUT_US / 1000u;
        if (timeout_ms == 0u)
        {
            timeout_ms = 1u;
        }
    }

    uint32_t loops = timeout_ms * 1000u;
    while (loops--)
    {
        uint8_t status = 0u;
        if (w25qxx_read_status1(dev, &status) != 0)
        {
            return -1;
        }
        if ((status & W25Q_STATUS_BUSY_MASK) == 0u)
        {
            return 0;
        }
        for (volatile int i = 0; i < 1000; ++i)
        {
            (void)i;
        }
    }
    return -1;
}

int w25qxx_read_status1(w25qxx_device_t *dev, uint8_t *status)
{
    return w25qxx_read_status_generic(dev, W25Q_CMD_READ_SR1, status);
}

int w25qxx_read_status2(w25qxx_device_t *dev, uint8_t *status)
{
    return w25qxx_read_status_generic(dev, W25Q_CMD_READ_SR2, status);
}

int w25qxx_write_status2(w25qxx_device_t *dev, uint8_t status)
{
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_WRITE_SR2;
    cmd.write_len = 1u;
    return cqspi_stig_execute(&dev->controller, &cmd, NULL, &status);
}

int w25qxx_disable_block_protect(w25qxx_device_t *dev)
{
    if (!dev)
    {
        return -1;
    }

    if (w25qxx_write_enable(dev) != 0)
    {
        return -1;
    }

    uint8_t sr2 = 0u;
    if (w25qxx_read_status2(dev, &sr2) != 0)
    {
        return -1;
    }

    uint8_t status_regs[2] = {0x00u, sr2};
    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_WRITE_SR1_SR2;
    cmd.write_len = 2u;
    if (cqspi_stig_execute(&dev->controller, &cmd, NULL, status_regs) != 0)
    {
        return -1;
    }

    return w25qxx_wait_busy_clear(dev, 100u);
}

int w25qxx_set_address_mode(w25qxx_device_t *dev, bool addr4b)
{
    if (!dev)
    {
        return -1;
    }

    if (dev->info.addr4b == addr4b)
    {
        return 0;
    }

    uint8_t opcode = addr4b ? W25Q_CMD_ENTER_4BYTE : W25Q_CMD_EXIT_4BYTE;
    if (w25qxx_send_simple_cmd(dev, opcode) != 0)
    {
        return -1;
    }

    dev->info.addr4b = addr4b;

    if (w25qxx_configure_write_default(dev) != 0)
    {
        return -1;
    }

    return w25qxx_select_read_mode(dev, dev->current_read_mode);
}

int w25qxx_enable_quad_mode(w25qxx_device_t *dev, bool enable)
{
    if (!dev)
    {
        return -1;
    }

    if (dev->info.quad_enabled == enable)
    {
        return 0;
    }

    uint8_t sr2 = 0u;
    if (w25qxx_read_status2(dev, &sr2) != 0)
    {
        return -1;
    }

    if (enable)
    {
        sr2 |= W25Q_STATUS_QE_MASK;
    }
    else
    {
        sr2 &= (uint8_t)~W25Q_STATUS_QE_MASK;
    }

    if (w25qxx_write_enable(dev) != 0)
    {
        return -1;
    }

    if (w25qxx_write_status2(dev, sr2) != 0)
    {
        return -1;
    }

    if (w25qxx_wait_busy_clear(dev, 100u) != 0)
    {
        return -1;
    }

    dev->info.quad_enabled = enable;
    return 0;
}

int w25qxx_direct_mode_begin(w25qxx_device_t *dev)
{
    if (!dev)
    {
        return -1;
    }
    if (dev->direct_mode_active)
    {
        return 0;
    }

    uint32_t cfg = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    cfg |= CQSPI_CFG_ENABLE | CQSPI_CFG_DIRECT;
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    if (dev->cached_read_config_valid)
    {
        w25qxx_reg_write(dev, CQSPI_REG_RD_INSTR, dev->cached_rd_instr);
        if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
        {
            return -1;
        }
        uint32_t mode_reg = w25qxx_reg_read(dev, CQSPI_REG_MODE_BIT);
        mode_reg &= ~CQSPI_MODE_BITS_MASK;
        mode_reg |= dev->cached_mode_bits & CQSPI_MODE_BITS_MASK;
        w25qxx_reg_write(dev, CQSPI_REG_MODE_BIT, mode_reg);
        if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
        {
            return -1;
        }
    }

    dev->direct_mode_active = true;
    return 0;
}

int w25qxx_direct_mode_end(w25qxx_device_t *dev)
{
    if (!dev)
    {
        return -1;
    }
    if (!dev->direct_mode_active)
    {
        return 0;
    }

    uint32_t cfg = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    cfg &= ~CQSPI_CFG_DIRECT;
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    dev->direct_mode_active = false;
    return 0;
}

volatile uint8_t *w25qxx_direct_base(const w25qxx_device_t *dev)
{
    if (!dev)
    {
        return NULL;
    }
    return dev->controller.ahb;
}

int w25qxx_direct_write(w25qxx_device_t *dev, uint32_t address, const uint8_t *data, size_t length)
{
    if (!dev || !data || length == 0u)
    {
        return -1;
    }
    if (!w25qxx_direct_base(dev))
    {
        return -1;
    }

    if (w25qxx_direct_mode_begin(dev) != 0)
    {
        return -1;
    }

    volatile uint8_t *flash = w25qxx_direct_base(dev);
    size_t remaining = length;
    uint32_t curr_addr = address;
    const uint8_t *curr_data = data;
    int result = 0;

    while (remaining > 0u)
    {
        uint32_t offset_in_page = curr_addr & (W25QXX_PAGE_SIZE - 1u);
        uint32_t chunk = W25QXX_PAGE_SIZE - offset_in_page;
        if (chunk > remaining)
        {
            chunk = (uint32_t)remaining;
        }

        if (w25qxx_write_enable(dev) != 0)
        {
            result = -1;
            break;
        }

        volatile uint8_t *dst = flash + curr_addr;
        for (uint32_t i = 0u; i < chunk; ++i)
        {
            dst[i] = curr_data[i];
        }

        if (w25qxx_wait_busy_clear(dev, 200u) != 0)
        {
            result = -1;
            break;
        }

        curr_addr += chunk;
        curr_data += chunk;
        remaining -= chunk;
    }

    int end_ret = w25qxx_direct_mode_end(dev);
    if (end_ret != 0)
    {
        result = -1;
    }

    return result;
}

int w25qxx_direct_read(w25qxx_device_t *dev, uint32_t address, uint8_t *data, size_t length)
{
    if (!dev || !data || length == 0u)
    {
        return -1;
    }
    if (!w25qxx_direct_base(dev))
    {
        return -1;
    }

    if (w25qxx_direct_mode_begin(dev) != 0)
    {
        return -1;
    }

    volatile const uint8_t *flash = w25qxx_direct_base(dev);
    const volatile uint8_t *src = flash + address;
    for (size_t i = 0u; i < length; ++i)
    {
        data[i] = src[i];
    }

    return w25qxx_direct_mode_end(dev);
}

int w25qxx_erase_subsector(w25qxx_device_t *dev, uint32_t address)
{
    if (!dev)
    {
        return -1;
    }
    if ((address % W25QXX_SUBSECTOR_SIZE) != 0u)
    {
        return -1;
    }

    if (w25qxx_write_enable(dev) != 0)
    {
        return -1;
    }

    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_SECTOR_ERASE_4K;
    cmd.addr_bytes = dev->info.addr4b ? 4u : 3u;
    cmd.address = address;
    if (cqspi_stig_execute(&dev->controller, &cmd, NULL, NULL) != 0)
    {
        return -1;
    }

    return w25qxx_wait_busy_clear(dev, 1000u);
}

int w25qxx_erase_block64(w25qxx_device_t *dev, uint32_t address)
{
    if (!dev)
    {
        return -1;
    }
    if ((address % W25QXX_BLOCK_SIZE_64K) != 0u)
    {
        return -1;
    }

    if (w25qxx_write_enable(dev) != 0)
    {
        return -1;
    }

    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_BLOCK_ERASE_64K;
    cmd.addr_bytes = dev->info.addr4b ? 4u : 3u;
    cmd.address = address;
    if (cqspi_stig_execute(&dev->controller, &cmd, NULL, NULL) != 0)
    {
        return -1;
    }

    return w25qxx_wait_busy_clear(dev, 5000u);
}

int w25qxx_issue_legacy_read(w25qxx_device_t *dev, uint32_t address, uint8_t *byte_out)
{
    if (!dev)
    {
        return -1;
    }

    uint8_t temp = 0u;
    uint8_t *target = byte_out ? byte_out : &temp;

    cqspi_stig_cmd_t cmd = {0};
    cmd.opcode = W25Q_CMD_LEGACY_READ;
    cmd.addr_bytes = 3u;
    cmd.address = address;
    cmd.read_len = 1u;

    return cqspi_stig_execute(&dev->controller, &cmd, target, NULL);
}

int w25qxx_enter_xip_144(w25qxx_device_t *dev, w25qxx_xip_state_t *state)
{
    if (!dev || !state)
    {
        return -1;
    }

    state->config = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    state->rd_instr = w25qxx_reg_read(dev, CQSPI_REG_RD_INSTR);
    state->mode_bit = w25qxx_reg_read(dev, CQSPI_REG_MODE_BIT);

    if (dev->direct_mode_active)
    {
        if (w25qxx_direct_mode_end(dev) != 0)
        {
            return -1;
        }
    }

    uint32_t cfg = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    cfg &= ~CQSPI_CFG_DIRECT;
    cfg |= CQSPI_CFG_ENABLE;
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    uint32_t xip_rd_instr = ((uint32_t)W25Q_CMD_QUAD_IO_READ << CQSPI_RD_OPCODE_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_1 & CQSPI_RD_TYPE_INSTR_MASK) << CQSPI_RD_TYPE_INSTR_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_4 & CQSPI_RD_TYPE_ADDR_MASK) << CQSPI_RD_TYPE_ADDR_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_4 & CQSPI_RD_TYPE_DATA_MASK) << CQSPI_RD_TYPE_DATA_LSB) |
                            (((uint32_t)4u & CQSPI_RD_DUMMY_MASK) << CQSPI_RD_DUMMY_LSB) |
                            (1u << CQSPI_RD_MODE_EN_LSB);
    w25qxx_reg_write(dev, CQSPI_REG_RD_INSTR, xip_rd_instr);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    uint32_t mode_reg = state->mode_bit & ~CQSPI_MODE_BITS_MASK;
    mode_reg |= 0x20u & CQSPI_MODE_BITS_MASK;
    w25qxx_reg_write(dev, CQSPI_REG_MODE_BIT, mode_reg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    cfg = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    cfg &= ~CQSPI_CFG_XIP_IMM;
    cfg |= CQSPI_CFG_XIP_NEXT;
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    cfg |= CQSPI_CFG_DIRECT;
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    dev->direct_mode_active = true;
    return 0;
}

int w25qxx_exit_xip(w25qxx_device_t *dev, const w25qxx_xip_state_t *state, uint32_t flush_address)
{
    if (!dev || !state)
    {
        return -1;
    }

    if (w25qxx_direct_mode_end(dev) != 0)
    {
        return -1;
    }

    uint32_t cfg = w25qxx_reg_read(dev, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_XIP_NEXT | CQSPI_CFG_XIP_IMM | CQSPI_CFG_DIRECT);
    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    w25qxx_reg_write(dev, CQSPI_REG_MODE_BIT, state->mode_bit);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    w25qxx_reg_write(dev, CQSPI_REG_RD_INSTR, state->rd_instr);
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    w25qxx_reg_write(dev, CQSPI_REG_CONFIG, state->config & ~(CQSPI_CFG_XIP_NEXT | CQSPI_CFG_XIP_IMM));
    if (cqspi_wait_idle(&dev->controller, dev->controller.read_timeout_us) != 0)
    {
        return -1;
    }

    if (w25qxx_issue_legacy_read(dev, flush_address, NULL) != 0)
    {
        return -1;
    }

    dev->direct_mode_active = false;
    return 0;
}

