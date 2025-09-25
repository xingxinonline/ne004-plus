#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"  // 直接使用QSPI底层接口

typedef uint32_t UINT32;

#define TEST_PAGE_SIZE          256u
#define TEST_SUBSECTOR_SIZE     4096u   // N25Q Subsector Erase是4KB (4096字节)

static const uint8_t g_xip_exec_stub_code[] = {
    0x5A, 0x20,       // movs r0, #0x5A
    0x70, 0x47        // bx lr
};

static const uint32_t g_xip_exec_expected_value = 0x5Au;

// Flash芯片类型定义
typedef enum {
    FLASH_TYPE_UNKNOWN = 0,
    FLASH_TYPE_W25Q    = 1,  // Winbond W25Q系列
    FLASH_TYPE_N25Q    = 2,  // Micron N25Q系列
    FLASH_TYPE_MX25L   = 3,  // Macronix MX25L系列
} flash_type_t;

// Flash信息结构体
typedef struct {
    uint8_t manuf_id;        // 厂家ID
    uint8_t memory_type;     // 存储器类型
    uint8_t capacity;        // 容量代码
    uint32_t size_bytes;     // 容量（字节）
    flash_type_t type;       // 芯片类型
    const char *type_name;   // 类型名称
} flash_info_t;

typedef enum {
    TIMER_SOURCE_DWT = 0,
    TIMER_SOURCE_SYSTICK = 1
} timer_source_t;

static timer_source_t g_timer_source = TIMER_SOURCE_DWT;
static uint32_t g_timer_frequency_hz = 0u;
static uint32_t g_systick_load_value = 0u;
static uint32_t g_systick_reload_ticks = 0u;
static volatile uint32_t g_systick_overflow_count = 0u;
static uint32_t g_cached_rd_instr = 0u;
static uint32_t g_cached_mode_bits = 0u;
static bool g_cached_read_config_valid = false;

static inline volatile uint32_t *qspi_reg_ptr(cqspi_dev_t *qspi, uint32_t offset) {
    return (volatile uint32_t *)(qspi->regs + offset);
}

static inline uint32_t qspi_reg_read(cqspi_dev_t *qspi, uint32_t offset) {
    return *qspi_reg_ptr(qspi, offset);
}

static inline void qspi_reg_write(cqspi_dev_t *qspi, uint32_t offset, uint32_t value) {
    *qspi_reg_ptr(qspi, offset) = value;
}

static int qspi_issue_legacy_read(cqspi_dev_t *qspi, uint32_t address, uint8_t *rx_byte) {
    if (!qspi) {
        return -1;
    }

    uint8_t temp = 0u;
    uint8_t *target = rx_byte ? rx_byte : &temp;

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x03,
        .addr_bytes = 3,
        .address = address,
        .read_len = 1,
        .write_len = 0,
        .dummy_cycles = 0,
        .mode_enable = false,
    };

    if (cqspi_stig_execute(qspi, &cmd, target, NULL) != 0) {
        return -1;
    }

    return 0;
}

static void qspi_configure_read_capture(cqspi_dev_t *qspi, uint32_t delay_cycles) {
    if (!qspi) {
        return;
    }

    uint32_t capture = qspi_reg_read(qspi, CQSPI_REG_RD_DATA_CAPTURE);
    capture &= ~((CQSPI_RD_CAPTURE_DELAY_MASK << CQSPI_RD_CAPTURE_DELAY_LSB) | CQSPI_RD_CAPTURE_BYPASS);

    if (delay_cycles == 0u) {
        capture |= CQSPI_RD_CAPTURE_BYPASS;
    } else {
        capture |= ((delay_cycles & CQSPI_RD_CAPTURE_DELAY_MASK) << CQSPI_RD_CAPTURE_DELAY_LSB);
    }

    qspi_reg_write(qspi, CQSPI_REG_RD_DATA_CAPTURE, capture);
}

static uint32_t qspi_select_read_delay(uint32_t sclk_hz) {
    if (sclk_hz <= 24000000u) {
        return 0u;
    } else if (sclk_hz <= 48000000u) {
        return 1u;
    } else if (sclk_hz <= 72000000u) {
        return 2u;
    } else if (sclk_hz <= 96000000u) {
        return 3u;
    }
    return 4u;
}

static int qspi_configure_speed_with_capture(cqspi_dev_t *qspi, uint32_t target_hz) {
    if (!qspi) {
        return -1;
    }

    if (cqspi_configure_clock(qspi, target_hz) != 0) {
        return -1;
    }

    uint32_t delay_cycles = qspi_select_read_delay(target_hz);
    qspi_configure_read_capture(qspi, delay_cycles);

    printf("QSPI clock configured to %lu Hz (read capture delay %lu cycles)\n",
           (unsigned long)qspi->current_sclk_hz,
           (unsigned long)delay_cycles);

    return 0;
}

// 启用或关闭Direct Access Controller (DAC)
static int set_direct_access_mode(cqspi_dev_t *qspi, bool enable) {
    if (!qspi || !qspi->regs) {
        return -1;
    }

    uint32_t config = qspi_reg_read(qspi, CQSPI_REG_CONFIG);

    if (enable) {
        config |= CQSPI_CFG_DIRECT;
        config |= CQSPI_CFG_ENABLE;
        qspi_reg_write(qspi, CQSPI_REG_CONFIG, config);
        if (cqspi_wait_idle(qspi, 1000u) != 0) {
            return -1;
        }

        if (g_cached_read_config_valid) {
            qspi_reg_write(qspi, CQSPI_REG_RD_INSTR, g_cached_rd_instr);
            if (cqspi_wait_idle(qspi, 1000u) != 0) {
                return -1;
            }

            uint32_t mode_reg = qspi_reg_read(qspi, CQSPI_REG_MODE_BIT);
            mode_reg &= ~CQSPI_MODE_BITS_MASK;
            mode_reg |= g_cached_mode_bits & CQSPI_MODE_BITS_MASK;
            qspi_reg_write(qspi, CQSPI_REG_MODE_BIT, mode_reg);
            if (cqspi_wait_idle(qspi, 1000u) != 0) {
                return -1;
            }
        }
    } else {
        config &= ~CQSPI_CFG_DIRECT;
        qspi_reg_write(qspi, CQSPI_REG_CONFIG, config);
        if (cqspi_wait_idle(qspi, 1000u) != 0) {
            return -1;
        }
    }

    return 0;
}

