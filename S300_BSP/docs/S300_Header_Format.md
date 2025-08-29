# S300芯片镜像Header格式规范

## 文档版本控制

| 版本 | 日期       | 作者         | 修订说明                       |
| ---- | ---------- | ------------ | ------------------------------ |
| 1.0  | 2025-08-29 | AI Assistant | 基于S300技术参考手册11.3节创建 |

---

## 1. 概述

S300芯片集成了控制系统、Cortex-M0和CPT内核，采用Quad Flash存储芯片运行的镜像。镜像中包括控制系统、Cortex-M0、CPT运行的程序，并存储静态数据。

### 1.1 镜像布局结构

镜像格式被划分为五部分：

```text
Flash起始地址: 0x08000000

+-------------------+
|     Header        | 256字节，存储各段信息
+-------------------+
|   Cortex-M4       | 控制系统程序段  
|  (控制系统)        |
+-------------------+
|   Cortex-M0       | Cortex-M0程序段
+-------------------+
|      CPT          | CPT程序段
+-------------------+
|     Other         | 数据段(可选，或存储CPT片段程序)
+-------------------+
```

### 1.2 Header功能

- **存储各段相关信息**：RomBoot读取这些信息用于验证、加载相关镜像
- **256字节固定长度**：包含控制系统程序段、Cortex-M0程序段、CPT程序段以及数据段信息
- **位于镜像首端**：从FLASH起始基址开始存储，便于加载、验证、执行等功能

---

## 2. Header布局详细规范

### 2.1 Header Layout表

| 偏移地址 | +0x00             | +0x04              | +0x08          | +0x0C          |
| -------- | ----------------- | ------------------ | -------------- | -------------- |
| **0x00** | **Cortex-M4 Pro** | **Addr**           | **Exe Addr**   | **Len**        |
| **0x10** | **Check**         | **Version**        |                |                |
| **0x20** | **Cortex-M0 Pro** | **Addr**           | **Exe Addr**   | **Len**        |
| **0x30** | **Check**         | **Version**        |                |                |
| **0x40** | **CPT Pro**       | **Addr**           | **Exe Addr**   | **Len**        |
| **0x50** | **Addr0**         | **Ram_addr0**      | **Ram_map0**   | **Size0**      |
| **0x60** | **Check0**        | **Addr1**          | **Ram_addr1**  | **Ram_map1**   |
| **0x70** | **Size1**         | **Check1**         | **Addr2**      | **Ram_addr2**  |
| **0x80** | **Ram_map2**      | **Size2**          | **Check2**     | **Addr3**      |
| **0x90** | **Ram_addr3**     | **Ram_map3**       | **Size3**      | **Check3**     |
| **0xA0** | **Addr4**         | **Ram_addr4**      | **Ram_map4**   | **Size4**      |
| **0xB0** | **Check4**        | **Version**        |                |                |
| **0xC0** | **Version**       | **Psram_cfg_addr** | **Pro(other)** |                |
| **0xD0** | **Other Addr**    | **Exe Addr**       | **Len**        | **Check**      |
| **0xE0** | **Version**       | **REFclock**       |                |                |
| **0xF0** | **Config**        | **foutclock**      | **Clkconfig0** | **Clkconfig1** |
| **0xFC** | **Head CRC32**    |                    |                |                |

---

## 3. Header字段详细说明

### 3.1 Pro字段 (Program Properties)

**用途**：表示对应段的相关属性  
**大小**：4字节  
**说明**：用户可根据不同需求配置启动模式

#### 3.1.1 Pro字段位定义

| 位域  | 字段名        | 描述             |
| ----- | ------------- | ---------------- |
| 31:25 | Reserve       | 保留位           |
| 24:21 | Run Type      | 程序运行方式配置 |
| 20    | Debug         | 校验功能控制     |
| 19:16 | Div           | FLASH分频值      |
| 15    | Base XIP      | XIP功能控制      |
| 14    | MODE          | MODE位控制       |
| 13    | DTR           | DTR支持控制      |
| 12    | QPI           | QPI支持控制      |
| 11    | Address Bytes | 地址字节数配置   |
| 10:9  | CPT Rom Boot  | CPT启动阶段      |
| 8     | Big Endian    | 字节序格式       |
| 7:2   | Boot Mode     | 程序加载运行方式 |
| 1:0   | Check Mode    | 验证算法选择     |

#### 3.1.2 Run Type详细配置 (位24:21)

