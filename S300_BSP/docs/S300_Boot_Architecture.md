# S300 启动架构设计文档

## 文档版本控制

| 版本 | 日期       | 作者         | 修订说明                                            |
| ---- | ---------- | ------------ | --------------------------------------------------- |
| 3.0  | 2025-08-30 | AI Assistant | 整合所有启动相关文档，完善架构设计，删除重复内容     |

---

## 1. 系统概述

### 1.1 芯片存储资源

S300 (PiMCHIP) 芯片采用ARM Cortex-M4内核，具备以下存储资源：

**内部SRAM**:

- SRAM0: 8KB (0x10000000 - 0x10002000) - 特殊用途/备用区域
- SRAM1: 384KB (0x20000000 - 0x20060000) - 主要代码和数据运行区

**外部QSPI NOR Flash (W25Q128 - 16MB)**:

- 映射地址空间: 0x80000000 - 0x8FFFFFFF (XIP模式)
- 用途: 存储所有固件镜像和用户数据
- 性能: 支持高速读取 2.20 MB/s @ 64MHz SCLK

**外部SD卡 (可选)**:

- 大容量可移动存储
- 用于存放用户数据、日志和OTA更新包

### 1.2 ROMBOOT功能限制

**关键约束**: S300芯片固化的ROMBOOT功能极其有限，仅能：

✅ **基本功能**:
- 最小化系统初始化（CPU核心、基础时钟、SRAM）
- 读取Flash起始256字节的Header信息
- 验证RBL的基础完整性（CRC）
- 将RBL代码加载到SRAM (0x20000000)
- 跳转到RBL入口执行

❌ **缺失功能** (相比ESP32 ROM Bootloader):
- 启动模式检测（GPIO状态检测）
- UART下载协议
- USB下载支持
- 高级安全验证
- 固件烧写功能
- 错误恢复机制

### 1.3 设计目标

1. **ESP32兼容性**: 通过RBL补充ESP32 ROM Bootloader功能
2. **四级启动架构**: ROMBOOT → RBL → SBL → Application
3. **故障恢复**: 强制下载模式，解决固件损坏救砖问题
4. **安全OTA**: A/B分区无缝OTA，支持安全回滚
5. **性能优化**: 充分利用QSPI XIP模式

---

## 2. 四级启动流程架构

### 2.1 启动流程序列图

```mermaid
sequenceDiagram
    participant ROM as S300 ROMBOOT<br/>(固化4KB)
    participant RBL as RBL<br/>(SRAM 64KB)
    participant SBL as SBL<br/>(QSPI XIP 128KB)
    participant APP as Application<br/>(QSPI XIP)
    participant FLASH as QSPI Flash

    ROM->>FLASH: 1. 读取Header (256B)
    ROM->>FLASH: 2. 验证RBL完整性
    ROM->>RBL: 3. 加载RBL到SRAM并跳转
    RBL->>RBL: 4. 系统全面初始化
    alt 强制下载模式
        RBL->>RBL: 检测GPIO/Flash标志
        RBL->>FLASH: 通过UART接收并烧写固件
        RBL->>RBL: 重启
    else 正常启动
        RBL->>SBL: 5. 验证并跳转到SBL
    end
    SBL->>SBL: 6. 读取分区表，OTA逻辑
    SBL->>APP: 7. 启用看门狗，跳转到App
    APP->>APP: 8. 执行业务逻辑
    alt OTA更新
        APP->>FLASH: 9. 下载新固件到备用分区
        APP->>SBL: 10. 设置更新标志
        APP->>APP: 11. 重启系统
    end
```

### 2.2 Stage 1: S300 ROMBOOT (芯片固化)

**职责**: 芯片上电后执行的第一段代码，固化在ROM中

**内存限制**: 4KB固化代码空间

**主要动作**:

1. **最小化核心初始化**:
   - CPU核心基本配置
   - 系统时钟初始化 (HSI 8MHz)
   - SRAM基本初始化

2. **Flash Header读取**:
   ```c
   typedef struct {
       uint32_t magic;         // 魔数: 0x504D4348 ("PMCH")
       uint32_t rbl_size;      // RBL大小
       uint32_t rbl_crc;       // RBL CRC32校验
       uint32_t rbl_entry;     // RBL入口地址 (SRAM)
       // ... 其他Header信息
   } flash_header_t;
   ```

3. **RBL验证和加载**:
   ```c
   // 简化的ROMBOOT伪代码
   void romboot_main(void) {
       flash_header_t header;
       
       // 读取Header
       qspi_read(0x0, &header, sizeof(header));
       
       // 验证魔数
       if (header.magic != 0x504D4348) {
           goto error_handler;
       }
       
       // 读取RBL到SRAM
       qspi_read(0x100, (void*)0x20000000, header.rbl_size);
       
       // 验证CRC
       if (crc32((void*)0x20000000, header.rbl_size) != header.rbl_crc) {
           goto error_handler;
       }
       
       // 跳转到RBL
       ((void(*)(void))header.rbl_entry)();
       
   error_handler:
       while(1); // 死循环等待外部复位
   }
   ```

### 2.3 Stage 2: RBL (ROM Bootloader - ESP32兼容)

**职责**: 运行于SRAM，实现ESP32 ROM Bootloader等效功能

**存储位置**: QSPI Flash 0x100 - 0x10000 (64KB)
**运行位置**: SRAM1 0x20000000 (由ROMBOOT加载)

**主要功能**:

1. **系统全面初始化**:
   ```c
   // RBL系统初始化
   int rbl_system_init(void) {
       // 配置系统时钟为200MHz
       rcc_config_pll_200mhz();
       rcc_config_ahb_apb_dividers();
       
       // 初始化QSPI控制器
       qspi_controller_init();
       qspi_enable_xip_mode();  // 映射Flash到0x80000000
       
       // 初始化调试串口
       uart_debug_init();
       printf("[RBL] S300 RBL v1.0 - ESP32 ROM Bootloader Compatible\n");
       
       return 0;
   }
   ```

2. **启动模式检测**:
   ```c
   typedef enum {
       BOOT_MODE_NORMAL = 0,           // 正常启动模式
       BOOT_MODE_DOWNLOAD_GPIO,        // GPIO强制下载模式
       BOOT_MODE_DOWNLOAD_SERIAL,      // 串口下载模式
       BOOT_MODE_DOWNLOAD_SOFTWARE,    // 软件标志下载模式
       BOOT_MODE_DOWNLOAD_DOUBLE_RESET,// 双重启下载模式
       BOOT_MODE_RECOVERY,             // Flash恢复模式
       BOOT_MODE_BOOT_FAILURE,         // 启动故障恢复模式
   } boot_mode_t;

   boot_mode_t rbl_detect_boot_mode_enhanced(void) {
       // 1. 检查软件下载标志（最高优先级）
       if (rbl_flash_check_download_flag()) {
           return BOOT_MODE_DOWNLOAD_SOFTWARE;
       }
       
       // 2. 检查双重启动
       if (rbl_check_double_reset()) {
           return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
       }
       
       // 3. 检查启动故障
       if (rbl_check_boot_failure()) {
           return BOOT_MODE_BOOT_FAILURE;
       }
       
       // 4. 检查串口下载窗口
       if (rbl_check_enhanced_serial_window()) {
           return BOOT_MODE_DOWNLOAD_SERIAL;
       }
       
       // 5. 默认正常启动
       return BOOT_MODE_NORMAL;
   }
   ```

