/**
 * @file s300_header.h
 * @brief S300 镜像 Header (256 Bytes) 布局定义与辅助宏
 *
 * 本定义严格对齐当前工程的 Header 生成/分析脚本的偏移：
 *  - 0x00-0x1F: Cortex-M4 段 (Pro/Addr/Exe/Len + Check/Version[12])
 *  - 0x20-0x3F: Cortex-M0 段 (同上)
 *  - 0x40-0x4F: DSP/CPT 段头 (Pro/Addr/Exe/Len)
 *  - 0x50-0xB3: DSP RAM 映射 (5 区*20B)：Addr, RamAddr, RamMap, Size, Check
 *  - 0xB4-0xBF: DSP Version[12]
 *  - 0xC0-0xCF: 其他/保留 (VersionC0, PsramCfgAddr, OtherPro, Reserved)
 *  - 0xD0-0xDF: Other 段 (Addr/Exe/Len/Check)
 *  - 0xE0-0xE8: 保留[3]
 *  - 0xEC:     RefClock
 *  - 0xF0:     FoutClock
 *  - 0xF4:     ClkConfig0
 *  - 0xF8:     ClkConfig1
 *  - 0xFC:     Header CRC32
 */

#ifndef S300_HEADER_H
#define S300_HEADER_H

#include <stdint.h>

#if defined(__GNUC__)
    #define S300_PACKED __attribute__((packed))
#else
    #define S300_PACKED
#endif

/*============================
 * Pro 字段位定义 (Table 4)
 *============================*/

/* Check mode [1:0] */
#define S300_PRO_CHECK_SUM              0x0u
#define S300_PRO_CHECK_CRC32            0x1u

/* Boot mode [7:2] (主要用于 CM4；CM0 必须为 1) */
#define S300_BOOT_XIP_FLASH             (0x00u << 2)
#define S300_BOOT_FLASH_RAM0            (0x01u << 2)
#define S300_BOOT_FLASH_RAM1            (0x02u << 2)
#define S300_BOOT_FLASH_PSRAM           (0x03u << 2)
#define S300_BOOT_SD_XIP                (0x04u << 2)
#define S300_BOOT_SD_RAM0               (0x05u << 2)
#define S300_BOOT_SD_RAM1               (0x06u << 2)
#define S300_BOOT_SD_PSRAM              (0x07u << 2)

/* Big endian [8] */
#define S300_PRO_BIG_ENDIAN             (1u << 8)

/* Address bytes [11] (0:3B, 1:4B) */
#define S300_PRO_ADDR_4B                (1u << 11)

/* QPI/DTR/MODE/BaseXIP (仅 CM4 段有效) */
#define S300_PRO_QPI                    (1u << 12)
#define S300_PRO_DTR                    (1u << 13)
#define S300_PRO_MODE                   (1u << 14)
#define S300_PRO_BASE_XIP               (1u << 15)

/* Div [19:16] (仅 CM4 段有效): 0:2分频, 1:4分频, ..., 15:32分频 */
#define S300_PRO_DIV_SHIFT              16u
#define S300_PRO_DIV_MASK               (0xFu << S300_PRO_DIV_SHIFT)

/* Debug [20] (仅 CM4 段有效): 0:校验使能, 1:禁止校验 */
#define S300_PRO_DEBUG_NO_VERIFY        (1u << 20)

/* Run type [24:21] (各核定义不同，按规范解释) */
#define S300_PRO_RUN_TYPE_SHIFT         21u
#define S300_PRO_RUN_TYPE_MASK          (0xFu << S300_PRO_RUN_TYPE_SHIFT)

/* 组合一个最小化的 CM4 RAM1 + CRC32 的 Pro 示例值 */
#define S300_PRO_CM4_RAM1_CRC32         (S300_BOOT_FLASH_RAM1 | S300_PRO_CHECK_CRC32)

/*============================
 * 通用 Segment 结构 (32B)
 *============================*/
typedef struct S300_PACKED
{
    uint32_t pro;          /* 0x00 属性 Pro */
    uint32_t addr;         /* 0x04 存储区起始地址 (Flash/SD) */
    uint32_t exe_addr;     /* 0x08 执行地址 (代码段有效) */
    uint32_t len;          /* 0x0C 段长度 (字节) */
    uint32_t check;        /* 0x10 校验预值 (见 Pro.check mode) */
    uint8_t  version[12];  /* 0x14 版本信息 (12B；DSP 段版本位置见整体结构) */
} s300_segment_t;

/*============================
 * DSP/CPT RAM 映射条目 (20B)
 *============================*/