**Cortex-M0配置**：
- Bit0: 1=只加载程序不运行，0=不做任何操作
- Bit1: 1=加载后执行，0=不执行
- Bit2: 1=CM0在加载程序前复位，0=在运行前复位
- Bit3: 1=热复位后加载并运行

**控制系统配置**：
- Bit0: 1=只加载程序不运行，0=不做任何操作
- Bit1: 1=加载后执行，0=不执行
- Bit2: 保留

**CPT配置**：
- Bit0: 1=只加载程序不运行，0=不做任何操作
- Bit1: 1=加载后执行，0=不执行
- Bit2: 1=CPT在加载程序前使能电源、时钟以及RAM

#### 3.1.3 其他关键位定义

**Debug位 (位20)**：
- 0: 使能校验
- 1: 禁止校验（仅控制程序段有效）

**Div位 (位19:16)**：
- FLASH分频值，0~15（仅控制程序段有效）
- 0: 2分频，1: 4分频，15: 32分频

**Boot Mode位 (位7:2)**：

对于**Cortex-M0**（必须为1）：
- 0x01: 程序在flash中存储，运行时在ram0(8k)

对于**控制系统**：
- 0x00: 控制程序直接在flash中以XIP模式运行
- 0x01: 控制程序在flash中存储，运行时在ram0(8k)
- 0x02: 控制程序在flash中存储，运行时在ram1(384k)
- 0x03~0x3f: 保留

对于**CPT**：
- 0x00: 禁止初始化PSRAM
- 0x01: 初始化PSRAM（ROMBOOT不加载CPT时使用）
- 0x02: 在加载CPT前对PSRAM初始化（ROMBOOT加载CPT时使用）

**Check Mode位 (位1:0)**：
- 0x00: 校验和
- 0x01: CRC32
- 0x10: 保留
- 0x11: 禁止校验，直接加载

### 3.2 Addr字段 (Address)

**用途**：表示对应段在存储区域(Flash)的起始地址  
**大小**：4字节  
**说明**：用于检索或加载对应段的内容

对于CPT字段，表示5个RAM区域：
- Addr0: PTCM
- Addr1: DTCM  
- Addr2: SRAM0
- Addr3: SRAM1
- Addr4: PSRAM

### 3.3 Exe Addr字段 (Execute Address)

**用途**：表示程序执行的起始地址  
**大小**：4字节  
**说明**：对于数据段(Other)没有意义

### 3.4 Len字段 (Length)

**用途**：表示对应段程序或数据的大小  
**大小**：4字节  
**单位**：byte

### 3.5 Check字段 (Checksum)

**用途**：表示对应段程序或数据的校验预值  
**大小**：4字节  
**说明**：根据Pro字段中Check Mode位的配置，可以是校验和或CRC32值

### 3.6 Version字段

**用途**：表示程序的版本信息  
**大小**：
- Cortex-M0、控制系统、Other字段：各12字节
- CPT段：24字节

### 3.7 CPT专用字段

#### 3.7.1 Ram_Addr字段

**用途**：表示CPT对应段相应的RAM地址映射  
**大小**：4字节  
**说明**：即在S300中的内存映射地址

CPT字段表示5个RAM区域：
- Ram_Addr0: PTCM映射地址
- Ram_Addr1: DTCM映射地址
- Ram_Addr2: SRAM0映射地址
- Ram_Addr3: SRAM1映射地址
- Ram_Addr4: PSRAM映射地址

#### 3.7.2 Ram_map字段

**用途**：表示CPT对应段相应的RAM地址映射的别名地址  
**大小**：4字节  
**说明**：此寄存器暂时保留，未来使用

#### 3.7.3 Size字段

**用途**：表示CPT对应段程序的大小  
**大小**：4字节  
**单位**：byte

对应5个RAM区域的大小：
- Size0: PTCM大小
- Size1: DTCM大小
- Size2: SRAM0大小
- Size3: SRAM1大小
- Size4: PSRAM大小

---

## 4. Header生成示例

### 4.1 C语言结构体定义

