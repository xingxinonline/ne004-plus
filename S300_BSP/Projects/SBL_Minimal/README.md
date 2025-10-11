# S300 SBL Minimal

最简SBL程序，用于测试RBL跳转功能是否正常。

## 功能特点

- ✅ 运行在Flash中（0x00100000开始）
- ✅ UART3串口输出（115200 8N1）
- ✅ 向量表重定位到Flash
- ✅ 心跳输出，确认SBL正常运行
- ✅ 显示系统信息（Flash地址、向量表、栈指针）

## 构建方法

```bash
cd S300_BSP/Projects/SBL_Minimal/GCC
make all                # 构建基本文件
make s300_image        # 生成S300格式镜像
make complete_image    # 构建完整系统镜像 (Header+RBL+SBL)
make analyze_complete  # 分析完整系统镜像结构
```

## 镜像文件说明

构建完成后会生成以下文件：

- `s300_sbl_minimal.bin` - 单独的SBL二进制文件
- `s300_sbl_minimal_complete.bin` - SBL+Header镜像 
- `s300_complete_system.bin` - **完整系统镜像 (推荐使用)**

### 完整系统镜像结构

`s300_complete_system.bin` 包含：

| 偏移地址 | 内容   | 大小   | 说明                 |
| -------- | ------ | ------ | -------------------- |
| 0x000000 | Header | 256B   | S300启动头信息       |
| 0x000100 | RBL    | ~11KB  | ROM Bootloader       |
| 0x010000 | SBL    | ~1.2KB | Secondary Bootloader |

## 测试流程

### 方案1：使用完整系统镜像（推荐）

1. **直接烧录完整镜像**：
   ```bash
   # 使用Flash编程工具烧录到0x80000000
   flashtool write 0x80000000 build/s300_complete_system.bin
   ```

2. **复位设备观察输出**：
   - 芯片ROMBOOT加载RBL到SRAM
   - RBL检测并跳转到SBL(XIP模式)
   - SBL输出启动信息和心跳

### 方案2：通过RBL下载SBL

1. **构建并下载RBL**：
   ```bash
   cd ../../build
   ninja s300_image
   # 使用Flash工具烧录RBL到0x80000000
   ```

2. **使用RBL下载SBL**：
   ```bash
   cd ../SBL_Minimal/tools
   # 等待RBL进入下载模式，然后：
   ../RBL_Minimal/tools/download.sh ../SBL_Minimal/GCC/build/s300_sbl_minimal.bin
   ```

3. **复位设备测试跳转**

## 预期输出

### RBL阶段：
```
==== S300 RBL Minimal v1.0 ====
Hello from SRAM RBL!
[RBL] Starting Phase 2: QSPI initialization...
[RBL] QSPI JEDEC read ok
[RBL] Phase 2 validation PASSED!
[RBL] Starting Phase 3: SBL validation...
[RBL] Phase 3 validation PASSED!
[RBL] SBL jump successful!
```

### SBL阶段：
```
========================================
    S300 SBL Minimal Test v1.0
========================================
✅ SBL started successfully!
✅ RBL jump to SBL works!
Build: Sep  2 2025 xx:xx:xx
Flash Address: 0x00100000
Vector Table: 0x00100000
Stack Pointer: 0x2000xxxx
========================================

[SBL] Heartbeat #1 - SBL running OK
[SBL] Heartbeat #2 - SBL running OK
...
```

## 地址映射

| 组件  | 地址范围              | 说明               |
| ----- | --------------------- | ------------------ |
| RBL   | 0x20000000-0x2000FFFF | SRAM1运行          |
| SBL   | 0x00100000-0x0017FFFF | Flash运行（512KB） |
| SRAM1 | 0x20000000-0x2000FFFF | SBL数据段和栈      |

## 调试方法

1. **串口监控**：
   ```bash
   cd ../RBL_Minimal/tools
   ./monitor.py /dev/ttyUSB0
   ```

2. **GDB调试**：
   ```bash
   make dbg
   ```

## 故障排除

### 如果看不到SBL输出：
1. 检查RBL是否检测到SBL：看Phase 3输出
2. 检查SBL是否正确下载到Flash：查看下载过程
3. 检查串口连接和波特率设置

### 如果RBL不跳转：
1. 检查SBL文件大小和格式
2. 检查Flash写入是否成功
3. 检查SBL header是否正确生成

## 文件说明

- `sbl_minimal.c` - 主程序源码
- `flash.ld` - Flash链接脚本
- `Makefile` - 构建脚本
- `README.md` - 本说明文档
