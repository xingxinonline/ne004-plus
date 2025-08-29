# QSPI XIP (Execute In Place) Demo

这个demo演示了如何在S300芯片上使用QSPI Flash的XIP（Execute In Place）功能。

## 功能概述

XIP允许CPU直接从QSPI Flash中执行代码和访问数据，无需先将其复制到SRAM中。这在以下场景中特别有用：
- 代码空间超过内部SRAM大小
- 需要节省SRAM空间
- 执行大型固件或应用程序
- 访问大量资源数据（字体、图像、配置数据等）

## Demo版本

### 基础版本 (main.c)
基础XIP功能演示，包含：
- XIP模式配置和退出
- 简单函数的XIP执行
- XIP数据访问测试
- 性能对比测试

### 高级版本 (main_advanced.c)
更全面的XIP应用场景演示，包含：
- 多种数据类型的XIP访问（数组、字符串、字体数据）
- 资源管理和数据处理
- 混合访问模式测试（XIP vs STIG）
- 详细的性能基准测试
- 带宽计算和分析

## Demo功能

### 1. QSPI初始化
- 初始化QSPI控制器
- 检测并配置W25Q128 Flash
- 启用Quad模式以提高性能

### 2. 准备Flash内容
- 在Flash中写入测试机器码（简单的加法函数）
- 在Flash中写入测试数据（数组、字符串、字体数据）
- 验证写入的内容

### 3. 配置XIP模式
- 配置QSPI控制器为直接访问模式
- 设置Quad I/O Fast Read命令(0xEB)
- 配置适当的dummy周期和模式字节

### 4. XIP数据访问测试
- 通过AHB总线直接读取Flash中的数据
- 验证数据的正确性
- 演示资源数据的可视化显示

### 5. XIP代码执行测试
- 调用存储在Flash中的函数
- 验证函数执行结果的正确性
- 测试多种参数组合

### 6. 性能对比测试
- 比较SRAM和XIP模式下的执行性能
- 使用SysTick定时器进行精确时间测量（支持GDB调试环境）
- 显示XIP模式的性能开销
- 计算数据访问带宽

### 7. 混合访问模式测试（高级版本）
- 同时使用XIP和STIG模式访问数据
- 验证两种模式的数据一致性
- 演示动态模式切换

## 地址映射

- **Flash物理地址**: 基于QSPI偏移
  - 基础版本:
    - 测试代码: 0x100000 (1MB偏移)
    - 测试数据: 0x110000 (1.1MB偏移)
  - 高级版本:
    - 代码段: 0x100000 (1MB偏移)
    - 数据段: 0x120000 (1.125MB偏移)
    - 字体数据: 0x140000 (1.25MB偏移)

- **XIP映射地址**: 0x08000000 + Flash偏移
  - 基础版本:
    - 测试代码: 0x08100000
    - 测试数据: 0x08110000
  - 高级版本:
    - 代码段: 0x08100000
    - 数据段: 0x08120000
    - 字体数据: 0x08140000

## 测试函数

demo中使用的测试函数机器码对应以下C代码：
```c
int xip_test_function(int a, int b) {
    return a + b + 42;
}
```

机器码（ARM Thumb指令）：
- `0x08, 0x44` - add r0, r0, r1  (a + b)
- `0x2A, 0x30` - adds r0, #42    (+ 42)
- `0x70, 0x47` - bx lr           (返回)

## 使用方法

### 编译

#### 基础版本
```bash
cd /home/xinhao/work/ne004-plus/S300_BSP
make PROJECT=Demo/QSPI_XIP_Demo XIP_VERSION=basic
# 或者
make PROJECT=Demo/QSPI_XIP_Demo  # 默认为basic版本
```

#### 高级版本
```bash
cd /home/xinhao/work/ne004-plus/S300_BSP
make PROJECT=Demo/QSPI_XIP_Demo XIP_VERSION=advanced
```

### 调试
```bash
make PROJECT=Demo/QSPI_XIP_Demo XIP_VERSION=basic dbg
make PROJECT=Demo/QSPI_XIP_Demo XIP_VERSION=advanced dbg
```

### 查看编译配置
```bash
cd S300_BSP/Projects/Demo/QSPI_XIP_Demo/GCC
make help
make print
```

