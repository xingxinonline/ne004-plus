# S300 产品化 BSP 方案（稳定版建议稿）

本文给出面向量产/长期维护的 S300 BSP 分层、目录组织、API 约束、构建与测试发布流程，并包含从现状到目标架构的迁移步骤。相较早期草案，本版细化了 SoC 基础驱动（RCC/IOMUX）与外设驱动（如 GPIO）的职责边界、对外 API 约束、质量门与提交规范，兼顾“最小可用”与“可持续扩展”。

## 1. 目标与原则

- 清晰分层：芯片(CMSIS/Device) / 驱动(Drivers) / 板级(Board) / 中间件(MW) / 算法(Algorithms) / 应用(Examples)
- 稳定 API：外设驱动对上暴露精简、稳定的 C 接口；对下仅依赖 Device 寄存器定义
- 可移植：板级差异（时钟、引脚）板内配置化，不污染驱动
- 可测试：驱动与算法可单独构建与示例验证
- 可扩展：统一编码规范、构建体系、目录模式，降低增量成本

## 2. 目录结构（目标）

```text
S300_BSP/
  CMSIS/
    Core/Include/                # CMSIS Core (已 vendor)
    Device/PiMCHIP/S300/
      Include/                   # s300.h、device.h、irqn 等
      Source/                    # system_S300.c, startup_S300.S, ld/
  Boards/
    <board_name>/                # 板级配置（引脚/时钟/外设选择）
      board.h
      clock_config.c             # 仅封装“本板”使用到的时钟开关/分频/复位组合
      pinmux_config.c            # 仅封装“本板”引脚复用、上下拉、驱动能力
  Drivers/
    SoC/                         # SoC 基础：仅提供通用、无板级耦合的底层能力
      Include/
        s300_rcc.h               # 时钟/复位/分频/选择器（不含板级策略）
        s300_iomux.h             # 复用/上下拉/驱动能力原语
      Source/
        s300_rcc.c
        s300_iomux.c
    GPIO/
      Include/s300_gpio.h
      Source/s300_gpio.c
    UART/
      Include/s300_uart.h
      Source/s300_uart.c
    I2C/ SPI/ DMA/ Timer/ ...（同构）
  Middleware/
    CMSIS-DSP/                   # 作为子模块或预编译库（可选）
  Algorithms/
    mfcc/
      include/
      src/
      tests/
  Projects/
    Examples/
      UART_HelloWorld/
        GCC/
        Src/
      MFCC_Demo/
        GCC/
        Src/
  tools/
    scripts/                     # 打包、版本、检查脚本
  docs/
    BSP_Productization.md        # 本文
  .github/
    prompts/
      bsp-expert.prompt.md       # LangGPT 提示词（团队共享）
```

## 3. 初始化与启动流程

- Reset -> `SystemInit()`（时钟基础、VTOR）
- `main()` -> `Board_Init()`（调用板级 clock/pinmux 配置）
- 驱动初始化（例如 `S300_UART_Init_115200(uart_id)`）
- 应用逻辑

建议：保持 SysTick 启动在 `main()` 完成（避免在 C 运行时初始化前产生中断）。

## 4. 驱动层设计（SoC 与外设解耦）

- 依赖：仅包含 `s300.h` 与必要 CMSIS 头；严禁在驱动层写死板级引脚或板型判断。
- SoC 层职责（Drivers/SoC）：
  - RCC：时钟门控、分频、复位、PLL/时钟源选择，接口面向“功能原语”，不包含“板级策略”。
  - IOMUX：引脚复用、上下拉、驱动能力等“原语”。
- 外设驱动层（Drivers/GPIO、Drivers/UART 等）：
  - GPIO：方向配置、输入输出读写、中断控制（内部可调用 IOMUX 原语）。
  - 仅通过 SoC 原语完成外设使能与引脚配置（或由 Board 层预配置）。
  - 对上暴露精简稳定的 C API，默认提供“最小便捷接口 + 可选配置化接口”。
