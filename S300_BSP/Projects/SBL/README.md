# S300 SBL (Second Bootloader) 项目

## 项目概述

S300 SBL是基于PiMCHIP S300微控制器的二级引导程序，提供ESP32兼容的OTA（Over-The-Air）更新功能。SBL运行在SRAM中，负责管理应用程序分区、验证固件完整性，并实现安全的应用程序启动。

## 🚀 主要特性

- **ESP32兼容性**: 完全兼容ESP32分区表格式和OTA机制
- **A/B分区管理**: 支持双分区OTA更新，确保系统稳定性
- **固件验证**: 完整的镜像头部验证和校验和检查
- **故障回滚**: 自动检测启动失败并回滚到稳定版本
- **分区发现**: 动态读取和解析Flash分区表
- **调试支持**: 完整的日志系统和调试信息输出
- **内存优化**: 仅使用64KB SRAM，为应用程序保留320KB空间

## 📁 项目结构

```
SBL/
├── Inc/                    # 头文件目录
│   ├── sbl_config.h       # SBL配置和调试宏
│   ├── sbl_main.h         # 主程序接口
│   ├── sbl_partition.h    # ESP32兼容分区表管理
│   └── sbl_ota.h          # OTA更新管理
├── Src/                    # 源文件目录
│   ├── sbl_main.c         # 主程序实现
│   ├── sbl_partition.c    # 分区表管理实现
│   └── sbl_ota.c          # OTA管理实现
├── GCC/                    # GCC构建系统
│   ├── Makefile           # 主构建文件
│   └── sbl.ld             # SBL专用链接脚本
├── build.sh               # 自动化构建脚本
└── README.md              # 项目文档
```

## 🛠️ 系统架构

### 启动序列

1. **ROMBOOT** → **RBL** → **SBL** → **APP**
2. SBL在SRAM中执行，不依赖Flash XIP
3. 动态读取分区表，选择合适的应用程序分区
4. 验证镜像完整性后跳转到应用程序

### 内存映射

| 区域       | 地址范围                | 大小  | 用途             |
| ---------- | ----------------------- | ----- | ---------------- |
| SBL SRAM   | 0x20000000 - 0x2000FFFF | 64KB  | SBL代码和数据    |
| APP SRAM   | 0x20010000 - 0x2005FFFF | 320KB | 应用程序使用     |
| QSPI Flash | 0x60000000 - 0x60FFFFFF | 16MB  | 分区表和应用程序 |

### 分区表结构（ESP32兼容）

| 分区名称        | 类型 | 子类型  | 偏移     | 大小   | 说明         |
| --------------- | ---- | ------- | -------- | ------ | ------------ |
| partition_table | data | 0xFF    | 0x8000   | 0x1000 | 分区表       |
| otadata         | data | ota     | 0x9000   | 0x2000 | OTA状态数据  |
| factory         | app  | factory | 0x10000  | 1MB    | 出厂应用程序 |
| ota_0           | app  | ota_0   | 0x110000 | 1MB    | OTA槽0       |
| ota_1           | app  | ota_1   | 0x210000 | 1MB    | OTA槽1       |

## 🔧 构建和使用

### 环境要求

- **工具链**: ARM GNU工具链 (arm-none-eabi-gcc)
- **构建工具**: GNU Make
- **调试器**: OpenOCD + J-Link 或 ST-Link
- **操作系统**: Linux / Windows WSL / macOS

### 快速开始

1. **克隆项目**

```bash
cd S300_BSP/Projects/SBL
```

2. **构建项目**

```bash
# 使用构建脚本（推荐）
./build.sh

# 或使用Make直接构建
cd GCC && make
```

3. **下载固件**

```bash
# 使用构建脚本
./build.sh -f

# 或使用Make
cd GCC && make flash
```

### 构建选项

```bash
# 构建调试版本
./build.sh -d

# 构建发布版本
./build.sh -r

# 清理后重新构建
./build.sh -c

# 构建并运行测试
./build.sh -t

# 构建并下载固件
./build.sh -f

# 显示项目信息
./build.sh info

# 验证构建结果
./build.sh verify
```

### Make目标

```bash
make all          # 构建项目
make debug        # 构建调试版本
make release      # 构建发布版本
make clean        # 清理构建文件
make flash        # 下载固件
make test         # 运行测试
make info         # 显示项目信息
make verify       # 验证构建
```

## 📊 技术细节

### SBL配置参数

| 参数                    | 值      | 说明           |
| ----------------------- | ------- | -------------- |
| SBL_VERSION             | "1.0.0" | SBL版本号      |
| SBL_MAX_PARTITIONS      | 16      | 最大分区数量   |
| SBL_MAX_RETRY_COUNT     | 3       | 最大重试次数   |
| SBL_WATCHDOG_TIMEOUT_MS | 30000   | 看门狗超时时间 |
| SBL_UART_BAUDRATE       | 115200  | 调试串口波特率 |

