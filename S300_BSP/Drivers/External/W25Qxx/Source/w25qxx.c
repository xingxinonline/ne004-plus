#include "w25qxx.h"
#include "qspi_cadence.h"
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

#define W25QXX_PAGE_SIZE            (256u)
#define W25QXX_SECTOR_SIZE          (4096u)
#define W25QXX_BLOCK32_SIZE         (32u * 1024u)
#define W25QXX_BLOCK64_SIZE         (64u * 1024u)

#define W25Q_CMD_WRITE_ENABLE       0x06u
#define W25Q_CMD_WRITE_DISABLE      0x04u
#define W25Q_CMD_READ_SR1           0x05u
#define W25Q_CMD_READ_SR2           0x35u
#define W25Q_CMD_WRITE_SR2          0x31u
#define W25Q_CMD_READ_JEDEC_ID      0x9Fu
#define W25Q_CMD_READ_UNIQUE_ID     0x4Bu
#define W25Q_CMD_READ_SFDP          0x5Au
#define W25Q_CMD_PAGE_PROGRAM       0x02u
#define W25Q_CMD_QUAD_PAGE_PROGRAM  0x32u
#define W25Q_CMD_FAST_READ          0x0Bu
#define W25Q_CMD_QUAD_READ          0x6Bu
#define W25Q_CMD_QUAD_IO_READ       0xEBu
#define W25Q_CMD_SECTOR_ERASE_4K    0x20u
#define W25Q_CMD_BLOCK_ERASE_32K    0x52u
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8u
#define W25Q_CMD_CHIP_ERASE         0xC7u
#define W25Q_CMD_ENTER_4BYTE        0xB7u
#define W25Q_CMD_EXIT_4BYTE         0xE9u
#define W25Q_CMD_ENABLE_RESET       0x66u
#define W25Q_CMD_RESET_DEVICE       0x99u
#define W25Q_CMD_READ_MANUF_DEVICE  0x90u

#define W25Q_STATUS_BUSY_MASK       0x01u
#define W25Q_STATUS_WEL_MASK        0x02u
#define W25Q_STATUS_QE_MASK         0x02u

typedef struct
{
	cqspi_dev_t qspi;
	cqspi_indirect_read_config_t read_cfg;
	cqspi_indirect_write_config_t write_cfg;
	cqspi_indirect_write_config_t write_quad_cfg;
	w25qxx_info_t info;
	bool initialized;
	bool quad_enabled;
	bool addr4b;
	uint8_t read_mode;
} w25qxx_context_t;

static w25qxx_context_t g_w25q_ctx;

static int w25q_apply_read_config(void)
{
	g_w25q_ctx.read_cfg.addr_bytes = g_w25q_ctx.addr4b ? 4u : 3u;
	return cqspi_configure_indirect_read(&g_w25q_ctx.qspi, &g_w25q_ctx.read_cfg);
}

static int w25q_apply_write_config(bool quad)
{
	cqspi_indirect_write_config_t *cfg = quad ? &g_w25q_ctx.write_quad_cfg : &g_w25q_ctx.write_cfg;
	cfg->addr_bytes = g_w25q_ctx.addr4b ? 4u : 3u;
	return cqspi_configure_indirect_write(&g_w25q_ctx.qspi, cfg);
}

static int w25q_send_simple_cmd(uint8_t opcode)
{
	cqspi_stig_cmd_t cmd = {
		.opcode = opcode,
	};
	return cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, NULL, NULL);
}

static int w25q_read_status(uint8_t opcode, uint8_t *value)
{
	if (!value)
	{
		return -1;
	}
	cqspi_stig_cmd_t cmd = {
		.opcode = opcode,
		.read_len = 1u,
	};
	return cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, value, NULL);
}

static int w25q_write_status_reg2(uint8_t value)
{
	cqspi_stig_cmd_t cmd = {
		.opcode = W25Q_CMD_WRITE_SR2,
		.write_len = 1u,
	};
	return cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, NULL, &value);
}

