# App Ymodem OTA Demo

基于S300芯片的Ymodem协议OTA升级演示应用程序。

## 项目概述

这是一个演示应用程序，展示了如何在S300 Cortex-M4平台上实现基于Ymodem协议的OTA(Over-The-Air)固件升级功能。

## 功能特性

### 核心功能
- **Ymodem协议支持**: 完整的Ymodem文件传输协议实现
- **OTA升级管理**: 固件接收、验证和更新流程
- **命令行界面**: 交互式控制台界面
- **状态监控**: 实时显示升级进度和系统状态
- **错误处理**: 完善的错误检测和恢复机制

### 技术特性
- **内存优化**: 使用364KB可用SRAM
- **CRC校验**: 数据完整性验证
- **Flash管理**: 安全的Flash分区操作
- **中断驱动**: 高效的UART通信
- **调试支持**: 完整的日志和调试信息

## 硬件要求

- **芯片**: PiMCHIP S300 (Cortex-M4F)
- **Flash**: 1MB (推荐)
- **SRAM**: 512KB (使用364KB)
- **时钟**: 200MHz系统时钟
- **接口**: UART3用于通信

## 软件架构

```
App_YmodemOTA/
├── Inc/                    # 头文件
│   ├── app_config.h       # 应用配置
│   ├── ymodem.h           # Ymodem协议
│   ├── app_ota.h          # OTA管理
│   └── app_main.h         # 主程序
├── Src/                    # 源文件
│   ├── app_main.c         # 主程序实现
│   ├── ymodem.c           # Ymodem协议实现
│   ├── app_ota.c          # OTA管理实现
│   └── system_s300_app.c  # 系统初始化
└── GCC/                    # 构建系统
    ├── Makefile           # 构建脚本
    ├── app.ld             # 链接脚本
    └── startup_s300_app.s # 启动文件
```

## 编译构建

### 环境要求
- ARM GCC工具链
- GNU Make
- Linux/WSL环境

### 构建命令
```bash
# 进入构建目录
cd S300_BSP/Projects/App_YmodemOTA/GCC

# 查看构建信息
make info

# 编译调试版本
make debug

# 编译发布版本
make release

# 清理构建文件
make clean

# 重新构建
make rebuild

# 验证构建结果
make verify

# 分析二进制文件
make analyze
```

### 构建输出
```
build/
├── bin/
│   ├── app_ymodem_ota.bin   # 二进制固件
│   ├── app_ymodem_ota.elf   # ELF文件
│   └── app_ymodem_ota.hex   # 十六进制文件
├── obj/                     # 目标文件
└── app_ymodem_ota.map       # 内存映射文件
```

## 使用说明

### 启动应用
1. 烧录固件到S300开发板
2. 连接UART3到PC
3. 打开串口终端(115200, 8N1)
4. 重启设备查看启动信息

### 命令行界面
```
S300 App Demo v1.0.0
====================
Available commands:
- help          Show help information
- version       Show version information
- status        Show system status
- info          Show system information
- ota start     Start OTA update via Ymodem
- ota status    Show OTA status
- ota cancel    Cancel OTA update
- reboot        Restart system

S300> _
```

### OTA升级流程
1. 在终端输入: `ota start`
2. 系统进入接收模式
3. 使用支持Ymodem的终端软件发送固件文件
4. 等待传输完成和验证
5. 系统自动重启完成升级

### 支持的终端软件
- **SecureCRT**: 文件 → 传输 → 发送Ymodem
- **Tera Term**: 文件 → 传输 → Ymodem → 发送
- **PuTTY + sz**: 使用sz命令发送文件
- **minicom**: Ctrl+A → S → ymodem

## 内存布局

```
Memory Layout (364KB SRAM):
+------------------+ 0x20000000
|   Vector Table   | 1KB
+------------------+ 0x20000400
|   .data          | ~8KB
+------------------+ 0x20002400
|   .bss           | ~16KB
+------------------+ 0x20006400
|   Heap           | 200KB
+------------------+ 0x20038400
|   Stack          | 139KB
+------------------+ 0x2005B000 (364KB)
```

## 配置选项

### app_config.h主要配置
```c
// 版本信息
#define APP_VERSION_MAJOR    1
#define APP_VERSION_MINOR    0
#define APP_VERSION_PATCH    0

// UART配置
#define APP_UART_PORT        3
#define APP_UART_BAUDRATE    115200

// Ymodem配置
#define YMODEM_PACKET_SIZE   1024
#define YMODEM_TIMEOUT_MS    10000

// OTA配置
#define APP_OTA_MAX_SIZE     (512 * 1024)  // 512KB
#define APP_OTA_START_ADDR   0x08080000    // Flash起始地址

// 调试配置
#define APP_DEBUG_ENABLED    1
#define APP_LOG_LEVEL        LOG_LEVEL_INFO
```

## 调试和诊断

### 调试输出
应用程序提供详细的调试信息：
```
[INFO] App: System initialized successfully
[INFO] UART: UART3 initialized (115200 bps)
[INFO] OTA: OTA manager initialized
[INFO] Ymodem: Protocol stack ready
[DEBUG] Main: Entering command loop
```

### 错误代码
| 代码 | 含义 | 处理方法 |
|------|------|----------|
| 0x01 | UART初始化失败 | 检查硬件连接 |
| 0x02 | Flash操作失败 | 检查Flash状态 |
| 0x03 | Ymodem协议错误 | 重新发送文件 |
| 0x04 | OTA验证失败 | 检查固件完整性 |
| 0x05 | 内存不足 | 减少缓冲区大小 |

### 性能监控
```bash
# 查看内存使用
S300> info

# 查看系统状态
S300> status

# 查看OTA状态
S300> ota status
```

## 故障排除

### 常见问题

1. **编译失败**
   - 检查工具链安装
   - 确认包含路径正确
   - 查看编译错误信息

2. **UART通信异常**
   - 检查波特率设置(115200)
   - 确认引脚连接正确
   - 检查电平匹配(3.3V)

3. **OTA传输失败**
   - 确认固件大小不超过限制
   - 检查文件完整性
   - 尝试降低传输速度

4. **系统重启异常**
   - 检查栈大小配置
   - 查看HardFault信息
   - 确认内存布局正确

### 调试模式
编译时添加调试标志：
```bash
make debug CFLAGS+="-DAPP_DEBUG_VERBOSE=1"
```

## 扩展开发

### 添加新命令
1. 在`app_main.h`中声明命令处理函数
2. 在`app_main.c`中实现函数
3. 在命令表中注册新命令

### 修改协议
1. 修改`ymodem.h`中的协议参数
2. 更新`ymodem.c`中的实现逻辑
3. 测试兼容性

### 优化性能
1. 调整缓冲区大小
2. 优化算法实现
3. 使用DMA传输

## 技术支持

### 文档资源
- S300技术参考手册
- Ymodem协议规范
- ARM Cortex-M4编程手册

### 开发工具
- ARM GCC Toolchain
- OpenOCD调试器
- VSCode开发环境

## 版本历史

### v1.0.0 (2024-01-15)
- 初始版本发布
- 完整的Ymodem协议支持
- OTA升级功能实现
- 命令行界面
- 调试和日志功能

## 许可证

本项目基于S300 BSP许可证发布。

---

**注意**: 这是一个演示项目，用于展示S300平台的OTA升级能力。在生产环境中使用前，请进行充分的测试和验证。
