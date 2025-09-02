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

## Phase 4: 串口下载 (YMODEM 简版)

- UART 接收状态机
- YMODEM 接收到 QSPI 写入
- 进度与错误回报

验收标准:

- 通过串口工具成功刷入 SBL 并启动

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
