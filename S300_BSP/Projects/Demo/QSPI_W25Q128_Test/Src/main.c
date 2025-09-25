#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "qspi_cadence.h"  // 直接使用QSPI底层接口

typedef uint32_t UINT32;

/*
 * Debug Flow 状态说明:
 *
 * 主阶段划分 (高8位):
 *   0x00: 初始化阶段
 *   0x10: Flash初始化阶段
 *   0x20: 基本功能测试阶段
 *   0x30: 四线(QSPI)测试阶段
 *   0x40: 程序完成阶段
 *
 * 子阶段划分 (低8位):
 *   0x01: 开始
 *   0x02: 成功完成
 *   0x03: 失败
 *   0x11: 写入相关开始
 *   0x12: 写入相关成功
 *   0x13: 写入相关失败
 *   0x21: 读取相关开始
 *   0x22: 读取相关成功
 *   0x23: 读取相关失败
 *   0x31: 验证相关开始
 *   0x32: 验证相关成功
 *   0x33: 验证相关失败
 *
 * 具体状态值:
 *   0x0001: 初始化开始
 *   0x0002: 板级初始化完成
 *   0x0003: 时钟初始化完成
 *   0x1001: Flash初始化开始
 *   0x1002: Flash初始化成功
 *   0x1003: Flash初始化失败
 *   0x2001: 擦除测试开始
 *   0x2002: 擦除测试成功
 *   0x2003: 擦除测试失败
 *   0x2011: 写入测试开始
 *   0x2012: 写入测试成功
 *   0x2013: 写入测试失败
 *   0x2021: 读取测试开始
 *   0x2022: 读取测试成功
 *   0x2023: 读取测试失败
 *   0x2031: 验证测试开始
 *   0x2032: 验证测试成功
 *   0x2033: 验证测试失败
 *   0x3001: 四线配置开始
 *   0x3002: 四线配置成功
 *   0x3003: 四线配置失败
 *   0x3011: 四线读取开始
 *   0x3012: 四线读取成功
 *   0x3013: 四线读取失败
 *   0x3021: 四线擦除开始
 *   0x3022: 四线擦除成功
 *   0x3023: 四线擦除失败
 *   0x3031: 四线写入开始
 *   0x3032: 四线写入成功
 *   0x3033: 四线写入失败
 *   0x3041: 四线IO测试开始
 *   0x3042: 四线IO测试成功
 *   0x3043: 四线IO测试失败
 *   0x4001: 所有测试通过
 *   0x4002: 程序正常结束
 *
 * 使用方法:
 * 通过读取DEBUG_FLOW_c的值，可以确定程序执行到了哪个阶段。
 * 如果程序异常终止，通过DEBUG_FLOW_c可以快速定位问题发生的位置。
 *
 * 当前测试策略:
 * 由于硬件调试阶段可能不知道Flash的具体型号和厂家信息，
 * 程序目前只进行Flash初始化和ID读取测试，不进行复杂的读写操作。
 * 这可以帮助确认：
 *   - Flash芯片是否存在
 *   - QSPI通信是否正常
 *   - 芯片的基本信息（厂家ID、设备ID、容量等）
 *
 * DEBUG_ARG1_c 格式: [厂家ID(8bit)][设备ID(8bit)][存储类型(8bit)][容量代码(8bit)]
 * 例如: 0xEF401518 表示 Winbond W25Q128 (EF=Winbond, 40=W25Q128)
 */

#define DEBUG_BASE    			(M4_SLV_RAM0_END - 0x3F) //RAM8K

#define DEBUG_ARG0				(DEBUG_BASE + 0x0000 ) //
#define DEBUG_ARG1				(DEBUG_BASE + 0x0004 ) //
#define DEBUG_ARG2				(DEBUG_BASE + 0x0008 ) //
#define DEBUG_ARG3				(DEBUG_BASE + 0x000C ) //

