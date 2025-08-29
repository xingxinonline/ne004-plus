# S300 最终Flash布局设计

## 分区分配

基于简洁性和实用性原则，最终确定如下分配：

- **Header + RBL**: 64KB (0x0 - 0x10000)
- **SBL**: 128KB (0x10000 - 0x30000) 
- **分区表**: 4KB (0x30000 - 0x31000)
- **OTA数据**: 8KB (0x31000 - 0x33000)
- **APP分区**: 0x33000 开始

## Flash布局

```
+-------------------+ 0x08000000 (Flash基址)
|   Header (256B)   | 0x08000000-0x08000100
+-------------------+ 0x08000100
|                   |
|   RBL (~64KB)     | 0x08000100-0x08010000
|                   | ESP32 ROM BL功能
+-------------------+ 0x08010000 (64KB边界)
|                   |
|                   |
|   SBL (128KB)     | 0x08010000-0x08030000
|                   | ESP32兼容SBL
|                   |
+-------------------+ 0x08030000 (192KB边界)
|  Partition Table  | 0x08030000-0x08031000 (4KB)
+-------------------+ 0x08031000
|    OTA Data       | 0x08031000-0x08033000 (8KB)
+-------------------+ 0x08033000 (204KB)
|                   |
|    APP (ota_0)    | ESP32 APP分区 (可配置大小)
|                   |
+-------------------+
|    APP (ota_1)    | ESP32 OTA备份分区
+-------------------+
|      NVS          | ESP32 NVS存储分区 
+-------------------+
|      ...          | 其他ESP32分区
+-------------------+
```

## SBL视角

SBL看到的Flash布局 (基址: 0x08010000):

```
+-------------------+ 0x00000000 (SBL视角起始)
|                   |
|   SBL Self        | SBL程序本身 (128KB空间)
|                   |
+-------------------+ 0x00020000 (+128KB)
|  Partition Table  | 分区表位置
+-------------------+ 0x00021000 (+132KB)
|    OTA Data       | OTA数据位置
+-------------------+ 0x00023000 (+140KB)
|    APP (ota_0)    | APP分区开始
+-------------------+
|    APP (ota_1)    | OTA备份分区
+-------------------+
|      NVS          | NVS存储分区
+-------------------+
```

## 关键特性

### 1. 空间分配合理
- **RBL**: 64KB足够实现ESP32 ROM BL功能
- **SBL**: 128KB预留充足的ESP32 SBL功能空间
- **分区表**: 4KB标准ESP32大小
- **OTA数据**: 8KB标准ESP32大小

### 2. 边界对齐
- 64KB: Header+RBL边界，便于管理
- 192KB: SBL结束边界，分区表开始
- 4KB对齐: 分区表和OTA数据对齐

### 3. ESP32兼容
- SBL完全不知道Header和RBL存在
- 分区表和OTA相对SBL基址的标准偏移
- 标准ESP32分区格式和API

## 工具支持

### 镜像生成
```bash
python tools/s300_image_gen.py \
    --rbl build/RBL.bin \
    --sbl build/SBL.bin \
    --app build/HelloWorld.bin \
    -o firmware.bin
```

### 大小限制检查
- RBL: 最大 ~64KB (65280字节)
- SBL: 最大 128KB (131072字节)
- 超出限制时构建失败

### 输出示例
```
Header: 256 字节
RBL: build/RBL.bin, 大小: 32768 字节, CRC32: 0x12345678
填充到64KB: 32512 字节
SBL: build/SBL.bin, 大小: 65536 字节, 偏移: 0x10000
SBL填充: 65536 字节
镜像生成完成: firmware.bin, 总大小: 196608 字节
```

## 开发流程

### 初次烧录
```bash
# 1. 编译所有组件
make -C Projects/RBL
make -C Projects/SBL  
make -C Projects/Demo/HelloWorld

# 2. 生成完整镜像
python tools/s300_image_gen.py \
    --rbl build/RBL.bin \
    --sbl build/SBL.bin \
    --app build/HelloWorld.bin \
    -o firmware.bin

# 3. 烧录到S300
s300_download_tool --image firmware.bin
```

### 日常开发 (ESP32兼容)
```bash
# 只编译和更新APP
make -C Projects/Demo/HelloWorld
rbl_download_tool --app build/HelloWorld.bin

# 或使用ESP32 OTA
esp32_ota_update --app build/HelloWorld.bin
```

## 总结

这个最终的Flash布局设计实现了：

1. **简洁**: 清晰的64KB+128KB分区边界
2. **实用**: 合理的空间分配，满足实际需求  
3. **兼容**: 完全ESP32兼容的SBL/APP开发体验
4. **可控**: 明确的大小限制和构建检查

开发者可以享受标准ESP32开发流程，同时获得S300硬件的性能优势。
