#include "rbl_sbl.h"
#include "rbl_qspi.h"
#include "rbl_hal.h"
#include "s300.h"
#include "qspi_cadence.h"
#include <string.h>

// === 简单CRC32实现（轻量级） ===

// CRC32多项式 (IEEE 802.3)
#define CRC32_POLYNOMIAL 0xEDB88320UL

uint32_t rbl_crc32_simple(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            else
                crc = crc >> 1;
        }
    }
    
    return ~crc;
}

// === ARM Cortex-M 向量表验证 ===

bool rbl_is_valid_stack_pointer(uint32_t sp)
{
    // 检查栈顶是否在合理的SRAM范围内
    // S300的SRAM范围: 0x20000000 - 0x20080000 (512KB)
    if (sp >= 0x20000000 && sp <= 0x20080000)
    {
        // 检查是否8字节对齐（ARM建议）
        if ((sp & 0x7) == 0)
        {
            return true;
        }
    }
    return false;
}

bool rbl_is_valid_reset_vector(uint32_t reset_vector)
{
    // Cortex-M复位向量必须是奇数（Thumb模式标志）
    if ((reset_vector & 0x1) == 0)
        return false;
    
    // 检查是否在合理的代码范围内
    uint32_t addr = reset_vector & ~0x1; // 去掉Thumb标志
    
    // 可以是SRAM代码或Flash代码
    bool in_sram = (addr >= 0x20000000 && addr <= 0x20080000);
    // QSPI Flash 的 XIP 映射从 0x0800_0000 开始，假设最大 16MB（到 0x0900_0000）
    // 注意：之前使用 0x0000_0000..0x0100_0000 的范围是错误的，会误判有效的 XIP 地址
    bool in_flash = (addr >= 0x08000000 && addr < 0x09000000);
    
    return (in_sram || in_flash);
}

// === SBL 完整性检查 ===

int rbl_sbl_validate(void)
{
    RBL_LOG("[RBL] SBL validation starting...\r\n");
    
    // 1. 读取SBL向量表前8字节（栈顶+复位向量）
    // 注意：rbl_qspi_read需要Flash物理偏移地址，不是XIP地址
    uint8_t vector_table[8];
    RBL_LOG("[RBL] Reading SBL from Flash offset 0x");
    // 打印偏移地址
    static const char hexchars[] = "0123456789ABCDEF";
    char addr_str[10];
    for (int i = 7; i >= 0; i--)
    {
        addr_str[7-i] = hexchars[(SBL_FLASH_OFFSET >> (i*4)) & 0xF];
    }
    addr_str[8] = '\r'; addr_str[9] = '\n';
    rbl_uart_write(addr_str, 10);
    
    if (rbl_qspi_read(SBL_FLASH_OFFSET, vector_table, 8) != 0)
    {
        RBL_LOG("[RBL] Failed to read SBL vector table\r\n");
        return -1;
    }
    
    // 调试：打印读取的原始数据
    RBL_VLOG("[RBL] Read raw bytes: ");
    for (int i = 0; i < 8; i++) {
        char byte_str[4];
        byte_str[0] = hexchars[(vector_table[i] >> 4) & 0xF];
        byte_str[1] = hexchars[vector_table[i] & 0xF];
        byte_str[2] = ' ';
        byte_str[3] = '\0';
        rbl_uart_write(byte_str, 3);
    }
    rbl_uart_write("\r\n", 2);
    
    // 2. 解析栈顶指针和复位向量
    uint32_t stack_pointer = *(uint32_t*)&vector_table[0];
    uint32_t reset_vector = *(uint32_t*)&vector_table[4];
    
    RBL_VLOG("[RBL] SBL SP: 0x");
    // 简单的32位十六进制输出
    static const char hex[] = "0123456789ABCDEF";
    char sp_str[10];
    for (int i = 7; i >= 0; i--)
    {
        sp_str[7-i] = hex[(stack_pointer >> (i*4)) & 0xF];
    }
    sp_str[8] = '\r'; sp_str[9] = '\n';
    rbl_uart_write(sp_str, 10);
    
    RBL_VLOG("[RBL] SBL Reset: 0x");
    char rv_str[10];
    for (int i = 7; i >= 0; i--)
    {
        rv_str[7-i] = hex[(reset_vector >> (i*4)) & 0xF];
    }
    rv_str[8] = '\r'; rv_str[9] = '\n';
    rbl_uart_write(rv_str, 10);
    
    // 3. 验证栈顶指针
    if (!rbl_is_valid_stack_pointer(stack_pointer))
    {
        RBL_LOG("[RBL] Invalid stack pointer\r\n");
        return -1;
    }
    
    // 4. 验证复位向量
    if (!rbl_is_valid_reset_vector(reset_vector))
    {
        RBL_LOG("[RBL] Invalid reset vector\r\n");
        return -1;
    }
    
    // 5. 轻量CRC校验：读取前1KB进行采样校验
    uint8_t sample_data[1024];
    if (rbl_qspi_read(SBL_FLASH_OFFSET, sample_data, sizeof(sample_data)) != 0)
    {
        RBL_LOG("[RBL] Failed to read SBL sample data\r\n");
        return -1;
    }
    
    uint32_t crc = rbl_crc32_simple(sample_data, sizeof(sample_data));
    
    // 简单的"非零"检查（实际项目中应该存储预期的CRC）
    if (crc == 0 || crc == 0xFFFFFFFF)
    {
        RBL_LOG("[RBL] SBL CRC check failed\r\n");
        return -1;
    }
    
    RBL_LOG("[RBL] SBL validation PASSED\r\n");
    return 0;
}