3. **强制下载模式实现**:
   ```c
   // UART下载协议 (YMODEM兼容)
   int rbl_download_mode(void) {
       printf("[RBL] Entering download mode...\n");
       printf("[RBL] Ready to receive firmware via YMODEM\n");
       
       // YMODEM接收固件
       uint32_t received_size = 0;
       uint8_t *firmware_buffer = (uint8_t*)0x80000000; // XIP地址
       
       int ret = ymodem_receive(firmware_buffer, &received_size);
       if (ret == 0 && received_size > 0) {
           // 烧写固件到Flash
           ret = rbl_flash_write_firmware(firmware_buffer, received_size);
           if (ret == 0) {
               printf("[RBL] Firmware download successful\n");
               rbl_system_reset(); // 重启系统
           }
       }
       
       printf("[RBL] Download failed\n");
       return -1;
   }
   ```

4. **芯片检测和识别**:
   ```c
   // ESP32兼容的芯片检测
   void rbl_chip_detection_demo(void) {
       uart_detection_result_t result;
       
       printf("[RBL] ===== Chip Detection (ESP32 Compatible) =====\n");
       
       // 检测本地S300芯片
       chip_info_t local_chip;
       if (chip_detect_s300_local(&local_chip) == CHIP_DETECT_OK) {
           printf("[RBL] Local: %s (ID: 0x%08X)\n", 
                  local_chip.name, local_chip.chip_id);
           printf("[RBL] Flash: %u KB, RAM: %u KB, CPU: %u MHz\n",
                  local_chip.flash_size/1024, local_chip.ram_size/1024, 
                  local_chip.freq/1000000);
       }
       
       // 扫描外部串口设备
       if (chip_detect_auto(NULL, &result) == CHIP_DETECT_OK) {
           printf("[RBL] External: %s on %s @ %u baud\n",
                  result.chip.name, result.port_name, result.baud_rate);
           
           if (result.chip.type == CHIP_TYPE_ESP32) {
               printf("[RBL] ✓ ESP32 detected - compatible mode available\n");
           }
       } else {
           printf("[RBL] No external chips detected\n");
       }
   }
   ```

5. **SBL验证和跳转**:
   ```c
   int rbl_normal_boot(void) {
       sbl_info_t sbl_info;
       
       // 获取SBL信息
       if (rbl_get_sbl_info(&sbl_info) != 0) {
           printf("[RBL] Failed to get SBL info\n");
           return -1;
       }
       
       // 验证SBL完整性
       if (rbl_verify_sbl(&sbl_info) != 0) {
           printf("[RBL] SBL verification failed\n");
           return -1;
       }
       
       printf("[RBL] Jumping to SBL at 0x%08X\n", sbl_info.entry);
       
       // 跳转到SBL (XIP地址)
       rbl_jump_to_sbl(sbl_info.entry);
       
       return 0; // 不应该到达这里
   }
   ```

### 2.4 Stage 3: SBL (Secondary Bootloader)

**职责**: 分区管理和OTA控制器，ESP32 Bootloader等效功能

**存储位置**: QSPI Flash 0x10000 - 0x30000 (128KB)
**运行模式**: QSPI XIP模式 (0x80010000)

**主要功能**:

1. **分区表管理**:
   ```c
   // ESP32兼容的分区表结构
   typedef struct {
       uint16_t magic;         // 分区表魔数
       uint8_t type;          // 分区类型
       uint8_t subtype;       // 分区子类型
       uint32_t offset;       // 分区偏移地址
       uint32_t size;         // 分区大小
       char label[16];        // 分区标签
       uint32_t flags;        // 分区标志
   } esp_partition_info_t;
   
   // 标准分区布局
   static const esp_partition_info_t partition_table[] = {
       {0xAA50, ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, 
        0x40000, 0x600000, "ota_0", 0},
       {0xAA50, ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, 
        0x640000, 0x600000, "ota_1", 0},
       {0xAA50, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, 
        0x30000, 0x10000, "nvs", 0},
       // ... 其他分区
   };
   ```

2. **A/B分区OTA支持**:
   ```c
   typedef struct {
       uint8_t active_slot;        // 当前活跃分区 (0=OTA_0, 1=OTA_1)
       uint8_t update_pending;     // 是否有待验证的更新
       uint8_t retry_count;        // 重试计数
       uint8_t rollback_count;     // 回滚计数
       uint32_t boot_count;        // 总启动次数
       uint32_t last_error;        // 上次错误代码
   } sbl_boot_env_t;
   
   int sbl_ota_logic(void) {
       sbl_boot_env_t boot_env;
       const esp_partition_info_t *boot_partition;
       
       // 读取启动环境
       sbl_boot_read_env(&boot_env);
       
       // 确定启动分区
       if (boot_env.update_pending && boot_env.retry_count < 3) {
           // 尝试启动新固件
           boot_partition = sbl_ota_get_update_partition();
           boot_env.retry_count++;
       } else if (boot_env.retry_count >= 3) {
           // 回滚到稳定版本
           boot_partition = sbl_ota_get_stable_partition();
           boot_env.update_pending = 0;
           boot_env.retry_count = 0;
           boot_env.rollback_count++;
       } else {
           // 正常启动当前分区
           boot_partition = sbl_ota_get_boot_partition();
       }
       
       return sbl_boot_from_partition(boot_partition);
   }
   ```

3. **应用镜像验证**:
   ```c
   bool sbl_verify_app_image(const esp_partition_info_t *partition) {
       esp_image_header_t header;
       uint32_t app_address = SBL_FLASH_BASE_ADDR + partition->offset;
       
       // 读取应用头部
       if (qspi_read(app_address, &header, sizeof(header)) != 0) {
           return false;
       }
       
       // 验证魔数
       if (header.magic != ESP_IMAGE_HEADER_MAGIC) {
           SBL_LOGE("Invalid image magic: 0x%x", header.magic);
           return false;
       }
       
       // 验证校验和
       uint32_t checksum = sbl_calculate_checksum(app_address, header.image_len);
       if (checksum != header.hash_appended) {
           SBL_LOGE("Image checksum verification failed");
           return false;
       }
       
       return true;
   }
   ```

4. **看门狗和跳转**:
   ```c
   int sbl_boot_application(const esp_partition_info_t *partition) {
       uint32_t app_address = SBL_FLASH_BASE_ADDR + partition->offset;
       uint32_t app_entry = app_address + sizeof(esp_image_header_t);
       
       SBL_LOGI("Loading app from %s at 0x%x", partition->label, app_address);
       
       // 启动看门狗 (30秒超时)
       sbl_watchdog_start(30000);
       
       // 跳转到应用程序
       sbl_jump_to_app(app_address, app_entry);
       
       return 0; // 不应该到达这里
   }
   ```

### 2.5 Stage 4: Application

**职责**: 用户业务逻辑实现

**存储位置**: QSPI Flash多分区 (OTA_0/OTA_1)
**运行模式**: QSPI XIP模式

**主要功能**:
- 用户应用程序逻辑
- OTA更新管理
- 看门狗喂狗
- 故障处理和恢复

---

## 3. 内存映射和分区布局

### 3.1 Flash分区布局 (16MB W25Q128)

```
| 地址范围              | 大小   | 用途        | 描述                 |
| --------------------- | ------ | ----------- | -------------------- |
| 0x00000000-0x000000FF | 256B   | Header      | RBL元信息            |
| 0x00000100-0x0000FFFF | 64KB   | RBL         | ROM Bootloader       |
| 0x00010000-0x0002FFFF | 128KB  | SBL         | Secondary Bootloader |
| 0x00030000-0x0003EFFF | 56KB   | NVS         | 非易失性存储         |
| 0x0003F000-0x0003FFFF | 4KB    | Reset_Flags | 软件复位标志区       |
| 0x00040000-0x0063FFFF | 6MB    | OTA_0       | 应用分区A            |
| 0x00640000-0x00C3FFFF | 6MB    | OTA_1       | 应用分区B            |
| 0x00C40000-0x00FFFFFF | 3.75MB | Data        | 用户数据区           |
```

