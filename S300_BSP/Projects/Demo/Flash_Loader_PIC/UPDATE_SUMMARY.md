# Flash Loader PIC - 更新总结

## 更新内容

### 1. 完善 flash_ops.S - 真实的 Flash 操作实现

#### ✅ 基于 Cadence QSPI 控制器
- 添加 QSPI 寄存器定义 (CMDCTRL, CMDADDRESS, etc.)
- 添加 W25Qxx Flash 命令 (READ, WREN, PP, SE, BE)
- 实现 STIG (Software Triggered Instruction Generator) 模式

#### ✅ flash_read_pic - Flash 读取
```assembly
- 使用 0x03 READ 命令
- 支持任意地址和长度
- 每次读取最多 8 字节 (STIG 限制)
- PC 相对寻址,真正的 PIC
```

#### ✅ flash_write_pic - Flash 页编程
```assembly
- 发送 WREN (Write Enable)
- 执行 Page Program (0x02)
- 处理 256 字节页边界
- 等待 WIP 位清除
- 每次写入最多 8 字节
```

#### ✅ flash_erase_pic - Flash 擦除
```assembly
- 支持三种擦除:
  * 0x20: 4KB 扇区擦除
  * 0x52: 32KB 块擦除
  * 0xD8: 64KB 块擦除
- 完整序列: WREN → ERASE → Wait WIP
```

#### ✅ crc32_calc_pic - CRC32 计算
```assembly
- 使用标准多项式 0x04C11DB7
- 8 位展开循环优化
- 与 OpenOCD 算法一致
```

#### ✅ 优化的宏定义
```assembly
.macro qspi_wait_idle      # 等待控制器空闲
.macro qspi_exec_cmd       # 执行 STIG 命令
.macro send_wren           # 发送写使能
.macro wait_wip            # 等待写完成
```

#### ✅ 错误处理
- 超时检测 (100,000 循环)
- 错误码: 1=IDLE超时, 2=CMD超时, 3=WIP超时
- 统一的错误退出点

### 2. 创建 main.c - SRAM1 主控程序

#### ✅ 内存定义
```c
#define SRAM0_BASE  0x10000000UL  // 8KB  - 动态加载区
#define SRAM1_BASE  0x20000000UL  // 384KB - 主程序区
#define QSPI_BASE   QSPI_CFG_BASE // 0x4000D000 - QSPI 控制器 (APB寄存器基址)
```

#### ✅ Flash Loader Context
```c
typedef struct {
    uint32_t load_addr;      // SRAM0 加载地址
    uint32_t code_size;      // 代码大小
    flash_read_fn_t read_fn; // 函数指针 (Thumb bit=1)
    flash_write_fn_t write_fn;
    flash_erase_fn_t erase_fn;
    crc32_fn_t crc32_fn;
} flash_loader_ctx_t;
```

#### ✅ 初始化流程
```c
int flash_loader_init(void) {
    1. 检查代码大小
    2. 复制 flash_ops.bin 到 SRAM0
    3. 内存屏障 (__DSB, __ISB)
    4. 解析函数指针 (设置 Thumb bit)
    5. 验证加载成功
}
```

#### ✅ 测试功能
```c
void flash_loader_test(void) {
    - 读取 Flash 前 256 字节
    - 显示十六进制数据
    - 计算 CRC32
    - 验证功能正确性
}
```

### 3. 文档更新

#### ✅ 修正内存地址
- **之前**: SRAM0 = 0x1FFF0000 ❌
- **现在**: SRAM0 = 0x10000000 ✅ (根据 s300_memmap.h)
- **之前**: SRAM1 = 0x20000000 ✅ (正确)

#### ✅ 更新文件
- README.md - 内存布局
- QUICKSTART.md - 内存地址
- 所有文档的 SRAM0 地址

### 4. 构建系统更新

#### ✅ Makefile 改进
```makefile
# 生成 flash_ops.inc 包含:
- C 头文件格式
- const uint8_t flash_ops_code[]
- const uint32_t flash_ops_code_size
```

#### ✅ CMakeLists.txt 更新
```cmake
# 正确的源文件路径:
- main.c (根目录)
- flash_ops_stub.c (构建时替换)
# 正确的驱动路径:
- qspi_cadence.c (Source/)
- rcc.c (Source/)
```

#### ✅ 新增文件
- flash_ops_stub.c - 临时存根
- main.c - 完整主程序

## 技术亮点

### 1. 完全兼容 OpenOCD 架构
- 寄存器操作方式
- 宏定义风格  
- 错误处理机制
- CRC32 算法

### 2. 真实硬件实现
- Cadence QSPI 控制器
- W25Qxx Flash 命令序列
- STIG 模式操作
- 超时保护

