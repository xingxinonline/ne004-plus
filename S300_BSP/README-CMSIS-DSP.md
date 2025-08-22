# 在 S300 BSP 中正式集成 CMSIS-DSP 指南

本文档说明如何在 S300 BSP 中统一启用并向客户交付 CMSIS-DSP。

## 目录结构要求

建议在 BSP 仓库的上级或同级提供完整 CMSIS-DSP 源码目录（本工程默认：`../../../../../CMSIS-DSP`）。
如路径不同，可在各项目的 Makefile 中覆盖变量 `CMSIS_DSP_ROOT`。

```text
ne004-plus/
  S300_BSP/
    tools/
      cmsis_dsp.mk  # 通用集成脚本
    Projects/
      Examples/
        ...
CMSIS-DSP/
  Include/
  PrivateInclude/
  Source/
  ...
```

## 一键集成方式（推荐）

在你的工程 Makefile 中：

1. 包含通用脚本

```makefile
include ../../../../tools/cmsis_dsp.mk
```

2. 追加编译参数与对象

```makefile
CFLAGS += $(CMSIS_DSP_CFLAGS)
OBJS   += $(CMSIS_DSP_OBJS)
```

该脚本将：
- 自动设置 CMSIS-DSP 头文件与编译宏（`ARM_MATH_CM4`, `DISABLEFLOAT16`）
- 自动加入聚合源码（Basic/Transform/FastMath 等）
- 提供从 `CMSIS-DSP/Source/...` 到本地 `build/...` 的通配编译规则

> 如希望改为静态库方式：设置 `CMSIS_DSP_BUILD_LIB=1`，并在链接时添加 `$(CMSIS_DSP_LIB)`。

## 编译器与优化建议

- Cortex-M4：`-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb`
- 性能：`-Ofast -ffast-math -ffunction-sections -fdata-sections` + 链接 `--gc-sections`
- 关闭 f16：`-DDISABLEFLOAT16`（M4 无 f16 硬件加速）

## 代码尺寸优化

- 尽量使用定尺寸初始化函数（如 `arm_cfft_init_64_f32`），方便链接器剔除无关表与代码
- 若空间紧张，可改为只编译用到的函数源文件（替换聚合文件）

## 面向客户的使用说明

1. 头文件
   - 客户代码包含 `#include "arm_math.h"`
2. 选择数据类型与 API
   - 浮点：`arm_*_f32` / 定点：`arm_*_q15`/`q31` 等
3. 初始化与调用
   - 复杂模块使用对应 init 函数（如 FFT、FIR）再调用计算函数
4. 线程/中断
   - 某些 API 会使用查表与全局常量表，注意与中断上下文的时序与栈使用

## 故障排查

- 头文件找不到：检查 `CMSIS_DSP_ROOT` 与 `$(CMSIS_DSP_CFLAGS)` 是否生效
- 链接缺符号：确认相应功能的聚合源码已包含（或添加对应单文件实现）
- 运行错误：检查 FPU 选项与 `ARM_MATH_CM4` 宏是否启用，确认系统时钟/FPU 初始化正确

## 示例工程

- `Projects/Examples/DSP_CMSIS_Quick` 已接入通用脚本并包含：向量加法、sin、RFFT 快测
- 可复制该示例为模板，快速创建客户 demo
