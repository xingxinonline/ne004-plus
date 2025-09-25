#include "qspi_cadence.h"
#include <stdint.h>
#include <string.h>

#define CQSPI_DEFAULT_TIMEOUT_US 500000u
#define CQSPI_DEFAULT_READ_TIMEOUT_US 10000u

static inline volatile uint32_t *cqspi_reg_ptr(const cqspi_dev_t *dev, uint32_t offset)
{
	return (volatile uint32_t *)(dev->regs + offset);
}

static void cqspi_controller_enable(cqspi_dev_t *dev, bool enable);

static uint32_t cqspi_readl(const cqspi_dev_t *dev, uint32_t offset)
{
	return *cqspi_reg_ptr(dev, offset);
}

static void cqspi_writel(const cqspi_dev_t *dev, uint32_t offset, uint32_t value)
{
	*cqspi_reg_ptr(dev, offset) = value;
}

static uint32_t cqspi_timeout_to_loops(const cqspi_dev_t *dev, uint32_t timeout_us)
{
	uint64_t ref = dev->ref_clk_hz ? (uint64_t)dev->ref_clk_hz : 1000000ull;
	uint64_t loops = (ref * timeout_us) / 1000000ull;
	if (loops == 0ull)
	{
		loops = timeout_us ? 1ull : 1ull;
	}
	if (loops > 0xFFFFFFFFull)
	{
		loops = 0xFFFFFFFFull;
	}
	return (uint32_t)loops;
}

static int cqspi_poll(const cqspi_dev_t *dev, uint32_t offset, uint32_t mask, bool zero, uint32_t timeout_us)
{
	uint32_t loops = cqspi_timeout_to_loops(dev, timeout_us ? timeout_us : dev->indirect_timeout_us);
	if (loops == 0u)
	{
		loops = 1u;
	}
	while (loops--)
	{
		uint32_t value = cqspi_readl(dev, offset);
		if (zero)
		{
			if ((value & mask) == 0u)
			{
				return 0;
			}
		}
		else
		{
			if ((value & mask) == mask)
			{
				return 0;
			}
		}
	}
	return -1;
}

int cqspi_wait_idle(const cqspi_dev_t *dev, uint32_t timeout_us)
{
	if (!dev)
	{
		return -1;
	}
	return cqspi_poll(dev, CQSPI_REG_CONFIG, CQSPI_CFG_IDLE, false, timeout_us ? timeout_us : dev->indirect_timeout_us);
}