### 3. Position Independent Code
- 所有地址使用 `adr` PC 相对
- 函数调用使用 `bl` 相对分支
- 数据访问通过运行时计算
- 可加载到任意 SRAM 地址

### 4. 完整的错误处理
- 每个操作都有超时检测
- 明确的错误码定义
- 统一的错误退出流程

## 使用示例

### 构建
```bash
cd S300_BSP/Projects/Demo/Flash_Loader_PIC

# 生成 PIC 代码
make flash_ops.inc

# 查看生成文件
ls -lh flash_ops.*
# flash_ops.bin  - 原始二进制
# flash_ops.inc  - C 数组格式
# flash_ops.lst  - 汇编清单
# flash_ops.dis  - 反汇编
```

### 运行
```bash
# 使用 OpenOCD 下载到 SRAM
openocd -f board/xxx.cfg \
  -c "load_image flash_loader_pic.bin 0x20000000" \
  -c "resume 0x20000000"
```

## 下一步

### ⚠️ 重要修复 (2025-10-31)

#### 问题: Flash 读取失败 (错误码 2 - STIG 命令超时)

**根本原因**:
- main.c 中的 `flash_loader_test()` 函数在调用 PIC 代码前禁用了 QSPI 控制器
- PIC 代码需要 QSPI 控制器处于 **启用状态** 才能执行 STIG 命令
- 禁用控制器导致 `qspi_exec_cmd` 宏中的超时

**修复方案**:
```c
// ❌ 错误做法 (已移除):
printf("Disabling QSPI for PIC direct access...\n");
uint32_t saved_config = *qspi_config;
*qspi_config = saved_config & ~(1u << 0);  // 禁用控制器

// ✅ 正确做法:
/* CRITICAL FIX: DO NOT disable QSPI controller!
 * The PIC code REQUIRES the controller to be ENABLED to execute STIG commands.
 * We only need to ensure:
 * 1. QSPI is NOT in XIP mode (already done by qspi_exit_xip_mode)
 * 2. QSPI is idle (already verified)
 * 3. QSPI is in Direct/STIG mode (default after XIP exit)
 */
```

**技术细节**:
1. QSPI 控制器状态:
   - **启用/禁用** - CONFIG 寄存器 bit 0
   - **XIP/Direct 模式** - CONFIG 寄存器 XIP_NEXT/XIP_IMM 位
   - **空闲/忙碌** - CONFIG 寄存器 bit 31 (IDLE)

2. PIC 代码要求:
   - 控制器必须 **启用** (ENABLE=1)
   - 控制器必须处于 **Direct/STIG 模式** (非 XIP)
   - 控制器必须 **空闲** (IDLE=1)

3. 正确流程:
   ```
   qspi_cadence_init() → 启用控制器
   qspi_exit_xip_mode() → 退出 XIP，进入 Direct/STIG 模式
   等待 IDLE → 确保控制器就绪
   [保持启用] → 不要禁用控制器
   调用 PIC 代码 → 成功执行 STIG 命令
   ```

**修改的文件**:
- `main.c` (flash_loader_test 函数)
  - 移除 QSPI 禁用代码
  - 移除 QSPI 重新启用代码
  - 添加详细注释说明原因

**预期结果**:
- ✅ flash_read_pic 返回 0 (成功)
- ✅ 正确读取 Flash 数据
- ✅ CRC32 计算正常
- ✅ 无超时错误

### 可选优化
1. **性能优化**
   - 使用 Indirect 模式替代 STIG (更快)
   - DMA 传输 (大数据量)
   - 并行操作 (多扇区擦除)

2. **功能扩展**
   - Quad SPI 模式 (1-4-4)
   - 4 字节地址模式
   - 状态寄存器写入
   - Unique ID 读取

3. **工具集成**
   - GDB 脚本自动化
   - OpenOCD flash driver
   - Bootloader 集成

## 参考资料

- OpenOCD STMQSPI: https://github.com/openocd-org/openocd/tree/master/contrib/loaders/flash/stmqspi
- S300 Memory Map: `S300_BSP/CMSIS/Device/PiMCHIP/S300/Include/s300_memmap.h`
- Cadence QSPI: `S300_BSP/Drivers/SoC/QSPI/Include/qspi_cadence.h`
- W25Qxx Datasheet: Winbond W25Q series

## 总结

✅ **完成了完整的 Flash Loader PIC 实现**
- 真实的 QSPI Flash 操作 (不再是 TODO)
- 符合 OpenOCD STMQSPI 架构
- 基于 S300 Cadence QSPI 控制器
- 完整的错误处理和超时机制
- Position Independent Code
- 双 SRAM 架构
- 可直接在硬件上运行测试

🎯 **项目可立即用于**:
- Flash 编程工具开发
- Bootloader 实现
- 固件更新系统
- OpenOCD flash driver 开发