static int write_enable(cqspi_dev_t *qspi);
static int wait_flash_ready(cqspi_dev_t *qspi, uint32_t timeout_ms);

// 通过Direct Access模式写入数据
static int flash_direct_write(cqspi_dev_t *qspi, uint32_t address, const uint8_t *data, uint32_t len) {
    if (!qspi || !qspi->ahb || !data || len == 0u) {
        return -1;
    }

    if (set_direct_access_mode(qspi, true) != 0) {
        return -1;
    }

    uint32_t remaining = len;
    uint32_t curr_addr = address;
    const uint8_t *curr_data = data;
    int result = 0;

    while (remaining > 0u) {
        uint32_t offset_in_page = curr_addr & (TEST_PAGE_SIZE - 1u);
        uint32_t chunk = TEST_PAGE_SIZE - offset_in_page;
        if (chunk > remaining) {
            chunk = remaining;
        }

        if (write_enable(qspi) != 0) {
            result = -1;
            break;
        }

        volatile uint8_t *flash_ptr = qspi->ahb + curr_addr;
        for (uint32_t i = 0; i < chunk; i++) {
            flash_ptr[i] = curr_data[i];
        }

        if (wait_flash_ready(qspi, 100) != 0) {
            result = -1;
            break;
        }

        curr_addr += chunk;
        curr_data += chunk;
        remaining -= chunk;
    }

    if (set_direct_access_mode(qspi, false) != 0) {
        result = -1;
    }

    return result;
}

// 通过Direct Access模式读取数据
static int flash_direct_read(cqspi_dev_t *qspi, uint32_t address, uint8_t *data, uint32_t len) {
    if (!qspi || !qspi->ahb || !data || len == 0u) {
        return -1;
    }

    if (set_direct_access_mode(qspi, true) != 0) {
        return -1;
    }

    volatile uint8_t *flash_ptr = qspi->ahb + address;
    for (uint32_t i = 0; i < len; i++) {
        data[i] = flash_ptr[i];
    }

    if (set_direct_access_mode(qspi, false) != 0) {
        return -1;
    }

    return 0;
}

// 直接读取JEDEC ID的函数 (使用标准RDID指令9Fh)
static int read_jedec_id(cqspi_dev_t *qspi, uint8_t *id_buf, uint32_t len) {
    if (!qspi || !id_buf || len < 3) {
        return -1;
    }

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x9F,      // RDID Read Identification (9Fh)
        .addr_bytes = 0,     // 不需要地址
        .read_len = 3,       // 读取3个字节 (Manufacturer ID + Memory Type + Capacity)
        .write_len = 0,      // 不写入数据
        .dummy_cycles = 0,   // 无dummy周期
        .mode_enable = false,
    };

    return cqspi_stig_execute(qspi, &cmd, id_buf, NULL);
}

// 识别Flash芯片类型的函数
static flash_type_t identify_flash_type(uint8_t manuf_id, uint8_t memory_type) {
    switch (manuf_id) {
        case 0xEF:  // Winbond
            if (memory_type == 0x40) {
                return FLASH_TYPE_W25Q;
            }
            break;
        case 0x20:  // Micron
            if (memory_type == 0xBA) {
                return FLASH_TYPE_N25Q;
            }
            break;
        case 0xC2:  // Macronix
            if (memory_type == 0x20) {
                return FLASH_TYPE_MX25L;
            }
            break;
    }
    return FLASH_TYPE_UNKNOWN;
}

// 根据容量代码计算Flash大小
static uint32_t capacity_to_size(uint8_t capacity) {
    if (capacity < 16u || capacity > 31u) {
        return 0u;
    }
    if (capacity >= 32u) {
        return 0u;
    }
    return (1u << capacity);
}

// 获取芯片类型名称
static const char* get_flash_type_name(flash_type_t type) {
    switch (type) {
        case FLASH_TYPE_W25Q:  return "Winbond W25Q";
        case FLASH_TYPE_N25Q:  return "Micron N25Q";
        case FLASH_TYPE_MX25L: return "Macronix MX25L";
        default:               return "Unknown";
    }
}

// 写使能命令
static int write_enable(cqspi_dev_t *qspi) {
    cqspi_stig_cmd_t cmd = {
        .opcode = 0x06,      // Write Enable
        .addr_bytes = 0,
        .read_len = 0,
        .write_len = 0,
        .dummy_cycles = 0,
        .mode_enable = false,
    };
    return cqspi_stig_execute(qspi, &cmd, NULL, NULL);
}

// 读取状态寄存器1
static int read_status_register(cqspi_dev_t *qspi, uint8_t *status) {
    cqspi_stig_cmd_t cmd = {
        .opcode = 0x05,      // Read Status Register
        .addr_bytes = 0,
        .read_len = 1,
        .write_len = 0,
        .dummy_cycles = 0,
        .mode_enable = false,
    };
    return cqspi_stig_execute(qspi, &cmd, status, NULL);
}

