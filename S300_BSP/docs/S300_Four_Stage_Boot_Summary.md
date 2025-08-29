# S300 四级启动架构说明

## 完整架构总结

基于您的需求和ESP32-S3的设计理念，我已经为S300设计了完整的四级启动架构：

### 架构层次

```
BootROM (4KB) → RBL (32KB) → SBL (64KB) → Application
     ↓             ↓            ↓             ↓
   固化ROM       Flash        Flash        Flash
   基础加载      下载模式      OTA管理      用户应用
```

### 功能分层

#### 1. BootROM (一级引导 - 4KB)
- **位置**: 芯片内部固化ROM
- **功能**: 
  - 最小化系统初始化
  - 读取Flash镜像头 (256字节)
  - 验证RBL完整性
  - 加载RBL到SRAM并跳转

#### 2. RBL - ROM Bootloader (二级引导 - 32KB)
- **位置**: Flash 0x1000-0x9000
- **运行**: SRAM (由BootROM加载)
- **功能** (类似ESP32-S3 ROM Bootloader):
  - 系统全面初始化
  - GPIO启动模式检测
  - UART下载模式支持
  - 基础安全验证
  - SBL验证和启动

#### 3. SBL - Second Bootloader (三级引导 - 64KB)  
- **位置**: Flash 0xA000-0x1A000
- **运行**: XIP模式直接执行
- **功能** (类似ESP32-S3 Second Stage Bootloader):
  - 分区表管理
  - OTA升级控制
  - 应用选择和验证
  - 高级安全功能
  - 故障检测和恢复

#### 4. Application (应用程序)
- **位置**: Factory/OTA分区 (2MB each)
- **运行**: XIP模式或SRAM
- **功能**: 用户业务逻辑

### Flash布局设计

| 分区名称 | 起始地址 | 大小 | 描述 |
|----------|----------|------|------|
| image_header | 0x000000 | 4KB | 镜像头(Header) |
| rbl | 0x001000 | 32KB | RBL二级引导 |
| partition_table | 0x009000 | 4KB | 分区表 |
| sbl | 0x00A000 | 64KB | SBL三级引导 |
| nvs | 0x01A000 | 24KB | NVS存储 |
| phy_init | 0x020000 | 4KB | PHY初始化数据 |
| factory | 0x021000 | 2MB | 出厂应用 |
| ota_0 | 0x221000 | 2MB | OTA分区0 |
| ota_1 | 0x421000 | 2MB | OTA分区1 |
| ota_data | 0x621000 | 8KB | OTA数据 |
| test | 0x623000 | 1MB | 测试固件 |
| storage | 0x723000 | ~9MB | 用户存储 |

### Header段设计 (256字节)

Header段包含所有启动相关信息，符合您的要求：

```c
typedef struct {
    uint32_t magic;              // "S300"魔数
    uint32_t version;            // 头版本
    uint32_t header_size;        // 256字节
    
    segment_info_t rbl_segment;  // RBL段信息
    segment_info_t sbl_segment;  // SBL段信息  
    segment_info_t app_segment;  // 应用段信息
    segment_info_t ctrl_segment; // 控制系统段(预留)
    
    // 全局校验信息
    uint32_t total_size;
    uint32_t timestamp;
    uint32_t global_crc32;
    uint8_t  global_hash[32];
} image_header_t;
```

### 启动流程

1. **BootROM**: 读取Header → 验证RBL → 加载RBL到SRAM
2. **RBL**: 初始化系统 → 检测启动模式 → 验证SBL → 启动SBL
3. **SBL**: 读取分区表 → OTA管理 → 选择应用 → 启动应用
4. **Application**: 运行用户代码

### 已实现的文件

#### 项目结构
```
S300_BSP/Projects/
├── RBL/                    # ROM Bootloader
│   ├── Include/
│   │   └── rbl.h          # RBL主头文件
│   ├── Src/
│   │   └── rbl_main.c     # RBL主程序
│   └── GCC/               # 构建配置
├── SBL/                    # Second Bootloader  
│   ├── Include/
│   │   ├── sbl.h          # SBL主头文件
│   │   ├── image_header.h # 镜像头定义
│   │   ├── partition_table.h # 分区表定义
│   │   └── ota.h          # OTA管理
│   ├── Src/
│   │   ├── sbl_main.c     # SBL主程序
│   │   ├── sbl_core.c     # 核心功能
│   │   ├── sbl_flash.c    # Flash管理
│   │   ├── sbl_download.c # 下载模式
│   │   └── sbl_utils.c    # 工具函数
│   └── GCC/
│       └── Makefile       # 构建配置
```

#### 文档
```
docs/
├── S300_Multi_Stage_Boot_Architecture.md  # 四级启动架构设计
├── S300_Boot_Architecture.md             # 原有架构文档
└── S300_Download_Circuit_Design.md       # 下载电路设计
```

### 优势和特点

1. **ESP32-S3兼容**: 架构设计完全参考ESP32-S3
2. **分层清晰**: 每级有明确的职责分工
3. **功能完整**: 支持下载、OTA、恢复、测试等模式
4. **Header驱动**: 统一的256字节Header管理所有段信息
5. **安全可靠**: 多重验证和故障恢复机制
6. **扩展性强**: 预留控制系统、M0、CPT段接口

### 下一步工作

1. **完成RBL实现**: 添加Flash操作、下载协议等功能
2. **更新SBL**: 适配新的Header和分区表结构
3. **工具链开发**: 创建镜像生成和分区管理工具
4. **测试验证**: 在硬件上验证完整启动流程
5. **文档完善**: 补充使用说明和迁移指南

这个架构完全满足您提出的Header段要求，同时提供了ESP32-S3级别的功能完整性。建议我们继续完善RBL的实现，然后进行整体测试。
