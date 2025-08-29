/*
 * S300 镜像头文件定义
 * 严格按照S300 ROMBOOT规范设计 - 256字节Header Layout
 * Header位于Flash起始地址，供ROMBOOT读取验证和加载
 */

#ifndef __IMAGE_HEADER_H__
#define __IMAGE_HEADER_H__

#include <stdint.h>

/* 11.3.1 Pro字段 - 段属性 (偏移0x00, 0x20, 0x40, 0xD0) */
#define PRO_RUN_TYPE_MASK           (0xF << 21)    // Bit[24:21] Run Type
#define PRO_RUN_TYPE_NONE           (0x0 << 21)    // 保留，不做任何运行

/* Cortex-M0 Run Type */
#define PRO_M0_LOAD_ONLY            (0x1 << 21)    // Bit0: 只加载，不运行
#define PRO_M0_LOAD_RUN             (0x2 << 21)    // Bit1: 加载后执行
#define PRO_M0_RESET_BEFORE_LOAD    (0x4 << 21)    // Bit2: 加载前复位
#define PRO_M0_HOT_RESET_RUN        (0x8 << 21)    // Bit3: 热复位后运行

/* 控制系统 Run Type */
#define PRO_CTRL_LOAD_ONLY          (0x1 << 21)    // Bit0: 只加载，不运行
#define PRO_CTRL_LOAD_RUN           (0x2 << 21)    // Bit1: 加载后执行

/* CPT Run Type */
#define PRO_CPT_LOAD_ONLY           (0x1 << 21)    // Bit0: 只加载，不运行
#define PRO_CPT_LOAD_RUN            (0x2 << 21)    // Bit1: 加载后执行
#define PRO_CPT_ENABLE_POWER        (0x4 << 21)    // Bit2: 使能电源、时钟、RAM

#define PRO_DEBUG_DISABLE           (1 << 20)      // Bit[20] 禁止校验(仅控制程序段)
#define PRO_FLASH_DIV_MASK          (0xF << 16)    // Bit[19:16] Flash分频值(仅控制程序段)
#define PRO_FLASH_DIV(x)            (((x) & 0xF) << 16)

#define PRO_BASE_XIP                (1 << 15)      // Bit[15] Base XIP
#define PRO_MODE                    (1 << 14)      // Bit[14] MODE支持
#define PRO_DTR                     (1 << 13)      // Bit[13] DTR支持(仅控制系统)
#define PRO_QPI                     (1 << 12)      // Bit[12] QPI支持
#define PRO_ADDRESS_4BYTE           (1 << 11)      // Bit[11] 4字节地址

#define PRO_CPT_ROM_BOOT_MASK       (0x3 << 9)     // Bit[10:9] CPT ROM启动阶段
#define PRO_CPT_ROM_BOOT_APP        (0x0 << 9)     // ARM启动后由APP引导
#define PRO_CPT_ROM_BOOT_PRIORITY   (0x2 << 9)     // ARM ROM中启动，优先于ARM

#define PRO_BIG_ENDIAN              (1 << 8)       // Bit[8] 大端字节序

#define PRO_BOOT_MODE_MASK          (0x3F << 2)    // Bit[7:2] 启动模式
/* 控制系统启动模式 */
#define PRO_CTRL_XIP                (0x00 << 2)    // XIP模式运行
#define PRO_CTRL_RAM0               (0x01 << 2)    // 运行在RAM0(8K)
#define PRO_CTRL_RAM1               (0x02 << 2)    // 运行在RAM1(384K)
/* Cortex-M0启动模式 */
#define PRO_M0_RAM0                 (0x01 << 2)    // 运行在RAM0(8K) - 必须为1
/* CPT PSRAM初始化模式 */
#define PRO_CPT_PSRAM_DISABLE       (0x00 << 2)    // 禁止初始化
#define PRO_CPT_PSRAM_ENABLE        (0x01 << 2)    // 初始化PSRAM
#define PRO_CPT_PSRAM_BEFORE_LOAD   (0x02 << 2)    // 加载前初始化

