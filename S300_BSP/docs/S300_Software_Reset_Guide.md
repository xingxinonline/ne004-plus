# S300 软件复位和OTA工具完整指南

## 📋 功能总览

您的S300项目现已包含完整的软件复位和OTA功能实现：

### ✅ App层实现状态

- **✅ 已实现** - HelloWorld App集成软件复位功能
- **✅ 已实现** - 命令行接口（串口命令处理）
- **✅ 已实现** - 启动状态监控和异常处理
- **✅ 已实现** - 多种触发方式支持

### ✅ 脚本工具状态

- **✅ 已实现** - s300_reset_tool.py（下载复位工具）
- **✅ 已实现** - s300_ota_tool.py（OTA完成复位工具）
- **✅ 已实现** - 现代包管理支持（uv）
- **✅ 已实现** - Windows Scoop集成
- **✅ 已实现** - Linux/macOS自动安装

---

## 🚀 快速安装

### Windows (使用Scoop，推荐)

```powershell
# 管理员权限运行 PowerShell
cd S300_BSP/tools
.\install.ps1

# 开发模式（包含Git和开发依赖）
.\install.ps1 -Dev
```

### Linux/macOS (使用uv，推荐)

```bash
cd S300_BSP/tools
./install.sh

# 开发模式
./install.sh --dev
```

### 传统方式

```bash
cd S300_BSP/tools
pip install -e .
```

---

## 🔄 两种复位功能详解

### 1. 下载复位（进入下载模式）

**目的**：让设备从正常运行状态进入Bootloader下载模式

**实现**：

- **App层触发**：`app_software_reset.c` 提供API
- **脚本工具**：`s300_reset_tool.py` 提供外部控制

**使用场景**：

```bash
# 串口命令触发
s300-reset serial /dev/ttyUSB0 --mode download

# 双重启模拟
s300-reset double-reset /dev/ttyUSB0 --interval 1.5

# 网络远程触发
s300-reset http 192.168.1.100 --mode download
```

### 2. OTA完成复位（启动新固件）

**目的**：固件下载完成后，重启到新固件

**实现**：

- **Bootloader处理**：清除下载标志，正常启动
- **脚本工具**：`s300_ota_tool.py` 管理整个OTA流程

**使用场景**：

```bash
# 完整OTA流程
s300-ota ota /dev/ttyUSB0 firmware.bin

# 分步操作
s300-ota trigger /dev/ttyUSB0      # 进入下载模式
s300-ota upload /dev/ttyUSB0 fw.bin    # 上传固件
s300-ota wait /dev/ttyUSB0              # 等待完成并重启
```

---

## 💻 App集成说明

### 文件结构

```text
Projects/HelloWorld/
├── Inc/
│   └── app_software_reset.h     # App软件复位头文件
├── Src/
│   ├── main.c                   # 更新的主程序（集成复位功能）
│   └── app_software_reset.c     # App软件复位实现
```

### 主要功能

1. **命令处理**：支持串口命令 `reset`, `download`, `status` 等
2. **启动监控**：自动检测启动成功/失败
3. **异常处理**：故障时自动进入恢复模式
4. **状态报告**：提供系统状态查询

### 使用方法

在您的App中调用：

```c
#include "app_software_reset.h"

int main(void) {
    // 基本初始化...
    
    // 初始化软件复位功能
    app_software_reset_init();
    
    // 主循环
    while(1) {
        // 处理软件复位循环任务
        app_software_reset_loop_handler();
        
        // 处理串口命令
        if (uart_has_data()) {
            char cmd[64];
            uart_read_line(cmd, sizeof(cmd));
            app_handle_command(cmd);
        }
        
        // 您的业务逻辑...
    }
}
```

---

## 🛠️ 脚本工具详解

### s300_reset_tool.py - 下载复位工具

**主要功能**：

- 扫描和检测S300设备
- 触发进入下载模式
- 双重启模拟
- 网络远程控制
- 设备状态查询

**常用命令**：

```bash
# 扫描所有串口的S300设备
s300-reset scan

# 触发下载模式
s300-reset serial COM3 --mode download      # Windows
s300-reset serial /dev/ttyUSB0 --mode download  # Linux

# 双重启模拟（2秒内按两次复位键的效果）
s300-reset double-reset /dev/ttyUSB0 --interval 1.5

# 查看设备状态
s300-reset status /dev/ttyUSB0

# 网络触发（如果设备支持）
s300-reset http 192.168.1.100 --mode download
```

### s300_ota_tool.py - OTA升级工具

**主要功能**：

- 完整OTA升级流程
- 固件上传和验证
- 升级状态监控
- 美观的进度显示

**常用命令**：