#define DEBUG_ST				(DEBUG_BASE + 0x0010 ) // 测试状态；OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_ISRS				(DEBUG_BASE + 0x0014 ) // 中断次数;目前程序中预设10次
#define DEBUG_CODE				(DEBUG_BASE + 0x0018 ) // 程序运行结束 OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_FLOW				(DEBUG_BASE + 0x001C ) //

//c语言调用
#define DEBUG_ARG0_c			(*((volatile UINT32*)(DEBUG_ARG0 ))) //
#define DEBUG_ARG1_c			(*((volatile UINT32*)(DEBUG_ARG1 ))) //
#define DEBUG_ARG2_c			(*((volatile UINT32*)(DEBUG_ARG2 ))) //
#define DEBUG_ARG3_c			(*((volatile UINT32*)(DEBUG_ARG3 ))) //

#define DEBUG_ST_c				(*((volatile UINT32*)(DEBUG_ST ))) // 测试状态；OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_ISRS_c			(*((volatile UINT32*)(DEBUG_ISRS ))) // 中断次数
#define DEBUG_CODE_c			(*((volatile UINT32*)(DEBUG_CODE ))) // 程序运行结束 OK:0xAAAAAAAA;ERR OTHER
#define DEBUG_FLOW_c			(*((volatile UINT32*)(DEBUG_FLOW ))) //

// Debug Flow 状态定义 (高8位:主阶段, 低8位:子阶段)
// 主阶段: 0x00=初始化, 0x10=Flash初始化, 0x20=基本测试, 0x30=四线测试, 0x40=完成
// 子阶段: 具体操作步骤
#define FLOW_INIT_START         0x0001  // 初始化开始
#define FLOW_INIT_BOARD         0x0002  // 板级初始化完成
#define FLOW_INIT_RCC           0x0003  // 时钟初始化完成
#define FLOW_INIT_FLASH_START   0x1001  // Flash初始化开始
#define FLOW_INIT_FLASH_OK      0x1002  // Flash初始化成功
#define FLOW_INIT_FLASH_FAIL    0x1003  // Flash初始化失败
#define FLOW_TEST_ERASE_START   0x2001  // 擦除测试开始
#define FLOW_TEST_ERASE_OK      0x2002  // 擦除测试成功
#define FLOW_TEST_ERASE_FAIL    0x2003  // 擦除测试失败
#define FLOW_TEST_WRITE_START   0x2011  // 写入测试开始
#define FLOW_TEST_WRITE_OK      0x2012  // 写入测试成功
#define FLOW_TEST_WRITE_FAIL    0x2013  // 写入测试失败
#define FLOW_TEST_READ_START    0x2021  // 读取测试开始
#define FLOW_TEST_READ_OK       0x2022  // 读取测试成功
#define FLOW_TEST_READ_FAIL     0x2023  // 读取测试失败
#define FLOW_TEST_VERIFY_START  0x2031  // 验证测试开始
#define FLOW_TEST_VERIFY_OK     0x2032  // 验证测试成功
#define FLOW_TEST_VERIFY_FAIL   0x2033  // 验证测试失败
#define FLOW_DAC_WRITE_START    0x2041  // DAC写入开始
#define FLOW_DAC_WRITE_OK       0x2042  // DAC写入成功
#define FLOW_DAC_WRITE_FAIL     0x2043  // DAC写入失败
#define FLOW_DAC_READ_START     0x2051  // DAC读取开始
#define FLOW_DAC_READ_OK        0x2052  // DAC读取成功
#define FLOW_DAC_READ_FAIL      0x2053  // DAC读取失败
#define FLOW_DAC_VERIFY_START   0x2061  // DAC验证开始
#define FLOW_DAC_VERIFY_OK      0x2062  // DAC验证成功
#define FLOW_DAC_VERIFY_FAIL    0x2063  // DAC验证失败
#define FLOW_QUAD_CONFIG_START  0x3001  // 四线配置开始
#define FLOW_QUAD_CONFIG_OK     0x3002  // 四线配置成功
#define FLOW_QUAD_CONFIG_FAIL   0x3003  // 四线配置失败
#define FLOW_QUAD_READ_START    0x3011  // 四线读取开始
#define FLOW_QUAD_READ_OK       0x3012  // 四线读取成功
#define FLOW_QUAD_READ_FAIL     0x3013  // 四线读取失败
#define FLOW_QUAD_ERASE_START   0x3021  // 四线擦除开始
#define FLOW_QUAD_ERASE_OK      0x3022  // 四线擦除成功
#define FLOW_QUAD_ERASE_FAIL    0x3023  // 四线擦除失败
#define FLOW_QUAD_WRITE_START   0x3031  // 四线写入开始
#define FLOW_QUAD_WRITE_OK      0x3032  // 四线写入成功
#define FLOW_QUAD_WRITE_FAIL    0x3033  // 四线写入失败
#define FLOW_QUAD_IO_START      0x3041  // 四线IO测试开始
#define FLOW_QUAD_IO_OK         0x3042  // 四线IO测试成功
#define FLOW_QUAD_IO_FAIL       0x3043  // 四线IO测试失败
#define FLOW_ALL_TESTS_PASS     0x4001  // 所有测试通过
#define FLOW_PROGRAM_END        0x4002  // 程序正常结束

