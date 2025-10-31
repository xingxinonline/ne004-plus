# Flash Loader with Position Independent Code (PIC)

参考 OpenOCD STMQSPI flash loader 实现的演示项目。

## 架构设计

```
+-------------------+
|   SRAM1 (主程序)   |  <- 运行控制逻辑
+-------------------+
        |
        | 动态加载
        v
+-------------------+
|   SRAM0 (执行区)   |  <- 加载并执行 flash 操作函数
+-------------------+
        |
        | 访问 QSPI
        v
+-------------------+
|   W25Qxx Flash    |
+-------------------+
```

## 核心概念

1. **位置无关代码 (PIC)**
   - 使用相对寻址而非绝对地址
   - 可以加载到任意 SRAM 地址执行
   - 通过 `adr` 指令获取运行时地址

2. **双 SRAM 架构**
   - SRAM1: 主控制程序常驻
   - SRAM0: 动态加载 flash 操作函数
   - 隔离执行环境,提高安全性

3. **内存布局**
   ```
   SRAM1 (0x2000_0000, 384KB):
     - 主程序代码
     - 数据缓冲区
     - 控制参数
   
   SRAM0 (0x1000_0000, 8KB):
     - Flash 读函数 (PIC)
     - Flash 写函数 (PIC)
     - Flash 擦除函数 (PIC)
     - CRC32 计算函数 (PIC)
   ```

## 文件结构

- `flash_ops.S` - 位置无关的 flash 操作汇编函数
- `loader.c` - 主控制程序 (SRAM1)
- `Makefile` - 构建系统 (参考 stmqspi)

## 构建

```bash
make
```

生成文件:
- `flash_ops.bin` - 原始二进制
- `flash_ops.inc` - C 头文件格式的字节数组
- `loader.elf` - 完整程序

## 参考

- OpenOCD: contrib/loaders/flash/stmqspi/
- Makefile 生成位置无关代码
- 字节数组转换: bin2char.sh