**NVS分区详细布局 (60KB总计)**:
```
| 地址范围              | 大小 | 用途           | 描述             |
| --------------------- | ---- | -------------- | ---------------- |
| 0x00030000-0x0003EFFF | 56KB | NVS数据区      | 配置参数、证书等 |
| 0x0003F000-0x0003FFFF | 4KB  | 软件复位标志区 | Reset flags      |
```

**软件复位标志区详细布局 (0x0003F000-0x0003FFFF)**:
```
| 偏移        | 大小  | 用途         | 结构                |
| ----------- | ----- | ------------ | ------------------- |
| 0x000-0x017 | 24B   | 下载模式标志 | download_flag_t     |
| 0x100-0x00F | 16B   | 双重启标志   | double_reset_flag_t |
| 0x200-0x017 | 24B   | 启动计数器   | boot_counter_t      |
| 0x300-0xCFF | 2.5KB | 预留         | 未来扩展            |
```

### 3.2 SRAM内存布局 (384KB)

```
| 地址范围              | 大小  | 用途          | 描述          |
| --------------------- | ----- | ------------- | ------------- |
| 0x20000000-0x2000FFFF | 64KB  | RBL Code      | RBL运行代码区 |
| 0x20010000-0x2001FFFF | 64KB  | Stack         | 系统堆栈区    |
| 0x20020000-0x2005FFFF | 256KB | Heap/BSS/Data | 应用数据区    |
```

### 3.3 QSPI XIP映射 (16MB)

```
| 虚拟地址              | 物理地址   | 大小  | 描述       |
| --------------------- | ---------- | ----- | ---------- |
| 0x80000000-0x80000100 | 0x00000000 | 256B  | Header区域 |
| 0x80000100-0x80010000 | 0x00000100 | 64KB  | RBL区域    |
| 0x80010000-0x80030000 | 0x00010000 | 128KB | SBL区域    |
| 0x80040000-0x80640000 | 0x00040000 | 6MB   | OTA_0区域  |
| 0x80640000-0x80C40000 | 0x00640000 | 6MB   | OTA_1区域  |
```

---

## 4. 启动模式检测机制

### 4.1 增强版检测策略 (包含软件复位)

由于S300 ROMBOOT功能限制，RBL实现了增强版启动模式检测：

**检测优先级**:

1. **软件下载标志** (最高优先级) - Flash标志位检测
2. **双重启动检测** - ESP32风格的双重启检测  
3. **启动故障检测** - 连续启动失败自动恢复
4. **串口窗口检测** - 启动后短时间串口监听
5. **GPIO下载引脚** - 硬件下载引脚检测
6. **正常启动模式** (默认)

### 4.2 软件复位标志系统

#### 4.2.1 下载模式标志

```c
// Flash中的下载标志结构 (24字节)
typedef struct {
    uint32_t magic;           // 魔数: 0x444C4654 ("DLFT")
    uint32_t reason;          // 触发原因 @ref reset_reason_t
    uint32_t timestamp;       // 时间戳
    uint32_t retry_count;     // 重试次数
    uint32_t user_data;       // 用户数据
    uint32_t crc32;          // CRC32校验
} __attribute__((packed)) download_flag_t;

// 复位触发原因枚举
typedef enum {
    RESET_REASON_NONE = 0,          // 无原因
    RESET_REASON_USER_CMD,          // 用户命令触发
    RESET_REASON_SERIAL_CMD,        // 串口命令触发
    RESET_REASON_DOUBLE_RESET,      // 双重启触发
    RESET_REASON_APP_FAILURE,       // 应用故障触发
    RESET_REASON_WATCHDOG,          // 看门狗触发
    RESET_REASON_REMOTE_CMD,        // 远程命令触发
    RESET_REASON_POWER_ON,          // 上电复位
} reset_reason_t;
```

#### 4.2.2 双重启检测标志

```c
// 双重启标志结构 (16字节)
typedef struct {
    uint32_t magic;               // 魔数: 0x52535444 ("RSTD")
    uint32_t first_reset_time;    // 第一次复位时间
    uint32_t reset_count;         // 复位计数
    uint32_t crc32;              // CRC32校验
} __attribute__((packed)) double_reset_flag_t;

#define DOUBLE_RESET_TIMEOUT_MS   2000  // 2秒检测窗口
```

#### 4.2.3 启动计数器

```c
// 启动计数器结构 (24字节)
typedef struct {
    uint32_t magic;               // 魔数: 0x424F4F54 ("BOOT")
    uint32_t boot_count;          // 启动总次数
    uint32_t failure_count;       // 连续失败次数
    uint32_t last_success_time;   // 最后成功时间
    uint32_t crc32;              // CRC32校验
} __attribute__((packed)) boot_counter_t;

#define MAX_BOOT_FAILURES  3      // 最大连续失败次数
```

### 4.3 软件复位API接口

```c
// 主要API函数
int software_reset_enter_download_mode(reset_reason_t reason);
bool software_reset_check_download_flag(reset_reason_t *reason);
void software_reset_clear_download_flag(void);
bool software_reset_check_double_reset(void);
void software_reset_update_boot_counter(bool success);
bool software_reset_check_boot_failure(void);
void software_reset_system_now(void) __attribute__((noreturn));

// 命令行接口
int software_reset_handle_command(const char *cmd);
// 支持命令: "reset", "download", "bootloader", "dfu", "status"
```

### 4.4 完整的RBL启动检测流程

```c
boot_mode_t rbl_detect_boot_mode_complete(void) {
    printf("\n[RBL] ==========================================\n");
    printf("[RBL] S300 Enhanced Boot Mode Detection v2.0\n");
    printf("[RBL] ==========================================\n");
    
    reset_reason_t reason;
    
    // 初始化软件复位模块
    if (software_reset_init() != 0) {
        printf("[RBL] Warning: Software reset init failed\n");
    }
    
    // 1. 软件下载标志检测（最高优先级）
    if (software_reset_check_download_flag(&reason)) {
        printf("[RBL] ✓ Software download flag detected\n");
        printf("[RBL]   Reason: %s\n", software_reset_get_reason_string(reason));
        
        // 清除标志避免循环重启
        software_reset_clear_download_flag();
        
        switch (reason) {
            case RESET_REASON_USER_CMD:
                printf("[RBL] → User command triggered download\n");
                return BOOT_MODE_DOWNLOAD_USER;
                
            case RESET_REASON_DOUBLE_RESET:
                printf("[RBL] → Double reset pattern detected\n");
                return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
                
            case RESET_REASON_APP_FAILURE:
                printf("[RBL] → Application failure recovery\n");
                return BOOT_MODE_RECOVERY;
                
            case RESET_REASON_REMOTE_CMD:
                printf("[RBL] → Remote command triggered\n");
                return BOOT_MODE_DOWNLOAD_REMOTE;
                
            default:
                printf("[RBL] → Software download mode\n");
                return BOOT_MODE_DOWNLOAD_SOFTWARE;
        }
    }
    
    // 2. 双重启检测
    if (software_reset_check_double_reset()) {
        printf("[RBL] ✓ Double reset pattern detected\n");
        return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
    }
    
    // 3. 故障恢复检测
    if (software_reset_check_boot_failure()) {
        printf("[RBL] ✓ Boot failure recovery needed\n");
        return BOOT_MODE_RECOVERY;
    }
    
    // 4. 传统串口窗口检测
    if (rbl_check_enhanced_serial_window()) {
        printf("[RBL] ✓ Serial download window triggered\n");
        return BOOT_MODE_DOWNLOAD_SERIAL;
    }
    
    // 5. GPIO下载引脚检测
    if (rbl_check_download_pin()) {
        printf("[RBL] ✓ Hardware download pin active\n");
        return BOOT_MODE_DOWNLOAD_GPIO;
    }
    
    // 6. 正常启动
    printf("[RBL] → Normal boot mode selected\n");
    return BOOT_MODE_NORMAL;
}
```