#define PRO_CHECK_MODE_MASK         (0x3)          // Bit[1:0] 校验模式
#define PRO_CHECK_SUM               (0x0)          // 校验和
#define PRO_CHECK_CRC32             (0x1)          // CRC32
#define PRO_CHECK_DISABLE           (0x3)          // 禁止校验

/* CPT RAM区域索引 */
#define CPT_RAM_PTCM                0              // PTCM
#define CPT_RAM_DTCM                1              // DTCM  
#define CPT_RAM_SRAM0               2              // SRAM0
#define CPT_RAM_SRAM1               3              // SRAM1
#define CPT_RAM_PSRAM               4              // PSRAM

/* CPT单个RAM区域描述结构 (20字节) */
typedef struct {
    uint32_t addr;               // 11.3.2 Flash地址
    uint32_t ram_addr;           // 11.3.7 RAM映射地址  
    uint32_t ram_map;            // 11.3.8 RAM别名地址(保留)
    uint32_t size;               // 11.3.9 段大小
    uint32_t check;              // 11.3.5 校验值
} __attribute__((packed)) cpt_ram_info_t;

/* S300 Header Layout - 严格按照表11-1规范定义 (256字节) */
typedef struct {
    /* 0x00-0x1F: Cortex-M4控制系统程序段 */
    uint32_t ctrl_pro;           // 0x00: Pro属性
    uint32_t ctrl_addr;          // 0x04: Flash地址  
    uint32_t ctrl_exe_addr;      // 0x08: 执行地址
    uint32_t ctrl_len;           // 0x0C: 长度
    uint32_t ctrl_check;         // 0x10: 校验值
    uint8_t  ctrl_version[12];   // 0x14: 版本信息
    
    /* 0x20-0x3F: Cortex-M0程序段 */
    uint32_t m0_pro;             // 0x20: Pro属性
    uint32_t m0_addr;            // 0x24: Flash地址
    uint32_t m0_exe_addr;        // 0x28: 执行地址
    uint32_t m0_len;             // 0x2C: 长度
    uint32_t m0_check;           // 0x30: 校验值
    uint8_t  m0_version[12];     // 0x34: 版本信息
    
    /* 0x40-0xBF: CPT程序段 */
    uint32_t cpt_pro;            // 0x40: Pro属性
    uint32_t cpt_addr;           // 0x44: Flash地址
    uint32_t cpt_exe_addr;       // 0x48: 执行地址
    uint32_t cpt_len;            // 0x4C: 长度
    cpt_ram_info_t cpt_rams[5];  // 0x50-0xB3: 5个RAM区域信息 (5*20=100字节)
    uint8_t  cpt_version[12];    // 0xB4-0xBF: CPT版本信息前12字节
    
    /* 0xC0-0xEF: CPT扩展+Other数据段 */
    uint8_t  cpt_version2[12];   // 0xC0-0xCB: CPT版本后12字节  
    uint32_t psram_cfg_addr;     // 0xCC: PSRAM配置地址
    uint32_t other_pro;          // 0xD0: Other Pro属性
    uint32_t other_addr;         // 0xD4: Other Flash地址
    uint32_t other_exe_addr;     // 0xD8: Other执行地址
    uint32_t other_len;          // 0xDC: Other长度
    uint32_t other_check;        // 0xE0: Other校验值
    uint8_t  other_version[12];  // 0xE4-0xEF: Other版本信息
    
    /* 0xF0-0xFF: 全局配置和校验 */
    uint32_t ref_clock;          // 0xF0: REF时钟
    uint32_t config;             // 0xF4: 配置
    uint32_t fout_clock;         // 0xF8: FOUT时钟
    uint32_t clk_config0;        // 0xFC: 时钟配置0
} __attribute__((packed)) s300_header_t;

/* 静态断言，确保结构体大小为256字节 */
_Static_assert(sizeof(s300_header_t) == 256, "s300_header_t must be 256 bytes");
_Static_assert(sizeof(cpt_ram_info_t) == 20, "cpt_ram_info_t must be 20 bytes");