// 读取状态寄存器2
static int read_status_register2(cqspi_dev_t *qspi, uint8_t *status) {
    cqspi_stig_cmd_t cmd = {
        .opcode = 0x35,      // Read Status Register 2
        .addr_bytes = 0,
        .read_len = 1,
        .write_len = 0,
        .dummy_cycles = 0,
        .mode_enable = false,
    };
    return cqspi_stig_execute(qspi, &cmd, status, NULL);
}

// 写状态寄存器2
static int write_status_register2(cqspi_dev_t *qspi, uint8_t status) {
    // 写使能
    if (write_enable(qspi) != 0) {
        return -1;
    }

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x31,      // Write Status Register 2
        .addr_bytes = 0,
        .read_len = 0,
        .write_len = 1,
        .dummy_cycles = 0,
        .mode_enable = false,
    };

    if (cqspi_stig_execute(qspi, &cmd, NULL, &status) != 0) {
        return -1;
    }

    // 等待状态寄存器写入完成
    return wait_flash_ready(qspi, 100); // 100ms超时
}

// 启用Quad模式 (设置QE位)
static int quad_enable(cqspi_dev_t *qspi) {
    uint8_t sr2;
    if (read_status_register2(qspi, &sr2) != 0) {
        return -1;
    }

    // 设置QE位 (bit 1)
    sr2 |= (1u << 1);

    return write_status_register2(qspi, sr2);
}

// 等待Flash就绪
static int wait_flash_ready(cqspi_dev_t *qspi, uint32_t timeout_ms) {
    uint8_t status;
    uint32_t start_time = 0; // 简化实现，实际应该使用系统时间

    do {
        if (read_status_register(qspi, &status) != 0) {
            return -1;
        }
        if ((status & 0x01) == 0) { // WIP bit cleared
            return 0;
        }
        // 简单的延时，实际应该使用更精确的延时
        for (volatile int i = 0; i < 1000; i++);
        start_time++;
    } while (start_time < timeout_ms * 1000);

    return -1; // Timeout
}

typedef struct {
    uint32_t config;
    uint32_t rd_instr;
    uint32_t mode_bit;
} qspi_xip_restore_t;

static int qspi_enter_xip_144(cqspi_dev_t *qspi, qspi_xip_restore_t *restore) {
    if (!qspi || !restore || !qspi->regs)
    {
        return -1;
    }

    restore->config = qspi_reg_read(qspi, CQSPI_REG_CONFIG);
    restore->rd_instr = qspi_reg_read(qspi, CQSPI_REG_RD_INSTR);
    restore->mode_bit = qspi_reg_read(qspi, CQSPI_REG_MODE_BIT);

    if (set_direct_access_mode(qspi, false) != 0) {
        return -1;
    }

    uint32_t cfg = qspi_reg_read(qspi, CQSPI_REG_CONFIG);
    cfg &= ~CQSPI_CFG_DIRECT;
    cfg |= CQSPI_CFG_ENABLE;
    qspi_reg_write(qspi, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    uint32_t xip_rd_instr = ((uint32_t)0xEB << CQSPI_RD_OPCODE_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_1 & CQSPI_RD_TYPE_INSTR_MASK) << CQSPI_RD_TYPE_INSTR_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_4 & CQSPI_RD_TYPE_ADDR_MASK) << CQSPI_RD_TYPE_ADDR_LSB) |
                            (((uint32_t)CQSPI_BUSWIDTH_4 & CQSPI_RD_TYPE_DATA_MASK) << CQSPI_RD_TYPE_DATA_LSB) |
                            (((uint32_t)4u & CQSPI_RD_DUMMY_MASK) << CQSPI_RD_DUMMY_LSB) |
                            (1u << CQSPI_RD_MODE_EN_LSB);
    qspi_reg_write(qspi, CQSPI_REG_RD_INSTR, xip_rd_instr);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    uint32_t mode_reg = restore->mode_bit & ~CQSPI_MODE_BITS_MASK;
    mode_reg |= (uint32_t)0x20u & CQSPI_MODE_BITS_MASK;
    qspi_reg_write(qspi, CQSPI_REG_MODE_BIT, mode_reg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    cfg = qspi_reg_read(qspi, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_XIP_IMM);
    cfg |= CQSPI_CFG_XIP_NEXT;
    qspi_reg_write(qspi, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    cfg |= CQSPI_CFG_DIRECT;
    qspi_reg_write(qspi, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    return 0;
}

static int qspi_exit_xip(cqspi_dev_t *qspi, const qspi_xip_restore_t *restore, uint32_t flush_address) {
    if (!qspi || !restore || !qspi->regs)
    {
        return -1;
    }

    if (set_direct_access_mode(qspi, false) != 0) {
        return -1;
    }

    uint32_t cfg = qspi_reg_read(qspi, CQSPI_REG_CONFIG);
    cfg &= ~(CQSPI_CFG_XIP_NEXT | CQSPI_CFG_XIP_IMM | CQSPI_CFG_DIRECT);
    qspi_reg_write(qspi, CQSPI_REG_CONFIG, cfg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    qspi_reg_write(qspi, CQSPI_REG_MODE_BIT, restore->mode_bit);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    qspi_reg_write(qspi, CQSPI_REG_RD_INSTR, restore->rd_instr);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    uint32_t restore_cfg = restore->config & ~(CQSPI_CFG_XIP_NEXT | CQSPI_CFG_XIP_IMM);
    qspi_reg_write(qspi, CQSPI_REG_CONFIG, restore_cfg);
    if (cqspi_wait_idle(qspi, 1000u) != 0) {
        return -1;
    }

    if (qspi_issue_legacy_read(qspi, flush_address, NULL) != 0) {
        return -1;
    }

    return 0;
}

// 写保护禁用 (清除状态寄存器中的写保护位)
static int write_protect_disable(cqspi_dev_t *qspi) {
    // 写使能
    if (write_enable(qspi) != 0) {
        return -1;
    }

    // 读取当前状态寄存器2的值，保持不变
    uint8_t sr2;
    if (read_status_register2(qspi, &sr2) != 0) {
        return -1;
    }

    // 写入状态寄存器1和2，清除SR1的保护位，保持SR2不变
    uint8_t status_regs[2] = {0x00, sr2};

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x01,      // Write Status Register
        .addr_bytes = 0,
        .read_len = 0,
        .write_len = 2,      // 写入2个字节
        .dummy_cycles = 0,
        .mode_enable = false,
    };

    if (cqspi_stig_execute(qspi, &cmd, NULL, status_regs) != 0) {
        return -1;
    }

    // 等待状态寄存器写入完成
    return wait_flash_ready(qspi, 100); // 100ms超时
}

// 子扇区擦除 (N25Q使用20h Subsector Erase, 通常是4KB)
static int erase_subsector(cqspi_dev_t *qspi, uint32_t address) {
    // 写使能
    if (write_enable(qspi) != 0) {
        return -1;
    }

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x20,      // N25Q Subsector Erase (20h, 4KB)
        .addr_bytes = 3,     // 24-bit address
        .address = address,  // 设置地址
        .read_len = 0,
        .write_len = 0,
        .dummy_cycles = 0,
        .mode_enable = false,
    };

    if (cqspi_stig_execute(qspi, &cmd, NULL, NULL) != 0) {
        return -1;
    }

    // 等待擦除完成 (Subsector Erase通常耗时较短)
    return wait_flash_ready(qspi, 1000); // 1秒超时
}

// 基础Flash信息读取测试
static int perform_basic_flash_test(cqspi_dev_t *qspi, flash_info_t *info) {
    if (!qspi || !info) {
        return -1;
    }

    // 读取JEDEC ID
    uint8_t jedec_id[3] = {0};
    if (read_jedec_id(qspi, jedec_id, sizeof(jedec_id)) != 0) {
        return -1;
    }

    // 解析ID信息
    info->manuf_id = jedec_id[0];
    info->memory_type = jedec_id[1];
    info->capacity = jedec_id[2];
    info->size_bytes = capacity_to_size(info->capacity);
    info->type = identify_flash_type(info->manuf_id, info->memory_type);
    info->type_name = get_flash_type_name(info->type);

    return 0;
}

static void cycle_counter_init(void) {
    bool dwt_available = false;

#if defined(DWT) && defined(CoreDebug)
    if ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0u) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0u;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        uint32_t before = DWT->CYCCNT;
        for (volatile int i = 0; i < 64; i++) {
            __NOP();
        }
        uint32_t after = DWT->CYCCNT;
        if ((after - before) > 0u) {
            dwt_available = true;
        } else {
            DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
        }
    }
