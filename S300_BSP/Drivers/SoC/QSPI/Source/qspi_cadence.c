#include "qspi_cadence.h"
#include "rcc.h"
#include "s300.h"
#include <string.h>
#include <stdio.h>

#define REG32(base, off) (*(volatile uint32_t *)((uintptr_t)(base) + (off)))

/* 控制是否使用控制器的间接模式。暂时禁用以调试问题。 */
#ifndef QSPI_USE_INDIRECT_READ
    #define QSPI_USE_INDIRECT_READ 0
#endif
#ifndef QSPI_USE_INDIRECT_WRITE
    #define QSPI_USE_INDIRECT_WRITE 0
#endif

qspi_cadence_t g_qspi =
{
    .reg = (volatile uint8_t *)QSPI_CFG_BASE,
    .ahb = (volatile uint8_t *)M4_SLV_FLASH_BASE,
    .ahb_size = (uint32_t)(M4_SLV_FLASH_END - M4_SLV_FLASH_BASE + 1u),
    .ref_clk_hz = 0u,
    .sclk_hz = 0u,
    .fifo_depth = 128u,
    .fifo_width = 4u,
    .page_size = 256u,
    .block_4k_units = 16u /* 16 x 4KB = 64KB */
};

static bool s_qspi_verbose = true;
void qspi_set_verbose(bool enable)
{
    s_qspi_verbose = enable;
}

static inline void qspi_enable(bool en)
{
    uint32_t v = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    if (en) v |= CQSPI_CFG_ENABLE;
    else v &= ~CQSPI_CFG_ENABLE;
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = v;
}

static inline int qspi_wait_idle(void)
{
    /* poll CONFIG.IDLE bit */
    for (uint32_t t = 0; t < 1000000u; ++t)
    {
        if ((REG32(g_qspi.reg, CQSPI_REG_CONFIG) >> CQSPI_CFG_IDLE_LSB) & 1u)
            return 0;
    }
    return -1;
}

static void qspi_set_baud(uint32_t ref_hz, uint32_t sclk)
{
    if (sclk == 0u) sclk = ref_hz / 4u;
    /* div = ceil(ref / (2*sclk)) - 1, clamp to 0..0xF */
    uint32_t div = (ref_hz + (2u * sclk - 1u)) / (2u * sclk);
    if (div > 0) div -= 1u;
    if (div > CQSPI_CFG_BAUD_MASK) div = CQSPI_CFG_BAUD_MASK;
    uint32_t v = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    v &= ~(CQSPI_CFG_BAUD_MASK << CQSPI_CFG_BAUD_LSB);
    v |= (div << CQSPI_CFG_BAUD_LSB);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = v;
}

static void qspi_set_mode_cpol0_cpha0(void)
{
    uint32_t v = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    v &= ~(CQSPI_CFG_CLK_POL | CQSPI_CFG_CLK_PHA);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = v;
}

static void qspi_exit_xip(void)
{
    /* Disable DIRECT and XIP_IMM, disable mode bit usage */
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_DIRECT | CQSPI_CFG_XIP_IMM);
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    /* clear any mode bit config */
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    rd &= ~(1u << CQSPI_RD_MODE_EN_LSB);
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
    REG32(g_qspi.reg, CQSPI_REG_MODE_BIT) = 0u;
}

static void qspi_set_cs(unsigned cs)
{
    /* no decoder; hardware maps CS0->1110, but u-boot helper converts; we just program nibble directly. Use CS0. */
    (void)cs;
    uint32_t v = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    v &= ~(CQSPI_CFG_CHIPSELECT_MASK << CQSPI_CFG_CHIPSELECT_LSB);
    v |= ((0xEu) << CQSPI_CFG_CHIPSELECT_LSB); /* CS0 active */
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = v;
}

static int qspi_exec_cmd(uint32_t cmdctrl)
{
    REG32(g_qspi.reg, CQSPI_REG_CMDCTRL) = cmdctrl;
    REG32(g_qspi.reg, CQSPI_REG_CMDCTRL) = cmdctrl | CQSPI_CMDCTRL_EXECUTE;
    
    /* 根据频率调整超时时间 - 高频时需要更多时间 */
    uint32_t timeout = 1000000u;
    if (g_qspi.sclk_hz >= 80000000u) {
        timeout = 2000000u;  /* 高频时增加超时 */
    } else if (g_qspi.sclk_hz >= 50000000u) {
        timeout = 1500000u;  /* 中高频时适当增加 */
    }
    
    /* wait complete */
    uint32_t t;
    for (t = 0; t < timeout; ++t)
    {
        uint32_t r = REG32(g_qspi.reg, CQSPI_REG_CMDCTRL);
        if ((r & CQSPI_CMDCTRL_INPROGRESS) == 0u) break;
        
        /* 每1000次循环添加一个小延时，让硬件有时间响应 */
        if ((t % 1000u) == 999u) {
            for (volatile uint32_t i = 0; i < 10u; i++) __NOP();
        }
    }
    
    /* 检查是否超时 */
    if (t >= timeout) {
        if (s_qspi_verbose) {
            printf("[QSPI] Command timeout @%lu Hz\n", (unsigned long)g_qspi.sclk_hz);
        }
        /* 清除状态并返回错误 */
        REG32(g_qspi.reg, CQSPI_REG_CMDCTRL) = 0u;
        return -1;
    }
    
    /* clear */
    REG32(g_qspi.reg, CQSPI_REG_CMDCTRL) = 0u;
    return qspi_wait_idle();
}

static void qspi_readdata_capture(unsigned delay)
{
    uint32_t v = REG32(g_qspi.reg, CQSPI_REG_RD_DATA_CAPTURE);
    /* BYPASS=0 时 delay 字段才生效；delay=0 则可置 BYPASS=1 以旁路 */
    v &= ~(CQSPI_RD_CAPTURE_DELAY_MASK << CQSPI_RD_CAPTURE_DELAY_LSB);
    if (delay == 0u)
    {
        v |= CQSPI_RD_CAPTURE_BYPASS;
    }
    else
    {
        v &= ~CQSPI_RD_CAPTURE_BYPASS;
        v |= ((delay & CQSPI_RD_CAPTURE_DELAY_MASK) << CQSPI_RD_CAPTURE_DELAY_LSB);
    }
    REG32(g_qspi.reg, CQSPI_REG_RD_DATA_CAPTURE) = v;
}

