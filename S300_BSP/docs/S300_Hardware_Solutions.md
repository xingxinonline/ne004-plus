# S300硬件约束下的启动模式和下载功能实现方案

## 背景问题

当前硬件设计缺少：
1. **启动模式引脚检测** - 无法通过硬件引脚判断是否进入下载模式
2. **下载复位电路** - 无法通过外部工具控制设备复位进入下载模式

## 解决方案概述

通过软件和协议层面的设计，完全替代硬件功能，实现可靠的固件下载和更新机制。

---

## 方案1：基于Flash标志位的软件下载模式

### 1.1 实现原理

使用Flash的专用区域存储下载模式标志，通过软件命令触发下载模式：

```
Flash布局扩展：
0xFFF000 - 0xFFFFFF : 系统标志区域 (4KB)
  ├── 0xFFF000 : 下载模式标志 (DOWNLOAD_FLAG_MAGIC)
  ├── 0xFFF004 : 下载原因 (用户命令/故障恢复)
  ├── 0xFFF008 : 尝试计数 (防止无限循环)
  └── 0xFFF00C : 时间戳
```

### 1.2 触发下载模式的方式

#### A. 应用程序主动触发
```c
// 在正常应用中实现
void app_enter_download_mode(void)
{
    // 设置下载标志
    flash_set_download_flag(DOWNLOAD_REASON_USER_REQUEST);
    
    // 软件复位
    NVIC_SystemReset();
}
```

#### B. 串口命令触发
```c
// 在应用的串口命令处理中
if (strcmp(cmd, "DOWNLOAD") == 0) {
    printf("Entering download mode...\r\n");
    app_enter_download_mode();
}
```

#### C. 远程触发（通过网络/蓝牙等）
```c
// 收到远程下载命令
void on_remote_download_request(void)
{
    flash_set_download_flag(DOWNLOAD_REASON_REMOTE);
    NVIC_SystemReset();
}
```

### 1.3 RBL中的检测逻辑

修改RBL的启动模式检测：

```c
bool rbl_check_software_download_mode(void)
{
    download_flag_t flag;
    
    // 读取下载标志
    if (flash_read_download_flag(&flag) != 0) {
        return false;
    }
    
    // 检查魔数
    if (flag.magic != DOWNLOAD_FLAG_MAGIC) {
        return false;
    }
    
    // 检查尝试计数，防止无限循环
    if (flag.retry_count > MAX_DOWNLOAD_RETRIES) {
        printf("[RBL] Too many download retries, clearing flag\r\n");
        flash_clear_download_flag();
        return false;
    }
    
    // 增加尝试计数
    flag.retry_count++;
    flash_write_download_flag(&flag);
    
    printf("[RBL] Software download mode triggered (reason: %d)\r\n", flag.reason);
    return true;
}
```

---

## 方案2：基于看门狗的故障恢复下载模式

### 2.1 自动故障检测

当应用程序出现严重故障时，自动进入下载模式：

```c
// 在SBL中启用看门狗
void sbl_start_application_with_watchdog(void)
{
    // 设置故障计数
    boot_env_t env;
    read_boot_environment(&env);
    env.boot_attempts++;
    
    if (env.boot_attempts > MAX_BOOT_ATTEMPTS) {
        printf("[SBL] Too many boot failures, marking for recovery\r\n");
        flash_set_download_flag(DOWNLOAD_REASON_BOOT_FAILURE);
        env.boot_attempts = 0;
        save_boot_environment(&env);
        NVIC_SystemReset();
        return;
    }
    
    save_boot_environment(&env);
    
    // 启动看门狗
    watchdog_start(APPLICATION_TIMEOUT_MS);
    
    // 跳转到应用
    jump_to_application();
}

// 在应用程序中
void app_main(void)
{
    // 应用启动成功，清除故障计数
    boot_env_t env;
    read_boot_environment(&env);
    env.boot_attempts = 0;
    save_boot_environment(&env);
    
    // 正常运行逻辑
    while (1) {
        // 定期喂狗
        watchdog_feed();
        
        // 业务逻辑
        app_task();
    }
}
```

---

## 方案3：基于串口超时的下载窗口

### 3.1 扩展当前的3秒窗口机制

优化现有的串口检测逻辑，提供更好的用户体验：

```c
bool rbl_check_enhanced_serial_window(void)
{
    uint32_t start_time = rbl_system_get_tick_ms();
    const uint32_t timeout_ms = 5000;  // 延长到5秒
    uint8_t rx_data;
    int char_count = 0;
    
    printf("\r\n");
    printf("===========================================\r\n");
    printf("  S300 RBL - Download Mode Available\r\n");
    printf("===========================================\r\n");
    printf("Press ANY KEY within 5 seconds to enter download mode\r\n");
    printf("Or send 'DOWNLOAD' command to force download mode\r\n");
    
    char command_buffer[16] = {0};
    int cmd_index = 0;
    
    while ((rbl_system_get_tick_ms() - start_time) < timeout_ms) {
        if (rbl_uart_read_nonblock(&rx_data, 1) > 0) {
            char_count++;
            
            // 简单按键检测
            if (char_count == 1) {
                printf("\r\n[RBL] Key pressed! Entering download mode...\r\n");
                return true;
            }
            
            // 命令检测
            if (rx_data >= ' ' && rx_data <= '~' && cmd_index < 15) {
                command_buffer[cmd_index++] = rx_data;
                if (strstr(command_buffer, "DOWNLOAD")) {
                    printf("\r\n[RBL] DOWNLOAD command received!\r\n");
                    return true;
                }
            }
        }
        
        // 倒计时显示
        uint32_t remaining = (timeout_ms - (rbl_system_get_tick_ms() - start_time)) / 1000;
        static uint32_t last_second = 0;
        if (remaining != last_second) {
            printf("Countdown: %lu seconds\r", (unsigned long)remaining);
            last_second = remaining;
        }
        
        rbl_system_delay_ms(50);
    }
    
    printf("\r\nTimeout! Continuing normal boot...\r\n");
    return false;
}
```