static int cqspi_direct_access_begin(cqspi_dev_t *dev, bool *restore_enabled)
{
	if (!dev || !restore_enabled || !dev->regs)
	{
		return -1;
	}
	*restore_enabled = dev->is_enabled;
	if (!dev->is_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	uint32_t cfg = cqspi_readl(dev, CQSPI_REG_CONFIG);
	cfg |= CQSPI_CFG_ENABLE | CQSPI_CFG_DIRECT;
	cqspi_writel(dev, CQSPI_REG_CONFIG, cfg);
	return cqspi_wait_idle(dev, dev->read_timeout_us);
}

static int cqspi_direct_access_end(cqspi_dev_t *dev, bool restore_enabled)
{
	if (!dev || !dev->regs)
	{
		return -1;
	}
	uint32_t cfg = cqspi_readl(dev, CQSPI_REG_CONFIG);
	cfg &= ~CQSPI_CFG_DIRECT;
	cqspi_writel(dev, CQSPI_REG_CONFIG, cfg);
	int ret = cqspi_wait_idle(dev, dev->read_timeout_us);
	if (!restore_enabled)
	{
		cqspi_controller_enable(dev, false);
	}
	return ret;
}

static void cqspi_controller_enable(cqspi_dev_t *dev, bool enable)
{
	if (!dev)
	{
		return;
	}
	uint32_t reg = cqspi_readl(dev, CQSPI_REG_CONFIG);
	if (enable)
	{
		if (!(reg & CQSPI_CFG_ENABLE))
		{
			reg |= CQSPI_CFG_ENABLE;
			cqspi_writel(dev, CQSPI_REG_CONFIG, reg);
		}
		dev->is_enabled = true;
	}
	else
	{
		if (reg & CQSPI_CFG_ENABLE)
		{
			reg &= ~CQSPI_CFG_ENABLE;
			cqspi_writel(dev, CQSPI_REG_CONFIG, reg);
		}
		dev->is_enabled = false;
	}
}

static uint32_t cqspi_div_round_up_u32(uint32_t dividend, uint32_t divisor)
{
	if (divisor == 0u)
	{
		return 0u;
	}
	return (dividend + divisor - 1u) / divisor;
}

static uint32_t cqspi_ns_to_ticks(const cqspi_dev_t *dev, uint32_t ns)
{
	if (!dev || dev->ref_clk_hz == 0u || ns == 0u)
	{
		return 0u;
	}
	uint64_t ticks = ((uint64_t)dev->ref_clk_hz * ns) + 999999ull;
	ticks /= 1000000ull;
	if (ticks > 0xFFull)
	{
		ticks = 0xFFull;
	}
	return (uint32_t)ticks;
}

static uint32_t cqspi_get_rd_level(const cqspi_dev_t *dev)
{
	uint32_t level = cqspi_readl(dev, CQSPI_REG_SDRAMLEVEL);
	level >>= CQSPI_SDRAMLEVEL_RD_LSB;
	level &= CQSPI_SDRAMLEVEL_RD_MASK;
	return level;
}

static uint32_t cqspi_get_wr_level(const cqspi_dev_t *dev)
{
	uint32_t level = cqspi_readl(dev, CQSPI_REG_SDRAMLEVEL);
	level >>= CQSPI_SDRAMLEVEL_WR_LSB;
	level &= CQSPI_SDRAMLEVEL_WR_MASK;
	return level;
}

static int cqspi_exec_flash_cmd(cqspi_dev_t *dev, uint32_t reg, uint32_t timeout_us)
{
	cqspi_writel(dev, CQSPI_REG_CMDCTRL, reg);
	cqspi_writel(dev, CQSPI_REG_CMDCTRL, reg | CQSPI_CMDCTRL_EXECUTE);
	int ret = cqspi_poll(dev, CQSPI_REG_CMDCTRL, CQSPI_CMDCTRL_INPROGRESS, true, timeout_us ? timeout_us : dev->read_timeout_us);
	if (ret != 0)
	{
		return ret;
	}
	return cqspi_wait_idle(dev, timeout_us ? timeout_us : dev->read_timeout_us);
}

int cqspi_init(cqspi_dev_t *dev, const cqspi_config_t *cfg)
{
	if (!dev || !cfg || cfg->reg_base == 0u)
	{
		return -1;
	}
	dev->regs = (volatile uint8_t *)cfg->reg_base;
	dev->ahb = cfg->ahb_base ? (volatile uint8_t *)cfg->ahb_base : NULL;
	dev->ref_clk_hz = cfg->ref_clk_hz;
	dev->current_sclk_hz = 0u;
	dev->indirect_timeout_us = CQSPI_DEFAULT_TIMEOUT_US;
	dev->read_timeout_us = CQSPI_DEFAULT_READ_TIMEOUT_US;
	dev->fifo_width_bytes = cfg->fifo_width_bytes ? cfg->fifo_width_bytes : 4u;
	uint32_t total_locations = CQSPI_SRAM_TOTAL_LOCATIONS;
	uint32_t partition = cfg->sram_partition & CQSPI_SRAM_PARTITION_MASK;
	if (partition == 0u || partition >= total_locations)
	{
		partition = total_locations / 2u;
		if (partition == 0u)
		{
			partition = 1u;
		}
	}
	dev->sram_partition_words = partition;
	dev->sram_write_partition_words = (total_locations > partition) ? (total_locations - partition) : 1u;
	dev->trigger_address = cfg->trigger_address;
	dev->current_cs = 0xFFu;
	dev->addr_bytes = 3u;
	dev->decode_cs = cfg->decode_cs;
	dev->is_enabled = false;

	cqspi_controller_enable(dev, false);

	uint32_t config_reg = 0u;
	if (dev->decode_cs)
	{
		config_reg |= CQSPI_CFG_DECODE;
	}
	cqspi_writel(dev, CQSPI_REG_CONFIG, config_reg);

	if (dev->trigger_address != 0u)
	{
		cqspi_writel(dev, CQSPI_REG_INDIRECTTRIGGER, dev->trigger_address);
	}

	cqspi_writel(dev, CQSPI_REG_SRAMPARTITION, dev->sram_partition_words & CQSPI_SRAM_PARTITION_MASK);
	cqspi_writel(dev, CQSPI_REG_RD_DATA_CAPTURE, CQSPI_RD_CAPTURE_BYPASS);
	uint32_t size_reg = cqspi_readl(dev, CQSPI_REG_SIZE);
	size_reg &= ~(CQSPI_SIZE_ADDR_MASK << CQSPI_SIZE_ADDR_LSB);
	size_reg |= ((dev->addr_bytes ? dev->addr_bytes - 1u : 0u) & CQSPI_SIZE_ADDR_MASK) << CQSPI_SIZE_ADDR_LSB;
	cqspi_writel(dev, CQSPI_REG_SIZE, size_reg);
	cqspi_writel(dev, CQSPI_REG_REMAP, 0u);
	cqspi_writel(dev, CQSPI_REG_MODE_BIT, 0u);
	cqspi_writel(dev, CQSPI_REG_WR_COMPLETION, CQSPI_WR_POLL_DISABLE);
	cqspi_writel(dev, CQSPI_REG_DMAPERIPH, 0u);
	cqspi_writel(dev, CQSPI_REG_TXTHRESH, 1u & CQSPI_TX_THRESH_MASK);
	cqspi_writel(dev, CQSPI_REG_RXTHRESH, 1u & CQSPI_RX_THRESH_MASK);
	cqspi_writel(dev, CQSPI_REG_INDIRECTRDWATERMARK, 1u);
	cqspi_writel(dev, CQSPI_REG_INDIRECTWRWATERMARK, 1u);
	cqspi_writel(dev, CQSPI_REG_IRQMASK, 0u);
	cqspi_writel(dev, CQSPI_REG_IRQSTATUS, CQSPI_IRQ_STATUS_MASK);

	return 0;
}

void cqspi_deinit(cqspi_dev_t *dev)
{
	if (!dev)
	{
		return;
	}
	cqspi_controller_enable(dev, false);
	dev->regs = NULL;
	dev->ahb = NULL;
	dev->current_sclk_hz = 0u;
	dev->is_enabled = false;
}

int cqspi_set_chip_select(cqspi_dev_t *dev, uint8_t cs)
{
	if (!dev || cs > CQSPI_CFG_CHIPSELECT_MASK)
	{
		return -1;
	}
	bool was_enabled = dev->is_enabled;
	cqspi_controller_enable(dev, false);
	uint32_t reg = cqspi_readl(dev, CQSPI_REG_CONFIG);
	reg &= ~(CQSPI_CFG_CHIPSELECT_MASK << CQSPI_CFG_CHIPSELECT_LSB);
	reg |= ((uint32_t)cs & CQSPI_CFG_CHIPSELECT_MASK) << CQSPI_CFG_CHIPSELECT_LSB;
	if (dev->decode_cs)
	{
		reg |= CQSPI_CFG_DECODE;
	}
	else
	{
		reg &= ~CQSPI_CFG_DECODE;
	}
	cqspi_writel(dev, CQSPI_REG_CONFIG, reg);
	if (was_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	dev->current_cs = cs;
	return 0;
}

int cqspi_configure_clock(cqspi_dev_t *dev, uint32_t sclk_hz)
{
	if (!dev || sclk_hz == 0u || dev->ref_clk_hz == 0u)
	{
		return -1;
	}
	uint64_t denom = (uint64_t)sclk_hz * 2ull;
	uint64_t temp = (uint64_t)dev->ref_clk_hz + denom - 1ull;
	uint64_t quotient = temp / denom;
	if (quotient == 0ull)
	{
		quotient = 1ull;
	}
	uint32_t div = (uint32_t)(quotient - 1ull);
	if (div > CQSPI_CFG_BAUD_MASK)
	{
		div = CQSPI_CFG_BAUD_MASK;
	}
	bool was_enabled = dev->is_enabled;
	cqspi_controller_enable(dev, false);
	uint32_t reg = cqspi_readl(dev, CQSPI_REG_CONFIG);
	reg &= ~(CQSPI_CFG_BAUD_MASK << CQSPI_CFG_BAUD_LSB);
	reg |= (div & CQSPI_CFG_BAUD_MASK) << CQSPI_CFG_BAUD_LSB;
	if (dev->decode_cs)
	{
		reg |= CQSPI_CFG_DECODE;
	}
	else
	{
		reg &= ~CQSPI_CFG_DECODE;
	}
	cqspi_writel(dev, CQSPI_REG_CONFIG, reg);
	if (was_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	uint32_t actual_div = div + 1u;
	if (actual_div == 0u)
	{
		actual_div = 1u;
	}
	dev->current_sclk_hz = dev->ref_clk_hz / (2u * actual_div);
	return 0;
}

int cqspi_configure_timing(cqspi_dev_t *dev, const cqspi_timing_cfg_t *timing)
{
	if (!dev || !timing)
	{
		return -1;
	}
	uint32_t sclk = dev->current_sclk_hz;
	if (sclk == 0u)
	{
		sclk = dev->ref_clk_hz ? dev->ref_clk_hz : 1u;
	}
	uint32_t tsclk = cqspi_div_round_up_u32(dev->ref_clk_hz ? dev->ref_clk_hz : 1u, sclk);
	uint32_t tshsl = cqspi_ns_to_ticks(dev, timing->tshsl_ns);
	if (tshsl < tsclk)
	{
		tshsl = tsclk;
	}
	if (tshsl > CQSPI_DELAY_TSHSL_MASK)
	{
		tshsl = CQSPI_DELAY_TSHSL_MASK;
	}
	uint32_t tchsh = cqspi_ns_to_ticks(dev, timing->tchsh_ns);
	if (tchsh > CQSPI_DELAY_TCHSH_MASK)
	{
		tchsh = CQSPI_DELAY_TCHSH_MASK;
	}
	uint32_t tslch = cqspi_ns_to_ticks(dev, timing->tslch_ns);
	if (tslch > CQSPI_DELAY_TSLCH_MASK)
	{
		tslch = CQSPI_DELAY_TSLCH_MASK;
	}
	uint32_t tsd2d = cqspi_ns_to_ticks(dev, timing->tsd2d_ns);
	if (tsd2d > CQSPI_DELAY_TSD2D_MASK)
	{
		tsd2d = CQSPI_DELAY_TSD2D_MASK;
	}
	bool was_enabled = dev->is_enabled;
	cqspi_controller_enable(dev, false);
	uint32_t delay_reg = ((tshsl & CQSPI_DELAY_TSHSL_MASK) << CQSPI_DELAY_TSHSL_LSB) |
						 ((tchsh & CQSPI_DELAY_TCHSH_MASK) << CQSPI_DELAY_TCHSH_LSB) |
						 ((tslch & CQSPI_DELAY_TSLCH_MASK) << CQSPI_DELAY_TSLCH_LSB) |
						 ((tsd2d & CQSPI_DELAY_TSD2D_MASK) << CQSPI_DELAY_TSD2D_LSB);
	cqspi_writel(dev, CQSPI_REG_DELAY, delay_reg);
	uint32_t capture_reg = cqspi_readl(dev, CQSPI_REG_RD_DATA_CAPTURE);
	if (timing->bypass)
	{
		capture_reg |= CQSPI_RD_CAPTURE_BYPASS;
	}
	else
	{
		capture_reg &= ~CQSPI_RD_CAPTURE_BYPASS;
	}
	capture_reg &= ~(CQSPI_RD_CAPTURE_DELAY_MASK << CQSPI_RD_CAPTURE_DELAY_LSB);
	capture_reg |= ((uint32_t)timing->read_delay & CQSPI_RD_CAPTURE_DELAY_MASK) << CQSPI_RD_CAPTURE_DELAY_LSB;
	if (timing->sample_edge)
	{
		capture_reg |= CQSPI_RD_CAPTURE_SAMPLE_EDGE;
	}
	else
	{
		capture_reg &= ~CQSPI_RD_CAPTURE_SAMPLE_EDGE;
	}
	if (timing->dqs_enable)
	{
		capture_reg |= CQSPI_RD_CAPTURE_DQS_EN;
	}
	else
	{
		capture_reg &= ~CQSPI_RD_CAPTURE_DQS_EN;
	}
	capture_reg &= ~(CQSPI_RD_CAPTURE_TX_DELAY_MASK << CQSPI_RD_CAPTURE_TX_DELAY_LSB);
	capture_reg |= ((uint32_t)timing->tx_delay & CQSPI_RD_CAPTURE_TX_DELAY_MASK) << CQSPI_RD_CAPTURE_TX_DELAY_LSB;
	cqspi_writel(dev, CQSPI_REG_RD_DATA_CAPTURE, capture_reg);
	if (was_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	return 0;
}

int cqspi_configure_indirect_read(cqspi_dev_t *dev, const cqspi_indirect_read_config_t *cfg)
{
	if (!dev || !cfg || cfg->addr_bytes == 0u || cfg->addr_bytes > 4u)
	{
		return -1;
	}
	if (cfg->dummy_cycles > CQSPI_RD_DUMMY_MASK)
	{
		return -1;
	}
	bool was_enabled = dev->is_enabled;
	cqspi_controller_enable(dev, false);
	uint32_t rd_instr = ((uint32_t)cfg->opcode << CQSPI_RD_OPCODE_LSB) |
						(((uint32_t)cfg->instr_width & CQSPI_RD_TYPE_INSTR_MASK) << CQSPI_RD_TYPE_INSTR_LSB) |
						(((uint32_t)cfg->addr_width & CQSPI_RD_TYPE_ADDR_MASK) << CQSPI_RD_TYPE_ADDR_LSB) |
						(((uint32_t)cfg->data_width & CQSPI_RD_TYPE_DATA_MASK) << CQSPI_RD_TYPE_DATA_LSB);
	if (cfg->dummy_cycles)
	{
		rd_instr |= ((uint32_t)cfg->dummy_cycles & CQSPI_RD_DUMMY_MASK) << CQSPI_RD_DUMMY_LSB;
	}
	if (cfg->mode_enable)
	{
		rd_instr |= 1u << CQSPI_RD_MODE_EN_LSB;
		uint32_t mode_reg = cqspi_readl(dev, CQSPI_REG_MODE_BIT);
		mode_reg &= ~CQSPI_MODE_BITS_MASK;
		mode_reg |= (uint32_t)cfg->mode_bits & CQSPI_MODE_BITS_MASK;
		cqspi_writel(dev, CQSPI_REG_MODE_BIT, mode_reg);
	}
	cqspi_writel(dev, CQSPI_REG_RD_INSTR, rd_instr);
	dev->addr_bytes = cfg->addr_bytes;
	uint32_t size_reg = cqspi_readl(dev, CQSPI_REG_SIZE);
	size_reg &= ~(CQSPI_SIZE_ADDR_MASK << CQSPI_SIZE_ADDR_LSB);
	size_reg |= ((uint32_t)(cfg->addr_bytes - 1u) & CQSPI_SIZE_ADDR_MASK) << CQSPI_SIZE_ADDR_LSB;
	cqspi_writel(dev, CQSPI_REG_SIZE, size_reg);
	if (was_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	return 0;
}

int cqspi_configure_indirect_write(cqspi_dev_t *dev, const cqspi_indirect_write_config_t *cfg)
{
	if (!dev || !cfg || cfg->addr_bytes == 0u || cfg->addr_bytes > 4u)
	{
		return -1;
	}
	bool was_enabled = dev->is_enabled;
	cqspi_controller_enable(dev, false);
	uint32_t wr_instr = ((uint32_t)cfg->opcode << CQSPI_WR_OPCODE_LSB) |
						(((uint32_t)cfg->addr_width & CQSPI_WR_TYPE_ADDR_MASK) << CQSPI_WR_TYPE_ADDR_LSB) |
						(((uint32_t)cfg->data_width & CQSPI_WR_TYPE_DATA_MASK) << CQSPI_WR_TYPE_DATA_LSB);
	cqspi_writel(dev, CQSPI_REG_WR_INSTR, wr_instr);
	uint32_t rd_instr = cqspi_readl(dev, CQSPI_REG_RD_INSTR);
	rd_instr &= ~(((uint32_t)CQSPI_RD_TYPE_INSTR_MASK << CQSPI_RD_TYPE_INSTR_LSB) |
				  ((uint32_t)CQSPI_RD_TYPE_ADDR_MASK << CQSPI_RD_TYPE_ADDR_LSB) |
				  ((uint32_t)CQSPI_RD_TYPE_DATA_MASK << CQSPI_RD_TYPE_DATA_LSB) |
				  ((uint32_t)CQSPI_RD_DUMMY_MASK << CQSPI_RD_DUMMY_LSB));
	rd_instr |= (((uint32_t)cfg->instr_width & CQSPI_RD_TYPE_INSTR_MASK) << CQSPI_RD_TYPE_INSTR_LSB) |
				(((uint32_t)cfg->addr_width & CQSPI_RD_TYPE_ADDR_MASK) << CQSPI_RD_TYPE_ADDR_LSB) |
				(((uint32_t)cfg->data_width & CQSPI_RD_TYPE_DATA_MASK) << CQSPI_RD_TYPE_DATA_LSB);
	if (cfg->mode_enable)
	{
		rd_instr |= 1u << CQSPI_RD_MODE_EN_LSB;
		uint32_t mode_reg = cqspi_readl(dev, CQSPI_REG_MODE_BIT);
		mode_reg &= ~CQSPI_MODE_BITS_MASK;
		mode_reg |= (uint32_t)cfg->mode_bits & CQSPI_MODE_BITS_MASK;
		cqspi_writel(dev, CQSPI_REG_MODE_BIT, mode_reg);
	}
	cqspi_writel(dev, CQSPI_REG_RD_INSTR, rd_instr);
	uint32_t size_reg = cqspi_readl(dev, CQSPI_REG_SIZE);
	size_reg &= ~(CQSPI_SIZE_ADDR_MASK << CQSPI_SIZE_ADDR_LSB);
	size_reg |= ((uint32_t)(cfg->addr_bytes - 1u) & CQSPI_SIZE_ADDR_MASK) << CQSPI_SIZE_ADDR_LSB;
	cqspi_writel(dev, CQSPI_REG_SIZE, size_reg);
	if (was_enabled)
	{
		cqspi_controller_enable(dev, true);
	}
	return 0;
}

int cqspi_stig_execute(cqspi_dev_t *dev, const cqspi_stig_cmd_t *cmd, void *rx, const void *tx)
{
	if (!dev || !cmd)
	{
		return -1;
	}
	if ((cmd->write_len > CQSPI_STIG_MEM_BANK_MAX_BYTES) || (cmd->read_len > CQSPI_STIG_MEM_BANK_MAX_BYTES))
	{
		return -1;
	}
	if (cmd->write_len && !tx)
	{
		return -1;
	}
	if (cmd->read_len && !rx)
	{
		return -1;
	}
	cqspi_controller_enable(dev, true);
	if (cqspi_poll(dev, CQSPI_REG_CMDCTRL, CQSPI_CMDCTRL_INPROGRESS, true, dev->read_timeout_us) != 0)
	{
		return -1;
	}
	uint32_t reg = ((uint32_t)cmd->opcode & CQSPI_CMDCTRL_OPCODE_MASK) << CQSPI_CMDCTRL_OPCODE_LSB;
	if (cmd->addr_bytes)
	{
		if (cmd->addr_bytes > 4u)
		{
			return -1;
		}
		reg |= ((uint32_t)(cmd->addr_bytes - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB;
		reg |= 1u << CQSPI_CMDCTRL_ADDR_EN_LSB;
		cqspi_writel(dev, CQSPI_REG_CMDADDRESS, cmd->address);
	}
	if (cmd->dummy_cycles)
	{
		if (cmd->dummy_cycles > CQSPI_CMDCTRL_DUMMY_MASK)
		{
			return -1;
		}
		reg |= ((uint32_t)cmd->dummy_cycles & CQSPI_CMDCTRL_DUMMY_MASK) << CQSPI_CMDCTRL_DUMMY_LSB;
	}
	if (cmd->mode_enable)
	{
		reg |= CQSPI_CMDCTRL_MODE_EN;
		uint32_t mode_reg = cqspi_readl(dev, CQSPI_REG_MODE_BIT);
		mode_reg &= ~CQSPI_MODE_BITS_MASK;
		mode_reg |= (uint32_t)cmd->mode_bits & CQSPI_MODE_BITS_MASK;
		cqspi_writel(dev, CQSPI_REG_MODE_BIT, mode_reg);
	}
	if (cmd->write_len)
	{
		reg |= ((uint32_t)(cmd->write_len - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB;
		reg |= 1u << CQSPI_CMDCTRL_WR_EN_LSB;
		uint32_t lower = 0u;
		size_t lower_len = cmd->write_len > 4u ? 4u : cmd->write_len;
		memcpy(&lower, tx, lower_len);
		cqspi_writel(dev, CQSPI_REG_CMDWRITEDATALOWER, lower);
		if (cmd->write_len > 4u)
		{
			uint32_t upper = 0u;
			memcpy(&upper, (const uint8_t *)tx + 4u, cmd->write_len - 4u);
			cqspi_writel(dev, CQSPI_REG_CMDWRITEDATAUPPER, upper);
		}
	}
	if (cmd->read_len)
	{
		reg |= ((uint32_t)(cmd->read_len - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB;
		reg |= 1u << CQSPI_CMDCTRL_RD_EN_LSB;
	}
	int ret = cqspi_exec_flash_cmd(dev, reg, dev->read_timeout_us);
	if (ret != 0)
	{
		return ret;
	}
	if (cmd->read_len)
	{
		uint32_t lower = cqspi_readl(dev, CQSPI_REG_CMDREADDATALOWER);
		if (cmd->read_len > 4u)
		{
			uint32_t upper = cqspi_readl(dev, CQSPI_REG_CMDREADDATAUPPER);
			memcpy(rx, &lower, 4u);
			memcpy((uint8_t *)rx + 4u, &upper, cmd->read_len - 4u);
		}
		else
		{
			memcpy(rx, &lower, cmd->read_len);
		}
	}
	cqspi_writel(dev, CQSPI_REG_CMDCTRL, 0u);
	return 0;
}

int cqspi_direct_read(cqspi_dev_t *dev, uint32_t address, void *buffer, size_t length)
{
	if (!dev || !buffer || length == 0u || !dev->ahb)
	{
		return -1;
	}
	bool restore_enabled = false;
	int ret = cqspi_direct_access_begin(dev, &restore_enabled);
	if (ret != 0)
	{
		return ret;
	}
	volatile uint8_t *flash_ptr = dev->ahb + address;
	uint8_t *dst = (uint8_t *)buffer;
	for (size_t i = 0u; i < length; ++i)
	{
		dst[i] = flash_ptr[i];
	}
	int idle_ret = cqspi_wait_idle(dev, dev->read_timeout_us);
	int end_ret = cqspi_direct_access_end(dev, restore_enabled);
	if (idle_ret != 0)
	{
		return idle_ret;
	}
	return end_ret;
}

int cqspi_direct_write(cqspi_dev_t *dev, uint32_t address, const void *buffer, size_t length)
{
	if (!dev || !buffer || length == 0u || !dev->ahb)
	{
		return -1;
	}
	bool restore_enabled = false;
	int ret = cqspi_direct_access_begin(dev, &restore_enabled);
	if (ret != 0)
	{
		return ret;
	}
	volatile uint8_t *flash_ptr = dev->ahb + address;
	const uint8_t *src = (const uint8_t *)buffer;
	for (size_t i = 0u; i < length; ++i)
	{
		flash_ptr[i] = src[i];
	}
	int idle_ret = cqspi_wait_idle(dev, dev->read_timeout_us);
	int end_ret = cqspi_direct_access_end(dev, restore_enabled);
	if (idle_ret != 0)
	{
		return idle_ret;
	}
	return end_ret;
}

int cqspi_indirect_read(cqspi_dev_t *dev, uint32_t address, void *buffer, size_t length, uint32_t timeout_us)
{
	if (!dev || !buffer || length == 0u)
	{
		return -1;
	}
	if (!dev->ahb)
	{
		return -1;
	}
#if SIZE_MAX > UINT32_MAX
	if (length > UINT32_MAX)
	{
		return -1;
	}
#endif
	uint32_t timeout = timeout_us ? timeout_us : dev->read_timeout_us;
	cqspi_controller_enable(dev, true);
	if (cqspi_wait_idle(dev, timeout) != 0)
	{
		return -1;
	}
	cqspi_writel(dev, CQSPI_REG_INDIRECTRDSTARTADDR, address);
	cqspi_writel(dev, CQSPI_REG_INDIRECTRDBYTES, (uint32_t)length);
	cqspi_writel(dev, CQSPI_REG_IRQSTATUS, CQSPI_IRQ_STATUS_MASK);
	cqspi_writel(dev, CQSPI_REG_IRQMASK, 0u);
	cqspi_writel(dev, CQSPI_REG_INDIRECTRD, CQSPI_INDIRECTRD_START);
	uint8_t *dst = (uint8_t *)buffer;
	size_t remaining = length;
	size_t mod_bytes = length & 0x3u;
	volatile uint32_t *ahb = (volatile uint32_t *)dev->ahb;
	uint32_t loops = cqspi_timeout_to_loops(dev, timeout);
	while (remaining > 0u)
	{
		uint32_t level = cqspi_get_rd_level(dev);
		if (level == 0u)
		{
			uint32_t status = cqspi_readl(dev, CQSPI_REG_INDIRECTRD);
			if ((status & CQSPI_INDIRECTRD_DONE) != 0u && remaining == 0u)
			{
				break;
			}
			if (loops == 0u)
			{
				goto read_fail;
			}
			loops--;
			continue;
		}
		loops = cqspi_timeout_to_loops(dev, timeout);
		size_t available = (size_t)level * dev->fifo_width_bytes;
		if (available > remaining)
		{
			available = remaining;
		}
		size_t chunk_words_bytes = available & ~0x3u;
		size_t words = chunk_words_bytes / 4u;
		while (words--)
		{
			uint32_t value = *ahb;
			memcpy(dst, &value, 4u);
			dst += 4u;
			remaining -= 4u;
		}
		size_t word_remain = remaining & ~0x3u;
		if (word_remain == 0u && mod_bytes > 0u && remaining == mod_bytes)
		{
			uint32_t value = *ahb;
			size_t copy_len = mod_bytes;
			memcpy(dst, &value, copy_len);
			dst += copy_len;
			remaining -= copy_len;
			mod_bytes = 0u;
		}
	}
	if (remaining != 0u)
	{
		goto read_fail;
	}
	if (cqspi_poll(dev, CQSPI_REG_INDIRECTRD, CQSPI_INDIRECTRD_DONE, false, timeout) != 0)
	{
		goto read_fail;
	}
	cqspi_writel(dev, CQSPI_REG_INDIRECTRD, CQSPI_INDIRECTRD_DONE);
	return 0;

read_fail:
	cqspi_writel(dev, CQSPI_REG_INDIRECTRD, CQSPI_INDIRECTRD_CANCEL);
	return -1;
}

int cqspi_indirect_write(cqspi_dev_t *dev, uint32_t address, const void *buffer, size_t length, uint32_t timeout_us)
{
	if (!dev || !buffer || length == 0u)
	{
		return -1;
	}
	if (!dev->ahb)
	{
		return -1;
	}
#if SIZE_MAX > UINT32_MAX
	if (length > UINT32_MAX)
	{
		return -1;
	}
#endif
	uint32_t timeout = timeout_us ? timeout_us : dev->indirect_timeout_us;
	cqspi_controller_enable(dev, true);
	if (cqspi_wait_idle(dev, timeout) != 0)
	{
		return -1;
	}
	cqspi_writel(dev, CQSPI_REG_INDIRECTWRSTARTADDR, address);
	cqspi_writel(dev, CQSPI_REG_INDIRECTWRBYTES, (uint32_t)length);
	cqspi_writel(dev, CQSPI_REG_IRQSTATUS, CQSPI_IRQ_STATUS_MASK);
	cqspi_writel(dev, CQSPI_REG_IRQMASK, 0u);
	cqspi_writel(dev, CQSPI_REG_INDIRECTWR, CQSPI_INDIRECTWR_START);
	const uint8_t *src = (const uint8_t *)buffer;
	size_t remaining = length;
	volatile uint32_t *ahb = (volatile uint32_t *)dev->ahb;
	uint32_t loops = cqspi_timeout_to_loops(dev, timeout);
	while (remaining > 0u)
	{
		uint32_t level = cqspi_get_wr_level(dev);
		uint32_t available_words = (dev->sram_write_partition_words > level) ? (dev->sram_write_partition_words - level) : 0u;
		if (available_words == 0u)
		{
			if (loops == 0u)
			{
				goto write_fail;
			}
			loops--;
			continue;
		}
		loops = cqspi_timeout_to_loops(dev, timeout);
		size_t space_bytes = (size_t)available_words * dev->fifo_width_bytes;
		if (space_bytes > remaining)
		{
			space_bytes = remaining;
		}
		size_t words = space_bytes / 4u;
		while (words--)
		{
			uint32_t value = 0u;
			memcpy(&value, src, 4u);
			*ahb = value;
			src += 4u;
			remaining -= 4u;
		}
		size_t leftover = space_bytes % 4u;
		if (leftover > 0u)
		{
			uint32_t value = 0xFFFFFFFFu;
			memcpy(&value, src, leftover);
			*ahb = value;
			src += leftover;
			remaining -= leftover;
		}
	}
	if (cqspi_poll(dev, CQSPI_REG_INDIRECTWR, CQSPI_INDIRECTWR_DONE, false, timeout) != 0)
	{
		goto write_fail;
	}
	cqspi_writel(dev, CQSPI_REG_INDIRECTWR, CQSPI_INDIRECTWR_DONE);
	return 0;

write_fail:
	cqspi_writel(dev, CQSPI_REG_INDIRECTWR, CQSPI_INDIRECTWR_CANCEL);
	return -1;
}