void qspi_cadence_init(uint32_t ref_clk_hz, uint32_t sclk_hz)
{
    /* Enable RCC clocks for QSPI on APB0 and AHB */
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
    rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_QSPIFLASH, false);
    rcc_set_cortex_m4_ahb_reset(RCC_CM4_AHB_QSPIFLASH, false);
    g_qspi.ref_clk_hz = ref_clk_hz;
    g_qspi.sclk_hz = sclk_hz ? sclk_hz : (ref_clk_hz / 4u);
    qspi_enable(false);
    /* config basic: mode 0, CS0, baud, disable decode */
    uint32_t cfg = 0u;
    cfg |= (0xEu << CQSPI_CFG_CHIPSELECT_LSB); /* CS0 */
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    qspi_set_mode_cpol0_cpha0();
    qspi_set_baud(ref_clk_hz, g_qspi.sclk_hz);
    qspi_set_cs(0);
    /* size: 24-bit addr default, page 256, block 64KB */
    uint32_t size = 0u;
    size |= ((3u - 1u) & CQSPI_SIZE_ADDR_MASK) << CQSPI_SIZE_ADDR_LSB; /* 3-bytes addr default */
    size |= (g_qspi.page_size << CQSPI_SIZE_PAGE_LSB);
    size |= (g_qspi.block_4k_units << CQSPI_SIZE_BLOCK_LSB);
    REG32(g_qspi.reg, CQSPI_REG_SIZE) = size;
    /* 设置 REMAP 为 AHB 窗口基址，使 CPU AHB 地址与控制器匹配 */
    REG32(g_qspi.reg, CQSPI_REG_REMAP) = (uint32_t)(uintptr_t)g_qspi.ahb;
    REG32(g_qspi.reg, CQSPI_REG_SRAMPARTITION) = (g_qspi.fifo_depth / 2u);
    /* Configure indirect trigger address to AHB aperture base */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTTRIGGER) = (uint32_t)(uintptr_t)g_qspi.ahb;
    /* Configure optimized watermarks for better performance */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDWATERMARK) = (g_qspi.fifo_depth / 4u);  /* 读：25% FIFO */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRWATERMARK) = (g_qspi.fifo_depth / 8u);  /* 写：12.5% FIFO */
    REG32(g_qspi.reg, CQSPI_REG_IRQMASK) = 0u;
    /* ensure we are not in XIP/direct mode left by bootrom */
    qspi_exit_xip();
    
    /* 根据频率动态调整时序参数 */
    uint32_t tshsl, tchsh, tslch, tsd2d;
    uint32_t capture_delay;
    
    if (g_qspi.sclk_hz >= 80000000u) {
        /* 高频 (>=80MHz): 更保守的时序 */
        tshsl = 255u;    /* 最大CS高时间 */
        tchsh = 50u;     /* CS保持时间 */
        tslch = 50u;     /* CS建立时间 */
        tsd2d = 255u;    /* 数据切换延时 */
        capture_delay = 3u;  /* 更大的捕获延时 */
    } else if (g_qspi.sclk_hz >= 50000000u) {
        /* 中高频 (50-80MHz): 适中时序 */
        tshsl = 200u;
        tchsh = 30u;
        tslch = 30u;
        tsd2d = 200u;
        capture_delay = 2u;
    } else if (g_qspi.sclk_hz >= 25000000u) {
        /* 中频 (25-50MHz): 标准时序 */
        tshsl = 150u;
        tchsh = 20u;
        tslch = 20u;
        tsd2d = 150u;
        capture_delay = 1u;
    } else {
        /* 低频 (<25MHz): 最小时序 */
        tshsl = 100u;
        tchsh = 10u;
        tslch = 10u;
        tsd2d = 100u;
        capture_delay = 0u;  /* 可以旁路 */
    }
    
    REG32(g_qspi.reg, CQSPI_REG_DELAY) =
        (tshsl << CQSPI_DELAY_TSHSL_LSB) |
        (tchsh << CQSPI_DELAY_TCHSH_LSB) |
        (tslch << CQSPI_DELAY_TSLCH_LSB) |
        (tsd2d << CQSPI_DELAY_TSD2D_LSB);
    qspi_readdata_capture(capture_delay);
    /* instruction bus widths single-single-single */
    uint32_t rd = (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_INSTR_LSB) |
                  (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_ADDR_LSB)  |
                  (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_DATA_LSB);
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
    uint32_t wr = (CQSPI_INST_TYPE_SINGLE << CQSPI_WR_TYPE_ADDR_LSB) |
                  (CQSPI_INST_TYPE_SINGLE << CQSPI_WR_TYPE_DATA_LSB);
    REG32(g_qspi.reg, CQSPI_REG_WR_INSTR) = wr;
    /* Clear any stale indirect status */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_DONE;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES) = 0u;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES) = 0u;
    /* 间接模式下不使用 DIRECT，DIRECT 用于 XIP/DAC 模式 */
    cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    cfg &= ~CQSPI_CFG_DIRECT;  /* 确保间接模式下关闭 DIRECT */
    REG32(g_qspi.reg, CQSPI_REG_CONFIG) = cfg;
    qspi_enable(true);
}