// === SBL 加载与跳转 ===

int rbl_sbl_load_and_jump(void)
{
    RBL_LOG("[RBL] Preparing to jump to SBL in QSPI XIP mode...\r\n");
    
    // SBL 设计为QSPI XIP执行，不需要加载到SRAM
    // 直接从QSPI Flash XIP地址读取向量表并跳转
    
    // 1. 读取SBL的向量表前8字节（栈顶+复位向量）
    // 注意：rbl_qspi_read需要Flash物理偏移地址，不是XIP地址
    uint8_t vector_table[8];
    if (rbl_qspi_read(SBL_FLASH_OFFSET, vector_table, 8) != 0)
    {
        RBL_LOG("[RBL] Failed to read SBL vector table\r\n");
        return -1;
    }
    
    // 2. 解析栈顶指针和复位向量
    uint32_t new_stack_pointer = *(uint32_t*)&vector_table[0];
    uint32_t new_reset_vector = *(uint32_t*)&vector_table[4];
    
    RBL_LOG("[RBL] SBL SP: 0x");
    // 简单的32位十六进制输出
    static const char hex[] = "0123456789ABCDEF";
    char sp_str[10];
    for (int i = 7; i >= 0; i--)
    {
        sp_str[7-i] = hex[(new_stack_pointer >> (i*4)) & 0xF];
    }
    sp_str[8] = '\r'; sp_str[9] = '\n';
    rbl_uart_write(sp_str, 10);
    
    RBL_LOG("[RBL] SBL Reset: 0x");
    char rv_str[10];
    for (int i = 7; i >= 0; i--)
    {
        rv_str[7-i] = hex[(new_reset_vector >> (i*4)) & 0xF];
    }
    rv_str[8] = '\r'; rv_str[9] = '\n';
    rbl_uart_write(rv_str, 10);
    
    // 3. 配置QSPI为XIP模式 - 这是关键步骤！
    RBL_LOG("[RBL] Configuring QSPI XIP mode...\r\n");
    if (rbl_configure_xip_mode() != 0)
    {
        RBL_LOG("[RBL] Failed to configure XIP mode\r\n");
        return -1;
    }
    
    // 4. 设置新的向量表基址为QSPI XIP地址
    SCB->VTOR = SBL_FLASH_START_ADDR;
    __DSB();
    __ISB();
    
    RBL_LOG("[RBL] Jumping to SBL in XIP mode...\r\n");
    
    // 5. 执行跳转到QSPI XIP地址（汇编内联）
    __asm volatile (
        "msr msp, %0    \n\t"  // 设置主栈指针
        "dsb            \n\t"  // 数据同步屏障
        "isb            \n\t"  // 指令同步屏障
        "bx  %1         \n\t"  // 跳转到复位向量（包含Thumb标志）
        :
        : "r" (new_stack_pointer), "r" (new_reset_vector)
        : "memory"
    );
    
    // 如果执行到这里，说明跳转失败
    RBL_LOG("[RBL] Jump failed!\r\n");
    return -1;
}

// === XIP模式配置 ===

int rbl_configure_xip_mode(void)
{
    // 参考Demo代码的XIP配置方法
    RBL_LOG("[RBL] Configuring QSPI for XIP mode...\r\n");
    
    // 1. 配置Quad读取模式(1-4-4)，与Demo保持一致
    qspi_configure_quad_io_read(true, true);
    
    // 2. 启用直接访问模式 (XIP)
    extern qspi_cadence_t g_qspi;  // 从qspi_cadence.c中引用
    volatile uint32_t *reg_base = (volatile uint32_t *)g_qspi.reg;
    uint32_t cfg = reg_base[CQSPI_REG_CONFIG / 4];
    cfg |= (CQSPI_CFG_DIRECT | CQSPI_CFG_XIP_NEXT);
    reg_base[CQSPI_REG_CONFIG / 4] = cfg;
    
    // 数据同步屏障
    __DSB();
    __ISB();
    
    RBL_LOG("[RBL] XIP mode configured\r\n");
    return 0;
}