- 引脚/时钟配置获取：
  - 推荐：驱动接受配置结构体或由 Board 提供 helper，例如 `S300_UART_Init(const s300_uart_cfg_t* cfg)`。
  - 轻量：驱动提供默认 pinmux（弱符号），Board 可覆写或在初始化前显式调用 SoC IOMUX/RCC 原语覆盖。
- 错误处理：
  - 非阻塞 API 返回错误码；
  - 轮询 API 具备合理超时；
  - 断言宏用于开发期（可由编译开关关闭）
- 中断：驱动只提供 ISR 入口与回调注册，默认弱符号，以应用层可选接管

UART API（示例，保持与现状兼容，同时提供配置化接口）

```c
void S300_UART_Init_115200(uint32_t idx);
void S300_UART_PutCharI(uint32_t idx, char c);
void S300_UART_PutStringI(uint32_t idx, const char* s);
```

后续可扩展配置化接口：

```c
typedef struct {
  uint32_t idx;      // UARTn
  uint32_t baud;     // 例：115200
  uint8_t  parity;   // 0:N,1:O,2:E
  uint8_t  stop;     // 1 or 2
  uint8_t  word_len; // 7/8/9 bits（按硬件支持）
  // 可选 DMA/中断配置
} s300_uart_cfg_t;
int S300_UART_Init(const s300_uart_cfg_t* cfg);
```

## 5. 板级层（Board）

- `Boards/BOARD_NAME/clock_config.c`：系统时钟树、外设时钟开关/复位组合（仅“组合”与“序列”，不在此定义寄存器原语）。
- `Boards/BOARD_NAME/pinmux_config.c`：引脚复用映射、上下拉等（调用 IOMUX 原语）。
- `board.h`：该板外设路由（如 `UART_DEBUG = 3` 与对应引脚）。
- 板级层不得直接读写寄存器，必须通过 SoC 原语，确保可移植性与一致性。

优点：同一套驱动在多板型间切换仅需替换 Board 层。

## 6. 构建与配置

- 构建系统：保持 Make（已有）并补充顶层 `Makefile.common` 供各示例复用；可选提供 CMake 以便 IDE 配置
- 配置：
  - 轻量：`config/` 下若干 `*.mk`/`*.h` 控制功能开关（LOG 开关、断言、优化级别）
  - 进阶：引入 Kconfig（如以后需要多配置组合）
- 产物：
  - 可选将 Drivers/Algorithms 编译为静态库（libdrivers.a, libmfcc.a），示例链接使用。
  - 统一提供 `compile_commands.json` 生成（便于 IDE 与静态检查）。

## 7. 编码规范与质量

- 统一格式：clang-format（Google 或 LLVM 风格小幅定制），提供 `.clang-format`
- 静态检查：clang-tidy 或 cppcheck（可选）
- 命名：
  - 驱动 `s300_<periph>_` 前缀；
  - Board 层 `Board_` 前缀；
  - 全局常量/宏使用大写下划线；
- 文档：每个驱动头文件顶部添加 API 简述与使用示例；RCC/IOMUX/GPIO 需列出“不可变寄存器序与值”的说明段，方便回归核对。

命名与路径示例（推荐约束）：

- 文件与目录：
  - 驱动文件：`Drivers/SoC/Source/s300_rcc.c`、`Drivers/UART/Source/s300_uart.c`、`Boards/<board>/pinmux_config.c`。
  - 头文件：`Drivers/SoC/Include/s300_rcc.h`、`Drivers/UART/Include/s300_uart.h`。
- 前缀与命名：
  - 驱动符号统一前缀：`s300_<periph>_`，如 `s300_rcc_init_pll()`、`s300_uart_init()`。
  - 内部静态函数使用下划线开头：`static int _pll_wait_lock(void);`。