int qspi_read_id(uint8_t *id, uint32_t len)
{
    if (!id || len == 0u) return -1;
    qspi_enable(true);
    /* STIG read command */
    uint32_t cmd = (W25Q_CMD_RDID << CQSPI_CMDCTRL_OPCODE_LSB) | (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   (((len - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB);
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
    uint32_t copy = len > 4u ? 4u : len;
    memcpy(id, &low, copy);
    if (len > 4u)
    {
        uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
        memcpy(id + 4u, &up, len - 4u);
    }
    return 0;
}

int qspi_read_device_id(uint8_t *dev_id)
{
    if (!dev_id) return -1;
    qspi_enable(true);
    /* Device ID command (ABh) with 3 dummy bytes */
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = 0x000000u; /* 3 dummy bytes */
    uint32_t cmd = (W25Q_CMD_DEVID << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   (0u << CQSPI_CMDCTRL_RD_BYTES_LSB); /* 1 byte */
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
    *dev_id = (uint8_t)(low & 0xFFu);
    return 0;
}

int qspi_read_manufacturer_device_id(uint8_t *mfg_id, uint8_t *dev_id)
{
    if (!mfg_id || !dev_id) return -1;
    qspi_enable(true);
    /* Manufacturer/Device ID command (90h) with address 000000h */
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = 0x000000u;
    uint32_t cmd = (W25Q_CMD_MANDEV << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   (((2u - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB); /* 2 bytes */
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
    *mfg_id = (uint8_t)(low & 0xFFu);
    *dev_id = (uint8_t)((low >> 8) & 0xFFu);
    return 0;
}

int qspi_read_unique_id(uint8_t *uid, uint32_t len)
{
    if (!uid || len == 0u) return -1;
    qspi_enable(true);
    /* Read Unique ID command (4Bh) with 4 dummy bytes */
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = 0x00000000u; /* 4 dummy bytes */
    uint32_t read_len = (len > 8u) ? 8u : len; /* Max 8 bytes per STIG */
    uint32_t cmd = (W25Q_CMD_UNIQUE << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((4u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   (((read_len - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB);
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
    uint32_t copy = read_len > 4u ? 4u : read_len;
    memcpy(uid, &low, copy);
    if (read_len > 4u)
    {
        uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
        memcpy(uid + 4u, &up, read_len - 4u);
    }
    return 0;
}

int qspi_read_sfdp(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    qspi_enable(true);
    
    uint8_t *p = buf;
    uint32_t remaining = len;
    uint32_t current_addr = addr;
    
    while (remaining > 0u)
    {
        uint32_t chunk = (remaining > 8u) ? 8u : remaining;
        REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = current_addr;
        uint32_t cmd = (W25Q_CMD_SFDP << CQSPI_CMDCTRL_OPCODE_LSB) |
                       (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                       (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                       (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                       (((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB) |
                       (8u << CQSPI_CMDCTRL_DUMMY_LSB); /* SFDP needs 8 dummy cycles */
        
        int rc = qspi_exec_cmd(cmd);
        if (rc) return rc;
        
        uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
        uint32_t copy = chunk > 4u ? 4u : chunk;
        memcpy(p, &low, copy);
        if (chunk > 4u)
        {
            uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
            memcpy(p + 4u, &up, chunk - 4u);
        }
        
        p += chunk;
        current_addr += chunk;
        remaining -= chunk;
    }
    return 0;
}

static int qspi_wren(void)
{
    uint32_t cmd = (W25Q_CMD_WREN << CQSPI_CMDCTRL_OPCODE_LSB);
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    
    /* 在高频下，WREN命令可能需要额外的时间才能生效 */
    /* 添加一个小延时以确保命令完全执行 */
    if (g_qspi.sclk_hz >= 80000000u) {
        /* 高频时增加延时 */
        for (volatile uint32_t i = 0; i < 100u; i++) __NOP();
    } else if (g_qspi.sclk_hz >= 50000000u) {
        /* 中高频时适量延时 */
        for (volatile uint32_t i = 0; i < 50u; i++) __NOP();
    }
    
    /* 验证WEL位是否已设置 */
    uint8_t sr1 = 0;
    rc = qspi_read_status(&sr1, NULL, NULL);
    if (rc) return rc;
    
    if ((sr1 & 0x02u) == 0) {
        /* WEL位未设置，可能是时序问题，再试一次 */
        rc = qspi_exec_cmd(cmd);
        if (rc) return rc;
        
        /* 再次添加延时 */
        if (g_qspi.sclk_hz >= 50000000u) {
            for (volatile uint32_t i = 0; i < 200u; i++) __NOP();
        }
        
        /* 再次验证 */
        rc = qspi_read_status(&sr1, NULL, NULL);
        if (rc) return rc;
        
        if ((sr1 & 0x02u) == 0) {
            /* 仍然失败，返回错误 */
            return -1;
        }
    }
    
    return 0;
}

static int qspi_write_sr12(uint8_t sr1, uint8_t sr2)
{
    int rc = qspi_wren();
    if (rc) return rc;
    REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATALOWER) = ((uint32_t)sr1) | (((uint32_t)sr2) << 8);
    uint32_t cmd = (W25Q_CMD_WRSR12 << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_WR_EN_LSB) |
                   (((2u - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(10u);
}

/* static int qspi_write_sr3(uint8_t sr3)
{
    int rc = qspi_wren();
    if (rc) return rc;
    REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATALOWER) = sr3;
    uint32_t cmd = (W25Q_CMD_WRSR3 << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_WR_EN_LSB) |
                   (((1u - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(10u);
} */

int qspi_unlock_all(void)
{
    uint8_t s1 = 0, s2 = 0, s3 = 0;
    int rc = qspi_read_status(&s1, &s2, &s3);
    if (rc) return rc;
    if (s_qspi_verbose)
        printf("[QSPI] Before unlock SR1=%02X SR2=%02X SR3=%02X\n", s1, s2, s3);
    /* Clear SR1 block protect bits BP[2:0]=0, TB=0, SRP0(SRWD)=0; preserve rest */
    uint8_t new1 = s1 & ~((uint8_t)0x3Cu /* BP2:4 + TB */ | (uint8_t)0x80u /* SRP0 */);
    uint8_t new2 = s2 & ~((uint8_t)0x40u /* SRP1 */ | (uint8_t)0x38u /* BP[5:3] */);
    /* Keep QE (bit1 of SR2) as-is. */
    rc = qspi_write_sr12(new1, new2);
    if (rc) return rc;
    /* SR3: typically holds drive strength/latency, no lock bits; keep as-is */
    rc = qspi_read_status(&s1, &s2, &s3);
    if (s_qspi_verbose)
        printf("[QSPI] After unlock SR1=%02X SR2=%02X SR3=%02X\n", s1, s2, s3);
    return rc;
}

int qspi_wait_ready(uint32_t timeout_ms)
{
    uint32_t loops = timeout_ms * 1000u; /* approx 1us polls */
    while (loops--)
    {
        uint32_t cmd = (W25Q_CMD_RDSR1 << CQSPI_CMDCTRL_OPCODE_LSB) |
                       (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                       (0u << CQSPI_CMDCTRL_RD_BYTES_LSB);
        int rc = qspi_exec_cmd(cmd);
        if (rc) return rc;
        uint32_t v = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER) & 0xFFu;
        if ((v & 0x01u) == 0u) return 0; /* WIP=0 ready */
        for (volatile uint32_t d = 0; d < 100u; ++d) __NOP();
    }
    return -1;
}

int qspi_erase_4k(uint32_t addr)
{
    int rc = qspi_wren();
    if (rc) return rc;
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = addr;
    uint32_t cmd = (W25Q_CMD_SE_4K << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(4000u);
}

int qspi_erase_64k(uint32_t addr)
{
    int rc = qspi_wren();
    if (rc) return rc;
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = addr;
    uint32_t cmd = (W25Q_CMD_BE_64K << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(8000u);
}

int qspi_erase_32k(uint32_t addr)
{
    int rc = qspi_wren();
    if (rc) return rc;
    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = addr;
    uint32_t cmd = (W25Q_CMD_BE_32K << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(6000u); /* 32KB erase timeout */
}

int qspi_chip_erase(void)
{
    int rc = qspi_wren();
    if (rc) return rc;
    uint32_t cmd = (W25Q_CMD_CE << CQSPI_CMDCTRL_OPCODE_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(200000u); /* up to seconds */
}

int qspi_reset_enable(void)
{
    uint32_t cmd = (W25Q_CMD_RSTEN << CQSPI_CMDCTRL_OPCODE_LSB);
    return qspi_exec_cmd(cmd);
}

int qspi_reset_device(void)
{
    uint32_t cmd = (W25Q_CMD_RST << CQSPI_CMDCTRL_OPCODE_LSB);
    return qspi_exec_cmd(cmd);
}

int qspi_software_reset(void)
{
    /* 软件复位序列：先使能复位，再执行复位 */
    int rc = qspi_reset_enable();
    if (rc) return rc;
    
    /* 短暂延时确保使能命令生效 */
    for (volatile uint32_t i = 0; i < 1000u; ++i) __NOP();
    
    rc = qspi_reset_device();
    if (rc) return rc;
    
    /* 复位后延时，等待设备重新初始化 */
    for (volatile uint32_t i = 0; i < 10000u; ++i) __NOP();
    
    if (s_qspi_verbose)
        printf("[QSPI] Software reset completed\n");
    
    return 0;
}

int qspi_set_quad_enable(bool enable)
{
    uint8_t sr1 = 0, sr2 = 0;
    int rc = qspi_read_status(&sr1, &sr2, NULL);
    if (rc) return rc;
    uint8_t new2 = sr2;
    if (enable) new2 |= 0x02u; /* QE bit1 */
    else new2 &= ~(uint8_t)0x02u;
    if (new2 == sr2) return 0; /* no change */
    return qspi_write_sr12(sr1, new2);
}

int qspi_set_address_mode_4byte(bool enable)
{
    /* 大于 16MiB 才需要 4B；此处按调用者需求发命令 */
    uint32_t cmd = ((enable ? W25Q_CMD_EN4B : W25Q_CMD_EX4B) << CQSPI_CMDCTRL_OPCODE_LSB);
    return qspi_exec_cmd(cmd);
}

void qspi_configure_quad_read(bool enable)
{
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    
    if (enable)
    {
        /* 配置 Fast Read Quad Output (0x6B): 指令单线，地址单线，数据四线 (1-1-4 模式) */
        rd &= ~((0xFFu) << CQSPI_RD_OPCODE_LSB);
        rd |= (W25Q_CMD_QUAD_READ << CQSPI_RD_OPCODE_LSB);
        
        /* 配置传输宽度：指令单线，地址单线，数据四线 */
        rd &= ~((0xFu) << CQSPI_RD_TYPE_INSTR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_INSTR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_ADDR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_ADDR_LSB);  /* 修复：地址应该是单线 */
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_DATA_LSB);
        rd |= (CQSPI_INST_TYPE_QUAD << CQSPI_RD_TYPE_DATA_LSB);
        
        /* 设置dummy cycles（0x6B命令需要8个dummy cycles） */
        rd &= ~(0x1Fu << CQSPI_RD_DUMMY_LSB);
        rd |= (8u << CQSPI_RD_DUMMY_LSB);
    }
    else
    {
        /* 恢复单线 Fast Read (0x0B) */
        rd &= ~((0xFFu) << CQSPI_RD_OPCODE_LSB);
        rd |= (W25Q_CMD_FAST << CQSPI_RD_OPCODE_LSB);
        
        /* 配置传输宽度：全部单线 */
        rd &= ~((0xFu) << CQSPI_RD_TYPE_INSTR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_INSTR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_ADDR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_ADDR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_DATA_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_DATA_LSB);
        
        /* 设置dummy cycles（0x0B命令需要8个dummy cycles） */
        rd &= ~(0x1Fu << CQSPI_RD_DUMMY_LSB);
        rd |= (8u << CQSPI_RD_DUMMY_LSB);
    }
    
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
}

void qspi_configure_quad_io_read(bool enable)
{
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    
    if (enable)
    {
        /* 配置 Fast Read Quad I/O (0xEB): 指令单线，地址四线，数据四线 (1-4-4 模式) */
        rd &= ~((0xFFu) << CQSPI_RD_OPCODE_LSB);
        rd |= (W25Q_CMD_QUAD_FAST << CQSPI_RD_OPCODE_LSB);  /* 0xEB */
        
        /* 配置传输宽度：指令单线，地址四线，数据四线 */
        rd &= ~((0xFu) << CQSPI_RD_TYPE_INSTR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_INSTR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_ADDR_LSB);
        rd |= (CQSPI_INST_TYPE_QUAD << CQSPI_RD_TYPE_ADDR_LSB);  /* 地址四线 */
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_DATA_LSB);
        rd |= (CQSPI_INST_TYPE_QUAD << CQSPI_RD_TYPE_DATA_LSB);
        
        /* 启用 Mode bits */
        rd |= (1u << CQSPI_RD_MODE_EN_LSB);
        
        /* 设置dummy cycles（0xEB命令通常需要6个dummy cycles） */
        rd &= ~(0x1Fu << CQSPI_RD_DUMMY_LSB);
        rd |= (6u << CQSPI_RD_DUMMY_LSB);
        
        if (s_qspi_verbose)
            printf("[QSPI] Configured Fast Read Quad I/O (0xEB, 1-4-4 mode)\n");
    }
    else
    {
        /* 恢复单线 Fast Read (0x0B) */
        rd &= ~((0xFFu) << CQSPI_RD_OPCODE_LSB);
        rd |= (W25Q_CMD_FAST << CQSPI_RD_OPCODE_LSB);
        
        /* 配置传输宽度：全部单线 */
        rd &= ~((0xFu) << CQSPI_RD_TYPE_INSTR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_INSTR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_ADDR_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_ADDR_LSB);
        
        rd &= ~((0xFu) << CQSPI_RD_TYPE_DATA_LSB);
        rd |= (CQSPI_INST_TYPE_SINGLE << CQSPI_RD_TYPE_DATA_LSB);
        
        /* 禁用 Mode bits */
        rd &= ~(1u << CQSPI_RD_MODE_EN_LSB);
        
        /* 设置dummy cycles（0x0B命令需要8个dummy cycles） */
        rd &= ~(0x1Fu << CQSPI_RD_DUMMY_LSB);
        rd |= (8u << CQSPI_RD_DUMMY_LSB);
        
        if (s_qspi_verbose)
            printf("[QSPI] Configured Fast Read (0x0B, 1-1-1 mode)\n");
    }
    
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
}

int qspi_read_quad_stig(uint32_t addr, void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    
    /* 专用的Quad STIG读取，使用Quad Output Fast Read (0x6B) 
     * 注意：0xEB需要地址也是Quad模式，硬件可能不支持，改用0x6B */
    uint8_t *pp = (uint8_t *)buf;
    uint32_t a = addr;
    uint32_t remain = len;
    
    while (remain)
    {
        uint32_t chunk = (remain > 8u) ? 8u : remain;
        REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = a;
        uint32_t cmd = (W25Q_CMD_QUAD_READ << CQSPI_CMDCTRL_OPCODE_LSB) |
                       (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                       (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                       (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                       (((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB) |
                       (8u << CQSPI_CMDCTRL_DUMMY_LSB);  /* 8 dummy cycles for 0x6B */
        
        int r = qspi_exec_cmd(cmd);
        if (r) return r;
        
        uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
        uint32_t take = (chunk > 4u) ? 4u : chunk;
        memcpy(pp, &low, take);
        if (chunk > 4u)
        {
            uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
            memcpy(pp + 4u, &up, chunk - 4u);
        }
        pp += chunk;
        a += chunk;
        remain -= chunk;
    }
    return 0;
}

int qspi_page_program(uint32_t addr, const void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    if (len > g_qspi.page_size) len = g_qspi.page_size;
    int rc = qspi_wren();
    if (rc) return rc;
    if (s_qspi_verbose)
    {
        uint8_t s1_dbg = 0;
        (void)qspi_read_status(&s1_dbg, NULL, NULL);
        printf("[QSPI] After WREN SR1=%02X (WEL=%u)\n", s1_dbg, (unsigned)(!!(s1_dbg & 0x02u)));
    }
    /* Ensure WEL is set before proceeding */
    {
        uint8_t s1 = 0;
        (void)qspi_read_status(&s1, NULL, NULL);
        if ((s1 & 0x02u) == 0u)
        {
            rc = qspi_wren();
            if (rc) return rc;
            (void)qspi_read_status(&s1, NULL, NULL);
            if (s_qspi_verbose)
                printf("[QSPI] Retry WREN SR1=%02X (WEL=%u)\n", s1, (unsigned)(!!(s1 & 0x02u)));
            if ((s1 & 0x02u) == 0u)
            {
                if (s_qspi_verbose)
                    printf("[QSPI] WEL not set before program (SR1=%02X)\n", s1);
                return -2;
            }
        }
    }
    /* 若禁用间接写，直接使用 STIG 小块写入 */
    if (!QSPI_USE_INDIRECT_WRITE)
    {
        const uint8_t *p8 = (const uint8_t *)buf;
        uint32_t off = 0;
        while (off < len)
        {
            uint32_t chunk = len - off;
            if (chunk > 8u) chunk = 8u;
            rc = qspi_wren();
            if (rc) return rc;
            REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = (addr + off);
            uint32_t lower = 0, upper = 0;
            memcpy(&lower, p8 + off, (chunk > 4u) ? 4u : chunk);
            if (chunk > 4u)
            {
                memcpy(&upper, p8 + off + 4u, chunk - 4u);
            }
            REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATALOWER) = lower;
            REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATAUPPER) = upper;
            uint32_t cmd = (W25Q_CMD_PP << CQSPI_CMDCTRL_OPCODE_LSB) |
                           (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                           (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                           (1u << CQSPI_CMDCTRL_WR_EN_LSB) |
                           (((chunk - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB);
            rc = qspi_exec_cmd(cmd);
            if (rc) return rc;
            rc = qspi_wait_ready(20u);
            if (rc) return rc;
            off += chunk;
        }
        return 0;
    }
    /* Indirect write setup (preferred) */
    /* Program WR_INSTR fields already single-single; set start addr */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRSTARTADDR) = addr;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES) = len;
    /* Configure write opcode */
    uint32_t wr = REG32(g_qspi.reg, CQSPI_REG_WR_INSTR);
    wr &= ~((0xFFu) << CQSPI_WR_OPCODE_LSB);
    wr |= (W25Q_CMD_PP << CQSPI_WR_OPCODE_LSB);
    REG32(g_qspi.reg, CQSPI_REG_WR_INSTR) = wr;
    if (s_qspi_verbose)
        printf("[QSPI] PP setup: WR_INSTR=%08lX STARTADDR=%06lX BYTES=%lu\n",
               (unsigned long)wr, (unsigned long)addr, (unsigned long)len);
    /* Clear DONE then trigger indirect write */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_START;
    if (s_qspi_verbose)
        printf("[QSPI] INDWR after START=%08lX SDRAM=%08lX\n",
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL));
    /* Small nudge */
    for (volatile uint32_t d = 0; d < 200u; ++d) __NOP();
    /* Write through AHB aperture (FIFO port at fixed base address) in chunks based on free space */
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t remaining = len;
    uint32_t guard = 0;
    /* 将 flash addr 映射到 AHB 窗口，顺序写入该窗口以向 FIFO 推送数据 */
    uintptr_t ahb_off = (addr & (g_qspi.ahb_size - 1u));
    volatile uint8_t *ahb8 = (volatile uint8_t *)((uintptr_t)g_qspi.ahb + ahb_off);
    volatile uint32_t *ahb32 = (volatile uint32_t *)((uintptr_t)g_qspi.ahb + ahb_off);
    bool logged_first_push = false;
    while (remaining)
    {
        uint32_t level_words = (REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL) >> CQSPI_SDRAMLEVEL_WR_LSB) & CQSPI_SDRAMLEVEL_WR_MASK;
        if (level_words > g_qspi.fifo_depth) level_words = g_qspi.fifo_depth; /* sanity */
        uint32_t free_words = g_qspi.fifo_depth - level_words;
        if (free_words == 0u)
        {
            if (s_qspi_verbose)
                if ((++guard % 50000u) == 0u)
                {
                    printf("[QSPI] waiting WR FIFO space... SDRAM=%08lX INDWR=%08lX WRBYTES=%08lX rem=%lu\n",
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR),
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES),
                           (unsigned long)remaining);
                }
            if (guard > 1000000u)
            {
                printf("[QSPI] WR FIFO no space timeout (SDRAM=%08lX INDWR=%08lX WRBYTES=%08lX) remaining=%lu\n",
                       (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
                       (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR),
                       (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES),
                       (unsigned long)remaining);
                break;
            }
            continue; /* wait for space */
        }
        guard = 0;
        uint32_t free_bytes = free_words * g_qspi.fifo_width;
        uint32_t chunk = (remaining < free_bytes) ? remaining : free_bytes;
        /* Prefer 32-bit writes for better bus efficiency */
        uint32_t words = chunk >> 2;
        for (uint32_t i = 0; i < words; ++i)
        {
            uint32_t w;
            memcpy(&w, p, sizeof w);
            *ahb32++ = w; /* 顺序写入窗口地址 */
            p += 4;
        }
        uint32_t tail = chunk & 3u;
        for (uint32_t i = 0; i < tail; ++i)
        {
            *ahb8++ = *p++;
        }
    if (!logged_first_push && s_qspi_verbose)
        {
            logged_first_push = true;
            uint32_t sdram = REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL);
            printf("[QSPI] pushed first chunk, SDRAM=%08lX WRBYTES=%08lX\n",
                   (unsigned long)sdram,
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES));
        }
        remaining -= chunk;
    }
    __DSB();
    /* If we couldn't stage all data, cancel */
    if (remaining != 0u)
    {
        REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_CANCEL;
        printf("[QSPI] IndirectWR aborted, not all data staged (remain=%lu).\n", (unsigned long)remaining);
        return -1;
    }
    /* Wait done or WRBYTES to drain to 0 */
    bool done = false;
    for (uint32_t t = 0; t < 1000000u; ++t)
    {
        uint32_t wrb = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES);
        if (wrb == 0u || (REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) & CQSPI_INDIRECTWR_DONE))
        {
            done = true;
            break;
        }
    }
    /* Clear done */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    if (!done)
    {
        /* Cancel and dump debug info */
        REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_CANCEL;
     if (s_qspi_verbose)
     {
         printf("[QSPI] IndirectWR timeout @0x%06lX len=%lu\n", (unsigned long)addr, (unsigned long)len);
         printf("  CFG=%08lX RD_INSTR=%08lX WR_INSTR=%08lX SIZE=%08lX\n",
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_CONFIG),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_RD_INSTR),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_WR_INSTR),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SIZE));
         printf("  SDRAMLEVEL=%08lX IRQSTS=%08lX INDWR=%08lX INDWRBYTES=%08lX\n",
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_IRQSTATUS),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR),
             (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES));
     }
        uint8_t s1 = 0, s2 = 0, s3 = 0;
        (void)qspi_read_status(&s1, &s2, &s3);
        printf("  SR1=%02X SR2=%02X SR3=%02X\n", s1, s2, s3);
        /* Fallback to STIG page program in <=8B chunks */
        printf("[QSPI] Falling back to STIG PP in small chunks...\n");
        const uint8_t *p8 = (const uint8_t *)buf;
        uint32_t off = 0;
        while (off < len)
        {
            uint32_t chunk = len - off;
            if (chunk > 8u) chunk = 8u;
            /* WREN before each PP */
            rc = qspi_wren();
            if (rc) return rc;
            if (s_qspi_verbose)
            {
                uint8_t s1c = 0;
                qspi_read_status(&s1c, NULL, NULL);
                printf("[QSPI] STIG PP chunk off=%lu len=%lu WEL=%u\n", (unsigned long)off, (unsigned long)chunk, (unsigned)(!!(s1c & 0x02u)));
            }
            REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = (addr + off);
            /* pack up to 8 bytes into write data regs */
            uint32_t lower = 0, upper = 0;
            memcpy(&lower, p8 + off, (chunk > 4u) ? 4u : chunk);
            if (chunk > 4u)
            {
                memcpy(&upper, p8 + off + 4u, chunk - 4u);
            }
            REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATALOWER) = lower;
            REG32(g_qspi.reg, CQSPI_REG_CMDWRITEDATAUPPER) = upper;
            uint32_t cmd = (W25Q_CMD_PP << CQSPI_CMDCTRL_OPCODE_LSB) |
                           (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                           (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                           (1u << CQSPI_CMDCTRL_WR_EN_LSB) |
                           (((chunk - 1u) & CQSPI_CMDCTRL_WR_BYTES_MASK) << CQSPI_CMDCTRL_WR_BYTES_LSB);
            rc = qspi_exec_cmd(cmd);
            if (rc) return rc;
            if (s_qspi_verbose)
            {
                uint8_t s1d = 0;
                qspi_read_status(&s1d, NULL, NULL);
                printf("[QSPI] STIG PP after exec SR1=%02X\n", s1d);
            }
            rc = qspi_wait_ready(20u);
            if (rc) return rc;
            off += chunk;
        }
        return 0;
    }
    return qspi_wait_ready(100u);
}

int qspi_page_program_quad(uint32_t addr, const void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    if (len > g_qspi.page_size) len = g_qspi.page_size;
    
    /* 四线页编程需要使用间接写模式 */
    int rc = qspi_wren();
    if (rc) return rc;
    
    if (s_qspi_verbose)
    {
        uint8_t s1_dbg = 0;
        (void)qspi_read_status(&s1_dbg, NULL, NULL);
        printf("[QSPI] Quad PP: After WREN SR1=%02X (WEL=%u)\n", s1_dbg, (unsigned)(!!(s1_dbg & 0x02u)));
    }
    
    /* 确保 WEL 已设置 */
    {
        uint8_t s1 = 0;
        (void)qspi_read_status(&s1, NULL, NULL);
        if ((s1 & 0x02u) == 0u)
        {
            rc = qspi_wren();
            if (rc) return rc;
            (void)qspi_read_status(&s1, NULL, NULL);
            if (s_qspi_verbose)
                printf("[QSPI] Quad PP: Retry WREN SR1=%02X (WEL=%u)\n", s1, (unsigned)(!!(s1 & 0x02u)));
            if ((s1 & 0x02u) == 0u)
            {
                if (s_qspi_verbose)
                    printf("[QSPI] Quad PP: WEL not set (SR1=%02X)\n", s1);
                return -2;
            }
        }
    }
    
    /* 配置写指令寄存器用于四线页编程 */
    uint32_t write_setup = (W25Q_CMD_PP_QUAD << CQSPI_WR_OPCODE_LSB) |
                          (CQSPI_INST_TYPE_SINGLE << CQSPI_WR_TYPE_ADDR_LSB) |  /* 地址仍使用单线 */
                          (CQSPI_INST_TYPE_QUAD << CQSPI_WR_TYPE_DATA_LSB);     /* 数据使用四线 */
    REG32(g_qspi.reg, CQSPI_REG_WR_INSTR) = write_setup;
    
    /* 确保之前的间接操作已完成 */
    uint32_t prev_status = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
    if (prev_status & (CQSPI_INDIRECTWR_START | CQSPI_INDIRECTWR_CANCEL)) {
        if (s_qspi_verbose)
            printf("[QSPI] Quad PP: Previous indirect write still active, waiting...\n");
        /* 等待之前的操作完成 */
        for (uint32_t i = 0; i < 100000u; i++) {
            prev_status = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
            if ((prev_status & (CQSPI_INDIRECTWR_START | CQSPI_INDIRECTWR_CANCEL)) == 0u)
                break;
            __NOP();
        }
        if (prev_status & (CQSPI_INDIRECTWR_START | CQSPI_INDIRECTWR_CANCEL)) {
            if (s_qspi_verbose)
                printf("[QSPI] Quad PP: Previous operation still active, aborting\n");
            return -4;
        }
    }
    
    /* 清除任何之前的完成状态 */
    if (prev_status & CQSPI_INDIRECTWR_DONE) {
        REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    }
    
    /* 配置间接写操作 */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRSTARTADDR) = addr;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES) = len;
    
    /* 启动间接写操作 */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_START;
    
    /* 写入数据到AHB接口 */
    const uint8_t *p8 = (const uint8_t *)buf;
    uint32_t written = 0;
    
    while (written < len)
    {
        /* 检查操作是否已完成 */
        uint32_t status = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
        if (status & CQSPI_INDIRECTWR_DONE)
            break;
            
        /* 一次写入4字节对齐的数据块 */
        uint32_t chunk = len - written;
        if (chunk >= 4u)
        {
            /* 按4字节写入 */
            uint32_t word;
            memcpy(&word, p8 + written, 4u);
            *((volatile uint32_t *)g_qspi.ahb) = word;
            written += 4u;
        }
        else
        {
            /* 剩余字节逐个写入 */
            for (uint32_t i = 0; i < chunk; i++)
            {
                *((volatile uint8_t *)g_qspi.ahb) = p8[written + i];
            }
            written += chunk;
        }
    }
    
    /* 等待间接写操作完成 */
    uint32_t timeout = 1000000u;  /* 增加超时时间 */
    if (g_qspi.sclk_hz >= 80000000u) {
        timeout = 2000000u;  /* 高频时更长的超时 */
    }
    
    while (--timeout > 0u)
    {
        uint32_t status = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
        if (status & CQSPI_INDIRECTWR_DONE)
            break;
            
        /* 每1000次循环检查一次，避免CPU过度占用 */
        if ((timeout % 1000u) == 0u) {
            /* 添加小延时让硬件有时间响应 */
            for (volatile uint32_t i = 0; i < 10u; i++) __NOP();
            
            /* 检查是否有错误状态 */
            uint32_t irq_status = REG32(g_qspi.reg, CQSPI_REG_IRQSTATUS);
            if (irq_status != 0u) {
                if (s_qspi_verbose)
                    printf("[QSPI] Quad PP: IRQ status=0x%08lX during wait\n", (unsigned long)irq_status);
                /* 清除中断状态 */
                REG32(g_qspi.reg, CQSPI_REG_IRQSTATUS) = irq_status;
            }
        }
    }
    
    if (timeout == 0u)
    {
        /* 超时处理：尝试取消操作 */
        if (s_qspi_verbose)
            printf("[QSPI] Quad PP: Timeout waiting for completion, attempting cancel\n");
        REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_CANCEL;
        
        /* 等待取消完成 */
        for (uint32_t i = 0; i < 10000u; i++) {
            uint32_t status = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
            if ((status & (CQSPI_INDIRECTWR_START | CQSPI_INDIRECTWR_CANCEL)) == 0u)
                break;
            __NOP();
        }
        
        return -3;
    }
    
    /* 清除完成标志 */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    
    /* 等待flash完成编程操作 */
    rc = qspi_wait_ready(20u);
    
    if (s_qspi_verbose)
        printf("[QSPI] Quad page program completed: %lu bytes @0x%06lX\n", 
               (unsigned long)len, (unsigned long)addr);
    
    return rc;
}

int qspi_read(uint32_t addr, void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    if (!QSPI_USE_INDIRECT_READ)
    {
        /* STIG FAST READ 0x0B 分块读取，提升性能 */
        uint8_t *pp = (uint8_t *)buf;
        uint32_t a = addr;
        uint32_t remain = len;
        while (remain)
        {
            uint32_t chunk = (remain > 8u) ? 8u : remain;  /* 保持8字节以确保稳定性 */
            REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = a;
            uint32_t cmd = (W25Q_CMD_FAST << CQSPI_CMDCTRL_OPCODE_LSB) |
                           (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                           (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                           (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                           (((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB) |
                           (8u << CQSPI_CMDCTRL_DUMMY_LSB);  /* 8 dummy cycles for FAST READ */
            int r = qspi_exec_cmd(cmd);
            if (r) return r;
            uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
            uint32_t take = (chunk > 4u) ? 4u : chunk;
            memcpy(pp, &low, take);
            if (chunk > 4u)
            {
                uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
                memcpy(pp + 4u, &up, chunk - 4u);
            }
            pp += chunk;
            a += chunk;
            remain -= chunk;
        }
        return 0;
    }
    /* Setup READ opcode to FAST READ 0x0B with 8 dummy cycles (1 byte dummy) */
    uint32_t rd = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    rd &= ~((0xFFu) << CQSPI_RD_OPCODE_LSB);
    rd |= (W25Q_CMD_FAST << CQSPI_RD_OPCODE_LSB);
    rd &= ~(0x1Fu << CQSPI_RD_DUMMY_LSB);
    rd |= (8u << CQSPI_RD_DUMMY_LSB); /* 8 cycles */
    REG32(g_qspi.reg, CQSPI_REG_RD_INSTR) = rd;
    /* Program address size 3 bytes */
    uint32_t sz = REG32(g_qspi.reg, CQSPI_REG_SIZE);
    sz &= ~CQSPI_SIZE_ADDR_MASK;
    sz |= ((3u - 1u) & CQSPI_SIZE_ADDR_MASK) << CQSPI_SIZE_ADDR_LSB;
    REG32(g_qspi.reg, CQSPI_REG_SIZE) = sz;
    /* Clear DONE then indirect read execute */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_DONE;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDSTARTADDR) = addr;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES) = len;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_START;
    uint8_t *p = (uint8_t *)buf;
    uintptr_t ahb_off = (addr & (g_qspi.ahb_size - 1u));
    volatile const uint8_t *ahb8 = (volatile const uint8_t *)((uintptr_t)g_qspi.ahb + ahb_off);
    uint32_t remaining = len;
    uint32_t guard = 0;
    while (remaining)
    {
        uint32_t level = (REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL) >> CQSPI_SDRAMLEVEL_RD_LSB) & CQSPI_SDRAMLEVEL_RD_MASK;
        if (level == 0u)
        {
            if (s_qspi_verbose)
                if ((++guard % 500000u) == 0u)
                {
                    printf("[QSPI] waiting RD FIFO data... SDRAM=%08lX INDREAD=%08lX RDBYTES=%08lX rem=%lu\n",
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD),
                           (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES),
                           (unsigned long)remaining);
                }
            if (guard > 3000000u)
            {
                /* Fallback to STIG READ */
                REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_CANCEL;
                printf("[QSPI] RD FIFO empty timeout, fallback STIG READ @0x%06lX len=%lu\n",
                       (unsigned long)addr, (unsigned long)remaining);
                uint8_t *pp = p;
                uint32_t a = addr;
                uint32_t remain = remaining;
                while (remain)
                {
                    uint32_t chunk = (remain > 8u) ? 8u : remain;
                    REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = a;
                    uint32_t cmd = (W25Q_CMD_READ << CQSPI_CMDCTRL_OPCODE_LSB) |
                                   (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                                   (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                                   (((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB);
                    int r = qspi_exec_cmd(cmd);
                    if (r) return r;
                    uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
                    uint32_t take = (chunk > 4u) ? 4u : chunk;
                    memcpy(pp, &low, take);
                    if (chunk > 4u)
                    {
                        uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
                        memcpy(pp + 4u, &up, chunk - 4u);
                    }
                    pp += chunk;
                    a += chunk;
                    remain -= chunk;
                }
                return 0;
            }
            continue;
        }
        uint32_t chunk = level * g_qspi.fifo_width;
        if (chunk > remaining) chunk = remaining;
        for (uint32_t i = 0; i < chunk; ++i)
        {
            *p++ = *ahb8++;
        }
        remaining -= chunk;
    }
    /* Wait Done */
    bool done = false;
    for (uint32_t t = 0; t < 1000000u; ++t)
    {
        if (REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) & CQSPI_INDIRECTRD_DONE)
        {
            done = true;
            break;
        }
    }
    /* Clear done */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_DONE;
    if (!done)
    {
        REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD) = CQSPI_INDIRECTRD_CANCEL;
        if (s_qspi_verbose)
        {
            printf("[QSPI] IndirectRD timeout @0x%06lX len=%lu, fallback STIG READ\n", (unsigned long)addr, (unsigned long)len);
            printf("  CFG=%08lX RD_INSTR=%08lX SIZE=%08lX SDRAMLEVEL=%08lX INDREAD=%08lX INDREADBYTES=%08lX\n",
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_CONFIG),
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_RD_INSTR),
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SIZE),
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD),
                   (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES));
        }
        /* STIG fallback */
        uint8_t *pp = (uint8_t *)buf;
        uint32_t remain = len;
        while (remain)
        {
            uint32_t chunk = (remain > 8u) ? 8u : remain;
            REG32(g_qspi.reg, CQSPI_REG_CMDADDRESS) = addr;
            uint32_t cmd = (W25Q_CMD_READ << CQSPI_CMDCTRL_OPCODE_LSB) |
                           (1u << CQSPI_CMDCTRL_ADDR_EN_LSB) |
                           (((3u - 1u) & CQSPI_CMDCTRL_ADD_BYTES_MASK) << CQSPI_CMDCTRL_ADD_BYTES_LSB) |
                           (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                           (((chunk - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB);
            int r = qspi_exec_cmd(cmd);
            if (r) return r;
            uint32_t low = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER);
            uint32_t take = (chunk > 4u) ? 4u : chunk;
            memcpy(pp, &low, take);
            if (chunk > 4u)
            {
                uint32_t up = REG32(g_qspi.reg, CQSPI_REG_CMDREADDATAUPPER);
                memcpy(pp + 4u, &up, chunk - 4u);
            }
            pp += chunk;
            addr += chunk;
            remain -= chunk;
        }
        return 0;
    }
    return qspi_wait_idle();
}

/* --- Debug helpers --- */

static int qspi_read_one(uint8_t opcode, uint8_t *val)
{
    if (!val) return -1;
    uint32_t cmd = ((uint32_t)opcode << CQSPI_CMDCTRL_OPCODE_LSB) |
                   (1u << CQSPI_CMDCTRL_RD_EN_LSB) |
                   (((1u - 1u) & CQSPI_CMDCTRL_RD_BYTES_MASK) << CQSPI_CMDCTRL_RD_BYTES_LSB);
    int rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    *val = (uint8_t)(REG32(g_qspi.reg, CQSPI_REG_CMDREADDATALOWER) & 0xFFu);
    return 0;
}

void qspi_dump_regs(const char *tag)
{
    if (!tag) tag = "";
    uint32_t cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    uint32_t rdinstr = REG32(g_qspi.reg, CQSPI_REG_RD_INSTR);
    uint32_t wrinstr = REG32(g_qspi.reg, CQSPI_REG_WR_INSTR);
    uint32_t size = REG32(g_qspi.reg, CQSPI_REG_SIZE);
    uint32_t delay = REG32(g_qspi.reg, CQSPI_REG_DELAY);
    uint32_t rdcap = REG32(g_qspi.reg, CQSPI_REG_RD_DATA_CAPTURE);
    uint32_t part = REG32(g_qspi.reg, CQSPI_REG_SRAMPARTITION);
    uint32_t remap = REG32(g_qspi.reg, CQSPI_REG_REMAP);
    uint32_t modeb = REG32(g_qspi.reg, CQSPI_REG_MODE_BIT);
    uint32_t sdram = REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL);
    uint32_t irqst = REG32(g_qspi.reg, CQSPI_REG_IRQSTATUS);
    uint32_t indrd = REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD);
    uint32_t indrb = REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES);
    uint32_t indwr = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR);
    uint32_t indwb = REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRBYTES);
    printf("[QSPI] dump %s\n", tag);
    printf("  CFG=%08lX SIZE=%08lX DELAY=%08lX RD_CAP=%08lX PART=%08lX\n",
           (unsigned long)cfg, (unsigned long)size, (unsigned long)delay,
           (unsigned long)rdcap, (unsigned long)part);
    printf("  RD_INSTR=%08lX WR_INSTR=%08lX REMAP=%08lX MODE=%08lX\n",
           (unsigned long)rdinstr, (unsigned long)wrinstr,
           (unsigned long)remap, (unsigned long)modeb);
    printf("  SDRAM=%08lX IRQSTS=%08lX IND_RD=%08lX RDBYTES=%08lX IND_WR=%08lX WRBYTES=%08lX\n",
           (unsigned long)sdram, (unsigned long)irqst,
           (unsigned long)indrd, (unsigned long)indrb,
           (unsigned long)indwr, (unsigned long)indwb);
}

int qspi_read_status(uint8_t *sr1, uint8_t *sr2, uint8_t *sr3)
{
    int rc = 0;
    if (sr1)
    {
        rc = qspi_read_one(W25Q_CMD_RDSR1, sr1);
        if (rc) return rc;
    }
    if (sr2)
    {
        rc = qspi_read_one(W25Q_CMD_RDSR2, sr2);
        if (rc) return rc;
    }
    if (sr3)
    {
        rc = qspi_read_one(W25Q_CMD_RDSR3, sr3);
        if (rc) return rc;
    }
    return 0;
}