typedef struct S300_PACKED
{
    uint32_t addr;      /* 存储区起始地址 (该 RAM 区域的子段) */
    uint32_t ram_addr;  /* 目标 RAM 地址映射 */
    uint32_t ram_map;   /* 别名映射 (保留) */
    uint32_t size;      /* 子段大小 */
    uint32_t check;     /* 子段校验值 */
} s300_dsp_ram_map_t;

/*============================
 * 整体 256 字节 Header
 *============================*/
typedef struct S300_PACKED
{
    /* 0x00 - 0x1F */
    s300_segment_t cm4;       /* Cortex-M4 段 */
    /* 0x20 - 0x3F */
    s300_segment_t cm0;       /* Cortex-M0 段 */

    /* 0x40 - 0x4F */
    struct S300_PACKED
    {
        uint32_t pro;         /* 0x40 DSP/CPT 段 Pro */
        uint32_t addr;        /* 0x44 存储地址 */
        uint32_t exe_addr;    /* 0x48 执行地址 */
        uint32_t len;         /* 0x4C 长度 */
    } dsp_hdr;

    /* 0x50 - 0xB3: 5 组 RAM 映射条目 (5 * 20B = 100B) */
    s300_dsp_ram_map_t dsp_ram[5];

    /* 0xB4 - 0xCB: DSP Version[24] */
    uint8_t dsp_version[24];

    /* 0xCC: Other 段 Pro */
    uint32_t other_pro;       /* 0xCC */

    /* 0xD0 - 0xDF: Other 段四元组 (Addr/Exe/Len/Check) */
    uint32_t other_addr;      /* 0xD0 */
    uint32_t other_exe_addr;  /* 0xD4 */
    uint32_t other_len;       /* 0xD8 */
    uint32_t other_check;     /* 0xDC */

    /* 0xE0 - 0xEB: Other Version[12] */
    uint8_t other_version[12];/* 0xE0-0xEB */

    /* 时钟/PLL 配置 */
    uint32_t refclock;        /* 0xEC 晶振 (Hz) */
    uint32_t foutclock;       /* 0xF0 PLL 输出 (Hz)；PLL 关闭时等于 refclock */
    uint32_t clkconfig0;      /* 0xF4 (timeout/postdiv1/2/fbdiv) */
    uint32_t clkconfig1;      /* 0xF8 (en/refdiv/frac) */

    /* Header CRC32 */
    uint32_t header_crc32;    /* 0xFC 对 0x00-0xFB 的 CRC32 */
} s300_header_t;

/* 编译期大小校验 (C11) */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    _Static_assert(sizeof(s300_header_t) == 256, "s300_header_t must be 256 bytes");
#endif

/*============================
 * PLL 配置字段辅助宏
 *============================*/
/* Clkconfig0: [31:18] timeout, [17:15] postdiv2, [14:12] postdiv1, [11:0] fbdiv */
#define S300_CLK0_TIMEOUT_SHIFT   18u
#define S300_CLK0_TIMEOUT_MASK    (0x3FFFu << S300_CLK0_TIMEOUT_SHIFT)
#define S300_CLK0_POSTDIV2_SHIFT  15u
#define S300_CLK0_POSTDIV2_MASK   (0x7u << S300_CLK0_POSTDIV2_SHIFT)
#define S300_CLK0_POSTDIV1_SHIFT  12u
#define S300_CLK0_POSTDIV1_MASK   (0x7u << S300_CLK0_POSTDIV1_SHIFT)
#define S300_CLK0_FBDIV_SHIFT     0u
#define S300_CLK0_FBDIV_MASK      (0xFFFu << S300_CLK0_FBDIV_SHIFT)

/* Clkconfig1: [31:30] en, [29:24] refdiv, [23:0] frac */
#define S300_CLK1_EN_SHIFT        30u
#define S300_CLK1_EN_MASK         (0x3u << S300_CLK1_EN_SHIFT)
#define S300_CLK1_EN_DISABLED     (0x0u << S300_CLK1_EN_SHIFT) /* 非 0x3 为不启用 */
#define S300_CLK1_EN_ENABLED      (0x3u << S300_CLK1_EN_SHIFT)
#define S300_CLK1_REFDIV_SHIFT    24u
#define S300_CLK1_REFDIV_MASK     (0x3Fu << S300_CLK1_REFDIV_SHIFT)
#define S300_CLK1_FRAC_SHIFT      0u
#define S300_CLK1_FRAC_MASK       (0xFFFFFFu << S300_CLK1_FRAC_SHIFT)

#endif /* S300_HEADER_H */