static int w25q_wait_ready(uint32_t timeout_us)
{
	if (timeout_us == 0u)
	{
		timeout_us = W25QXX_DEFAULT_TIMEOUT_US;
	}
	const uint32_t step_us = 50u;
	uint32_t loops = (timeout_us / step_us) + 1u;
	while (loops--)
	{
		uint8_t sr1 = 0u;
		if (w25q_read_status(W25Q_CMD_READ_SR1, &sr1) != 0)
		{
			return -1;
		}
		if ((sr1 & W25Q_STATUS_BUSY_MASK) == 0u)
		{
			return 0;
		}
	}
	return -1;
}

static int w25q_write_enable(void)
{
	if (w25q_send_simple_cmd(W25Q_CMD_WRITE_ENABLE) != 0)
	{
		return -1;
	}
	uint8_t sr1 = 0u;
	if (w25q_read_status(W25Q_CMD_READ_SR1, &sr1) != 0)
	{
		return -1;
	}
	return (sr1 & W25Q_STATUS_WEL_MASK) ? 0 : -1;
}

static uint32_t w25q_id_capacity_to_size(uint8_t capacity)
{
	if (capacity < 16u || capacity > 31u)
	{
		return 0u;
	}
	if (capacity >= 32u)
	{
		return 0u;
	}
	return (1u << capacity);
}

static int w25q_set_quad_enable(bool enable)
{
	uint8_t sr2 = 0u;
	if (w25q_read_status(W25Q_CMD_READ_SR2, &sr2) != 0)
	{
		return -1;
	}
	bool current = (sr2 & W25Q_STATUS_QE_MASK) != 0u;
	if (current == enable)
	{
		return 0;
	}
	if (w25q_write_enable() != 0)
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
	if (w25q_write_status_reg2(sr2) != 0)
	{
		return -1;
	}
	if (w25q_wait_ready(W25QXX_DEFAULT_TIMEOUT_US) != 0)
	{
		return -1;
	}
	g_w25q_ctx.quad_enabled = enable;
	g_w25q_ctx.info.quad_enabled = enable;
	return 0;
}

static int w25q_set_address_length(bool addr4b)
{
	if (addr4b == g_w25q_ctx.addr4b)
	{
		return 0;
	}
	uint8_t opcode = addr4b ? W25Q_CMD_ENTER_4BYTE : W25Q_CMD_EXIT_4BYTE;
	if (w25q_send_simple_cmd(opcode) != 0)
	{
		return -1;
	}
	g_w25q_ctx.addr4b = addr4b;
	g_w25q_ctx.info.addr4b = addr4b;
	return 0;
}

static void w25q_fill_default_configs(void)
{
	g_w25q_ctx.read_cfg.opcode = W25Q_CMD_FAST_READ;
	g_w25q_ctx.read_cfg.addr_bytes = 3u;
	g_w25q_ctx.read_cfg.instr_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.read_cfg.addr_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.read_cfg.data_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.read_cfg.dummy_cycles = 8u;
	g_w25q_ctx.read_cfg.mode_enable = false;
	g_w25q_ctx.read_cfg.mode_bits = 0u;

	g_w25q_ctx.write_cfg.opcode = W25Q_CMD_PAGE_PROGRAM;
	g_w25q_ctx.write_cfg.addr_bytes = 3u;
	g_w25q_ctx.write_cfg.instr_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.write_cfg.addr_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.write_cfg.data_width = CQSPI_BUSWIDTH_1;
	g_w25q_ctx.write_cfg.mode_enable = false;
	g_w25q_ctx.write_cfg.mode_bits = 0u;

	g_w25q_ctx.write_quad_cfg = g_w25q_ctx.write_cfg;
	g_w25q_ctx.write_quad_cfg.opcode = W25Q_CMD_QUAD_PAGE_PROGRAM;
	g_w25q_ctx.write_quad_cfg.data_width = CQSPI_BUSWIDTH_4;
}

static int w25q_configure_bus(uint32_t sclk_hz)
{
	if (cqspi_set_chip_select(&g_w25q_ctx.qspi, 0u) != 0)
	{
		return -1;
	}
	if (cqspi_configure_clock(&g_w25q_ctx.qspi, sclk_hz) != 0)
	{
		return -1;
	}
	const cqspi_timing_cfg_t timing = {
		.tshsl_ns = 50u,
		.tchsh_ns = 50u,
		.tslch_ns = 50u,
		.tsd2d_ns = 50u,
		.read_delay = 2u,
		.tx_delay = 0u,
		.bypass = false,
		.sample_edge = false,
		.dqs_enable = false,
	};
	if (cqspi_configure_timing(&g_w25q_ctx.qspi, &timing) != 0)
	{
		return -1;
	}
	return 0;
}

