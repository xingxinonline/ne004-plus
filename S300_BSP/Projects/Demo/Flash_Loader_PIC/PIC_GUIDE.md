# Position Independent Code (PIC) 示例说明

## 什么是位置无关代码 (PIC)?

位置无关代码是一种编写代码的方式,使得代码可以被加载到内存的任何位置执行,而不需要修改指令。

### 关键技术

1. **相对寻址**
```asm
# 错误的方式 (绝对地址)
ldr r0, =0x20001000    # 硬编码地址

# 正确的方式 (相对地址)
adr r0, data_label     # 使用当前 PC 计算偏移
```

2. **PC-相对跳转**
```asm
# 使用相对分支
b   target_label       # 相对于当前位置的偏移
bl  function_label     # 同样使用相对偏移
```

3. **位置无关数据访问**
```asm
# 使用 adr 获取数据地址
adr r0, const_table    # r0 = 运行时地址
ldr r1, [r0]           # 加载数据
```

## OpenOCD STMQSPI 方法

### 编译选项
```makefile
# 关键编译选项
-fPIC                  # 位置无关代码
-fno-common            # 避免共同块
-defsym=_start=0       # 链接到地址 0
```

### 为什么链接到地址 0?

链接到地址 0 使得所有偏移量都是相对的:
```
假设代码实际加载到 0x1FFF1000:
  函数 A 在偏移 0x00  -> 实际地址 0x1FFF1000
  函数 B 在偏移 0x40  -> 实际地址 0x1FFF1040
  数据   在偏移 0x100 -> 实际地址 0x1FFF1100
```

### 汇编代码模式

```asm
.text
.thumb_func

my_function:
    push  {r4, lr}
    
    # 获取数据的运行时地址
    adr   r4, my_data
    ldr   r0, [r4]
    
    # 调用其他 PIC 函数
    bl    other_function
    
    pop   {r4, pc}
    
.align 2
my_data:
    .word 0x12345678
```

## 在 S300 上的应用

### 架构图
```
Host/OpenOCD                 S300 MCU
    |                           |
    | 1. 下载 PIC code      +--------+
    |--------------------->|  SRAM0  | <- 加载 flash_ops.bin
    |                      +--------+
    |                           |
    | 2. 设置参数           +--------+
    |--------------------->|  SRAM1  | <- 主程序 + 参数
    |                      +--------+
    |                           |
    | 3. 启动执行               |
    |--------------------->  跳转到 SRAM0
    |                           |
    |                      执行 flash 操作
    |                           |
    | 4. 读取结果              |
    |<---------------------|
```

### 内存布局示例

```
SRAM0 (0x1FFF0000 - 0x1FFF2FFF, 12KB)
  +0x0000: flash_read_pic()
  +0x0040: flash_write_pic()
  +0x00C0: flash_erase_pic()
  +0x0100: crc32_calc_pic()
  +0x0150: function_table
  +0x0200: 临时数据

SRAM1 (0x20000000 - 0x2000FFFF, 64KB)
  +0x0000: main() 和控制代码
  +0x1000: 参数传递区
  +0x2000: 数据缓冲区
  +0x8000: 堆栈
```

## 测试 PIC 代码

### 验证方法

1. **加载到不同地址测试**
```c
// 测试 1: 加载到 SRAM0 开头
memcpy(0x1FFF0000, flash_ops_code, size);
fn_ptr = (func_t)(0x1FFF0000 | 1);
result1 = fn_ptr(params);

// 测试 2: 加载到 SRAM0 中间
memcpy(0x1FFF1000, flash_ops_code, size);
fn_ptr = (func_t)(0x1FFF1000 | 1);
result2 = fn_ptr(params);

// 应该得到相同的结果
assert(result1 == result2);
```

2. **反汇编检查**
```bash
# 检查没有绝对地址引用
arm-none-eabi-objdump -d flash_ops.elf | grep -E "(ldr.*=0x|mov.*#0x[2-9])"

# 应该只看到相对指令
#   adr r0, ...
#   bl  ...
#   b   ...
```

## 常见问题

### Q1: 为什么需要设置 Thumb 位 (|1)?
```c
// ARM Cortex-M 只支持 Thumb 模式
// 跳转地址的最低位必须为 1
fn_ptr = (func_t)(addr | 1);  // 设置 Thumb 位
```

### Q2: 如何处理全局变量?
```c
// 避免使用全局变量,改用:
// 1. 传递参数
// 2. 使用栈变量
// 3. 使用相对数据段 (.rodata)
```

### Q3: 调试 PIC 代码?
```gdb
# 在 GDB 中设置断点
(gdb) add-symbol-file flash_ops.elf 0x1FFF1000
(gdb) b flash_read_pic
(gdb) c
```

## 参考资料

- OpenOCD 源码: `contrib/loaders/flash/stmqspi/`
- ARM Thumb-2 指令集参考
- Position Independent Executables (PIE) 标准
