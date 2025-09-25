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
        printf("DAC write failed: write protect disable failed\n");
        return -1;
    }

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
            printf("DAC verify failed: data mismatch at offset %u\n", i);
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

    printf("Program start\n");
    printf("Clear status\n");
    printf("Clock initialization completed\n");

    uint32_t ahb_clk = rcc_get_clock(RCC_CLOCK_AHB);
    if (ahb_clk == 0u)
    {
        ahb_clk = SystemCoreClock;
    }
    printf("AHB clock frequency: %u Hz\n", ahb_clk);

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
        if (cqspi_configure_clock(&qspi_dev, 24000000u) != 0) {
            printf("Clock configuration failed\n");
            init_result = -1;
        } else {
            printf("QSPI initialization success\n");
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
        // Flash信息格式: [厂家ID(8bit)][设备ID(8bit)][存储类型(8bit)][容量代码(8bit)]
        printf("Flash info: Manufacturer ID=0x%02X, Device ID=0x%02X, Capacity Code=0x%02X, Type=%s, Size=%u bytes\n",
               flash_info.manuf_id, flash_info.memory_type, flash_info.capacity,
               flash_info.type_name, flash_info.size_bytes);

        printf("Flash ID read success\n");
    }
    else
    {
        printf("Flash ID read failed\n");
    }

    printf("Program end normally\n");

    // Set final result at the very end to avoid early simulation termination
    // Current test scope: Flash initialization + ID reading + full read/write test
    if (init_result != 0)
    {
        printf("Initialization failed\n");
    }
    else if (test_result != 0)
    {
        printf("ID read failed\n");
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