- 头文件包含约束：
  - 驱动层仅包含 Device 头与必要 CMSIS 头：`#include "s300.h"`，禁止包含 Board 头或示例代码头。
  - 板级层仅通过 SoC 原语（RCC/IOMUX 等）做组合，禁止直接读写寄存器。
  - 外设驱动需要 pinmux/clock 时，优先依赖 Board 提供的 helper 或配置结构体，不在驱动中硬编码引脚。

示例：

```c
// Drivers/SoC/Include/s300_rcc.h
int s300_rcc_init_pll(uint16_t refdiv, uint16_t fbdiv, uint32_t frac,
                      uint16_t postdiv1, uint16_t postdiv2, uint32_t pll_sel);

// Boards/EVB/clock_config.c（仅组合与调用原语）
void Board_ClockInit(void) {
    // 调用 SoC 原语；此处不直接写寄存器
    (void)s300_rcc_init_pll(6, 768, 0, 2, 2, /*pll_sel=*/0);
}
```

## 8. 日志、断言与错误码

- `log.h`：轻量日志宏，默认路由到 UART（可编译关闭）
- `assert.h`：`S300_ASSERT(x)`，失败进入 `Error_Handler()`（可弱符号）
- 错误码：统一 `s300_status_t`（0 成功，负数错误）

## 9. 中断、DMA 与功耗

- 中断：向量表弱符号 + 驱动注册回调，应用可覆盖
- DMA：提供轻量 DMA 抽象（通道、请求源、回调），UART/I2S 等可选走 DMA
- 低功耗：预留 `s300_pm.h` 接口（clock gate, sleep/wakeup 钩子）。
  - SoC 层需提供 clock gate 列表与可达性检查，供 PM 统一管理。

## 10. 链接、启动与升级

- 链接脚本：`ld/` 下维护 `sram.ld` 与 `flash.ld` 两套；保持向量表可重定位
- 启动：`startup_S300.S` + `system_S300.c`（已完成）
- 升级：预留 `ota/boot` 目录规范（后续接入 bootloader 时使用）

## 11. 测试与 CI/CD

- 单元测试：算法与非硬件逻辑可用 host 构建（可选）
- 硬件测试：每个驱动一个最小示例（UART/I2C/SPI/I2S/DMA），自动化脚本可驱动串口比对
- CI：
  - 编译检查：arm-none-eabi-gcc 交叉编译
  - 样例构建产物与二进制大小统计
  - 可选：生成 `compile_commands.json` 供 IDE 使用

## 12. 版本与发布

- 语义化版本：`bsp/vX.Y.Z`
- 变更日志：`CHANGELOG.md`
- Release 工件：示例固件、静态库（可选）、头文件包

### 12.1 兼容策略（Compatibility Policy）

- 历史 API 兼容：
  - 在 `*_compat.h` 中提供内联函数或宏适配，映射到新 SoC 原语；适配层仅做别名，不改变寄存器写值与顺序。
  - 适配层内部统一调用新实现，避免双份逻辑。
- 退场与过渡：
  - 为历史 API 设定明确的“弃用周期”（例如保留两个小版本），在文档与编译日志中给出弃用提示宏（可通过宏开关隐藏）。
  - 在迁移指南中列出对应关系与替代用法，给出最小修改示例。
- 例：

```c
// Drivers/SoC/Include/s300_rcc_compat.h（可选）
#include "s300_rcc.h"

// 旧 API -> 新原语（仅示意，参数需与真实签名匹配）
#define init_cortex_m4_pll(refdiv, fbdiv, frac, post1, post2) \
    s300_rcc_init_pll((refdiv), (fbdiv), (frac), (post1), (post2), /*pll_sel*/0)
```

## 13. 现状到目标的迁移计划（建议两阶段）

