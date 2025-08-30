# S300 IDF 一键下载工具使用指南

## 概述

S300 IDF 是一个类似于 ESP32 `idf.py` 的开发工具，实现了一键自动下载功能，无需手动双击复位按钮或在串口窗口期内按键。

## 特性

✅ **自动串口检测** - 无需手动指定串口设备  
✅ **自动下载模式触发** - 支持多种触发方式  
✅ **一键构建下载** - 类似 ESP32 的开发体验  
✅ **智能错误处理** - 自动尝试多种下载方法  
✅ **配置记忆** - 自动保存串口配置  
✅ **完整工具链** - 构建、烧录、监控一体化  

## 安装依赖

### 基本工具（必需）
```bash
# Ubuntu/Debian
sudo apt install make build-essential

# 串口工具（任选其一）
sudo apt install minicom       # 推荐
sudo apt install screen       # 或者
sudo apt install picocom      # 或者

# YMODEM传输工具（推荐）
sudo apt install lrzsz        # 提供sz/rz命令
```

### Python支持（可选，增强功能）
```bash
pip install pyserial          # 用于DTR复位功能
```

## 基本使用

### 1. 构建固件
```bash
./s300_idf.sh build
```

### 2. 一键下载（ESP32风格）
```bash
./s300_idf.sh flash
```

### 3. 串口监控
```bash
./s300_idf.sh monitor
```

### 4. 完整开发流程
```bash
./s300_idf.sh build flash monitor
```

## 高级用法

### 指定串口设备
```bash
./s300_idf.sh --port /dev/ttyUSB0 flash
```

### 指定波特率
```bash
./s300_idf.sh --baud 921600 flash
```

### 清理构建
```bash
./s300_idf.sh clean
```

### 组合命令
```bash
# 清理、构建、下载、监控
./s300_idf.sh clean build flash monitor

# 仅构建和下载
./s300_idf.sh build flash

# 下载并监控
./s300_idf.sh flash monitor
```

## 自动下载原理

工具会按以下顺序尝试触发下载模式：

### 1. 软件触发（优先）
- 向设备发送 `DOWNLOAD\r\n` 命令
- 利用我们之前实现的软件下载标志功能

### 2. DTR复位触发
- 使用串口的DTR信号模拟双重复位
- 触发双重启检测机制

### 3. 串口窗口触发
- 发送空格键到设备
- 在5秒启动窗口期内触发下载模式

### 4. 手动指导
- 如果自动触发失败，提供手动操作指导
- 支持双击复位按钮等方式

## 下载方法优先级

### 1. sz/rz (YMODEM) - 推荐
```bash
# 自动使用sz命令进行YMODEM传输
sz --ymodem --1k firmware.bin
```

### 2. 项目脚本
```bash
# 查找并使用项目自带的下载脚本
./flash_program.sh program
./flash_programmer.py
```

### 3. 手动指导
- 提供详细的手动下载步骤
- 支持多种串口工具

## 配置文件

工具会自动创建 `.s300_idf_config` 配置文件：

```bash
# S300 IDF 配置文件
SAVED_PORT="/dev/ttyUSB0"
SAVED_BAUD="115200"
```

首次使用后，会记住串口配置，后续无需重复指定。

## 故障排除

### 1. 找不到串口设备
```bash
# 检查串口设备
ls /dev/ttyUSB* /dev/ttyACM*

# 检查权限
sudo usermod -a -G dialout $USER
# 重新登录后生效
```

### 2. 下载模式触发失败
```bash
# 手动方法1：双击复位按钮（3秒内）
# 手动方法2：在5秒倒计时内按任意键
# 手动方法3：发送DOWNLOAD命令
```

### 3. YMODEM传输失败
```bash
# 安装lrzsz
sudo apt install lrzsz

# 或使用项目脚本
./flash_program.sh program
```

### 4. 串口权限问题
```bash
# 临时解决
sudo chmod 666 /dev/ttyUSB0

# 永久解决
sudo usermod -a -G dialout $USER
```

## 与ESP32 IDF对比

| 功能         | ESP32 IDF                    | S300 IDF                            |
| ------------ | ---------------------------- | ----------------------------------- |
| 构建         | `idf.py build`               | `./s300_idf.sh build`               |
| 烧录         | `idf.py flash`               | `./s300_idf.sh flash`               |
| 监控         | `idf.py monitor`             | `./s300_idf.sh monitor`             |
| 组合         | `idf.py build flash monitor` | `./s300_idf.sh build flash monitor` |
| 自动复位     | ✅                            | ✅                                   |
| 自动检测串口 | ✅                            | ✅                                   |
| 配置记忆     | ✅                            | ✅                                   |

## 开发工作流示例

### 典型开发流程
```bash
# 1. 修改代码后重新构建和下载
./s300_idf.sh build flash

# 2. 查看运行日志
./s300_idf.sh monitor

# 3. 完整测试流程
./s300_idf.sh clean build flash monitor
```

### 生产烧录流程
```bash
# 批量烧录（可写成脚本）
for device in /dev/ttyUSB*; do
    ./s300_idf.sh --port $device flash
done
```

## 扩展功能

### 自定义构建命令
编辑脚本中的构建部分，支持cmake、ninja等：

```bash
# 在build_firmware()函数中添加
if [[ -f "CMakeLists.txt" ]]; then
    cmake -B build && cmake --build build
fi
```

### 集成IDE
可以将此脚本集成到VS Code、CLion等IDE的构建任务中：

```json
// .vscode/tasks.json
{
    "label": "S300 Flash",
    "type": "shell",
    "command": "./s300_idf.sh",
    "args": ["build", "flash"],
    "group": "build"
}
```

这样就实现了完全类似ESP32的开发体验，无需任何手动操作即可完成固件下载！
