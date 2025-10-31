# Flash Loader PIC Demo - 快速开始指南

## 项目概述

这是一个基于 OpenOCD STMQSPI flash loader 架构实现的演示项目,展示了如何:

1. **编写位置无关代码 (PIC)** - 可以加载到任意地址执行
2. **双 SRAM 架构** - SRAM1 运行主程序,SRAM0 执行 flash 操作
3. **动态代码加载** - 运行时复制代码到 SRAM0 并执行

## 文件结构

```
Flash_Loader_PIC/
├── README.md              # 架构说明
├── PIC_GUIDE.md          # PIC 详细教程
├── Makefile              # 构建系统 (OpenOCD 风格)
├── build.sh              # 自动化构建脚本
├── bin2char.sh           # 二进制转 C 数组工具
├── flash_ops.S           # PIC 汇编实现
├── flash_ops.ld          # 链接脚本
├── example_usage.c       # 使用示例
├── Inc/
│   └── flash_ops.h       # 函数接口定义
├── Src/
│   └── main.c            # 主程序示例
└── CMakeLists.txt        # CMake 配置
```

## 构建步骤

### 方法 1: 使用 Makefile (推荐)

```bash
cd S300_BSP/Projects/Demo/Flash_Loader_PIC

# 1. 构建 PIC 汇编代码
make flash_ops.inc

# 查看生成的文件
ls -lh flash_ops.*

# 2. 查看反汇编
cat flash_ops.lst | less

# 3. 清理
make clean
```

### 方法 2: 使用自动脚本

```bash
chmod +x build.sh
./build.sh
```

### 方法 3: 使用 CMake (完整项目)

```bash
mkdir build && cd build
cmake .. -G "Ninja"
ninja
```

## 关键概念

### 1. 位置无关代码 (PIC)

```c
/* 错误: 使用绝对地址 */
uint32_t *ptr = (uint32_t *)0x20001000;

/* 正确: 使用相对寻址 */
adr r0, data_label  // 运行时计算实际地址
```

### 2. 内存布局

```
SRAM1 (0x20000000, 384KB):  主程序
  ├── main()
  ├── 控制逻辑
  └── 数据缓冲区

SRAM0 (0x10000000, 8KB):  动态加载区
  ├── flash_read_pic()
  ├── flash_write_pic()
  ├── flash_erase_pic()
  └── crc32_calc_pic()
```

### 3. 使用流程

```c
// 1. 包含生成的头文件
#include "flash_ops.inc"

// 2. 复制到 SRAM0
memcpy((void*)SRAM0_ADDR, flash_ops_code, sizeof(flash_ops_code));

// 3. 内存屏障
__DSB();
__ISB();

// 4. 获取函数指针 (注意 Thumb 位)
flash_read_fn_t fn = (flash_read_fn_t)(SRAM0_ADDR | 1);

// 5. 调用函数
int result = fn(flash_addr, buffer, len, qspi_base);
```

## 生成的文件说明

### flash_ops.bin
- 原始二进制文件
- 可以直接复制到任意 SRAM 地址
- 大小通常 < 1KB

### flash_ops.inc
- C 头文件格式
- 包含字节数组 `flash_ops_code[]`
- 可以直接 `#include` 到 C 代码中

### flash_ops.lst
- 汇编列表文件
- 包含机器码和汇编对应关系
- 用于调试和验证

### flash_ops.dis
- 反汇编文件
- 查看实际生成的指令
- 验证没有绝对地址引用

## 调试技巧

### 1. 验证 PIC 属性

```bash
# 检查是否有绝对地址引用
arm-none-eabi-objdump -d flash_ops.elf | grep "ldr.*=0x"

# 应该只看到相对指令:
#   adr r0, ...
#   bl  ...
#   ldr r0, [pc, #offset]
```

### 2. 测试不同加载地址

```c
// 加载到地址 A
test_at_address(0x1FFF0000);

// 加载到地址 B
test_at_address(0x1FFF1000);

// 结果应该相同!
```

### 3. GDB 调试

```gdb
# 加载符号表
(gdb) add-symbol-file flash_ops.elf 0x1FFF1000

# 设置断点
(gdb) b flash_read_pic

# 查看寄存器
(gdb) info registers

# 单步执行
(gdb) si
```

## 常见问题

### Q: 为什么需要 `| 1` (Thumb 位)?

A: ARM Cortex-M 只支持 Thumb 模式,函数地址最低位必须为 1:

```c
fn_ptr = (func_t)(addr | 1);  // 正确
fn_ptr = (func_t)(addr);       // 错误,会导致 UsageFault
```

### Q: 为什么链接到地址 0?

A: 这样所有偏移都是相对的,可以加载到任意地址:

```
函数在偏移 0x40 -> 加载到 0x1FFF1000 时实际地址 0x1FFF1040
```

### Q: 如何处理数据段?

A: 将数据放在 `.rodata` 段,使用 `adr` 指令访问:

```asm
adr r0, my_data
ldr r1, [r0]

my_data:
    .word 0x12345678
```

### Q: 能用 C 语言写 PIC 代码吗?

A: 可以,但需要谨慎:

```c
// 使用 -fPIC 编译
int __attribute__((section(".text"))) 
pic_function(int param) {
    // 避免全局变量
    // 避免静态变量
    // 只使用栈和参数
    return param + 42;
}
```

## 性能对比

与普通代码相比,PIC 代码:

- **代码大小**: +5-10% (额外的间接寻址)
- **执行速度**: 几乎相同 (现代 CPU 有分支预测)
- **加载时间**: 更快 (无需重定位)

## 扩展应用

### 1. OpenOCD Flash Driver

OpenOCD 可以使用这个 loader:

```tcl
# OpenOCD 配置
flash bank qspi s300qspi 0x0 0 0 0 $_TARGETNAME
flash write_image erase firmware.bin 0x0
```

### 2. Bootloader

在 bootloader 中动态加载和执行:

```c
// 从 flash 读取应用代码
memcpy(SRAM0, flash_app_code, size);

// 跳转执行
void (*app)(void) = (void (*)(void))(SRAM0 | 1);
app();
```

### 3. 固件更新

安全的固件更新流程:

```c
// 1. 校验新固件
uint32_t crc = crc32_calc_pic(new_fw, size);

// 2. 擦除 flash
flash_erase_pic(addr, ERASE_64K, qspi_base);

// 3. 写入新固件
flash_write_pic(addr, new_fw, size, qspi_base);

// 4. 验证
flash_read_pic(addr, verify_buf, size, qspi_base);
assert(memcmp(new_fw, verify_buf, size) == 0);
```

## 参考资料

- [OpenOCD STMQSPI Driver](https://github.com/openocd-org/openocd/tree/master/contrib/loaders/flash/stmqspi)
- ARM Thumb-2 指令集手册
- Position Independent Executables (PIE) 规范
- S300 技术参考手册

## 许可证

SPDX-License-Identifier: GPL-2.0-or-later

基于 OpenOCD 项目的开源实现。