/* Header字段访问宏 */
#define S300_HEADER_MAGIC_CHECK(hdr)    ((hdr)->ctrl_pro != 0)
#define S300_HEADER_CTRL_VALID(hdr)     (((hdr)->ctrl_pro & PRO_RUN_TYPE_MASK) != PRO_RUN_TYPE_NONE)
#define S300_HEADER_M0_VALID(hdr)       (((hdr)->m0_pro & PRO_RUN_TYPE_MASK) != PRO_RUN_TYPE_NONE)
#define S300_HEADER_CPT_VALID(hdr)      (((hdr)->cpt_pro & PRO_RUN_TYPE_MASK) != PRO_RUN_TYPE_NONE)

/* 工具函数声明 */
uint32_t s300_header_crc32(const s300_header_t *header);
int s300_header_validate(const s300_header_t *header);
void s300_header_init(s300_header_t *header);

/* RBL在Header中的定义(等同控制系统段) */
#define RBL_PRO_DEFAULT     (PRO_CTRL_LOAD_RUN | PRO_CTRL_RAM1 | PRO_CHECK_CRC32)
#define RBL_LOAD_ADDR       0x20002000          // 加载到RAM1起始
#define RBL_ENTRY_POINT     0x20002000          // 入口地址
#define RBL_MAX_SIZE        (64 * 1024)         // 最大64KB

/* Flash布局定义 - 基于ROMBOOT Header加载 */
#define S300_FLASH_BASE         0x08000000      // Flash基址
#define S300_HEADER_OFFSET      0x000000        // Header: 0KB
#define S300_HEADER_SIZE        0x000100        // Header: 256字节
#define S300_RBL_OFFSET         0x000100        // RBL: 256字节开始  
#define S300_RBL_MAX_SIZE       0x010000        // RBL: 最大64KB
#define S300_SBL_OFFSET         0x010100        // SBL: 64KB+256字节开始
#define S300_SBL_MAX_SIZE       0x010000        // SBL: 最大64KB

/* S300 Header Layout - 严格按照规范定义 (256字节) */
typedef struct {
    /* 0x00-0x1F: Cortex-M4控制系统程序段 */
    uint32_t ctrl_pro;           // 0x00: Pro属性
    uint32_t ctrl_addr;          // 0x04: Flash地址
    uint32_t ctrl_exe_addr;      // 0x08: 执行地址
    uint32_t ctrl_len;           // 0x0C: 长度
    uint32_t ctrl_check;         // 0x10: 校验值
    uint8_t  ctrl_version[12];   // 0x14: 版本信息
    
    /* 0x20-0x3F: Cortex-M0程序段 */
    uint32_t m0_pro;             // 0x20: Pro属性
    uint32_t m0_addr;            // 0x24: Flash地址
    uint32_t m0_exe_addr;        // 0x28: 执行地址
    uint32_t m0_len;             // 0x2C: 长度
    uint32_t m0_check;           // 0x30: 校验值
    uint8_t  m0_version[12];     // 0x34: 版本信息
    
    /* 0x40-0xBF: CPT程序段 */
    uint32_t cpt_pro;            // 0x40: Pro属性
    uint32_t cpt_addr;           // 0x44: Flash地址
    uint32_t cpt_exe_addr;       // 0x48: 执行地址
    uint32_t cpt_len;            // 0x4C: 长度
    cpt_ram_info_t cpt_rams[5];  // 0x50-0xB3: 5个RAM区域信息
    uint8_t  cpt_version[12];    // 0xB4: 版本信息(24字节的前12字节)
    
    /* 0xC0-0xDF: Other数据段 */
    uint8_t  cpt_version2[12];   // 0xC0: CPT版本信息后12字节
    uint32_t psram_cfg_addr;     // 0xCC: PSRAM配置地址
    uint32_t other_pro;          // 0xD0: Other Pro属性
    uint32_t other_addr;         // 0xD4: Other Flash地址
    uint32_t other_exe_addr;     // 0xD8: Other执行地址
    uint32_t other_len;          // 0xDC: Other长度
    uint32_t other_check;        // 0xE0: Other校验值
    uint8_t  other_version[12];  // 0xE4: Other版本信息
    
    /* 0xF0-0xFF: 全局配置和校验 */
    uint32_t ref_clock;          // 0xF0: REF时钟
    uint32_t config;             // 0xF4: 配置
    uint32_t fout_clock;         // 0xF8: FOUT时钟
    uint32_t clk_config0;        // 0xFC: 时钟配置0
    uint32_t clk_config1;        // 0x100: 时钟配置1(实际0xFF位置)
    uint32_t header_crc32;       // 0x104: Header CRC32(实际应该调整到0xFC)
} __attribute__((packed)) s300_header_t;