---

## 方案4：实现"虚拟复位电路"

### 4.1 基于双重启检测

模拟ESP32的双重启检测机制：

```c
#define DOUBLE_RESET_TIMEOUT_MS  2000
#define DOUBLE_RESET_FLAG_ADDR   0xFFF100

typedef struct {
    uint32_t magic;
    uint32_t timestamp;
    uint32_t count;
} double_reset_flag_t;

bool rbl_check_double_reset(void)
{
    double_reset_flag_t flag;
    uint32_t current_time = rbl_system_get_tick_ms();
    
    // 读取标志
    if (flash_read(DOUBLE_RESET_FLAG_ADDR, &flag, sizeof(flag)) != 0) {
        memset(&flag, 0, sizeof(flag));
    }
    
    // 检查是否在双重启时间窗口内
    if (flag.magic == DOUBLE_RESET_MAGIC) {
        uint32_t elapsed = current_time - flag.timestamp;
        if (elapsed < DOUBLE_RESET_TIMEOUT_MS) {
            flag.count++;
            if (flag.count >= 2) {
                printf("[RBL] Double reset detected! Entering download mode\r\n");
                // 清除标志
                flash_erase_sector(DOUBLE_RESET_FLAG_ADDR);
                return true;
            }
        } else {
            // 超时，重置计数
            flag.count = 1;
        }
    } else {
        // 首次启动
        flag.magic = DOUBLE_RESET_MAGIC;
        flag.count = 1;
    }
    
    // 更新时间戳
    flag.timestamp = current_time;
    
    // 写回标志
    flash_erase_sector(DOUBLE_RESET_FLAG_ADDR);
    flash_write(DOUBLE_RESET_FLAG_ADDR, &flag, sizeof(flag));
    
    return false;
}
```

### 4.2 使用方法

用户可以通过快速按复位按钮两次（2秒内）进入下载模式，无需额外硬件。

---

## 方案5：网络和蓝牙远程下载

### 5.1 实现OTA下载服务

```c
// HTTP服务器端点处理
void http_handle_enter_download(httpd_req_t *req)
{
    printf("Remote download request received\r\n");
    
    // 发送响应
    httpd_resp_send(req, "Entering download mode...", -1);
    
    // 延时确保响应发送完成
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 设置下载标志并复位
    flash_set_download_flag(DOWNLOAD_REASON_REMOTE);
    esp_restart();
}

// 蓝牙命令处理
void bt_handle_download_command(const char* command)
{
    if (strcmp(command, "ENTER_DOWNLOAD") == 0) {
        printf("Bluetooth download request\r\n");
        flash_set_download_flag(DOWNLOAD_REASON_BLUETOOTH);
        esp_restart();
    }
}
```

---

## 完整的启动模式检测流程

### 修改后的RBL检测逻辑

```c
boot_mode_t rbl_detect_boot_mode_enhanced(void)
{
    printf("[RBL] Enhanced boot mode detection starting...\r\n");
    
    // 1. 检查软件下载标志（最高优先级）
    if (rbl_check_software_download_mode()) {
        return BOOT_MODE_DOWNLOAD_SOFTWARE;
    }
    
    // 2. 检查双重启动
    if (rbl_check_double_reset()) {
        return BOOT_MODE_DOWNLOAD_DOUBLE_RESET;
    }
    
    // 3. 检查应用故障标志
    if (rbl_check_application_failure()) {
        return BOOT_MODE_RECOVERY;
    }
    
    // 4. 检查串口下载窗口
    if (rbl_check_enhanced_serial_window()) {
        return BOOT_MODE_DOWNLOAD_SERIAL;
    }
    
    // 5. 默认正常启动
    return BOOT_MODE_NORMAL;
}
```

---

## 实施建议

### 阶段1：立即可实施
1. 实现**软件下载标志**机制
2. 优化**串口窗口检测**
3. 在应用中添加**命令行下载触发**

### 阶段2：增强功能  
1. 实现**双重启检测**
2. 添加**故障自动恢复**
3. 实现**网络远程下载**

### 阶段3：生产优化
1. 优化Flash布局和磨损均衡
2. 添加安全验证机制
3. 实现批量生产工具

这套方案完全不依赖额外硬件，通过软件手段实现了ESP32同等的下载和恢复功能，甚至在某些方面更加灵活和强大。