## 输出示例

### 基础版本输出
```
QSPI XIP (Execute In Place) Demo
================================
AHB clock: 192000000 Hz

=== Step 1: QSPI Initialization ===
QSPI frequency: 48000000 Hz
Flash detected: Quad=YES, 4ByteAddr=NO

=== Step 2: Prepare Flash Content ===
Preparing Flash content for XIP test...
  Erasing 64KB block @0x100000 for code
  Erasing 64KB block @0x110000 for data
  Programming test function (6 bytes)
  Programming test data (40 bytes)
Flash content prepared successfully

... (详细测试步骤)

=== XIP Demo Completed Successfully ===
All tests passed!
Maximum working frequency: 48000000 Hz (~48 MHz)
```

### 高级版本输出
```
Advanced QSPI XIP (Execute In Place) Demo
=========================================
System Core Clock: 192000000 Hz
AHB Clock: 192000000 Hz

=== Step 1: QSPI Initialization ===
QSPI frequency: 48000000 Hz (~48 MHz)
Flash Configuration:
  Quad Mode: Enabled
  4-Byte Address Mode: Disabled

... (包含字体数据可视化显示)

  XIP Font Data (first character):
    ..###...
    .#...#..
    .#...#..
    .######.
    .#...#..
    .#...#..
    .#...#..
    ........

=== Step 9: Performance Benchmark ===
  Performance comparison (10000 iterations × 32 bytes):
    SRAM: 2048512 cycles (checksum: 5120000)
    XIP:  8945632 cycles (checksum: 5120000)
    Checksums match: OK
    XIP slowdown factor: 4.37x
    SRAM bandwidth: 300 MB/s
    XIP bandwidth:  68 MB/s
```

## 技术细节

### 测试结果

#### ✅ 基础演示 (XIP_VERSION=basic) - 完全成功
- **XIP数据访问**: 4/4 数据元素正确读取
- **XIP代码执行**: 4/4 函数调用测试通过
- **性能测试**: SRAM与XIP结果一致

#### ✅ 高级演示 (XIP_VERSION=advanced) - 完全成功  
- **资源管理**: 字体数据XIP访问正常
- **调试功能**: 寄存器状态输出完整
- **错误检查**: 全面的验证和错误处理

### QSPI配置

**关键修复**: 使用Fast Read Quad Output (0x6B, 1-1-4模式)而非Quad I/O (0xEB, 1-4-4模式)
- 指令: 单线传输
- 地址: 单线传输  
- 数据: 四线传输
- Dummy周期: 8个
- 与SPL配置保持一致，确保稳定性

### 地址转换
- Flash物理地址通过AHB总线映射到0x08000000开始的地址空间
- CPU可以直接访问这个地址空间中的代码和数据

### 性能优化
- 启用Quad模式提高传输速度
- 使用适当的dummy周期平衡速度和可靠性
- 考虑Flash的访问模式和缓存策略

### 计时机制
- **SysTick定时器**: 提供微秒级精度的性能测量
- **GDB兼容**: 在调试环境下DWT计时器可能不可靠，SysTick确保计时准确性
- **时间基准**: 1ms中断周期，支持高精度计算

## 注意事项

1. **Flash准备**: XIP模式需要Flash中预先存储有效的代码和数据
2. **性能**: XIP执行比SRAM慢2-5倍，但节省内存空间
3. **缓存**: 如果芯片支持指令缓存，XIP性能会显著提升
4. **调试**: XIP代码的调试需要特殊的调试器配置
5. **对齐**: 代码和数据的Flash地址应按适当边界对齐

## 扩展应用

基于此demo，可以实现：
- 大型应用程序的XIP执行
- 固件的分级加载（关键代码在SRAM，其他在XIP）
- 资源文件的XIP访问（字体、图像、音频数据）
- 动态代码加载和执行
- 多区域内存管理
- 压缩数据的XIP访问

## 文件结构

```
QSPI_XIP_Demo/
├── README.md              # 本文档
├── GCC/
│   └── Makefile           # 构建脚本（支持多版本）
└── Src/
    ├── main.c             # 基础版本XIP demo
    ├── main_advanced.c    # 高级版本XIP demo
    └── xip_test_code.c    # XIP测试代码示例（参考用）
```