static int w25q_read_jedec_id(uint8_t id[3])
{
	cqspi_stig_cmd_t cmd = {
		.opcode = W25Q_CMD_READ_JEDEC_ID,
		.read_len = 3u,
	};
	return cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, id, NULL);
}

static int w25q_stig_read(uint8_t opcode, uint32_t address, uint8_t addr_bytes, uint8_t dummy_cycles, uint8_t *buffer, uint32_t length)
{
	while (length > 0u)
	{
		uint32_t chunk = length;
		if (chunk > CQSPI_STIG_MEM_BANK_MAX_BYTES)
		{
			chunk = CQSPI_STIG_MEM_BANK_MAX_BYTES;
		}
		cqspi_stig_cmd_t cmd = {
			.opcode = opcode,
			.addr_bytes = addr_bytes,
			.address = address,
			.dummy_cycles = dummy_cycles,
			.read_len = (uint8_t)chunk,
		};
		if (cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, buffer, NULL) != 0)
		{
			return -1;
		}
		address += chunk;
		buffer += chunk;
		length -= chunk;
	}
	return 0;
}

int w25qxx_init(w25qxx_info_t *info, bool want_quad, bool want_4byte_addr)
{
	if (!info)
	{
		return -1;
	}

	memset(info, 0, sizeof(*info));

	memset(&g_w25q_ctx, 0, sizeof(g_w25q_ctx));

	rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
	rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
	rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_QSPIFLASH, false);
	rcc_set_cortex_m4_ahb_reset(RCC_CM4_AHB_QSPIFLASH, false);

	uint32_t ref_clk = rcc_get_clock(RCC_CLOCK_AHB);
	if (ref_clk == 0u)
	{
		ref_clk = SystemCoreClock;
	}

	cqspi_config_t cfg = {
		.reg_base = QSPI_CFG_BASE,
		.ahb_base = M4_SLV_FLASH_BASE,
		.ref_clk_hz = ref_clk,
		.trigger_address = M4_SLV_FLASH_BASE,
		.sram_partition = CQSPI_SRAM_TOTAL_LOCATIONS / 2u,
		.fifo_width_bytes = 4u,
		.decode_cs = false,
	};

	if (cqspi_init(&g_w25q_ctx.qspi, &cfg) != 0)
	{
		return -1;
	}

	g_w25q_ctx.qspi.indirect_timeout_us = W25QXX_DEFAULT_TIMEOUT_US;
	g_w25q_ctx.qspi.read_timeout_us = W25QXX_READ_TIMEOUT_US;

	w25q_fill_default_configs();

	if (w25q_configure_bus(W25QXX_DEFAULT_SCLK_HZ) != 0)
	{
		return -1;
	}
	if (w25q_apply_read_config() != 0)
	{
		return -1;
	}
	if (w25q_apply_write_config(false) != 0)
	{
		return -1;
	}

	uint8_t id[3] = {0};
	if (w25q_read_jedec_id(id) != 0)
	{
		return -1;
	}

	uint32_t size = w25q_id_capacity_to_size(id[2]);
	info->manuf_id = id[0];
	info->memory_type = id[1];
	info->capacity = id[2];
	info->size_bytes = size;
	info->page_size = W25QXX_PAGE_SIZE;
	info->sector_size = W25QXX_SECTOR_SIZE;
	info->block_size = W25QXX_BLOCK64_SIZE;
	info->quad_enabled = false;
	info->addr4b = false;
	memset(info->unique_id, 0, sizeof(info->unique_id));

	memcpy(&g_w25q_ctx.info, info, sizeof(*info));
	g_w25q_ctx.initialized = true;
	g_w25q_ctx.read_mode = 0u;

	if (want_4byte_addr && size > (1u << 24))
	{
		if (w25q_set_address_length(true) != 0)
		{
			return -1;
		}
	}

	if (want_quad)
	{
		if (w25q_set_quad_enable(true) != 0)
		{
			return -1;
		}
	}
	else
	{
		uint8_t sr2 = 0u;
		if (w25q_read_status(W25Q_CMD_READ_SR2, &sr2) == 0)
		{
			bool quad = (sr2 & W25Q_STATUS_QE_MASK) != 0u;
			g_w25q_ctx.quad_enabled = quad;
			g_w25q_ctx.info.quad_enabled = quad;
		}
	}

	g_w25q_ctx.info.addr4b = g_w25q_ctx.addr4b;

	uint8_t uid[8] = {0};
	if (w25qxx_read_unique_id(uid, sizeof(uid)) == 0)
	{
		memcpy(g_w25q_ctx.info.unique_id, uid, sizeof(uid));
	}

	*info = g_w25q_ctx.info;

	return 0;
}