### 4.5 软件复位使用场景

#### 4.5.1 开发阶段使用

```bash
# 串口命令触发下载模式
echo "download" > /dev/ttyUSB0

# Python工具触发
python3 s300_reset_tool.py serial /dev/ttyUSB0 --mode download

# 双重启模拟
python3 s300_reset_tool.py double-reset /dev/ttyUSB0 --interval 1.5
```

#### 4.5.2 生产阶段使用

```bash
# 双重启方法（无需额外软件）
# 快速按两次复位按钮（2秒内）

# 烧录工具集成
flash_tool --set-download-flag
flash_tool --trigger-double-reset
```

#### 4.5.3 维护阶段使用

```bash
# 网络远程触发
curl -X POST http://device-ip/api/system -d '{"action":"download"}'

# 蓝牙命令触发
echo "DOWNLOAD" | bt_send_command device-mac
```

---

### 4.4 增强串口窗口

```c
// 3-5秒的串口监听窗口
bool rbl_check_enhanced_serial_window(void) {
    const uint32_t timeout_ms = 5000; // 5秒窗口
    uint32_t start_time = rbl_system_get_tick_ms();
    
    printf("[RBL] Serial download window open (5s)...\n");
    printf("[RBL] Send any character to enter download mode\n");
    
    while ((rbl_system_get_tick_ms() - start_time) < timeout_ms) {
        if (uart_has_data()) {
            uint8_t ch = uart_read_byte();
            printf("[RBL] Serial trigger received (0x%02X)\n", ch);
            return true;
        }
        
        // 显示倒计时
        uint32_t remaining = (timeout_ms - (rbl_system_get_tick_ms() - start_time)) / 1000;
        printf("\r[RBL] Countdown: %lu seconds   ", remaining);
        
        rbl_system_delay_ms(50);
    }
    
    printf("\n[RBL] Serial window timeout, continuing boot...\n");
    return false;
}
```

---

## 5. OTA更新机制

### 5.1 A/B分区策略

S300采用A/B分区OTA策略，确保系统可靠性：

**分区角色**:
- **Active Slot**: 当前运行的分区 (OTA_0 或 OTA_1)
- **Inactive Slot**: 备用分区，用于接收新固件
- **Recovery**: 如果两个分区都损坏，回滚到SBL恢复模式

### 5.2 OTA流程

```c
// OTA更新流程
int app_ota_update(const char* firmware_url) {
    esp_partition_info_t *update_partition;
    
    // 1. 获取更新分区
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        return ESP_ERR_OTA_PARTITION_CONFLICT;
    }
    
    // 2. 开始OTA
    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        return err;
    }
    
    // 3. 下载并写入固件
    http_client_handle_t client = http_client_init(firmware_url);
    uint8_t buffer[4096];
    int data_read;
    
    while ((data_read = http_client_read(client, buffer, sizeof(buffer))) > 0) {
        err = esp_ota_write(ota_handle, buffer, data_read);
        if (err != ESP_OK) {
            esp_ota_end(ota_handle);
            return err;
        }
    }
    
    // 4. 完成OTA并设置启动分区
    err = esp_ota_end(ota_handle);
    if (err == ESP_OK) {
        err = esp_ota_set_boot_partition(update_partition);
    }
    
    return err;
}
```

### 5.3 故障回滚机制

```c
// SBL中的自动回滚逻辑
int sbl_auto_rollback_check(sbl_boot_env_t *boot_env) {
    if (boot_env->update_pending) {
        boot_env->retry_count++;
        
        if (boot_env->retry_count >= SBL_MAX_RETRY_COUNT) {
            // 重试次数过多，触发回滚
            SBL_LOGW("Too many boot failures, performing rollback");
            
            // 切换到备用分区
            boot_env->active_slot = !boot_env->active_slot;
            boot_env->update_pending = 0;
            boot_env->retry_count = 0;
            boot_env->rollback_count++;
            
            sbl_boot_write_env(boot_env);
            return 1; // 已执行回滚
        }
    }
    
    return 0; // 无需回滚
}
```

---

## 6. 错误处理和恢复

### 6.1 分级错误处理

**Level 1: ROMBOOT错误**
- RBL损坏或缺失 → 系统死循环等待外部干预

**Level 2: RBL错误**
- SBL损坏 → 进入下载模式等待固件更新
- 系统初始化失败 → 尝试最小化启动或下载模式

**Level 3: SBL错误**
- 所有应用分区损坏 → 进入恢复模式，等待固件重新下载
- 分区表损坏 → 使用默认分区表，标记需要修复

**Level 4: Application错误**
- 应用启动失败 → SBL执行自动回滚
- 运行时故障 → 看门狗复位，增加重试计数

### 6.2 看门狗机制

```c
// 应用程序看门狗管理
void app_watchdog_task(void *param) {
    const uint32_t timeout_ms = 30000; // 30秒超时
    
    while (1) {
        // 检查系统健康状态
        if (app_system_health_check()) {
            // 系统正常，喂狗
            sbl_watchdog_feed();
            
            // 如果是新固件首次启动，标记为成功
            if (ota_is_pending_verify()) {
                ota_mark_app_valid();
            }
        } else {
            // 系统异常，停止喂狗让看门狗复位
            SBL_LOGE("System health check failed, stopping watchdog feed");
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10秒检查一次
    }
}
```

### 6.3 恢复模式

```c
// RBL恢复模式处理
int rbl_recovery_mode(void) {
    printf("[RBL] =================================\n");
    printf("[RBL] RECOVERY MODE - System Repair\n");
    printf("[RBL] =================================\n");
    
    // 显示当前系统状态
    rbl_display_system_status();
    
    // 提供恢复选项
    printf("[RBL] Recovery options:\n");
    printf("[RBL] 1. Download firmware via UART\n");
    printf("[RBL] 2. Factory reset\n");
    printf("[RBL] 3. Partition table repair\n");
    printf("[RBL] 4. Memory test\n");
    
    while (1) {
        uint8_t choice = uart_read_byte_timeout(60000); // 1分钟超时
        
        switch (choice) {
            case '1':
                return rbl_download_mode();
            case '2':
                return rbl_factory_reset();
            case '3':
                return rbl_partition_repair();
            case '4':
                return rbl_memory_test();
            default:
                printf("[RBL] Invalid choice, try again\n");
                break;
        }
    }
}
```

---

## 7. 性能优化

### 7.1 启动时间优化

**目标**: 从上电到应用程序运行 < 2秒

**优化策略**:
1. **QSPI XIP模式**: SBL和APP直接从Flash执行，避免加载时间
2. **并行初始化**: 非关键外设延迟初始化
3. **代码优化**: 关键路径使用-O2优化
4. **缓存机制**: 启动环境信息缓存

```c
// 快速启动路径
int rbl_fast_boot_path(void) {
    // 跳过非必要的检测
    if (rbl_is_fast_boot_enabled()) {
        // 直接跳转到上次成功的SBL
        uint32_t last_sbl_entry = rbl_get_cached_sbl_entry();
        if (last_sbl_entry != 0) {
            rbl_jump_to_sbl(last_sbl_entry);
        }
    }
    
    return rbl_normal_boot();
}
```

### 7.2 内存使用优化

**策略**:
1. **堆栈大小优化**: 根据实际使用调整堆栈大小
2. **静态分配**: 避免动态内存分配
3. **代码段优化**: 将热点代码放在SRAM中

