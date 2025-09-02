# RBL Roadmap

本文档描述 RBL 开发分阶段目标与待办。

## Phase 0: Minimal Baseline (当前)

- 基于 SRAM 的极简 RBL 验证 (无 printf)
- 可编译、可反汇编、可通过 GDB 装载到 SRAM 运行
- 生成 S300 标准 Header 与 complete 镜像

验收标准:

- `make -C GCC all` 成功，产生 .elf/.bin/.hex/.dis
- `make -C GCC s300_image` 成功，产生 header.bin/complete.bin
- 串口可见启动 banner 与心跳

## Phase 1: 基础外设抽象

- 独立出最小 UART 与 GPIO 模块
- 提供轻量 log (可开关)
- 提供简易延时与时钟配置抽象

验收标准:

- 统一的 `uart_write(const char*,size_t)` 接口
- 日志可通过宏开关裁剪

## Phase 2: QSPI Flash 初始化

- 初始化 QSPI 控制器，识别 W25Q128
- 读 ID、读/写/擦基本指令
- 基础保护位读写

验收标准:

- 可读出 JEDEC ID
- 可读写一页并校验

## Phase 3: SBL 完整性检查与跳转

- SBL 起始地址与边界配置
- 栈顶与复位向量检查
- CRC32/采样校验 (轻量)
- 跳转封装 (重定位/VTOR 设置)

验收标准:

- SBL 校验失败转入下载模式；成功则跳转

### Phase 4: 串口下载功能（YMODEM简版）
**目标**: 实现基础的串口下载协议
**验收标准**: 能够通过串口接收并写入Flash

#### 4.1 YMODEM协议简化版
- [x] SOH/STX帧解析
- [x] 数据包校验（校验和）
- [x] ACK/NAK握手
- [x] 文件接收状态机

#### 4.2 Flash写入集成
- [x] 擦除目标扇区
- [x] 按包写入数据
- [x] 写入后校验

#### 4.3 下载流程
- [x] 进入下载模式提示
- [x] 等待YMODEM传输
- [x] 传输完成重启

---

## 提交规范

本项目遵循 **Angular 提交规范 + 中文 + Emoji** 格式，确保提交信息清晰、可检索、便于回滚。

### 提交格式模板

```
<emoji> <type>(<scope>): <subject>

<正文段落1>

<正文段落2>
```

### 1. 提交头格式

**`<emoji> <type>(<scope>): <subject>`**

- **Emoji**: 单个表情符号，增强可读性与分类
- **Type**: 变更类型（必选）
- **Scope**: 影响范围（推荐使用）
- **Subject**: 简短描述，中文，50字以内，末尾不加标点

### 2. Type 类型 + Emoji 对照

| Type | Emoji | 说明 | 示例 |
|------|-------|------|------|
| feat | ✨ | 新功能 | `✨ feat(rbl): 新增YMODEM下载协议` |
| fix | 🐛 | 修复缺陷 | `🐛 fix(qspi): 修复JEDEC ID读取失败` |
| docs | 📝 | 文档更新 | `📝 docs(readme): 更新构建说明` |
| style | 🎨 | 代码风格 | `🎨 style(hal): 统一缩进格式` |
| refactor | ♻️ | 重构 | `♻️ refactor(uart): 提取HAL抽象层` |
| perf | ⚡️ | 性能优化 | `⚡️ perf(qspi): 优化频率配置` |
| test | ✅ | 测试相关 | `✅ test(sbl): 新增校验测试用例` |
| build | 🏗️ | 构建系统 | `🏗️ build(makefile): 新增LOG开关` |
| ci | 🤖 | CI/CD配置 | `🤖 ci(github): 配置自动构建` |
| chore | 🧹 | 杂项维护 | `🧹 chore(tools): 更新依赖脚本` |

### 3. Scope 范围约定

- **rbl**: RBL核心功能
- **hal**: 硬件抽象层
- **qspi**: QSPI控制器
- **uart**: 串口功能
- **sbl**: SBL引导管理
- **ymodem**: YMODEM协议
- **download**: 下载管理
- **makefile**: 构建系统
- **docs**: 文档相关

跨模块可用加号连接：`rbl+qspi`

### 4. 提交正文结构

```
1. 背景
- 变更动机：为什么改
- 问题定位：原行为/报错/缺陷

2. 方案与实现  
- 核心修改点（3-6条）
- 影响面：哪些模块/接口

3. 兼容性与迁移
- 向后兼容/有破坏
- 迁移建议

4. 验证
- 构建：命令/结果
- 运行：期望输出
- 测试：关键用例
```

### 5. 示例提交

```
✨ feat(rbl): Phase 4串口下载功能YMODEM简版

1. 背景
- 实现串口下载功能，支持YMODEM协议简版
- SBL校验失败时自动进入下载模式

2. 方案与实现
- 新增rbl_ymodem.h/.c模块，实现YMODEM协议解析
- 新增rbl_download.h/.c模块，集成Flash写入管理
- 主程序集成下载流程，校验失败时自动切换
- 支持CRC16校验、ACK/NAK握手机制

3. 兼容性与迁移
- 向后兼容，新增功能不影响现有流程
- 增加stdio.h依赖，格式化日志输出

4. 验证
- 构建：make all LOG=1 成功
- 镜像：48644字节，自动复制到目标路径
- 功能：SBL校验失败时进入下载等待
```

### 6. 工具使用

**标准提交**：
```bash
git commit -m "✨ feat(rbl): Phase 4串口下载功能YMODEM简版" \
           -m "1. 背景..." \
           -m "2. 方案与实现..."
```

**多段落提交**：
```bash
git commit  # 进入编辑器编写完整提交信息
```

---

## 完成情况

- [x] **Phase 0**: 工程搭建 ✅
- [x] **Phase 1**: 最小HAL实现 ✅  
- [x] **Phase 2**: QSPI初始化与读写 ✅
- [x] **Phase 3**: SBL完整性检查与跳转 ✅
- [x] **Phase 4**: 串口下载功能（YMODEM简版） ✅

## Phase 5: 生产辅助

- 写保护策略 (仅 RBL 保护)
- 烧录脚本 (J-Link/OpenOCD)
- 版本与构建信息嵌入

## Phase 6: 稳定性 & 文档

- 边界条件与错误处理完善
- README/使用手册/故障排查
- 基础自动化构建检查

---

更新节奏：每完成一个 Phase，提交 PR 并打标签。