#endif

    if (dwt_available) {
        g_timer_source = TIMER_SOURCE_DWT;
        g_timer_frequency_hz = SystemCoreClock;
        printf("Timing source: DWT cycle counter\n");
        return;
    }

    g_timer_source = TIMER_SOURCE_SYSTICK;
    g_timer_frequency_hz = (SystemCoreClock != 0u) ? (SystemCoreClock / 8u) : 0u;

    SysTick->CTRL = 0u;
    SysTick->LOAD = 0xFFFFFFu;
    SysTick->VAL = 0u;
    g_systick_load_value = SysTick->LOAD;
    g_systick_reload_ticks = g_systick_load_value + 1u;
    g_systick_overflow_count = 0u;

    // Use AHB/8 clock to extend overflow window (~0.7s @192MHz core)
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    g_timer_frequency_hz = (SystemCoreClock != 0u) ? (SystemCoreClock / 8u) : 0u;
    if (g_timer_frequency_hz == 0u && SystemCoreClock != 0u) {
        g_timer_frequency_hz = SystemCoreClock; // fallback to full clock if division underflows
    }

    // Enable overflow interrupt to accumulate full periods
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    __enable_irq();

    printf("Timing source: SysTick fallback (AHB/8)\n");
}

static uint64_t get_time_ticks(void) {
    if (g_timer_source == TIMER_SOURCE_DWT) {
        return (uint64_t)DWT->CYCCNT;
    }

    uint32_t overflow_snapshot;
    uint32_t current_down;
    do {
        overflow_snapshot = g_systick_overflow_count;
        current_down = SysTick->VAL;
    } while (overflow_snapshot != g_systick_overflow_count);

    uint64_t base_ticks = (uint64_t)overflow_snapshot * (uint64_t)g_systick_reload_ticks;
    uint32_t elapsed_in_cycle = g_systick_load_value - current_down;
    return base_ticks + (uint64_t)elapsed_in_cycle;
}

static uint32_t ticks_to_us(uint64_t ticks) {
    if (g_timer_frequency_hz == 0u) {
        return 0u;
    }
    uint64_t temp = ticks * 1000000ULL;
    temp /= (uint64_t)g_timer_frequency_hz;
    return (uint32_t)temp;
}

static uint32_t ticks_to_ms(uint64_t ticks) {
    if (g_timer_frequency_hz == 0u) {
        return 0u;
    }
    uint64_t temp = ticks * 1000ULL;
    temp /= (uint64_t)g_timer_frequency_hz;
    return (uint32_t)temp;
}

void SysTick_Handler(void) {
    if (g_timer_source == TIMER_SOURCE_SYSTICK) {
        g_systick_overflow_count++;
    }
}