int w25qxx_read_device_id(uint8_t *dev_id)
{
	if (!g_w25q_ctx.initialized || !dev_id)
	{
		return -1;
	}
	uint8_t mfg = 0u;
	uint8_t dev = 0u;
	if (w25qxx_read_manufacturer_device_id(&mfg, &dev) != 0)
	{
		return -1;
	}
	*dev_id = dev;
	return 0;
}

int w25qxx_read_manufacturer_device_id(uint8_t *mfg_id, uint8_t *dev_id)
{
	if (!g_w25q_ctx.initialized || !mfg_id || !dev_id)
	{
		return -1;
	}
	cqspi_stig_cmd_t cmd = {
		.opcode = W25Q_CMD_READ_MANUF_DEVICE,
		.addr_bytes = 3u,
		.address = 0u,
		.read_len = 2u,
	};
	uint8_t buffer[2] = {0};
	if (cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, buffer, NULL) != 0)
	{
		return -1;
	}
	*mfg_id = buffer[0];
	*dev_id = buffer[1];
	return 0;
}

int w25qxx_read_unique_id(uint8_t *uid, uint32_t len)
{
	if (!g_w25q_ctx.initialized || !uid || len == 0u)
	{
		return -1;
	}
	if (len > 8u)
	{
		len = 8u;
	}
	cqspi_stig_cmd_t cmd = {
		.opcode = W25Q_CMD_READ_UNIQUE_ID,
		.dummy_cycles = 32u,
		.read_len = (uint8_t)len,
	};
	return cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, uid, NULL);
}

int w25qxx_read_sfdp(uint32_t addr, uint8_t *buf, uint32_t len)
{
	if (!g_w25q_ctx.initialized || !buf || len == 0u)
	{
		return -1;
	}
	uint8_t addr_bytes = g_w25q_ctx.addr4b ? 4u : 3u;
	return w25q_stig_read(W25Q_CMD_READ_SFDP, addr, addr_bytes, 8u, buf, len);
}

int w25qxx_configure_read_mode(int mode)
{
	if (!g_w25q_ctx.initialized)
	{
		return -1;
	}
	cqspi_indirect_read_config_t cfg = g_w25q_ctx.read_cfg;
	switch (mode)
	{
	case 0:
		cfg.opcode = W25Q_CMD_FAST_READ;
		cfg.instr_width = CQSPI_BUSWIDTH_1;
		cfg.addr_width = CQSPI_BUSWIDTH_1;
		cfg.data_width = CQSPI_BUSWIDTH_1;
		cfg.dummy_cycles = 8u;
		cfg.mode_enable = false;
		cfg.mode_bits = 0u;
		break;
	case 1:
		if (!g_w25q_ctx.quad_enabled)
		{
			return -1;
		}
		cfg.opcode = W25Q_CMD_QUAD_READ;
		cfg.instr_width = CQSPI_BUSWIDTH_1;
		cfg.addr_width = CQSPI_BUSWIDTH_1;
		cfg.data_width = CQSPI_BUSWIDTH_4;
		cfg.dummy_cycles = 8u;
		cfg.mode_enable = false;
		cfg.mode_bits = 0u;
		break;
	case 2:
		if (!g_w25q_ctx.quad_enabled)
		{
			return -1;
		}
		cfg.opcode = W25Q_CMD_QUAD_IO_READ;
		cfg.instr_width = CQSPI_BUSWIDTH_1;
		cfg.addr_width = CQSPI_BUSWIDTH_4;
		cfg.data_width = CQSPI_BUSWIDTH_4;
		cfg.dummy_cycles = 6u;
		cfg.mode_enable = false;
		cfg.mode_bits = 0u;
		break;
	default:
		return -1;
	}
	g_w25q_ctx.read_cfg = cfg;
	g_w25q_ctx.read_mode = (uint8_t)mode;
	return w25q_apply_read_config();
}