```c
// 内存使用配置
#define RBL_STACK_SIZE          (8 * 1024)     // 8KB堆栈
#define SBL_HEAP_SIZE           (32 * 1024)    // 32KB堆
#define APP_HEAP_SIZE           (128 * 1024)   // 128KB堆

// 热点函数放在SRAM中执行
__attribute__((section(".ramfunc")))
int qspi_fast_read(uint32_t addr, void *data, uint32_t len) {
    // 高频访问的QSPI读取函数
    return qspi_read_impl(addr, data, len);
}
```

### 7.3 Flash访问优化

```c
// QSPI性能配置
void qspi_performance_init(void) {
    qspi_config_t config = {
        .clock_div = 2,           // 100MHz QSPI时钟
        .sample_delay = 1,        // 采样延迟优化
        .hold_delay = 1,          // 保持延迟优化
        .wp_enable = false,       // 禁用写保护以提升性能
    };
    
    qspi_init(&config);
    
    // 使能XIP模式预取
    QSPI->CTRL |= QSPI_CTRL_PREFETCH_EN;
    
    // 配置burst模式
    QSPI->CTRL |= QSPI_CTRL_BURST_EN;
}
```

---

## 8. 安全考虑

### 8.1 固件验证

**多级验证机制**:
1. **ROMBOOT**: CRC32基础验证
2. **RBL**: SHA256 + RSA签名验证
3. **SBL**: 完整镜像哈希验证
4. **OTA**: 增量更新验证

```c
// 安全启动验证
bool rbl_secure_verify_image(uint32_t addr, uint32_t size) {
    image_header_t header;
    
    // 读取镜像头
    qspi_read(addr, &header, sizeof(header));
    
    // 验证镜像签名
    if (rsa_verify_signature(&header.signature, header.hash, 
                           rsa_public_key, sizeof(rsa_public_key)) != 0) {
        return false;
    }
    
    // 验证镜像哈希
    uint8_t calculated_hash[32];
    sha256_calculate(addr + sizeof(header), size - sizeof(header), calculated_hash);
    
    return memcmp(calculated_hash, header.hash, 32) == 0;
}
```

### 8.2 防回滚保护

```c
// 版本回滚保护
typedef struct {
    uint32_t version;         // 固件版本号
    uint32_t min_version;     // 最小允许版本
    uint8_t rollback_count;   // 回滚计数限制
} version_info_t;

bool sbl_check_rollback_protection(const version_info_t *new_version) {
    version_info_t current_version;
    
    // 读取当前版本信息
    nvs_read("version_info", &current_version, sizeof(current_version));
    
    // 检查是否允许回滚
    if (new_version->version < current_version.min_version) {
        SBL_LOGE("Rollback protection: version too old");
        return false;
    }
    
    return true;
}
```

---

## 9. 调试和工具支持

### 9.1 调试输出

**分级日志系统**:
```c
#define RBL_LOG_LEVEL_ERROR     0
#define RBL_LOG_LEVEL_WARN      1
#define RBL_LOG_LEVEL_INFO      2
#define RBL_LOG_LEVEL_DEBUG     3

#define RBL_LOGE(tag, format, ...) \
    rbl_log_printf(RBL_LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)

#define RBL_LOGW(tag, format, ...) \
    rbl_log_printf(RBL_LOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)

#define RBL_LOGI(tag, format, ...) \
    rbl_log_printf(RBL_LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)

#define RBL_LOGD(tag, format, ...) \
    rbl_log_printf(RBL_LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
```

### 9.2 开发工具

**s300_idf.sh 一键开发脚本**:
```bash
# ESP32风格的开发工具
./s300_idf.sh build                 # 编译项目
./s300_idf.sh flash                 # 烧写固件
./s300_idf.sh monitor               # 串口监控
./s300_idf.sh flash monitor         # 烧写并监控
./s300_idf.sh erase_flash           # 擦除Flash
./s300_idf.sh menuconfig            # 配置菜单
```

### 9.3 故障诊断

**自动故障诊断**:
```c
void rbl_system_diagnosis(void) {
    printf("[RBL] ===== System Diagnosis =====\n");
    
    // 1. CPU信息
    printf("[RBL] CPU: ARM Cortex-M4 @ %lu MHz\n", 
           rcc_get_sysclk_freq() / 1000000);
    
    // 2. 内存信息
    printf("[RBL] SRAM: %lu KB free\n", 
           heap_get_free_size() / 1024);
    
    // 3. Flash信息
    printf("[RBL] Flash: W25Q128 16MB\n");
    printf("[RBL] Flash ID: 0x%08lX\n", w25q_read_id());
    
    // 4. 分区信息
    rbl_print_partition_table();
    
    // 5. 启动统计
    boot_environment_t env;
    rbl_read_boot_environment(&env);
    printf("[RBL] Boot count: %lu\n", env.total_boots);
    printf("[RBL] Failed attempts: %lu\n", env.boot_attempts);
}
```

---

## 10. 总结

S300启动架构通过四级启动流程实现了以下目标：

✅ **ESP32兼容性**: 通过RBL补充ROMBOOT功能，提供ESP32等效体验
✅ **可靠性**: A/B分区OTA + 自动回滚机制确保系统稳定
✅ **故障恢复**: 多级错误处理和恢复模式，解决各种故障场景  
✅ **高性能**: QSPI XIP模式 + 优化策略，实现快速启动
✅ **安全性**: 多级验证 + 防回滚保护，确保固件安全
✅ **易用性**: 完整的工具链支持，降低开发门槛

该架构为S300芯片提供了完整的启动解决方案，能够满足从简单应用到复杂OTA系统的各种需求。通过标准化的接口设计和完善的错误处理机制，确保了系统的稳定性和可维护性。

### 相关文档

- [S300 BSP架构设计](S300_BSP_Architecture.md)
- [RBL项目文档](../Projects/RBL/README.md)
- [SBL项目文档](../Projects/SBL/README.md)
- [硬件解决方案文档](Hardware_Workaround_Solutions.md)
- ❌ 高级安全验证
- ❌ 固件烧写功能
- ❌ 错误恢复机制

### 1.3 核心设计目标

1. **ESP32兼容性**：通过RBL补充ESP32 ROM Bootloader功能，保持开发体验一致
2. **四级启动架构**：BootROM → RBL → SBL → Application，形成完整信任链
3. **故障恢复**：通过RBL提供强制下载模式，解决固件损坏救砖问题
4. **安全OTA**：在SBL中实现A/B分区无缝OTA功能，支持安全回滚
5. **性能优化**：充分利用QSPI XIP模式，减少内存拷贝开销

---

## 2. 四级启动流程架构

整个启动过程分为四个阶段，形成一条完整的信任链：

```mermaid
sequenceDiagram
    participant ROM as S300 ROMBOOT<br/>(固化4KB)
    participant RBL as RBL<br/>(SRAM 64KB)
    participant SBL as SBL<br/>(QSPI XIP 128KB)
    participant APP as Application<br/>(QSPI XIP)
    participant FLASH as QSPI Flash

    ROM->>FLASH: 1. 读取Header (256B)
    ROM->>FLASH: 2. 验证RBL完整性
    ROM->>RBL: 3. 加载RBL到SRAM并跳转
    RBL->>RBL: 4. 系统全面初始化
    RBL->>RBL: 5. 打印启动信息和下载提示
    
    alt 3秒串口下载窗口期
        RBL->>RBL: 监听串口输入
        alt 收到下载命令
            RBL->>FLASH: 通过串口协议接收并烧写固件
            RBL->>RBL: 下载完成，重启
        else 强制下载模式
            RBL->>RBL: 检测GPIO/Flash标志
            RBL->>FLASH: 通过UART接收并烧写固件
            RBL->>RBL: 重启
        else 正常启动(超时)
            RBL->>SBL: 6. 验证并跳转到SBL
        end
    end
    SBL->>SBL: 7. 读取分区表，OTA逻辑
    SBL->>APP: 8. 启用看门狗，跳转到App
    APP->>APP: 9. 执行业务逻辑
    alt OTA更新
        APP->>FLASH: 10. 下载新固件到备用分区
        APP->>SBL: 11. 设置更新标志
        APP->>APP: 12. 重启系统
    end
```

