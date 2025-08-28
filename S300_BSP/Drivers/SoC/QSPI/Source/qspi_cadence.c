#include "qspi_cadence.h"
#include "rcc.h"
#include "s300.h"
#include <string.h>
#include <stdio.h>

#define REG32(base, off) (*(volatile uint32_t *)((uintptr_t)(base) + (off)))

/* 控制是否使用控制器的间接模式。默认关闭，走 STIG，最稳妥。 */
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
    /* wait complete */
    for (uint32_t t = 0; t < 1000000u; ++t)
    {
        uint32_t r = REG32(g_qspi.reg, CQSPI_REG_CMDCTRL);
        if ((r & CQSPI_CMDCTRL_INPROGRESS) == 0u) break;
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
    /* Configure conservative watermarks: quarter of FIFO */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDWATERMARK) = (g_qspi.fifo_depth / 4u);
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWRWATERMARK) = (g_qspi.fifo_depth / 4u);
    REG32(g_qspi.reg, CQSPI_REG_IRQMASK) = 0u;
    /* ensure we are not in XIP/direct mode left by bootrom */
    qspi_exit_xip();
    /* conservative delays */
    REG32(g_qspi.reg, CQSPI_REG_DELAY) =
        (200u << CQSPI_DELAY_TSHSL_LSB) |
        (20u  << CQSPI_DELAY_TCHSH_LSB) |
        (20u  << CQSPI_DELAY_TSLCH_LSB) |
        (200u << CQSPI_DELAY_TSD2D_LSB);
    qspi_readdata_capture(2u);
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
    /* 仅在启用间接模式时开启 DIRECT（供 AHB FIFO 端口使用） */
    cfg = REG32(g_qspi.reg, CQSPI_REG_CONFIG);
    if (QSPI_USE_INDIRECT_READ || QSPI_USE_INDIRECT_WRITE)
        cfg |= CQSPI_CFG_DIRECT;
    else
        cfg &= ~CQSPI_CFG_DIRECT;
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

static int qspi_wren(void)
{
    uint32_t cmd = (W25Q_CMD_WREN << CQSPI_CMDCTRL_OPCODE_LSB);
    return qspi_exec_cmd(cmd);
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

static int qspi_write_sr3(uint8_t sr3)
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
}

int qspi_unlock_all(void)
{
    uint8_t s1 = 0, s2 = 0, s3 = 0;
    int rc = qspi_read_status(&s1, &s2, &s3);
    if (rc) return rc;
    printf("[QSPI] Before unlock SR1=%02X SR2=%02X SR3=%02X\n", s1, s2, s3);
    /* Clear SR1 block protect bits BP[2:0]=0, TB=0, SRP0(SRWD)=0; preserve rest */
    uint8_t new1 = s1 & ~((uint8_t)0x3Cu /* BP2:4 + TB */ | (uint8_t)0x80u /* SRP0 */);
    uint8_t new2 = s2 & ~((uint8_t)0x40u /* SRP1 */ | (uint8_t)0x38u /* BP[5:3] */);
    /* Keep QE (bit1 of SR2) as-is. */
    rc = qspi_write_sr12(new1, new2);
    if (rc) return rc;
    /* SR3: typically holds drive strength/latency, no lock bits; keep as-is */
    rc = qspi_read_status(&s1, &s2, &s3);
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

int qspi_chip_erase(void)
{
    int rc = qspi_wren();
    if (rc) return rc;
    uint32_t cmd = (W25Q_CMD_CE << CQSPI_CMDCTRL_OPCODE_LSB);
    rc = qspi_exec_cmd(cmd);
    if (rc) return rc;
    return qspi_wait_ready(200000u); /* up to seconds */
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

int qspi_page_program(uint32_t addr, const void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    if (len > g_qspi.page_size) len = g_qspi.page_size;
    int rc = qspi_wren();
    if (rc) return rc;
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
            printf("[QSPI] Retry WREN SR1=%02X (WEL=%u)\n", s1, (unsigned)(!!(s1 & 0x02u)));
            if ((s1 & 0x02u) == 0u)
            {
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
    printf("[QSPI] PP setup: WR_INSTR=%08lX STARTADDR=%06lX BYTES=%lu\n",
           (unsigned long)wr, (unsigned long)addr, (unsigned long)len);
    /* Clear DONE then trigger indirect write */
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_DONE;
    REG32(g_qspi.reg, CQSPI_REG_INDIRECTWR) = CQSPI_INDIRECTWR_START;
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
        if (!logged_first_push)
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

int qspi_read(uint32_t addr, void *buf, uint32_t len)
{
    if (!buf || len == 0u) return -1;
    if (!QSPI_USE_INDIRECT_READ)
    {
        /* STIG READ 0x03 分块读取，最兼容 */
        uint8_t *pp = (uint8_t *)buf;
        uint32_t a = addr;
        uint32_t remain = len;
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
        printf("[QSPI] IndirectRD timeout @0x%06lX len=%lu, fallback STIG READ\n", (unsigned long)addr, (unsigned long)len);
        printf("  CFG=%08lX RD_INSTR=%08lX SIZE=%08lX SDRAMLEVEL=%08lX INDREAD=%08lX INDREADBYTES=%08lX\n",
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_CONFIG),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_RD_INSTR),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SIZE),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_SDRAMLEVEL),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRD),
               (unsigned long)REG32(g_qspi.reg, CQSPI_REG_INDIRECTRDBYTES));
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