static inline uint8_t throughput_pattern(uint32_t offset) {
    uint8_t low = (uint8_t)(offset & 0xFFu);
    uint8_t mid = (uint8_t)((offset >> 8) & 0xFFu);
    uint8_t high = (uint8_t)((offset >> 16) & 0xFFu);
    uint8_t value = (uint8_t)(0x5Au ^ low);
    value = (uint8_t)(value + (uint8_t)(0x33u ^ mid));
    value = (uint8_t)(value ^ high);
    return value;
}

static int measure_subsector_erase_time(cqspi_dev_t *qspi, uint32_t address, uint32_t *duration_us) {
    if (!qspi) {
        return -1;
    }

    uint32_t aligned_address = address & ~(TEST_SUBSECTOR_SIZE - 1u);
    uint64_t start_ticks = get_time_ticks();
    int rc = erase_subsector(qspi, aligned_address);
    uint64_t elapsed_ticks = get_time_ticks() - start_ticks;

    if (duration_us != NULL) {
        *duration_us = ticks_to_us(elapsed_ticks);
    }

    if (rc == 0) {
        printf("STIG subsector erase completed in %lu us (%lu ms)\n",
               (unsigned long)ticks_to_us(elapsed_ticks),
               (unsigned long)ticks_to_ms(elapsed_ticks));
    }

    return rc;
}

static int dac_write_pattern_range(cqspi_dev_t *qspi, uint32_t address, uint32_t length) {
    if (!qspi || !qspi->ahb || length == 0u) {
        return -1;
    }

    uint8_t page_buffer[TEST_PAGE_SIZE];
    uint32_t remaining = length;
    uint32_t curr_addr = address;

    while (remaining > 0u) {
        uint32_t offset_in_page = curr_addr & (TEST_PAGE_SIZE - 1u);
        uint32_t chunk = TEST_PAGE_SIZE - offset_in_page;
        if (chunk > remaining) {
            chunk = remaining;
        }

        for (uint32_t i = 0; i < chunk; i++) {
            uint32_t global_offset = (curr_addr + i) - address;
            page_buffer[i] = throughput_pattern(global_offset);
        }

        if (flash_direct_write(qspi, curr_addr, page_buffer, chunk) != 0) {
            return -1;
        }

        curr_addr += chunk;
        remaining -= chunk;
    }

    return 0;
}

static int dac_read_and_verify_range(cqspi_dev_t *qspi,
                                     uint32_t address,
                                     uint32_t length,
                                     uint32_t *mismatch_offset,
                                     uint8_t *expected_value,
                                     uint8_t *actual_value) {
    if (!qspi || !qspi->ahb || length == 0u) {
        return -1;
    }

    uint8_t page_buffer[TEST_PAGE_SIZE];
    uint32_t remaining = length;
    uint32_t curr_addr = address;
    uint32_t total_offset = 0u;

    while (remaining > 0u) {
        uint32_t chunk = (remaining > TEST_PAGE_SIZE) ? TEST_PAGE_SIZE : remaining;

        if (flash_direct_read(qspi, curr_addr, page_buffer, chunk) != 0) {
            return -1;
        }

        for (uint32_t i = 0; i < chunk; i++) {
            uint8_t expected = throughput_pattern(total_offset + i);
            uint8_t actual = page_buffer[i];
            if (actual != expected) {
                if (mismatch_offset) {
                    *mismatch_offset = total_offset + i;
                }
                if (expected_value) {
                    *expected_value = expected;
                }
                if (actual_value) {
                    *actual_value = actual;
                }
                return -1;
            }
        }

        curr_addr += chunk;
        total_offset += chunk;
        remaining -= chunk;
    }

    return 0;
}

static int test_dac_throughput_1mb(cqspi_dev_t *qspi, flash_info_t *info) {
    if (!qspi || !info || info->size_bytes == 0u) {
        return -1;
    }

    const uint32_t test_size = 1024u * 1024u; // 1MB
    const uint32_t block_size = 65536u;        // 64KB block erase

    if (info->size_bytes < test_size) {
        printf("Flash size ( %lu bytes ) too small for 1MB DAC test\n",
               (unsigned long)info->size_bytes);
        return -1;
    }

    uint32_t region_start = info->size_bytes - test_size;
    region_start &= ~(block_size - 1u);
    uint32_t region_end = region_start + test_size;

    printf("Preparing 1MB DAC test region: 0x%08lX - 0x%08lX\n",
           (unsigned long)region_start,
           (unsigned long)(region_end - 1u));

    // 先逐个子扇区擦除1MB区域，确保写入空间干净
    const uint32_t block_count = test_size / block_size;
    for (uint32_t i = 0; i < block_count; i++) {
        uint32_t block_address = region_start + (i * block_size);
        if (erase_subsector(qspi, block_address) != 0) {
            printf("Failed to erase first subsector of block at 0x%08lX\n",
                   (unsigned long)block_address);
            return -1;
        }
        // 擦除此块内剩余的子扇区
        for (uint32_t subsector = TEST_SUBSECTOR_SIZE;
             subsector < block_size;
             subsector += TEST_SUBSECTOR_SIZE) {
            uint32_t subsector_addr = block_address + subsector;
            if (erase_subsector(qspi, subsector_addr) != 0) {
                printf("Failed to erase subsector at 0x%08lX\n",
                       (unsigned long)subsector_addr);
                return -1;
            }
        }
    }

    printf("Erase for 1MB DAC test region completed\n");

    uint64_t write_start = get_time_ticks();
    if (dac_write_pattern_range(qspi, region_start, test_size) != 0) {
        printf("DAC 1MB write failed\n");
        return -1;
    }
    uint64_t write_ticks = get_time_ticks() - write_start;

    uint64_t read_start = get_time_ticks();
    uint32_t mismatch_offset = 0u;
    uint8_t expected_value = 0u;
    uint8_t actual_value = 0u;
    int verify_result = dac_read_and_verify_range(qspi,
                                                  region_start,
                                                  test_size,
                                                  &mismatch_offset,
                                                  &expected_value,
                                                  &actual_value);
    uint64_t read_ticks = get_time_ticks() - read_start;

    printf("DAC 1MB write time: %lu us (%lu ms)\n",
        (unsigned long)ticks_to_us(write_ticks),
        (unsigned long)ticks_to_ms(write_ticks));
    printf("DAC 1MB read time: %lu us (%lu ms)\n",
        (unsigned long)ticks_to_us(read_ticks),
        (unsigned long)ticks_to_ms(read_ticks));

    if (verify_result != 0) {
        printf("DAC 1MB verify failed at offset %lu (expected 0x%02X, actual 0x%02X)\n",
               (unsigned long)mismatch_offset,
               expected_value,
               actual_value);
        return -1;
    }

    uint32_t write_us = ticks_to_us(write_ticks);
    uint32_t read_us = ticks_to_us(read_ticks);
    if (write_us > 0u) {
        uint64_t throughput_write = ((uint64_t)test_size * 1000000ULL) / (uint64_t)write_us;
        printf("Approximate write throughput: %lu bytes/s\n", (unsigned long)throughput_write);
    }
    if (read_us > 0u) {
        uint64_t throughput_read = ((uint64_t)test_size * 1000000ULL) / (uint64_t)read_us;
        printf("Approximate read throughput: %lu bytes/s\n", (unsigned long)throughput_read);
    }

    printf("DAC 1MB throughput test completed successfully\n");
    return 0;
}