### 调试日志级别

- **SBL_LOGE**: 错误信息（红色）
- **SBL_LOGW**: 警告信息（黄色）
- **SBL_LOGI**: 一般信息（绿色）
- **SBL_LOGD**: 调试信息（蓝色）

### OTA状态管理

```c
typedef enum {
    ESP_OTA_SLOT_FACTORY = 0,  // 出厂分区
    ESP_OTA_SLOT_0 = 1,        // OTA槽0
    ESP_OTA_SLOT_1 = 2,        // OTA槽1
} esp_ota_slot_t;

typedef struct {
    uint32_t seq_label;        // 序列号
    esp_ota_slot_t active_slot; // 活动槽
    bool update_pending;       // 更新待处理
    uint32_t crc;             // 校验和
} esp_ota_data_t;
```

## 🧪 测试和验证

### 基本测试

```bash
# 运行所有测试
./build.sh test

# 或使用Make
make test
```

测试内容包括：
- 二进制文件大小检查（< 64KB）
- ELF文件结构验证
- 栈和堆配置检查
- 符号表分析

### 功能验证

1. **分区表读取**: 验证ESP32分区表解析
2. **OTA数据管理**: 验证OTA状态读写
3. **镜像验证**: 验证应用程序头部检查
4. **启动选择**: 验证分区选择逻辑
5. **故障恢复**: 验证回滚机制

## 🐛 调试支持

### 串口调试

- **串口**: UART3 (PA10/PA11)
- **波特率**: 115200
- **格式**: 8N1

### GDB调试

```bash
# 启动OpenOCD（另一个终端）
make flash-openocd

# 启动GDB调试
make gdb
```

### 日志输出示例

```
========================================
S300 SBL v1.0.0 - ESP32 Compatible Bootloader
Build: Dec 21 2024 15:30:25
========================================
CPU: ARM Cortex-M4F @ 192 MHz
Flash: QSPI XIP Mode Enabled
SRAM: 384 KB Available
========================================
[INFO ] BOOT: Initializing partition table...
[INFO ] PART: Reading partition table from offset 0x8000
[INFO ] PART: Partition table loaded successfully, 5 partitions found

==================== Partition Table ====================
Name             Type     SubType  Offset     Size       Flags
-----------------------------------------------------------
factory          app      factory  0x00010000 0x00100000 0x0000
ota_0            app      ota_0    0x00110000 0x00100000 0x0000
ota_1            app      ota_1    0x00210000 0x00100000 0x0000
otadata          data     ota      0x00009000 0x00002000 0x0000
nvs              data     nvs      0x0000B000 0x00006000 0x0000
===========================================================
Total partitions: 5

[INFO ] BOOT: Initializing OTA system...
[INFO ] OTA : Found OTA data partition: otadata (offset=0x9000, size=0x2000)
[INFO ] OTA : OTA data loaded: seq=1, active_slot=0, pending=0
[INFO ] BOOT: Selected boot partition: factory (offset=0x10000, size=0x100000)
[INFO ] BOOT: Verifying application image...
[INFO ] OTA : Image header validation passed
[INFO ] OTA : Image size: 65536 bytes
[INFO ] OTA : Entry point: 0x60010101
[INFO ] BOOT: Application load address: 0x60010000
[INFO ] BOOT: Application entry point: 0x60010101
[INFO ] BOOT: Starting application...
```

## 📈 性能指标

- **启动时间**: < 100ms（从RBL到APP）
- **内存占用**: < 64KB SRAM
- **分区表解析**: < 10ms
- **镜像验证**: < 50ms（1MB镜像）
- **代码大小**: ~20KB（发布版本）

## 🔗 兼容性

### ESP32兼容特性

- ✅ 分区表格式完全兼容
- ✅ OTA数据结构兼容
- ✅ 镜像头部格式兼容
- ✅ 错误码定义兼容
- ✅ API接口兼容

### S300特定适配

- 🔧 ARM Cortex-M4指令集适配
- 🔧 SRAM运行模式适配
- 🔧 QSPI Flash XIP适配
- 🔧 中断向量表适配

## 📝 更新日志

### v1.0.0 (2024-12-21)
- ✨ 首次发布
- ✨ ESP32兼容分区表支持
- ✨ A/B分区OTA机制
- ✨ 完整的固件验证
- ✨ 自动故障回滚
- ✨ 调试日志系统

## 🤝 贡献指南

1. Fork项目
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'feat: add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建Pull Request

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🆘 支持与联系

- **问题反馈**: [GitHub Issues](https://github.com/example/s300-sbl/issues)
- **技术讨论**: [GitHub Discussions](https://github.com/example/s300-sbl/discussions)
- **邮件联系**: support@example.com

---

**注意**: 本项目是S300 BSP的组成部分，需要与RBL配合使用以实现完整的4级引导架构。