int w25qxx_read(uint32_t addr, void *buf, uint32_t len)
{
	if (!g_w25q_ctx.initialized || !buf || len == 0u)
	{
		return -1;
	}
	if (w25q_apply_read_config() != 0)
	{
		return -1;
	}
	return cqspi_indirect_read(&g_w25q_ctx.qspi, addr, buf, len, W25QXX_READ_TIMEOUT_US);
}

static int w25q_page_write_common(uint32_t addr, const void *buf, uint32_t len, bool quad)
{
	if (!g_w25q_ctx.initialized || !buf || len == 0u || len > W25QXX_PAGE_SIZE)
	{
		return -1;
	}
	if (((addr & (W25QXX_PAGE_SIZE - 1u)) + len) > W25QXX_PAGE_SIZE)
	{
		return -1;
	}
	if (quad && !g_w25q_ctx.quad_enabled)
	{
		return -1;
	}
	if (w25q_write_enable() != 0)
	{
		return -1;
	}
	if (w25q_apply_write_config(quad) != 0)
	{
		return -1;
	}
	if (cqspi_indirect_write(&g_w25q_ctx.qspi, addr, buf, len, W25QXX_DEFAULT_TIMEOUT_US) != 0)
	{
		return -1;
	}
	return w25q_wait_ready(W25QXX_DEFAULT_TIMEOUT_US);
}

int w25qxx_write_page(uint32_t addr, const void *buf, uint32_t len)
{
	return w25q_page_write_common(addr, buf, len, false);
}

int w25qxx_write_page_quad(uint32_t addr, const void *buf, uint32_t len)
{
	return w25q_page_write_common(addr, buf, len, true);
}

static int w25q_erase_common(uint32_t addr, uint32_t size, uint8_t opcode)
{
	if (!g_w25q_ctx.initialized)
	{
		return -1;
	}
	if ((addr % size) != 0u)
	{
		return -1;
	}
	if (w25q_write_enable() != 0)
	{
		return -1;
	}
	cqspi_stig_cmd_t cmd = {
		.opcode = opcode,
		.addr_bytes = g_w25q_ctx.addr4b ? 4u : 3u,
		.address = addr,
	};
	if (cqspi_stig_execute(&g_w25q_ctx.qspi, &cmd, NULL, NULL) != 0)
	{
		return -1;
	}
	return w25q_wait_ready(W25QXX_DEFAULT_TIMEOUT_US);
}

int w25qxx_erase_4k(uint32_t addr)
{
	return w25q_erase_common(addr, W25QXX_SECTOR_SIZE, W25Q_CMD_SECTOR_ERASE_4K);
}

int w25qxx_erase_32k(uint32_t addr)
{
	return w25q_erase_common(addr, W25QXX_BLOCK32_SIZE, W25Q_CMD_BLOCK_ERASE_32K);
}

int w25qxx_erase_64k(uint32_t addr)
{
	return w25q_erase_common(addr, W25QXX_BLOCK64_SIZE, W25Q_CMD_BLOCK_ERASE_64K);
}

int w25qxx_chip_erase(void)
{
	if (!g_w25q_ctx.initialized)
	{
		return -1;
	}
	if (w25q_write_enable() != 0)
	{
		return -1;
	}
	if (w25q_send_simple_cmd(W25Q_CMD_CHIP_ERASE) != 0)
	{
		return -1;
	}
	return w25q_wait_ready(60000000u);
}

int w25qxx_software_reset(void)
{
	if (!g_w25q_ctx.initialized)
	{
		return -1;
	}
	if (w25q_send_simple_cmd(W25Q_CMD_ENABLE_RESET) != 0)
	{
		return -1;
	}
	return w25q_send_simple_cmd(W25Q_CMD_RESET_DEVICE);
}