static int test_xip_mode_144(cqspi_dev_t *qspi, flash_info_t *info, bool refresh_pattern) {
    if (!qspi || !info || !qspi->ahb || !qspi->regs) {
        return -1;
    }

    if (info->type != FLASH_TYPE_W25Q) {
        printf("Skipping XIP test: flash type %s not Winbond W25Q\n", info->type_name);
        return 0;
    }

    if (info->size_bytes < (2u * TEST_SUBSECTOR_SIZE)) {
        printf("Skipping XIP test: flash density too small\n");
        return -1;
    }

    uint32_t subsector_base = info->size_bytes - (2u * TEST_SUBSECTOR_SIZE);
    subsector_base &= ~(TEST_SUBSECTOR_SIZE - 1u);
    uint32_t test_address = subsector_base;
    const uint32_t sample_len = TEST_PAGE_SIZE;
    const uint32_t exec_offset = sample_len;
    const uint32_t exec_address = test_address + exec_offset;

    uint8_t program_buffer[TEST_PAGE_SIZE];
    uint8_t baseline_buffer[TEST_PAGE_SIZE];
    uint8_t xip_buffer[TEST_PAGE_SIZE];
    uint8_t exit_buffer[TEST_PAGE_SIZE];
    uint8_t exec_verify[sizeof(g_xip_exec_stub_code)];

    for (uint32_t i = 0; i < sample_len; ++i) {
        program_buffer[i] = throughput_pattern(i);
    }

    printf("Starting XIP 1-4-4 dummy=4 test at 0x%08lX\n", (unsigned long)test_address);

    if (refresh_pattern) {
        if (erase_subsector(qspi, subsector_base) != 0) {
            printf("XIP test: subsector erase failed\n");
            return -1;
        }

        if (flash_direct_write(qspi, test_address, program_buffer, sample_len) != 0) {
            printf("XIP test: pattern program failed\n");
            return -1;
        }

        if (flash_direct_write(qspi, exec_address, g_xip_exec_stub_code,
                                (uint32_t)sizeof(g_xip_exec_stub_code)) != 0) {
            printf("XIP test: execution stub program failed\n");
            return -1;
        }
    } else {
        printf("XIP test: reuse existing pattern (skip erase/program)\n");
    }

    if (flash_direct_read(qspi, exec_address, exec_verify,
                           (uint32_t)sizeof(exec_verify)) != 0) {
        printf("XIP test: execution stub readback failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < (uint32_t)sizeof(exec_verify); ++i) {
        if (exec_verify[i] != g_xip_exec_stub_code[i]) {
            printf("XIP test: execution stub verify mismatch at byte %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, g_xip_exec_stub_code[i], exec_verify[i]);
            return -1;
        }
    }

    if (flash_direct_read(qspi, test_address, baseline_buffer, sample_len) != 0) {
        printf("XIP test: baseline read failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < sample_len; ++i) {
        if (baseline_buffer[i] != program_buffer[i]) {
            printf("XIP test: baseline verify mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, program_buffer[i], baseline_buffer[i]);
            return -1;
        }
    }

    qspi_xip_restore_t restore = {0};
    int ret = -1;
    bool restore_needed = true;

    if (qspi_enter_xip_144(qspi, &restore) != 0) {
        printf("Failed to enter XIP 1-4-4 mode\n");
        goto exit_restore;
    }

    volatile const uint8_t *xip_ptr = qspi->ahb + test_address;
    for (uint32_t i = 0; i < sample_len; ++i) {
        xip_buffer[i] = xip_ptr[i];
    }

    for (uint32_t i = 0; i < sample_len; ++i) {
        if (xip_buffer[i] != program_buffer[i]) {
            printf("XIP test: XIP read mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                   (unsigned long)i, program_buffer[i], xip_buffer[i]);
            goto exit_restore;
        }
    }

    printf("XIP test: XIP read verified successfully\n");

    typedef uint32_t (*xip_exec_fn_t)(void);
    uintptr_t exec_ptr = (uintptr_t)qspi->ahb + (uintptr_t)exec_address;
    xip_exec_fn_t exec_fn = (xip_exec_fn_t)(exec_ptr | (uintptr_t)1u);
    uint32_t exec_result = exec_fn();

    if (exec_result != g_xip_exec_expected_value) {
     printf("XIP test: instruction execution returned 0x%08lX (expected 0x%08lX)\n",
         (unsigned long)exec_result,
         (unsigned long)g_xip_exec_expected_value);
     goto exit_restore;
    }

    printf("XIP test: instruction fetch execution result 0x%08lX verified\n",
        (unsigned long)exec_result);

    ret = 0;

exit_restore:
    if (restore_needed) {
        if (qspi_exit_xip(qspi, &restore, test_address) != 0) {
            printf("Failed to exit XIP mode cleanly\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        if (flash_direct_read(qspi, test_address, exit_buffer, sample_len) != 0) {
            printf("XIP test: post-exit read failed\n");
            ret = -1;
        } else {
            for (uint32_t i = 0; i < sample_len; ++i) {
                if (exit_buffer[i] != program_buffer[i]) {
                    printf("XIP test: post-exit verify mismatch at %lu (expected 0x%02X, actual 0x%02X)\n",
                           (unsigned long)i, program_buffer[i], exit_buffer[i]);
                    ret = -1;
                    break;
                }
            }
        }
    }

    if (ret == 0) {
        printf("XIP 1-4-4 dummy=4 test completed successfully\n");
    }

    return ret;
}

// Direct Access Controller (DAC) 读写测试
static int perform_direct_access_test(cqspi_dev_t *qspi, flash_info_t *info) {
    if (!qspi || !info || !qspi->ahb) {
        return -1;
    }

    uint32_t test_address = info->size_bytes - (3u * TEST_SUBSECTOR_SIZE); // 倒数第3个子扇区
    test_address &= ~(TEST_SUBSECTOR_SIZE - 1u);

    uint8_t write_buffer[TEST_PAGE_SIZE];
    uint8_t direct_read_buffer[TEST_PAGE_SIZE];

    for (uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
        write_buffer[i] = (uint8_t)(0xA5u ^ i);
    }

    // 写保护禁用和四线模式启用现在在切换时钟之前完成

    if (erase_subsector(qspi, test_address) != 0) {
        printf("DAC write failed: subsector erase failed\n");
        return -1;
    }

    // DAC 写入
    printf("DAC write start\n");
    if (flash_direct_write(qspi, test_address, write_buffer, TEST_PAGE_SIZE) != 0) {
        printf("DAC write failed\n");
        return -1;
    }
    printf("DAC write success\n");

    // DAC 读取
    printf("DAC read start\n");
    if (flash_direct_read(qspi, test_address, direct_read_buffer, TEST_PAGE_SIZE) != 0) {
        printf("DAC read failed\n");
        return -1;
    }
    printf("DAC read success\n");

    // 数据校验
    printf("DAC verify start\n");
    for (uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
        if (direct_read_buffer[i] != write_buffer[i]) {
            printf("DAC verify failed: data mismatch at offset %lu\n", (unsigned long)i);
            return -1;
        }
    }
    printf("DAC verify success\n");

    return 0;
}

int main(void)
{
    printf("Initialization start\n");

    board_init();
    printf("Board initialization completed\n");

    cycle_counter_init();

    printf("Program start\n");
    printf("Clear status\n");
    printf("Clock initialization completed\n");

    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0u)
    {
        ahb_clk = SystemCoreClock;
    }
    printf("AHB clock frequency: %lu Hz\n", (unsigned long)ahb_clk);

    // 初始化QSPI控制器
    printf("QSPI initialization start\n");

    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_QSPIFLASH, true);
    rcc_set_cortex_m4_ahb_clock(RCC_CM4_AHB_QSPIFLASH, true);
    rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_QSPIFLASH, false);
    rcc_set_cortex_m4_ahb_reset(RCC_CM4_AHB_QSPIFLASH, false);

    cqspi_dev_t qspi_dev;
    cqspi_config_t qspi_cfg = {
        .reg_base = QSPI_CFG_BASE,
        .ahb_base = M4_SLV_FLASH_BASE,
        .ref_clk_hz = ahb_clk,
        .trigger_address = M4_SLV_FLASH_BASE,
        .sram_partition = 128,  // 128个32位字
        .fifo_width_bytes = 4u,
        .decode_cs = false,
    };

    int init_result = cqspi_init(&qspi_dev, &qspi_cfg);
    if (init_result != 0)
    {
        printf("QSPI initialization failed\n");
        // Continue to program end instead of infinite loop
    }
    else
    {
        // 设置QSPI时钟
        if (qspi_configure_speed_with_capture(&qspi_dev, 24000000u) != 0) {
            printf("Clock configuration failed\n");
            init_result = -1;
        } else {
            printf("QSPI initialization success\n");

            // 配置Quad读写指令
            cqspi_indirect_write_config_t quad_write_cfg = {
                .opcode = 0x32,  // Page Program (单线)
                .addr_bytes = 3,
                .instr_width = CQSPI_BUSWIDTH_1,
                .addr_width = CQSPI_BUSWIDTH_1,
                .data_width = CQSPI_BUSWIDTH_4,  // 单线数据
                .mode_enable = false,
                .mode_bits = 0
            };

            cqspi_indirect_read_config_t quad_read_cfg = {
                .opcode = 0x6B,  // Read Data (单线)
                .addr_bytes = 3,
                .instr_width = CQSPI_BUSWIDTH_1,
                .addr_width = CQSPI_BUSWIDTH_1,
                .data_width = CQSPI_BUSWIDTH_4,
                .dummy_cycles = 8,
                .mode_enable = false,
                .mode_bits = 0
            };

            if (cqspi_configure_indirect_write(&qspi_dev, &quad_write_cfg) != 0) {
                printf("Quad write configuration failed\n");
                init_result = -1;
            } else if (cqspi_configure_indirect_read(&qspi_dev, &quad_read_cfg) != 0) {
                printf("Quad read configuration failed\n");
                init_result = -1;
            } else {
                printf("Quad mode configured\n");
                g_cached_rd_instr = qspi_reg_read(&qspi_dev, CQSPI_REG_RD_INSTR);
                g_cached_mode_bits = qspi_reg_read(&qspi_dev, CQSPI_REG_MODE_BIT) & CQSPI_MODE_BITS_MASK;
                g_cached_read_config_valid = true;
            }
        }
    }

    // 执行基础Flash信息读取测试
    flash_info_t flash_info = {0};
    int test_result = -1;
    if (init_result == 0)
    {
        test_result = perform_basic_flash_test(&qspi_dev, &flash_info);
        // 将Flash信息存储到debug变量中
        if (test_result == 0)
        {
            // Flash信息格式: [厂家ID(8bit)][设备ID(8bit)][存储类型(8bit)][容量代码(8bit)]
            printf("Flash info: Manufacturer ID=0x%02X, Device ID=0x%02X, Capacity Code=0x%02X, Type=%s, Size=%lu bytes\n",
                flash_info.manuf_id, flash_info.memory_type, flash_info.capacity,
                flash_info.type_name, (unsigned long)flash_info.size_bytes);

            printf("Flash ID read success\n");
        }
        else
        {
            printf("Flash ID read failed\n");
        }
    }

    // 固定24MHz后执行后续操作
    int erase_time_result = -1;
    int dac_throughput_result = -1;
    int xip_test_24mhz_result = -1;
    int xip_test_48mhz_result = -1;
    int xip_test_96mhz_result = -1;

    if (init_result == 0 && test_result == 0)
    {
        // 在切换时钟之前，先进行写保护禁用和四线模式启用
        if (write_protect_disable(&qspi_dev) != 0) {
            printf("Write protect disable failed before throughput test\n");
            test_result = -1;
        } else if (quad_enable(&qspi_dev) != 0) {
            printf("Quad enable failed before throughput test\n");
            test_result = -1;
        } else {
            uint32_t erase_time_us = 0u;
            uint32_t erase_address = flash_info.size_bytes - TEST_SUBSECTOR_SIZE;
            erase_time_result = measure_subsector_erase_time(&qspi_dev, erase_address, &erase_time_us);

            if (erase_time_result == 0) {
                printf("Measured erase time at address 0x%08lX\n", (unsigned long)erase_address);
                dac_throughput_result = test_dac_throughput_1mb(&qspi_dev, &flash_info);
                if (dac_throughput_result == 0) {
                    xip_test_24mhz_result = test_xip_mode_144(&qspi_dev, &flash_info, true);

                    if (xip_test_24mhz_result == 0) {
                        printf("Reconfiguring QSPI for 48MHz XIP validation\n");
                        if (qspi_configure_speed_with_capture(&qspi_dev, 48000000u) != 0) {
                            printf("Failed to configure QSPI clock to 48MHz for XIP test\n");
                            xip_test_48mhz_result = -1;
                        } else {
                            printf("Starting XIP test at 48MHz\n");
                            xip_test_48mhz_result = test_xip_mode_144(&qspi_dev, &flash_info, false);

                            if (xip_test_48mhz_result == 0) {
                                printf("Reconfiguring QSPI for 96MHz XIP validation\n");
                                if (qspi_configure_speed_with_capture(&qspi_dev, 96000000u) != 0) {
                                    printf("Failed to configure QSPI clock to 96MHz for XIP test\n");
                                    xip_test_96mhz_result = -1;
                                } else {
                                    printf("Starting XIP test at 96MHz\n");
                                    xip_test_96mhz_result = test_xip_mode_144(&qspi_dev, &flash_info, false);
                                }
                            }
                        }

                        if (qspi_configure_speed_with_capture(&qspi_dev, 24000000u) != 0) {
                            printf("Warning: failed to restore QSPI clock to 24MHz after high-speed XIP tests\n");
                        }
                    }
                }
            }
        }
    }

    // 如果基础测试成功，执行完整的Flash功能测试 (保持原有逻辑作为备选)
    int dac_test_result = -1;
    if (test_result == 0 && dac_throughput_result == 0 &&
        xip_test_24mhz_result == 0 && xip_test_48mhz_result == 0 &&
        xip_test_96mhz_result == 0)
    {
        dac_test_result = perform_direct_access_test(&qspi_dev, &flash_info);
    }

    

    printf("Program end normally\n");

    // Set final result at the very end to avoid early simulation termination
    // Current test scope: Flash initialization + ID reading + erase timing + DAC throughput + full read/write test
    if (init_result != 0)
    {
        printf("Initialization failed\n");
    }
    else if (test_result != 0)
    {
        printf("ID read failed\n");
    }
    else if (erase_time_result != 0)
    {
        printf("Erase timing test failed\n");
    }
    else if (dac_throughput_result != 0)
    {
        printf("DAC throughput test failed\n");
    }
    else if (xip_test_24mhz_result != 0)
    {
        printf("XIP mode test at 24MHz failed\n");
    }
    else if (xip_test_48mhz_result != 0)
    {
        printf("XIP mode test at 48MHz failed\n");
    }
    else if (xip_test_96mhz_result != 0)
    {
        printf("XIP mode test at 96MHz failed\n");
    }
    else if (dac_test_result != 0)
    {
        printf("DAC test failed\n");
    }
    else
    {
        printf("All tests successful\n");
    }

    while (1)
    {
        __WFI();
    }
}