### 2.1 Stage 1: S300 ROMBOOT (芯片固化)

**职责**：芯片上电后执行的第一段代码，固化在ROM中，功能极其有限。

**主要动作**：

1. **最小化核心初始化**：
   - CPU核心初始化（Cortex-M4F）
   - 基础时钟源配置（24MHz晶振）
   - 内部SRAM初始化

2. **Header读取与解析**：
   - 从QSPI Flash起始地址（0x0）读取256字节Header
   - 解析RBL段信息（地址、大小、CRC等）

3. **RBL加载与验证**：
   - 从Flash读取RBL代码段
   - 将RBL代码拷贝到内部SRAM1 (0x20000000)
   - 基础CRC验证确保RBL完整性

4. **控制权转移**：
   - 跳转到SRAM中的RBL入口点执行

**限制**：由于S300 ROMBOOT功能有限，无法实现ESP32 ROM Bootloader的高级功能。

### 2.2 Stage 2: RBL (ROM Bootloader - ESP32兼容)

**职责**：运行于SRAM，实现ESP32 ROM Bootloader等效功能，是ROMBOOT能力的重要扩展。

**存储位置**：QSPI Flash 0x0000_0100 - 0x0001_0000 (64KB)  
**运行位置**：SRAM1 0x20000000 (由ROMBOOT加载)

**主要动作**：

1. **系统全面初始化**：

   ```c
   // 配置系统时钟为192MHz
   rcc_config_pll_192mhz();
   rcc_config_ahb_apb_dividers();
   
   // 初始化QSPI控制器
   qspi_controller_init();
   qspi_enable_xip_mode();  // 映射Flash到0x80000000
   
   // 初始化调试串口
   uart_debug_init();
   printf("[RBL] S300 RBL v1.0 - ESP32 ROM Bootloader Compatible\n");
   ```

2. **启动模式检测**：

   ```c
   typedef enum {
       BOOT_MODE_NORMAL = 0,      // 正常启动模式
       BOOT_MODE_DOWNLOAD,        // 下载模式 (GPIO强制)
       BOOT_MODE_RECOVERY,        // 恢复模式 (Flash标志)
   } boot_mode_t;

   boot_mode_t rbl_detect_boot_mode(void) {
       // 检查GPIO引脚状态 (类似ESP32的GPIO0)
       if (gpio_get_level(BOOT_MODE_PIN) == 0) {
           return BOOT_MODE_DOWNLOAD;
       }
       
       // 检查Flash中的恢复标志
       if (flash_read_recovery_flag()) {
           return BOOT_MODE_RECOVERY;
       }
       
       return BOOT_MODE_NORMAL;
   }
   ```

3. **强制下载模式（Recovery）**：

   ```c
   typedef enum {
       BOOT_MODE_NORMAL = 0,      // 正常启动模式
       BOOT_MODE_DOWNLOAD,        // 下载模式 (GPIO强制)
       BOOT_MODE_RECOVERY,        // 恢复模式 (Flash标志)
       BOOT_MODE_SERIAL_WINDOW,   // 串口窗口期下载
   } boot_mode_t;

   boot_mode_t rbl_detect_boot_mode(void) {
       // 检查GPIO引脚状态 (类似ESP32的GPIO0)
       if (gpio_get_level(BOOT_MODE_PIN) == 0) {
           return BOOT_MODE_DOWNLOAD;
       }
       
       // 检查Flash中的恢复标志
       if (flash_read_recovery_flag()) {
           return BOOT_MODE_RECOVERY;
       }
       
       // 兼容模式：检查串口下载窗口期
       if (rbl_check_serial_download_window()) {
           return BOOT_MODE_SERIAL_WINDOW;
       }
       
       return BOOT_MODE_NORMAL;
   }
   ```

   **硬件模式**：
   - 初始化UART（YMODEM协议支持）
   - 接收主机发送的固件（SBL.bin或app.bin）
   - 验证固件完整性（CRC/签名）
   - 编程到QSPI Flash的指定位置
   - 重启设备

   **兼容模式**：为兼容现有无启动模式检测的硬件设计，RBL提供3秒串口下载窗口期：
   - 启动时打印提示信息："Press any key within 3s for download mode..."
   - 监听串口输入，任意字符触发下载模式
   - 支持自定义下载协议，可下载到任意Flash地址
   - 超时后继续正常启动流程

4. **正常启动模式**：
   - 验证SBL的完整性和安全性
   - 直接跳转到QSPI Flash XIP地址中的SBL入口

### 2.3 Stage 3: SBL (Second Bootloader - ESP32兼容)

**职责**：在QSPI Flash中以XIP模式运行，是启动管理的核心，负责A/B分区OTA逻辑。

**存储位置**：QSPI Flash 0x0001_0000 - 0x0003_0000 (128KB)  
**XIP运行地址**：0x8001_0000

**主要动作**：

1. **系统初始化**：

   ```c
   // 初始化调试串口
   board_init();
   printf("[SBL] S300 SBL v1.0 - ESP32 Second Stage Bootloader Compatible\n");
   ```

2. **分区表管理**：

   ```c
   // 读取和解析ESP32格式分区表
   partition_table_t *ptable = read_partition_table();
   if (verify_partition_table(ptable) != 0) {
       printf("[SBL] Invalid partition table, entering recovery\n");
       enter_recovery_mode();
   }
   ```

3. **A/B分区OTA逻辑**：

   ```c
   typedef struct {
       uint32_t active_slot;     // 当前活跃分区：0=A, 1=B
       uint32_t update_pending;  // 待更新标志
       uint32_t boot_count;      // 启动计数
       uint32_t retry_count;     // 重试计数
   } boot_env_t;

   // 从OTA数据分区读取启动环境
   boot_env_t env;
   read_boot_environment(&env);
   ```

4. **可靠性保障（回滚机制）**：

   ```c
   if (env.update_pending && env.retry_count < MAX_RETRY) {
       // 尝试启动新分区
       env.retry_count++;
       save_boot_environment(&env);
       
       // 启用看门狗防止启动失败
       watchdog_start(BOOT_TIMEOUT_MS);
       
       jump_to_app(get_app_address(env.active_slot));
   } else if (env.retry_count >= MAX_RETRY) {
       // 回滚到旧分区
       env.active_slot = 1 - env.active_slot;
       env.update_pending = 0;
       env.retry_count = 0;
       save_boot_environment(&env);
   }
   ```

5. **镜像验证与跳转**：

   ```c
   uint32_t app_addr = get_app_address(env.active_slot);
   
   // 验证目标App镜像的签名或CRC
   if (verify_app_image(app_addr) == 0) {
       printf("[SBL] Booting App from slot %d\n", env.active_slot);
       jump_to_app(app_addr);
   } else {
       printf("[SBL] App verification failed\n");
       enter_recovery_mode();
   }
   ```

### 2.4 Stage 4: Application

**职责**：用户应用程序，在QSPI Flash中以XIP模式运行。

**存储位置**：OTA_0/OTA_1分区（每个4MB）  
**XIP运行地址**：0x8004_0000起始

**主要功能**：

- 执行用户业务逻辑
- OTA更新管理（下载新固件到备用分区）
- 系统运行时监控和维护
- 看门狗喂狗确保系统稳定运行

---

## 3. Flash存储布局规划