```bash
# 完整OTA流程（一键完成）
s300-ota ota /dev/ttyUSB0 firmware.bin

# 分步操作
s300-ota trigger /dev/ttyUSB0 --method command
s300-ota wait /dev/ttyUSB0 --timeout 30
s300-ota upload /dev/ttyUSB0 firmware.bin
s300-ota info /dev/ttyUSB0

# 只触发下载模式
s300-ota trigger /dev/ttyUSB0 --method double_reset
```

---

## 📦 包管理优势

### 使用uv的优点

1. **极快的依赖解析**：比pip快10-100倍
2. **确定性构建**：锁定文件确保一致性
3. **现代工具链**：支持最新Python特性
4. **跨平台兼容**：Windows/Linux/macOS统一体验

### 使用Scoop的优点（Windows）

1. **无需管理员权限**：用户级别安装
2. **包管理一致性**：类似Linux包管理器体验
3. **自动环境变量**：无需手动配置PATH
4. **版本管理**：轻松切换不同版本

---

## 🔧 开发和调试

### 安装开发环境

```bash
# Windows
.\install.ps1 -Dev

# Linux/macOS
./install.sh --dev
```

### 代码格式化和检查

```bash
cd S300_BSP/tools

# 代码格式化
uv run black .

# 类型检查
uv run mypy .

# 运行测试
uv run pytest
```

### 添加新功能

1. 编辑 `s300_reset_tool.py` 或 `s300_ota_tool.py`
2. 运行格式化：`uv run black .`
3. 测试功能：`uv run python s300_reset_tool.py --help`
4. 提交代码

---

## 📊 Flash分区布局（已更新）

```text
| 地址范围              | 大小   | 用途        | 描述                 |
| --------------------- | ------ | ----------- | -------------------- |
| 0x00000000-0x000000FF | 256B   | Header      | RBL元信息            |
| 0x00000100-0x0000FFFF | 64KB   | RBL         | ROM Bootloader       |
| 0x00010000-0x0002FFFF | 128KB  | SBL         | Secondary Bootloader |
| 0x00030000-0x0003EFFF | 60KB   | NVS         | 非易失性存储         |
| 0x0003F000-0x0003FFFF | 4KB    | Reset_Flags | 软件复位标志区       |
| 0x00040000-0x0063FFFF | 6MB    | OTA_0       | 应用分区A            |
| 0x00640000-0x00C3FFFF | 6MB    | OTA_1       | 应用分区B            |
| 0x00C40000-0x00FFFFFF | 3.75MB | Data        | 用户数据区           |
```

**软件复位标志区详细**：

- `0x3F000 + 0x000`: 下载模式标志 (24字节)
- `0x3F000 + 0x100`: 双重启标志 (16字节)
- `0x3F000 + 0x200`: 启动计数器 (24字节)

---

## 🎯 使用示例

### 场景1：开发调试

```bash
# 连接设备，查看状态
s300-reset status /dev/ttyUSB0

# 需要更新固件时，进入下载模式
s300-reset serial /dev/ttyUSB0 --mode download

# 使用其他工具上传固件...
```

### 场景2：生产测试

```bash
# 扫描连接的设备
s300-reset scan

# 使用双重启进入下载模式（无需软件）
# 物理操作：快速按两次复位按钮（2秒内）

# 或脚本模拟
s300-reset double-reset /dev/ttyUSB0
```

### 场景3：完整OTA升级

```bash
# 一键完成整个升级流程
s300-ota ota /dev/ttyUSB0 new_firmware.bin

# 查看升级后状态
s300-reset status /dev/ttyUSB0
```

### 场景4：远程维护

```bash
# 网络触发（需要设备支持网络）
s300-reset http 192.168.1.100 --mode download

# 或通过蓝牙（需要设备支持蓝牙）
# （集成在脚本中，自动检测）
```

---

## ⚠️ 注意事项

1. **权限问题**：
   - Linux: 确保用户在`dialout`组中
   - Windows: 可能需要管理员权限
   - macOS: 通常无需特殊权限

2. **串口识别**：
   - Windows: COM1, COM2, COM3...
   - Linux: /dev/ttyUSB0, /dev/ttyACM0...
   - macOS: /dev/cu.usbserial-*...

3. **时序要求**：
   - 双重启检测窗口：2秒
   - 串口响应超时：3秒
   - OTA升级超时：根据固件大小

4. **故障恢复**：
   - 连续3次启动失败自动进入恢复模式
   - 可使用`factory_reset`命令清除所有标志

---

## 🎉 总结

您现在拥有完整的S300软件复位和OTA解决方案：

✅ **App实现**：完全集成到HelloWorld项目  
✅ **脚本工具**：两个专业工具（复位+OTA）  
✅ **现代包管理**：uv + Scoop支持  
✅ **跨平台支持**：Windows/Linux/macOS  
✅ **多种触发方式**：串口/网络/双重启/故障恢复  
✅ **完整文档**：详细使用说明和API文档  

这套方案完全解决了您**没有硬件复位电路**的问题，并提供了比传统硬件方案更加灵活和强大的功能！