/* 重新定义正确的256字节Header */
typedef struct {
    /* 0x00-0x1F: Cortex-M4控制系统程序段 */
    uint32_t ctrl_pro;           // 0x00: Pro属性
    uint32_t ctrl_addr;          // 0x04: Flash地址  
    uint32_t ctrl_exe_addr;      // 0x08: 执行地址
    uint32_t ctrl_len;           // 0x0C: 长度
    uint32_t ctrl_check;         // 0x10: 校验值
    uint8_t  ctrl_version[12];   // 0x14: 版本信息
    
    /* 0x20-0x3F: Cortex-M0程序段 */
    uint32_t m0_pro;             // 0x20: Pro属性
    uint32_t m0_addr;            // 0x24: Flash地址
    uint32_t m0_exe_addr;        // 0x28: 执行地址
    uint32_t m0_len;             // 0x2C: 长度
    uint32_t m0_check;           // 0x30: 校验值
    uint8_t  m0_version[12];     // 0x34: 版本信息
    
    /* 0x40-0xBF: CPT程序段 */
    uint32_t cpt_pro;            // 0x40: Pro属性
    uint32_t cpt_addr;           // 0x44: Flash地址
    uint32_t cpt_exe_addr;       // 0x48: 执行地址
    uint32_t cpt_len;            // 0x4C: 长度
    cpt_ram_info_t cpt_rams[5];  // 0x50-0xB3: 5个RAM区域信息 (5*20=100字节)
    uint8_t  cpt_version[12];    // 0xB4: 版本信息前12字节
    
    /* 0xC0-0xEF: CPT版本后12字节 + PSRAM配置 + Other数据段 */
    uint8_t  cpt_version2[12];   // 0xC0: CPT版本后12字节
    uint32_t psram_cfg_addr;     // 0xCC: PSRAM配置地址
    uint32_t other_pro;          // 0xD0: Other Pro属性
    uint32_t other_addr;         // 0xD4: Other Flash地址
    uint32_t other_exe_addr;     // 0xD8: Other执行地址
    uint32_t other_len;          // 0xDC: Other长度
    uint32_t other_check;        // 0xE0: Other校验值
    uint8_t  other_version[12];  // 0xE4: Other版本信息
    
    /* 0xF0-0xFF: 全局配置和校验 */
    uint32_t ref_clock;          // 0xF0: REF时钟
    uint32_t config;             // 0xF4: 配置
    uint32_t fout_clock;         // 0xF8: FOUT时钟
    uint32_t clk_config0;        // 0xFC: 时钟配置0
    uint32_t clk_config1;        // 0x100: 时钟配置1
    uint32_t header_crc32;       // 0x104: Header CRC32
} __attribute__((packed)) s300_header_v1_t;
#define PRO_CTRL_RAM1               (0x02 << 2)    // 运行在RAM1(384K)
/* Cortex-M0启动模式 */
#define PRO_M0_RAM0                 (0x01 << 2)    // 运行在RAM0(8K) - 必须为1
/* CPT PSRAM初始化模式 */
#define PRO_CPT_PSRAM_DISABLE       (0x00 << 2)    // 禁止初始化
#define PRO_CPT_PSRAM_ENABLE        (0x01 << 2)    // 初始化PSRAM
#define PRO_CPT_PSRAM_BEFORE_LOAD   (0x02 << 2)    // 加载前初始化

#define PRO_CHECK_MODE_MASK         (0x3)          // Bit[1:0] 校验模式
#define PRO_CHECK_SUM               (0x0)          // 校验和
#define PRO_CHECK_CRC32             (0x1)          // CRC32
#define PRO_CHECK_DISABLE           (0x3)          // 禁止校验