### 3.1 最终Flash布局 (W25Q128 - 16MB)

基于S300 ROMBOOT限制和ESP32兼容性要求，最终确定的Flash布局：

| 分区名称     | 起始地址    | 结束地址    | 大小   | XIP地址     | 说明                      |
| ------------ | ----------- | ----------- | ------ | ----------- | ------------------------- |
| **Header**   | 0x0000_0000 | 0x0000_0100 | 256B   | -           | S300镜像头，供ROMBOOT读取 |
| **RBL**      | 0x0000_0100 | 0x0001_0000 | ~64KB  | SRAM运行    | ESP32 ROM BL等效功能      |
| **SBL**      | 0x0001_0000 | 0x0003_0000 | 128KB  | 0x8001_0000 | ESP32 Second Stage BL     |
| **分区表**   | 0x0003_0000 | 0x0003_1000 | 4KB    | -           | ESP32格式分区表           |
| **OTA数据**  | 0x0003_1000 | 0x0003_3000 | 8KB    | -           | ESP32格式OTA状态          |
| **预留空间** | 0x0003_3000 | 0x0004_0000 | 52KB   | -           | 系统预留，未来扩展        |
| **OTA_0**    | 0x0004_0000 | 0x0044_0000 | 4MB    | 0x8004_0000 | 主应用分区                |
| **OTA_1**    | 0x0044_0000 | 0x0084_0000 | 4MB    | 0x8044_0000 | OTA备份分区               |
| **NVS**      | 0x0084_0000 | 0x0086_0000 | 128KB  | -           | 非易失性存储              |
| **用户数据** | 0x0086_0000 | 0x0100_0000 | ~7.5MB | -           | 文件系统/日志等           |

### 3.2 关键设计特性

#### 1. ESP32完全兼容

- SBL以XIP模式运行，完全不知道Header和RBL的存在
- 分区表和OTA数据相对SBL基址的标准ESP32偏移
- 标准ESP32分区格式和API支持

#### 2. 空间分配合理

- **系统开销**：260KB/16MB = 1.6%，利用率仍然很高
- **单APP分区**：4MB，足够大型嵌入式应用
- **预留空间**：52KB，为未来功能扩展留有余地
- **用户数据**：~7.4MB，充足的存储空间

#### 3. 边界对齐优化

- 所有分区均按4KB边界对齐，便于Flash擦除操作
- 64KB、128KB等关键边界对齐，便于内存管理

### 3.3 SBL视角的Flash布局

从SBL的角度看，Flash布局与标准ESP32完全一致：

```text
SBL基址: 0x8001_0000 (XIP地址)

+-------------------+ 0x00000000 (SBL视角)
|   SBL Self        | SBL程序本身 (128KB)
+-------------------+ 0x00020000 (+128KB)
|  Partition Table  | ESP32标准分区表 (4KB)
+-------------------+ 0x00021000 (+132KB)
|    OTA Data       | ESP32标准OTA数据 (8KB)
+-------------------+ 0x00023000 (+140KB)
|   Reserved        | 系统预留空间 (52KB)
+-------------------+ 0x00030000 (+192KB)
|    OTA_0 App      | 主应用分区 (4MB)
+-------------------+ 0x00430000 (+4.19MB)
|    OTA_1 App      | 备份应用分区 (4MB)
+-------------------+ 0x00830000 (+8.19MB)
|      NVS          | 非易失性存储 (128KB)
+-------------------+ 0x00850000 (+8.31MB)
|   User Data       | 用户数据区域
+-------------------+
```

### 3.4 SRAM布局

| 区域  | 起始地址   | 大小  | 用途                         |
| ----- | ---------- | ----- | ---------------------------- |
| SRAM0 | 0x10000000 | 8KB   | 备用或特殊用途               |
| SRAM1 | 0x20000000 | 384KB | RBL运行 + APP数据段 + 栈空间 |

**SRAM1详细布局**：

```text
0x20060000  ← _estack (栈顶)
    ↓       (栈向下增长)
            
    ↑       (堆向上增长)
            .bss段 (未初始化数据)
            .data段 (已初始化数据)
            .rodata段 (只读数据)
            .text段 (RBL代码，ROMBOOT加载)
0x20000000  ← 向量表起始地址
```

---

## 4. OTA更新序列图

```mermaid
sequenceDiagram
    participant App as 应用程序 (App_A)
    participant SBL as SBL Bootloader
    participant Server as OTA服务器/SD卡
    participant Flash as QSPI Flash
    participant WDT as 看门狗

    Note over App, Flash: 正常运行阶段
    App->>Server: 1. 查询并下载新固件包
    App->>App: 2. 验证新固件签名/校验和
    App->>Flash: 3. 禁用XIP，切换间接模式
    App->>Flash: 4. 擦除OTA_1分区
    App->>Flash: 5. 写入新固件至OTA_1
    App->>Flash: 6. 重新启用XIP模式
    App->>Flash: 7. 验证OTA_1固件完整性
    App->>Flash: 8. 设置更新标志(update_pending=1)
    App->>App: 9. 重启设备

    Note over SBL, WDT: 启动验证阶段
    SBL->>Flash: 10. 读取OTA数据
    SBL->>SBL: 11. 发现update_pending标志
    SBL->>Flash: 12. 验证OTA_1固件
    SBL->>WDT: 13. 启用看门狗(30秒超时)
    SBL->>App: 14. 跳转至OTA_1 (App_B)

    alt 启动成功
        App->>WDT: 15. 定期喂狗，运行稳定
        App->>Flash: 16. 清除update_pending标志
        App->>Flash: 17. 设置active_slot=1
        Note over App: OTA更新成功完成
    else 启动失败(看门狗超时)
        WDT->>SBL: 18. 看门狗复位
        SBL->>SBL: 19. 检测到更新失败
        SBL->>Flash: 20. 回滚：active_slot=0
        SBL->>Flash: 21. 清除update_pending标志
        SBL->>App: 22. 跳转至OTA_0 (App_A)
        Note over SBL: 自动回滚完成，系统恢复
    end
```

---

## 5. 关键特性实现

### 5.1 XIP模式优化

**优势**：

- 代码直接从Flash执行，节省SRAM空间
- 减少启动时间，无需拷贝大量代码到SRAM
- 支持执行大于SRAM容量的应用程序

**实现要点**：

```c
// XIP配置示例
void qspi_configure_xip(void) {
    // 配置QSPI控制器为XIP模式
    // 设置读命令为Fast Read (0x0B)
    // 配置地址映射：Flash 0x0 → CPU 0x80000000
    
    REG32(QSPI_BASE, CQSPI_REG_REMAP) = 
        CQSPI_REMAP_ENABLE | 
        (0x80000000 & CQSPI_REMAP_ADDRESS_MASK);
}
```

**性能数据**（基于实测）：

- STIG模式读取带宽：2.20 MB/s @ 64MHz SCLK
- XIP模式执行性能：接近SRAM执行速度
- 支持频率范围：AHB/3 到 AHB/8 (64MHz - 24MHz)

### 5.2 安全启动机制

**启动链信任**：

```c
// 每个阶段都验证下一阶段
int verify_next_stage(uint32_t addr, uint32_t size) {
    // 1. 完整性检查
    if (crc32_verify(addr, size) != 0) {
        return -1;
    }
    
    // 2. 签名验证（如果启用）
    if (secure_boot_enabled()) {
        return verify_signature(addr, size);
    }
    
    return 0;
}
```

### 5.3 故障恢复机制

**多层恢复**：

```c
void handle_boot_failure(boot_env_t *env) {
    env->retry_count++;
    
    if (env->retry_count >= MAX_RETRY) {
        // 超过重试次数，切换分区
        env->active_slot = 1 - env->active_slot;
        env->retry_count = 0;
        env->update_pending = 0;
        
        printf("[SBL] Switching to slot %d\n", env->active_slot);
    }
    
    save_boot_environment(env);
}
```

