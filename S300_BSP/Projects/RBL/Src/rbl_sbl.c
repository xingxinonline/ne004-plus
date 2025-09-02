#include "rbl_sbl.h"
#include "rbl_qspi.h"
#include "rbl_hal.h"
#include "s300.h"
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
    bool in_flash = (addr >= 0x00000000 && addr <= 0x01000000); // 16MB Flash范围
    
    return (in_sram || in_flash);
}

// === SBL 完整性检查 ===

int rbl_sbl_validate(void)
{
    RBL_LOG("[RBL] SBL validation starting...\r\n");
    
    // 1. 读取SBL向量表前8字节（栈顶+复位向量）
    uint8_t vector_table[8];
    if (rbl_qspi_read(SBL_FLASH_START_ADDR, vector_table, 8) != 0)
    {
        RBL_LOG("[RBL] Failed to read SBL vector table\r\n");
        return -1;
    }
    
    // 2. 解析栈顶指针和复位向量
    uint32_t stack_pointer = *(uint32_t*)&vector_table[0];
    uint32_t reset_vector = *(uint32_t*)&vector_table[4];
    
    RBL_LOG("[RBL] SBL SP: 0x");
    // 简单的32位十六进制输出
    static const char hex[] = "0123456789ABCDEF";
    char sp_str[10];
    for (int i = 7; i >= 0; i--)
    {
        sp_str[7-i] = hex[(stack_pointer >> (i*4)) & 0xF];
    }
    sp_str[8] = '\r'; sp_str[9] = '\n';
    rbl_uart_write(sp_str, 10);
    
    RBL_LOG("[RBL] SBL Reset: 0x");
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
    if (rbl_qspi_read(SBL_FLASH_START_ADDR, sample_data, sizeof(sample_data)) != 0)
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
    RBL_LOG("[RBL] Loading SBL to SRAM...\r\n");
    
    // 1. 确定要加载的SBL大小（简化：固定64KB）
    const uint32_t sbl_size = 64 * 1024; // 64KB
    
    // 2. 分块加载SBL到SRAM（避免大块内存分配）
    const uint32_t chunk_size = 1024; // 1KB per chunk
    uint8_t *sram_dst = (uint8_t*)SBL_SRAM_LOAD_ADDR;
    
    for (uint32_t offset = 0; offset < sbl_size; offset += chunk_size)
    {
        uint32_t current_chunk = (offset + chunk_size <= sbl_size) ? chunk_size : (sbl_size - offset);
        
        if (rbl_qspi_read(SBL_FLASH_START_ADDR + offset, sram_dst + offset, current_chunk) != 0)
        {
            RBL_LOG("[RBL] SBL load failed at offset\r\n");
            return -1;
        }
        
        // 进度指示（每16KB）
        if ((offset % (16 * 1024)) == 0)
        {
            RBL_LOG(".");
        }
    }
    
    RBL_LOG("\r\n[RBL] SBL loaded, preparing jump...\r\n");
    
    // 3. 读取新的向量表
    uint32_t *new_vector_table = (uint32_t*)SBL_SRAM_LOAD_ADDR;
    uint32_t new_stack_pointer = new_vector_table[0];
    uint32_t new_reset_vector = new_vector_table[1];
    
    // 4. 设置新的向量表基址
    SCB->VTOR = SBL_SRAM_LOAD_ADDR;
    __DSB();
    
    RBL_LOG("[RBL] Jumping to SBL...\r\n");
    
    // 5. 执行跳转（汇编内联）
    __asm volatile (
        "msr msp, %0    \n\t"  // 设置主栈指针
        "bx  %1         \n\t"  // 跳转到复位向量（包含Thumb标志）
        :
        : "r" (new_stack_pointer), "r" (new_reset_vector)
        : "memory"
    );
    
    // 如果执行到这里，说明跳转失败
    RBL_LOG("[RBL] Jump failed!\r\n");
    return -1;
}
