# S300 RBL - 专业级ROM引导加载程序

## 🚀 项目概述

S300 RBL是专为PiMCHIP S300芯片设计的ROM引导加载程序(ROM Bootloader)，提供安全可靠的系统启动和固件更新功能。

### 🔥 核心特性

- **🔒 安全启动**: SBL完整性验证，防止启动损坏固件
- **📡 Ymodem协议**: 支持标准Ymodem文件传输，兼容主流串口工具
- **🛡️ Flash保护**: 自动写保护管理，保护关键启动代码
- **🔧 直接烧录**: 通过JTAG/DAPLink直接烧录，支持生产环境
- **⚡ 高性能**: 仅占用12%的SRAM，启动时间<100ms

## 📋 快速开始

### 1. 构建项目
```bash
# 一键构建
./build.sh build

# 查看项目信息
./build.sh info
```

### 2. 通过Ymodem下载固件
```bash
# 连接串口 (115200,8,N,1)
# 重启设备，3秒内按任意键进入下载模式
# 发送YMODEM命令
> YMODEM
# 在串口工具中选择"发送文件-Ymodem"
```

### 3. 直接Flash烧录 (生产用)
```bash
# 使用ST-Link烧录
./flash_program.sh build-and-program

# 使用J-Link烧录  
./flash_program.sh -i jlink program

# 查看Flash状态
./flash_program.sh info
```

## � 芯片检测功能 (ESP32兼容)

### 自动芯片识别
RBL集成了强大的串口芯片检测功能，类似ESP32的`esptool.py`，能够自动识别连接的芯片类型：

#### 支持的芯片类型
- **PiMCHIP S300** (Cortex-M4) - 本芯片
- **ESP32系列** - ESP32, ESP32-C3, ESP32-S3, ESP32-C6
- **ESP8266** - 经典WiFi芯片
- **STM32系列** - F4, H7等ARM芯片
- **其他** - AT命令兼容芯片

#### 检测方法
1. **ESP32 SYNC协议** - 使用ESP32标准的SYNC命令
2. **AT命令检测** - 发送AT命令识别ESP8266等
3. **S300专用协议** - 本地芯片的INFO/VERSION命令
4. **自动波特率** - 支持常用波特率自动检测

### 使用Python检测工具

```bash
# 自动检测所有串口的芯片
python3 chip_detect_tool.py

# 检测指定串口
python3 chip_detect_tool.py --port /dev/ttyUSB0

# 列出所有可用串口
python3 chip_detect_tool.py --list

# 调试模式 (显示原始响应)
python3 chip_detect_tool.py --debug
```

#### 典型输出示例
```
Auto-detecting chips on all serial ports...
[INFO] Found 3 serial ports: /dev/ttyUSB0, /dev/ttyUSB1, /dev/ttyACM0

Scanning /dev/ttyUSB0...
[INFO] Trying ESP32 detection...
[INFO] ESP32 SYNC response received
✓ Found ESP32 on /dev/ttyUSB0

==================================================
Chip Detection Result  
==================================================
Port: /dev/ttyUSB0 @ 115200 baud
Chip Type: ESP32
Chip Name: ESP32
Chip Family: ESP32
Chip ID: 0x00f01d83
Bootloader Mode: Yes
Detection Method: detect_esp32_chip
==================================================
```

### C语言API接口

RBL也提供了完整的C语言芯片检测API：

```c
#include "chip_detection.h"

// 自动检测芯片
uart_detection_result_t result;
int ret = chip_detect_auto(NULL, &result);
if (ret == CHIP_DETECT_OK && result.detected) {
    printf("Found: %s on %s\n", result.chip.name, result.port_name);
    chip_print_detection_result(&result);
}

// 检测指定端口
ret = chip_detect_port("/dev/ttyUSB0", 115200, &result);

// ESP32兼容检测
ret = chip_detect_esp32_compatible("/dev/ttyUSB0", &result);

// 列出所有串口
char ports[16][32];
int count = chip_list_serial_ports(ports, 16);
```

### 集成到RBL启动流程

芯片检测功能已集成到RBL的下载模式中：

```c
// RBL启动时的芯片识别
void rbl_show_chip_info(void) {
    chip_info_t local_info;
    if (chip_detect_s300_local(&local_info) == CHIP_DETECT_OK) {
        printf("[RBL] Local Chip: %s\n", local_info.name);
        printf("[RBL] Chip ID: 0x%08X\n", local_info.chip_id);
        printf("[RBL] Flash: %u KB, RAM: %u KB\n", 
               local_info.flash_size/1024, local_info.ram_size/1024);
    }
    
    // 扫描连接的外部芯片
    uart_detection_result_t external;
    if (chip_detect_auto("ttyUSB*", &external) == CHIP_DETECT_OK) {
        printf("[RBL] External Chip: %s on %s\n", 
               external.chip.name, external.port_name);
    }
}
```

## 🛡️ 安全特性

### SBL完整性验证
- **ARM向量表检查**: 验证栈指针和复位处理程序
- **内容完整性**: 采样检测Flash是否完整写入
- **CRC32校验**: 数据完整性验证
- **错误恢复**: 验证失败自动进入下载模式

### Flash写保护
- **硬件保护**: 利用W25Q128写保护功能
- **分区保护**: 只保护RBL区域，应用区域可正常更新
- **状态管理**: 烧录前自动解保护，烧录后自动加保护

## 📊 技术规格

| 项目 | 规格 |
|------|------|
| 目标芯片 | PiMCHIP S300 (Cortex-M4) |
| Flash | W25Q128JW (16MB) |
| SRAM | 256KB |
| 串口 | UART3, 115200bps |
| 代码大小 | ~17KB |
| 内存占用 | ~32KB (12.5%) |
| 启动时间 | <100ms |

## 🔍 命令参考

### 串口命令 (下载模式)
- `INFO` - 显示芯片和系统信息
- `YMODEM` - 启动Ymodem文件接收
- `ERASE` - 擦除应用程序Flash区域
- `QUIT` - 退出下载模式并重启

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
/* 串口下载配置 */
#define RBL_DOWNLOAD_TIMEOUT_MS     3000    // 下载窗口期
#define RBL_DOWNLOAD_BAUD_RATE      115200  // 波特率

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
- 检测启动模式:
  - **正常启动**: 检查并跳转到SBL
  - **串口下载**: 3秒窗口期等待下载命令
  - **恢复模式**: 强制进入下载模式

### 3. 下载协议
兼容ESP32 ROM Bootloader协议:
- 自动波特率检测
- 二进制数据包传输
- 实时进度反馈
- CRC校验保护

## 调试信息

### 串口输出示例
```
[RBL] S300 ROM Bootloader v1.0.0
[RBL] Build: Dec 19 2024 10:30:45
[RBL] Chip: S300, Board: Generic EVB
[RBL] System clock: 168MHz
[RBL] QSPI Flash: W25Q128 (16MB)
[RBL] Boot mode: Normal
[RBL] Checking SBL at 0x8000...
[RBL] SBL verified, jumping...
```

### 错误处理
- CRC校验失败 → 自动进入恢复模式
- SBL损坏 → 等待串口下载
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
- [S300 RBL Serial Download](../../../docs/S300_RBL_Serial_Download.md)

## 版本历史

### v1.0.0 (2024-12-19)
- 初始版本
- 实现基本启动功能
- 支持串口下载
- ESP32协议兼容

## 联系信息

技术支持: 请参考项目文档或提交Issue