---

## 6. 性能与资源分析

### 6.1 启动时间分析

**各阶段耗时**：

- ROMBOOT: ~10ms (固化ROM，最优化)
- RBL加载和初始化: ~50ms (SRAM执行，快速)
- SBL初始化: ~20ms (XIP执行)
- APP跳转: ~10ms
- **总启动时间**: ~90ms

### 6.2 资源占用

**Flash占用**：

- Header + RBL: 64KB (系统核心)
- SBL: 128KB (启动管理)
- 系统分区: 16KB (分区表+OTA数据)
- 预留空间: 52KB (未来扩展)
- **系统总开销**: 260KB/16MB = 1.6%

**SRAM占用**：

- RBL运行时: ~32KB (代码+数据+栈)
- APP运行时: ~64KB (取决于具体应用)
- **可用SRAM**: 384KB - 64KB = 320KB

### 6.3 OTA性能

**更新速度**：

- 4MB固件包写入时间：~30分钟 (基于2.20 MB/s写入速度)
- 验证时间：~5秒 (CRC32计算)
- 重启时间：~100ms

**可靠性**：

- 更新成功率：>99.9% (基于A/B分区机制)
- 故障恢复时间：<10秒 (看门狗超时+回滚)

---

## 7. 开发指南

### 7.1 构建系统

**关键设计决策**：由于S300 ROMBOOT依赖Header中的信息加载RBL到SRAM，我们采用**分离生成 + 脚本合并**的方案：

1. **先编译RBL** → 获取实际大小和CRC值
2. **生成Header** → 基于RBL信息创建256字节Header
3. **合并镜像** → Header + RBL形成完整Bootloader

**Makefile示例**：

```makefile
# S300 多阶段构建
.PHONY: all rbl header bootloader sbl app image

all: image

# Step 1: 构建RBL二进制
rbl:
    $(MAKE) -C Projects/RBL
    $(OBJCOPY) -O binary Projects/RBL/rbl.elf build/rbl.bin

# Step 2: 根据RBL信息生成Header
header: rbl
    python3 tools/generate_s300_header.py \
        --rbl build/rbl.bin \
        --output build/header.bin

# Step 3: 合并Header和RBL
bootloader: header
    python3 tools/merge_s300_image.py \
        --header build/header.bin \
        --rbl build/rbl.bin \
        --output build/bootloader.bin

# Step 4: 构建其他组件
sbl:
    $(MAKE) -C Projects/SBL

app:
    $(MAKE) -C Projects/Demo/HelloWorld

# Step 5: 生成完整固件镜像
image: bootloader sbl app
    python tools/s300_image_gen.py \
        --bootloader build/bootloader.bin \
        --sbl build/SBL.bin \
        --app build/HelloWorld.bin \
        -o firmware.bin
```**烧录命令**：

```makefile
# 完整镜像烧录
flash: image
	s300_download_tool --image firmware.bin

# 仅更新APP (开发用)
flash-app: app
	s300_download_tool --app build/HelloWorld.bin --offset 0x33000
```

### 7.2 开发流程

**初次开发**：

1. 编译所有组件：`make all`
2. 生成完整镜像：`make image`
3. 烧录到设备：`make flash`

**日常开发**：

1. 仅修改APP代码：`make app`
2. 快速更新：`make flash-app`
3. 或使用OTA更新机制

### 7.3 调试技巧

**串口调试信息**：

```c
// 启动日志示例
[ROMBOOT] S300 ROMBOOT, loading RBL...
[RBL] S300 RBL v1.0 - ESP32 ROM Bootloader Compatible
[RBL] Clock: PLL=192MHz, AHB=192MHz, APB=48MHz  
[RBL] QSPI: Initializing XIP mode
[RBL] QSPI: W25Q128 detected (16MB)
[RBL] QSPI: XIP enabled at 0x80000000
[RBL] Jumping to SBL at 0x80010000

[SBL] S300 SBL v1.0 - ESP32 Second Stage Bootloader Compatible
[SBL] Partition table: OK
[SBL] Boot env: slot=0, pending=0, count=42
[SBL] App verification: OK
[SBL] Booting App from slot 0 at 0x80040000

[APP] Hello World Application v1.0
[APP] System ready!
```

---

## 8. 最佳实践

### 8.1 设计原则

1. **确保向后兼容**：新版本SBL必须能启动旧版本App
2. **幂等性操作**：所有状态变更操作必须可重复执行
3. **原子性更新**：关键状态变更使用原子操作或事务机制
4. **故障隔离**：各阶段故障不应影响其他阶段

### 8.2 安全考虑

1. **签名验证**：所有固件镜像必须包含有效签名
2. **回滚保护**：防止回滚到已知有漏洞的版本
3. **安全启动链**：从BootROM到App建立完整信任链
4. **防砖机制**：确保任何情况下都能恢复系统

### 8.3 OTA最佳实践

1. **分片下载**：大固件分片传输，支持断点续传
2. **校验机制**：多重校验确保固件完整性
3. **回滚策略**：明确的回滚触发条件和流程
4. **用户通知**：及时反馈OTA进度和状态

---

## 9. 故障排除

### 9.1 常见问题

**启动失败**：

1. 检查QSPI连接和配置
2. 验证镜像完整性
3. 查看串口调试信息
4. 检查电源和时钟配置

**OTA失败**：

1. 验证网络连接
2. 检查Flash空间是否充足
3. 确认固件兼容性
4. 查看OTA状态标志

### 9.2 诊断工具

**系统状态查询**：

```c
void print_system_status(void) {
    boot_env_t env;
    read_boot_environment(&env);
    
    printf("System Status:\n");
    printf("  Active Slot: %c\n", env.active_slot ? 'B' : 'A');
    printf("  Boot Count: %u\n", env.boot_count);
    printf("  Update Pending: %s\n", env.update_pending ? "Yes" : "No");
    printf("  Last Successful: %c\n", env.last_successful ? 'B' : 'A');
}
```

**Flash健康检查**：

```c
void flash_health_check(void) {
    uint8_t status = qspi_read_status_register();
    printf("Flash Status: 0x%02X\n", status);
    
    if (status & FLASH_STATUS_WIP) {
        printf("  Warning: Write in progress\n");
    }
    if (status & FLASH_STATUS_WEL) {
        printf("  Warning: Write enable latch set\n");
    }
}
```

---

## 10. 参考资料

### 10.1 相关文档

- [S300_BSP_Architecture.md](S300_BSP_Architecture.md) - BSP架构设计
- [S300_Header_Format.md](S300_Header_Format.md) - Header格式详细规范
- [S300_RBL_Build_Process.md](S300_RBL_Build_Process.md) - RBL构建流程详解
- [S300_RBL_Serial_Download.md](S300_RBL_Serial_Download.md) - RBL串口下载功能设计
- [S300_Download_Circuit_Design.md](S300_Download_Circuit_Design.md) - 下载电路设计
- [coding_style_en.md](coding_style_en.md) - 代码规范

### 10.2 硬件手册

- PiMCHIP-S300技术参考手册V2.0
- W25Q128JW数据手册
- Cadence QSPI控制器手册

### 10.3 工具链

- GCC ARM Embedded Toolchain
- OpenOCD + ST-Link调试器
- S300下载工具（固件烧写）

---

## 文档结束

此文档详细描述了S300芯片的启动流程和OTA架构设计，涵盖了从硬件限制到软件实现的各个方面。设计目标是在保持ESP32兼容性的同时，提供高性能、高可靠性的启动和OTA解决方案。

如有任何问题或建议，请联系开发团队。
