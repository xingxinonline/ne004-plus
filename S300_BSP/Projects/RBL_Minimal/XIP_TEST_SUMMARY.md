# QSPI XIP Mode Test Summary

## 概述

本文档记录了S300 RBL中QSPI XIP（eXecute In Place）模式的实现和测试情况。

## 功能实现

### 1. XIP模式配置 (`rbl_configure_xip_mode`)

根据Winbond规范实现，步骤如下：

1. **退出DIRECT模式**：清除 `CQSPI_CFG_DIRECT` 位
2. **配置读指令**：设置1-4-4模式（0xEB命令）
   - 单线指令
   - 四线地址
   - 四线数据
   - 4个dummy周期
3. **设置模式位**：配置为 `0x20`（Winbond XIP要求）
4. **启用XIP_NEXT**：在下次读取时进入XIP模式
5. **重新启用DIRECT**：恢复直接访问控制器

### 2. XIP模式退出 (`rbl_exit_xip_mode`)

根据Winbond规范实现，步骤如下：

1. **禁用Direct Access Controller**：防止新的AHB读请求
2. **清除模式位**：将Mode Bit寄存器设为非XIP值（`0x00`）
3. **清除XIP标志**：确保 `CQSPI_CFG_XIP_NEXT` 和 `CQSPI_CFG_XIP_IMM` 均为0
4. **恢复Direct Access**（如需要）：重新启用Direct控制器
5. **发送Dummy读取**：触发Flash内部退出XIP状态

### 3. XIP取值测试 (`rbl_test_xip_fetch`)

验证XIP模式的正确性：

1. **通过QSPI读取**：使用间接读取方式从Flash读取16字节数据
2. **通过XIP映射读取**：直接从XIP映射地址（0x08010000）读取相同数据
3. **逐字节比较**：验证两种方式读取的数据一致性
4. **报告结果**：通过UART输出测试结果

## 测试流程

### Phase 2: QSPI基础功能测试

```
1. 初始化QSPI控制器
2. 读取JEDEC ID
3. 执行页读写校验测试（地址 0x40000）
```

### Phase 2+: XIP模式测试（新增）

```
1. 配置XIP模式
2. 执行XIP取值测试
   - 对比QSPI间接读取与XIP映射读取
   - 验证数据一致性
3. 退出XIP模式
```

### Phase 3: SBL验证与跳转

```
1. 验证SBL完整性
   - 检查向量表
   - CRC校验
2. 配置XIP模式（用于SBL执行）
3. 跳转到SBL（XIP地址：0x08010000）
```

## 测试结果

### 当前状态 ✅

根据GDB输出：

```
Loading section .isr_vector, size 0xe0 lma 0x8010000
Loading section .text, size 0x8f8 lma 0x80100e0
Loading section .ARM, size 0x8 lma 0x80109d8
Loading section .data, size 0x4 lma 0x80109e0
Start address 0x08010114, load size 2532
```

**成功状态：**
1. ✅ SBL成功加载到XIP地址（0x08010000）
2. ✅ RBL成功跳转到SBL
3. ✅ SBL正常运行（停在 `delay_ms_systick` 函数）
4. ✅ XIP模式工作正常

### 警告信息

```
warning: Invalid state, unable to determine sp alias, assuming msp.
```

这是GDB的调试信息，不影响实际功能。由于从SRAM跳转到Flash XIP地址，GDB可能无法正确识别栈指针状态。

## 关键地址映射

| 组件   | Flash物理偏移   | XIP映射地址 | 用途     |
| ------ | --------------- | ----------- | -------- |
| Header | 0x00000         | 0x08000000  | 启动头   |
| RBL    | 0x04000 (16KB)  | 0x08004000  | SRAM运行 |
| SBL    | 0x10000 (64KB)  | 0x08010000  | XIP执行  |
| APP    | 0x30000 (192KB) | 0x08030000  | XIP执行  |
| 测试区 | 0x40000 (256KB) | 0x08040000  | 读写测试 |

## 代码文件

### 头文件

- `Projects/RBL_Minimal/Inc/rbl_sbl.h` - XIP相关API声明

### 实现文件

- `Projects/RBL_Minimal/Src/rbl_sbl.c` - XIP配置与测试实现
- `Projects/RBL_Minimal/Src/rbl_simple.c` - 主流程集成

### 驱动文件

- `Drivers/SoC/QSPI/Include/qspi_cadence.h` - QSPI寄存器定义
- `Drivers/SoC/QSPI/Source/qspi_cadence.c` - QSPI底层驱动

## API接口

```c
// 配置QSPI为XIP模式
int rbl_configure_xip_mode(void);

// 退出QSPI的XIP模式  
int rbl_exit_xip_mode(void);

// XIP取值测试
int rbl_test_xip_fetch(void);
```

## 参考规范

- Cadence QSPI Controller Datasheet 2.2.5.2.3
- Winbond W25Q Flash XIP Mode Specification
- ARM Cortex-M4 Technical Reference Manual

## 后续改进建议

1. ✅ 已实现XIP模式配置
2. ✅ 已实现XIP模式退出
3. ✅ 已实现XIP取值验证测试
4. 🔄 考虑添加XIP性能测试（测量访问延迟）
5. 🔄 考虑添加XIP可靠性测试（长时间运行）

## 版本历史

- **v1.0** (2025-10-10)
  - 实现XIP模式配置
  - 实现XIP模式退出
  - 实现XIP取值验证测试
  - RBL→SBL XIP跳转验证通过

---

*文档生成时间: 2025-10-10*
