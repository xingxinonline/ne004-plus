# S300 RBL Minimal - 极简ROM引导加载程序

## 🚀 项目概述

S300 RBL Minimal是专为PiMCHIP S300芯片设计的极简ROM引导加载程序(ROM Bootloader)，专注于安全可靠的系统启动，去除了下载功能以减少代码大小和复杂性。

### 🔥 核心特性

- **🔒 安全启动**: SBL完整性验证，防止启动损坏固件
- **🛡️ Flash保护**: 自动写保护管理，保护关键启动代码
- **🔧 直接烧录**: 通过JTAG/DAPLink直接烧录，支持生产环境
- **⚡ 高性能**: 仅占用12%的SRAM，启动时间<100ms
- **🎯 极简设计**: 移除下载功能，专注于启动流程

## 📋 快速开始

### 1. 构建项目
```bash
# 一键构建
./build.sh build

# 查看项目信息
./build.sh info
```

### 2. 直接Flash烧录 (生产用)
```bash
# 使用ST-Link烧录
./flash_program.sh build-and-program

# 使用J-Link烧录  
./flash_program.sh -i jlink program

# 查看Flash状态
./flash_program.sh info
```

## 📊 技术规格

| 项目     | 规格                     |
| -------- | ------------------------ |
| 目标芯片 | PiMCHIP S300 (Cortex-M4) |
| Flash    | W25Q128JW (16MB)         |
| SRAM     | 256KB                    |
| 串口     | UART3, 115200bps         |
| 代码大小 | ~12KB                    |
| 内存占用 | ~24KB (9.4%)             |
| 启动时间 | <100ms                   |

## 🔍 命令参考

### 构建命令
- `./build.sh build` - 构建项目
- `./build.sh clean` - 清理构建文件
- `./build.sh info` - 显示项目信息

### 烧录命令
- `./flash_program.sh program` - 烧录RBL
- `./flash_program.sh info` - 显示Flash信息
- `./flash_program.sh build-and-program` - 构建并烧录

### 环境要求
- ARM GCC工具链 (arm-none-eabi-gcc)
- GNU Make
- Python 3.6+

### 构建命令

```bash
# 进入构建目录
cd GCC

# 编译RBL
make all

# 生成调试版本
make debug

# 生成完整S300镜像
make s300_image

# 查看大小信息
make size

# 清理构建文件
make clean
```

### 输出文件
- `build/rbl.bin` - RBL二进制文件
- `build/rbl.elf` - ELF调试文件
- `build/rbl_header.bin` - S300头部文件
- `build/s300_rbl_complete.bin` - 完整镜像(头部+RBL)

## 配置说明

### 主要配置 (rbl_config.h)

```c
/* Flash布局配置 */
#define RBL_FLASH_SBL_ADDR          0x8000  // SBL地址
#define RBL_FLASH_APP_ADDR          0x40000 // 应用程序地址

/* 系统配置 */
#define RBL_SYSTEM_CLOCK_MHZ        168     // 系统时钟
#define RBL_DEBUG_UART              UART3   // 调试串口
```

### Flash布局

| 地址范围          | 大小  | 用途        | 说明              |
| ----------------- | ----- | ----------- | ----------------- |
| 0x000000-0x0000FF | 256B  | S300 Header | ROMBOOT读取       |
| 0x000100-0x007FFF | ~32KB | RBL         | ROM Bootloader    |
| 0x008000-0x03FFFF | 224KB | SBL         | Second Bootloader |
| 0x040000-0x23FFFF | 2MB   | App主分区   | OTA_0             |
| 0x240000-0x43FFFF | 2MB   | App备份分区 | OTA_1             |
| 0x440000-0xFFEFFF | 12MB  | 用户数据    | 自由使用          |
| 0xFFF000-0xFFFFFF | 4KB   | 系统参数    | OTA状态等         |

## 启动流程

### 1. ROMBOOT阶段
- 读取Flash 0x0处的256字节Header
- 验证Header CRC32
- 加载RBL到SRAM (0x20000000)
- 跳转到RBL入口

### 2. RBL阶段
- 系统初始化 (时钟、UART、QSPI)
- 检查并跳转到SBL
- 如果SBL验证失败，系统复位

### 3. 极简设计
- 无下载模式
- 无串口窗口检测
- 专注于快速、安全启动

## 调试信息

### 串口输出示例
```
[RBL] S300 ROM Bootloader Minimal v1.0.0
[RBL] Build: Dec 19 2024 10:30:45
[RBL] Chip: S300, Board: Generic EVB
[RBL] System clock: 168MHz
[RBL] QSPI Flash: W25Q128 (16MB)
[RBL] Checking SBL at 0x8000...
[RBL] SBL verified, jumping...
```

### 错误处理
- CRC校验失败 → 系统复位
- SBL损坏 → 系统复位
- Flash读取错误 → 系统复位

## 开发说明

### 添加新功能
1. 在`Inc/`目录添加头文件
2. 在`Src/`目录添加源文件
3. 更新`Makefile`的`SOURCES`列表
4. 重新编译测试

### 调试技巧
- 使用串口输出调试信息
- 通过`make disasm`查看反汇编
- 使用GDB进行在线调试

### 注意事项
- RBL运行在SRAM中，注意内存限制
- 保持代码精简，避免使用大型库
- 硬件相关代码需要根据实际S300规格调整

## 相关文档

- [S300 Boot Architecture](../../../docs/S300_Boot_Architecture_Final.md)
- [S300 Header Format](../../../docs/S300_Header_Format.md)
- [S300 RBL Build Process](../../../docs/S300_RBL_Build_Process.md)

## 版本历史

### v1.0.0 (2024-12-19)
- 初始版本
- 实现基本启动功能
- 移除下载功能
- 极简设计

## 联系信息

技术支持: 请参考项目文档或提交Issue