#define TEST_PAGE_SIZE          256u
#define TEST_SUBSECTOR_SIZE     4096u   // N25Q Subsector Erase是4KB (4096字节)

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

// 启用或关闭Direct Access Controller (DAC)
static int set_direct_access_mode(cqspi_dev_t *qspi, bool enable) {
    if (!qspi || !qspi->regs) {
        return -1;
    }

    volatile uint32_t *config_reg = (volatile uint32_t *)(qspi->regs + CQSPI_REG_CONFIG);
    uint32_t config = *config_reg;

    if (enable) {
        config |= CQSPI_CFG_DIRECT;
        config |= CQSPI_CFG_ENABLE;
    } else {
        config &= ~CQSPI_CFG_DIRECT;
    }

    *config_reg = config;

    return cqspi_wait_idle(qspi, 1000u);
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

// 写保护禁用 (清除状态寄存器中的写保护位)
static int write_protect_disable(cqspi_dev_t *qspi) {
    // 写使能
    if (write_enable(qspi) != 0) {
        return -1;
    }

    // 发送Write Status Register命令，清除写保护位
    // 对于大多数Flash芯片，状态寄存器第7位是SRP (Status Register Protect)
    // 第2-5位是BP (Block Protect)位，需要全部清零
    uint8_t status_reg = 0x00;  // 清除所有保护位

    cqspi_stig_cmd_t cmd = {
        .opcode = 0x01,      // Write Status Register
        .addr_bytes = 0,
        .read_len = 0,
        .write_len = 1,
        .dummy_cycles = 0,
        .mode_enable = false,
    };

    if (cqspi_stig_execute(qspi, &cmd, NULL, &status_reg) != 0) {
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

    if (write_protect_disable(qspi) != 0) {
        DEBUG_FLOW_c = FLOW_DAC_WRITE_FAIL;
        return -1;
    }

    if (erase_subsector(qspi, test_address) != 0) {
        DEBUG_FLOW_c = FLOW_DAC_WRITE_FAIL;
        return -1;
    }

    // DAC 写入
    DEBUG_FLOW_c = FLOW_DAC_WRITE_START;
    if (flash_direct_write(qspi, test_address, write_buffer, TEST_PAGE_SIZE) != 0) {
        DEBUG_FLOW_c = FLOW_DAC_WRITE_FAIL;
        return -1;
    }
    DEBUG_FLOW_c = FLOW_DAC_WRITE_OK;

    // DAC 读取
    DEBUG_FLOW_c = FLOW_DAC_READ_START;
    if (flash_direct_read(qspi, test_address, direct_read_buffer, TEST_PAGE_SIZE) != 0) {
        DEBUG_FLOW_c = FLOW_DAC_READ_FAIL;
        return -1;
    }
    DEBUG_FLOW_c = FLOW_DAC_READ_OK;

    // 数据校验
    DEBUG_FLOW_c = FLOW_DAC_VERIFY_START;
    for (uint32_t i = 0; i < TEST_PAGE_SIZE; i++) {
        if (direct_read_buffer[i] != write_buffer[i]) {
            DEBUG_FLOW_c = FLOW_DAC_VERIFY_FAIL;
            return -1;
        }
    }
    DEBUG_FLOW_c = FLOW_DAC_VERIFY_OK;

    return 0;
}

int main(void)
{
    DEBUG_FLOW_c = FLOW_INIT_START; // 初始化开始

    board_init();
    DEBUG_FLOW_c = FLOW_INIT_BOARD; // 板级初始化完成

    DEBUG_CODE_c = 0; // Program start
    DEBUG_ST_c = 0;   // Clear status
    DEBUG_FLOW_c = FLOW_INIT_RCC; // 时钟初始化完成

    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0u)
    {
        ahb_clk = SystemCoreClock;
    }
    DEBUG_ARG0_c = ahb_clk; // Store AHB clock frequency

    // 初始化QSPI控制器
    DEBUG_FLOW_c = FLOW_INIT_FLASH_START; // QSPI初始化开始

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
        DEBUG_FLOW_c = FLOW_INIT_FLASH_FAIL; // QSPI初始化失败
        // Continue to program end instead of infinite loop
    }
    else
    {
        // 设置QSPI时钟
        if (cqspi_configure_clock(&qspi_dev, 24000000u) != 0) {
            DEBUG_FLOW_c = FLOW_INIT_FLASH_FAIL; // 时钟配置失败
            init_result = -1;
        } else {
            DEBUG_FLOW_c = FLOW_INIT_FLASH_OK; // QSPI初始化成功
        }
    }

    // 执行基础Flash信息读取测试
    flash_info_t flash_info;
    int test_result = -1;
    if (init_result == 0)
    {
        test_result = perform_basic_flash_test(&qspi_dev, &flash_info);
    }

    // 如果基础测试成功，执行完整的Flash功能测试
    int dac_test_result = -1;
    if (test_result == 0)
    {
        dac_test_result = perform_direct_access_test(&qspi_dev, &flash_info);
    }

    // 将Flash信息存储到debug变量中
    if (test_result == 0)
    {
        // DEBUG_ARG1_c 格式: [厂家ID(8bit)][设备ID(8bit)][存储类型(8bit)][容量代码(8bit)]
        DEBUG_ARG1_c = (flash_info.manuf_id << 24) | (flash_info.memory_type << 16) |
                      (flash_info.capacity << 8) | (uint8_t)flash_info.type;
        DEBUG_ARG2_c = flash_info.size_bytes;
        DEBUG_ARG3_c = 0;  // 保留字段

        DEBUG_FLOW_c = FLOW_INIT_FLASH_OK; // Flash ID读取成功
    }
    else
    {
        DEBUG_FLOW_c = FLOW_INIT_FLASH_FAIL; // Flash ID读取失败
    }

    DEBUG_FLOW_c = FLOW_PROGRAM_END; // 程序正常结束

    // Set final result at the very end to avoid early simulation termination
    // Current test scope: Flash initialization + ID reading + full read/write test
    if (init_result != 0)
    {
        DEBUG_CODE_c = 0xEEEEEEEE; // Init failed - set at program end
    }
    else if (test_result != 0)
    {
        DEBUG_CODE_c = 0xEEEEEEEE; // ID read failed - set at program end
    }
    else if (dac_test_result != 0)
    {
        DEBUG_CODE_c = 0xEEEEEEEE; // DAC test failed - set at program end
    }
    else
    {
        DEBUG_CODE_c = 0xAAAAAAAA; // All tests successful - set at program end
    }

    while (1)
    {
        __WFI();
    }
}
