# S300 RBL Ymodem下载协议使用指南

## 🚀 功能特性

S300 RBL现在支持强大的Ymodem协议下载功能，具备以下特性：

### ✅ Ymodem协议支持
- **标准兼容**: 完全兼容Ymodem协议标准
- **CRC16校验**: 确保数据传输完整性
- **文件信息**: 自动获取文件名和大小
- **错误恢复**: 自动重传机制，最多重试10次
- **进度显示**: 实时显示传输进度

### ✅ 交互式命令
- **INFO**: 显示芯片信息
- **YMODEM**: 启动Ymodem文件接收
- **ERASE**: 擦除应用程序区域
- **QUIT**: 退出下载模式

### ✅ Flash管理
- **智能擦除**: 自动擦除所需扇区
- **地址保护**: 防止擦除RBL区域
- **写入验证**: CRC校验确保写入正确

## 📋 使用方法

### 1. 进入下载模式

通过以下任一方式进入下载模式：
- **串口触发**: 在3秒启动窗口期内按任意键
- **GPIO触发**: 将GPIO0拉低后复位（需要硬件支持）
- **恢复模式**: Flash出错时自动进入

### 2. 命令交互

进入下载模式后，RBL会显示：
```
=================================================
S300 RBL Download Mode - Ymodem Protocol Support
=================================================
Commands:
  INFO     - Show chip information
  YMODEM   - Start Ymodem file reception
  ERASE    - Erase application area
  QUIT     - Exit download mode
=================================================

[RBL READY] Type commands or use Ymodem to upload firmware
> 
```

### 3. 使用Ymodem下载固件

#### 步骤1: 发送YMODEM命令
```
> YMODEM
[RBL] Starting Ymodem reception...
[RBL] Please send file using Ymodem protocol
```

#### 步骤2: 在串口工具中发送文件

**Tera Term使用方法:**
1. 菜单: File → Transfer → YMODEM → Send
2. 选择要发送的bin文件
3. 点击Open开始传输

**SecureCRT使用方法:**
1. 菜单: Transfer → Send Ymodem
2. 选择文件并发送

**minicom使用方法:**
```bash
# 在minicom中按Ctrl-A然后按S
# 选择ymodem协议
# 选择要发送的文件
```

#### 步骤3: 监控传输进度
```
[YMODEM] File: firmware.bin, Size: 65536 bytes
[RBL] Progress: 1024/65536 bytes (1%)
[RBL] Progress: 2048/65536 bytes (3%)
...
[RBL] Progress: 65536/65536 bytes (100%)
[YMODEM] File reception completed
[RBL] File received successfully!
```

### 4. 验证和重启

下载完成后：
```
> QUIT
[RBL] Exiting download mode
[RBL] Download mode finished
```

RBL将重启并尝试启动新下载的固件。

## 🔧 支持的串口工具

### Windows平台
- **Tera Term** ⭐推荐
- **SecureCRT**
- **PuTTY** (需要额外的Ymodem工具)
- **HyperTerminal**

### Linux平台
- **minicom** ⭐推荐
- **cutecom**
- **picocom** + sz/rz工具

### macOS平台
- **CoolTerm**
- **minicom**
- **screen** + sz/rz工具

## 📐 技术规格

### 协议参数
- **包大小**: 128字节或1024字节
- **校验方式**: CRC16
- **超时时间**: 3秒
- **最大重试**: 10次
- **波特率**: 115200bps

### Flash布局
```
0x000000 - 0x00FFFF : RBL区域 (64KB) - 受保护
0x010000 - 0xFFFFFF : 应用程序区域 (可写入)
```

### 内存使用
- **代码大小**: ~15KB
- **RAM使用**: ~31KB (12%的可用SRAM)
- **Flash缓冲**: 1KB数据包缓冲区

## ⚠️ 注意事项

### 文件格式要求
- **支持格式**: 纯二进制文件(.bin)
- **最大大小**: 建议不超过15MB
- **起始地址**: 文件将写入0x10000开始的区域

### 安全注意
- **地址保护**: 自动防止覆盖RBL区域
- **完整性检查**: CRC16确保数据正确性
- **错误恢复**: 传输失败时可重新尝试

### 故障排除
1. **传输中断**: 检查串口连接，重新发送YMODEM命令
2. **CRC错误**: 检查串口设置，确保无流控制
3. **Flash错误**: 先执行ERASE命令清除Flash
4. **超时错误**: 确保串口工具正确配置Ymodem协议

## 📊 性能指标

### 传输速度
- **理论速度**: ~11KB/s (115200bps)
- **实际速度**: ~8-10KB/s (考虑协议开销)
- **1MB文件**: 约2分钟传输时间

### 可靠性
- **错误检测**: CRC16提供99.9%以上的错误检测率
- **自动重传**: 网络抖动或噪声导致的错误自动恢复
- **超时保护**: 避免无限等待

## 🎯 最佳实践

### 开发建议
1. **文件准备**: 使用objcopy生成纯二进制文件
2. **大小检查**: 确保固件大小合适
3. **测试验证**: 下载后检查固件运行是否正常
4. **版本管理**: 建议在固件中包含版本信息

### 生产使用
1. **自动化**: 可以编写脚本自动化下载流程
2. **批量生产**: 支持快速批量烧录
3. **质量控制**: 利用CRC校验确保烧录质量
4. **备份恢复**: 保留好的固件版本便于恢复

---

**总结**: S300 RBL的Ymodem协议支持提供了专业级的固件下载体验，兼容性好、速度快、可靠性高，适合开发和生产使用。