```c
#pragma pack(1)

typedef struct {
    uint32_t pro;           // 程序属性
    uint32_t addr;          // Flash起始地址
    uint32_t exe_addr;      // 执行起始地址
    uint32_t len;           // 段长度
    uint32_t check;         // 校验值
    uint8_t  version[12];   // 版本信息
} segment_header_t;

typedef struct {
    uint32_t addr;          // Flash地址
    uint32_t ram_addr;      // RAM映射地址
    uint32_t ram_map;       // RAM别名地址(保留)
    uint32_t size;          // 大小
    uint32_t check;         // 校验值
} cpt_ram_info_t;

typedef struct {
    // Cortex-M4段信息
    segment_header_t cortex_m4;
    
    // Cortex-M0段信息  
    segment_header_t cortex_m0;
    
    // CPT段信息
    uint32_t cpt_pro;
    uint32_t cpt_addr;
    uint32_t cpt_exe_addr;
    uint32_t cpt_len;
    
    // CPT RAM配置 (5个RAM区域)
    cpt_ram_info_t cpt_ram[5];
    
    uint8_t cpt_version[24];
    uint32_t psram_cfg_addr;
    uint32_t other_pro;
    
    // Other段信息
    segment_header_t other;
    
    // 时钟配置
    uint32_t ref_clock;
    uint32_t config;
    uint32_t fout_clock;
    uint32_t clk_config0;
    uint32_t clk_config1;
    
    // Header CRC32
    uint32_t head_crc32;
    
} s300_header_t;

#pragma pack()
```

### 4.2 Header初始化示例

```c
void init_s300_header(s300_header_t *header) {
    memset(header, 0, sizeof(s300_header_t));
    
    // Cortex-M4 (控制系统) 配置
    header->cortex_m4.pro = 0x00000003;  // XIP模式，CRC32校验
    header->cortex_m4.addr = 0x00010000; // Flash地址
    header->cortex_m4.exe_addr = 0x80010000; // XIP执行地址
    header->cortex_m4.len = 0x00020000;  // 128KB
    
    // Cortex-M0配置
    header->cortex_m0.pro = 0x00000007;  // RAM模式，CRC32校验
    header->cortex_m0.addr = 0x00030000; // Flash地址
    header->cortex_m0.exe_addr = 0x10000000; // SRAM0执行地址
    header->cortex_m0.len = 0x00002000;  // 8KB
    
    // CPT配置
    header->cpt_pro = 0x00000003;        // 标准配置
    header->cpt_addr = 0x00032000;       // Flash地址
    header->cpt_exe_addr = 0x00000000;   // CPT执行地址
    header->cpt_len = 0x00008000;        // 32KB
    
    // Other段配置
    header->other.pro = 0x00000003;      // 数据段配置
    header->other.addr = 0x0003A000;     // Flash地址
    header->other.len = 0x00006000;      // 24KB
    
    // 时钟配置
    header->ref_clock = 24000000;        // 24MHz参考时钟
    header->fout_clock = 192000000;      // 192MHz输出时钟
}
```

---

## 5. 校验和计算

### 5.1 CRC32算法

```c
uint32_t calculate_crc32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}
```

### 5.2 校验和算法

```c
uint32_t calculate_checksum(const uint8_t *data, uint32_t length) {
    uint32_t sum = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        sum += data[i];
    }
    
    return sum;
}
```

---

## 6. 使用注意事项

### 6.1 字节序

- S300芯片支持大端和小端字节序
- 通过Pro字段的Big Endian位(位8)配置
- 建议使用小端字节序以保持与ARM架构一致

### 6.2 地址对齐

- 所有地址建议按4字节对齐
- Flash地址建议按扇区大小对齐(4KB)
- RAM地址必须在有效范围内

### 6.3 校验配置

- 推荐使用CRC32校验以提高可靠性
- 开发调试阶段可临时禁用校验
- 生产环境必须启用校验功能

### 6.4 版本管理

- Version字段用于固件版本跟踪
- 建议采用语义化版本号格式
- 便于固件升级和回滚管理

---

## 7. 相关工具

### 7.1 Header生成工具

```bash
# 生成S300 Header工具
s300_header_gen --cortex-m4 firmware.bin \
                --cortex-m0 m0_firmware.bin \
                --cpt cpt_firmware.bin \
                --other data.bin \
                -o s300_header.bin
```

### 7.2 Header验证工具

```bash
# 验证Header格式
s300_header_verify s300_header.bin

# 解析Header信息
s300_header_parse s300_header.bin --verbose
```

---

## 8. 参考资料

- PiMCHIP-S300技术参考手册 第11.3节
- S300_Boot_Architecture_Final.md
- S300_BSP_Architecture.md

---

## 文档结束

此文档详细描述了S300芯片镜像Header的格式规范，包括各字段的定义、配置方法和使用注意事项。开发者可根据此规范正确生成和解析S300镜像Header，确保系统正常启动和运行。