- 阶段 1（1-2 天）
  - 保持现有示例可用。
  - 抽出 Drivers/SoC 骨架：`s300_rcc.{h,c}`, `s300_iomux.{h,c}`（UART 暂时通过 Board 层显式配置）。
  - 新增 Boards/BOARD_NAME/（把 UART3 的 pinmux/clock 组合迁移过去），BSP 对外接口 `Board_Init()` 统一调用。
  - 顶层 Makefile.common 与 .clang-format、.gitattributes。
  - 质量门（最小化）：交叉编译 PASS、最小示例串口输出正常、RCC/IOMUX 接口核对寄存器写值与顺序一致。
- 阶段 2（按需推进）
  - UART 切换到配置化接口；新增 I2C/SPI/GPIO/Timer 骨架。
  - 引入轻量日志与断言；完善错误码。
  - 拆分 MFCC 到 Algorithms/，新增 MFCC 示例工程。
  - 可选将 CMSIS-DSP 整理为外部子模块或本地编译库。
  - 引入基础 CI（编译与二进制大小统计），可选静态检查。

## 14. 与当前仓库差异与建议

- 你已完成：CMSIS Core vendor、SysTick 集中到 system、UART 驱动独立、.gitignore 引入
- 建议下一步（最小集）：

  1) 新建 Boards/BOARD_NAME/ 并迁移 UART3 pinmux/clock 组合（避免直接在示例中写寄存器）。
  2) 新建 Drivers/SoC/{rcc, iomux} 骨架；RCC 优先抽取“原语”并建立寄存器写序清单（只迁移，不改值）。
  3) 提供 Makefile.common 并让示例引用，减少重复。
  4) 约定提交规范（Angular + 中文 + emoji）与模块化 PR 粒度。

---

### A. RCC 迁移指引（附录）

- 目标：将旧 `cortex-m4-i2s/driver/rcc.{h,c}` 的寄存器定义与写序迁移为 `Drivers/SoC/{Include,Source}/s300_rcc.{h,c}`；外部示例改为调用 Board 层 `clock_config.c`，由其调用 SoC RCC 原语。
- 约束：
  - 不改变寄存器地址、位定义、写入值与顺序；如需更名，仅作用在 C 函数/枚举/宏名，不触达寄存器宏展开。
  - 若旧代码依赖 `register.h`，统一改为包含 `s300.h`，并在 RCC 头内做最小别名以兼容（必要时）。
- 建议 API：
  - `int s300_rcc_init_pll(uint16_t refdiv, uint16_t fbdiv, uint32_t frac, uint16_t postdiv1, uint16_t postdiv2, uint32_t pll_sel);`
  - `void s300_rcc_apb_clock_enable(uint32_t bus, uint32_t mask, bool en);`
  - `void s300_rcc_ahb_clock_enable(uint32_t mask, bool en);`
  - `void s300_rcc_reset_apb(uint32_t bus, uint32_t mask, bool assert_n);`
  - `uint32_t s300_rcc_get_clock(enum s300_clock_id id);`

> 注意：以上仅是“原语”示例，实际实现必须与硬件寄存器完全一致。若历史 API 需保留，可在 `s300_rcc_compat.h` 提供薄适配（宏或内联函数），但内部统一调用新原语。

### B. 质量门（Quality Gates）

- Build：交叉编译通过（无警告或仅文档型）。
- Lint/Typecheck：基础静态检查（可选）。
- Unit/Smoke：
  - RCC：PLL 初始化 + APB/AHB 门控 + Reset 读写烟测。
  - UART：默认 115200 输出“Hello”。
- 回归：寄存器写值与顺序对比旧版本保持一致（脚本或人工核对）。

### C. 提交规范（Angular + 中文 + emoji）

- 示例：`feat(rcc): 抽取 RCC 原语并迁移到 Drivers/SoC 🎛️`
- 说明包含：变更动机、目录映射、兼容性说明、验证方式、后续事项。

---

如认可该方案，我可以直接提交：

- Boards/BOARD_NAME/ 骨架文件
- Drivers/SoC/{rcc, iomux} 骨架
- 顶层 Makefile.common 与一个示例改造（UART_HelloWorld）
并保持现有功能与二进制行为不变。