/* 段信息结构 (32字节) */
typedef struct {
    uint32_t magic;              // 段魔数
    uint32_t offset;             // Flash偏移地址
    uint32_t size;               // 段大小
    uint32_t load_addr;          // 加载地址
    uint32_t entry_point;        // 入口地址
    uint32_t crc32;              // CRC32校验
    uint32_t flags;              // 段标志
    uint32_t reserved;           // 预留
} __attribute__((packed)) segment_info_t;

/* S300镜像头结构 (256字节) - 供ROMBOOT读取 */
typedef struct {
    /* 头信息 (16字节) */
    uint32_t magic;              // 魔数: S300_HEADER_MAGIC
    uint32_t version;            // 头版本: S300_HEADER_VERSION
    uint32_t header_size;        // 头大小: 256
    uint32_t segment_count;      // 段数量
    
    /* 控制系统程序段 (32字节) */
    segment_info_t ctrl_segment;
    
    /* Cortex-M0程序段 (32字节) */
    segment_info_t m0_segment;
    
    /* CPT程序段 (32字节) */
    segment_info_t cpt_segment;
    
    /* RBL程序段 (32字节) - 实现ESP32 ROM BL功能 */
    segment_info_t rbl_segment;
    
    /* SBL程序段 (32字节) - 等同ESP32 Second BL */
    segment_info_t sbl_segment;
    
    /* 数据段(other) (32字节) */
    segment_info_t data_segment;
    
    /* 全局信息 (48字节) */
    uint32_t total_size;         // 总镜像大小
    uint32_t timestamp;          // 构建时间戳
    uint32_t global_crc32;       // 全局CRC32
    uint32_t boot_flags;         // 启动标志
    uint32_t reserved1[8];       // 预留空间
    
    /* 校验信息 (16字节) */
    uint32_t header_crc32;       // Header自身CRC32
    uint32_t reserved2[3];       // 预留
} __attribute__((packed)) s300_image_header_t;

/* 静态断言，确保结构体大小为256字节 */
_Static_assert(sizeof(s300_image_header_t) == 256, "s300_image_header_t must be 256 bytes");
_Static_assert(sizeof(segment_info_t) == 32, "segment_info_t must be 32 bytes");

/* 段标志位定义 */
#define SEGMENT_FLAG_ENABLE         0x01    // 段有效
#define SEGMENT_FLAG_LOAD_SRAM      0x02    // 加载到SRAM
#define SEGMENT_FLAG_XIP            0x04    // XIP执行
#define SEGMENT_FLAG_COMPRESSED     0x08    // 压缩数据
#define SEGMENT_FLAG_ENCRYPTED      0x10    // 加密数据
#define SEGMENT_FLAG_SIGNED         0x20    // 已签名

/* 启动标志位定义 */
#define BOOT_FLAG_SECURE_BOOT       0x01    // 安全启动
#define BOOT_FLAG_FLASH_ENCRYPTION  0x02    // Flash加密
#define BOOT_FLAG_JTAG_DISABLE      0x04    // 禁用JTAG
#define BOOT_FLAG_DOWNLOAD_DISABLE  0x08    // 禁用下载模式

/* 预定义段偏移 (基于新的Flash布局) */
#define S300_HEADER_OFFSET          0x000000    // Header偏移: 0KB
#define S300_HEADER_SIZE            0x000100    // Header大小: 256字节

#define RBL_OFFSET                  0x000100    // RBL偏移: 256字节
#define RBL_SIZE                    0x007F00    // RBL大小: ~32KB
#define SBL_OFFSET                  0x008000    // SBL偏移: 32KB
#define SBL_SIZE                    0x010000    // SBL大小: 64KB
#define PARTITION_TABLE_OFFSET      0x018000    // 分区表偏移: 96KB
#define PARTITION_TABLE_SIZE        0x001000    // 分区表大小: 4KB

/* 应用分区从128KB开始 */
#define APP_START_OFFSET            0x020000    // 应用起始: 128KB

/* Header操作函数声明 */
int s300_header_read(s300_image_header_t *header);
int s300_header_write(const s300_image_header_t *header);
int s300_header_verify(const s300_image_header_t *header);
uint32_t s300_header_calculate_crc(const s300_image_header_t *header);

#endif /* __IMAGE_HEADER_H__ */
