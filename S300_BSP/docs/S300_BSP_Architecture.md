# S300 CMSIS BSP 架构设计（兼容迁移优先 v1）

本文定义 S300 平台 BSP 的目标分层、目录组织、接口契约与兼容迁移策略，确保在早期保持旧 Demo 可用（保留旧接口/函数签名），并逐步将寄存器/基地址/位定义迁移至 CMSIS Device 头或 regs 专属文件，降低一次性重构带来的调试成本。

## 1. 设计目标与原则

- 兼容迁移优先：旧对外 API/函数名/错误码尽量不变；通过 compat/shim 适配到新 HAL/Driver。
- CMSIS 设备化：所有寄存器、基地址与位定义集中到 Device 头（或 regs/*.h），实现文件禁止魔法数。
- 清晰分层：Core/Device → Drivers(SoC/Periph) → Boards → Middleware → Algorithms → Projects/Examples。
- 可测试：每步提交可编译、可下载、可打印；提供最小示例与寄存器等价性检查清单。
- 可扩展：统一命名、错误码与构建开关（BOARD、LEGACY_API），支持多板差异配置。

## 2. 目录分层与职责

目标放置于仓库现有的 `S300_BSP/` 下（与 `BSP_Productization.md` 一致），建议结构：

```text
S300_BSP/
  CMSIS/
    Core/Include/                     # CMSIS Core（vendor）
    Device/PiMCHIP/S300/
      Include/                        # s300.h（寄存器、基地址、IRQn 等）
      Source/                         # system_S300.c, startup_S300.S, ld/
  Drivers/
    SoC/                              # 无板级耦合的底层原语
      Include/
        s300_rcc.h                    # 时钟/复位/门控/分频/选择器
        s300_iomux.h                  # 复用/上下拉/驱动能力
      Source/
        s300_rcc.c
        s300_iomux.c
    GPIO/
      Include/s300_gpio.h
      Source/s300_gpio.c
    UART/
      Include/s300_uart.h
      Source/s300_uart.c
    DMA/ I2C/ SPI/ TIMER/ WDG/ ...    # 同构
    compat/                           # 旧 API 适配层（薄封装）
      Include/ *.h
      Source/  *.c
  Boards/
    generic_evb/
      board.h                         # 本板外设路由（如 UART_DEBUG=3）
      clock_config.c                  # 仅“组合”与“序列”，调用 SoC 原语
      pinmux_config.c                 # 仅“组合”，调用 IOMUX 原语
    <other_board>/ ...
  Projects/
    Examples/
      UART_HelloWorld/
        GCC/ Src/
      I2S_Loopback/
        GCC/ Src/
  tools/
    scripts/                          # 打包、检查、尺寸统计
  docs/
    BSP_Productization.md
    S300_BSP_Architecture.md          # 本文
```

关键约束：

- 驱动层仅包含 `s300.h` 与必要 CMSIS 头；严禁包含板级头或示例头。
- 板级层只能调用 SoC 原语（RCC/IOMUX 等）做“组合”；不得直接读写寄存器。
- 示例不得直接写寄存器做 pinmux/clock，必须走板级 API（极少数教学型示例需显著标注）。

## 3. 接口契约（缩略草案）

状态码与断言：

```c
typedef int32_t s300_status_t;  // 0=OK, <0=ERR
```

RCC 原语（示例）：

```c
int s300_rcc_init_pll(uint16_t refdiv, uint16_t fbdiv, uint32_t frac,
                      uint16_t postdiv1, uint16_t postdiv2, uint32_t pll_sel);
void s300_rcc_apb_clock_enable(uint32_t bus, uint32_t mask, bool en);
void s300_rcc_ahb_clock_enable(uint32_t mask, bool en);
void s300_rcc_reset_apb(uint32_t bus, uint32_t mask, bool assert_n);
uint32_t s300_rcc_get_clock(uint32_t id);
```

IOMUX 原语（示例）：

```c
int s300_iomux_set_func(uint32_t port, uint32_t pin, uint32_t func);
int s300_iomux_set_pull(uint32_t port, uint32_t pin, uint32_t pull);
int s300_iomux_set_drv(uint32_t port, uint32_t pin, uint32_t drv);
```

GPIO（最小）：

```c
int s300_gpio_init(uint32_t port, uint32_t pin, uint32_t dir);
int s300_gpio_write(uint32_t port, uint32_t pin, uint32_t val);
int s300_gpio_read(uint32_t port, uint32_t pin, uint32_t* val);
```

UART（便捷 + 配置化）：

```c
void S300_UART_Init_115200(uint32_t idx);          // 旧 API 兼容（便捷）
void S300_UART_PutCharI(uint32_t idx, char c);
void S300_UART_PutStringI(uint32_t idx, const char* s);

typedef struct {
  uint32_t idx; uint32_t baud; uint8_t parity; uint8_t stop; uint8_t word_len;
} s300_uart_cfg_t;
int S300_UART_Init(const s300_uart_cfg_t* cfg);     // 新 API（配置化）
```

DMA（占位）：

```c
typedef void (*s300_dma_cb_t)(void* user, int event);
int s300_dma_config(uint32_t ch, /* src/dst/len/req 等 */);
int s300_dma_start(uint32_t ch, s300_dma_cb_t cb, void* user);
int s300_dma_stop(uint32_t ch);
```

以上为缩略草案；实际以硬件手册与现有 Demo 行为对齐为准。

## 4. 兼容层策略（compat/shim）

- 构建开关：`LEGACY_API=1|0`（早期默认 1）。
- 旧 API 名称维持不变；在 `Drivers/compat/` 中以宏/内联或薄封装映射到新实现。
- 行为语义与返回值保持一致；必要差异需在编译日志与文档标注。
- 日志路径可加前缀区分 `[LEGACY]` vs `[NEW]` 以便灰度比对。

## 5. 寄存器定义归位（CMSIS Device / regs）

- 统一集中在 `CMSIS/Device/PiMCHIP/S300/Include/s300.h`（推荐）。
- 如需分拆，可在 `Drivers/SoC/Include/regs/xxx_regs.h` 做二级拆分，但仍由 `s300.h` 统一包含入口。
- 驱动/板级/示例实现文件禁止出现魔法数寄存器地址与位定义。
- 提供寄存器等价性检查清单：地址/偏移/位/复位值；必要时用 `_Static_assert` 做编译期校验。

## 6. 构建与路径约定

- Make 变量：`BOARD=<name>`，`LEGACY_API=1|0`，`BUILD_TYPE=Debug|Release`。
- Include 顺序：CMSIS → Drivers/SoC → Drivers/Periph/Include → Boards/Board → Projects/Examples（以示意替代尖括号）。
- 产物：可选生成 `compile_commands.json`；工具脚本输出二进制尺寸统计。

## 7. 迁移路线（两阶段建议）

阶段 1（最小闭环，保持现状可用）：

1) 抽出 SoC 原语：`s300_rcc.{h,c}`、`s300_iomux.{h,c}`（仅迁移，不改值与顺序）。
2) 新建 `Boards/generic_evb/`：把现有 UART3 的 pinmux/clock 组合迁移到 `pinmux_config.c/clock_config.c`。
3) compat：为 UART 提供最小 shim，旧 Demo 无需改动即可链接运行。
4) 顶层 Make 参入 `LEGACY_API` 开关；串口心跳打印可加 `[LEGACY]` 标记。

阶段 2（扩展与清理）：

1) UART 提供配置化接口；逐步为 GPIO/DMA/I2S 提供骨架与示例。
2) 统一错误码、日志与断言；完善头文件注释与用法示例。
3) 选取 1-2 个应用板，完善 `Boards/Board/` 配置与差异说明（以示意替代尖括号）。
4) 回归清单：寄存器写值/顺序对比、串口/中断/DMA 烟测。

## 8. 质量门（Quality Gates）

- Build：交叉编译通过（禁致命警告）。
- Smoke：Minimal/HelloWorld 串口输出；关键外设的最小用例（UART/GPIO/DMA/I2S）。
- 等价性：寄存器写值与顺序与旧 Demo 对齐（脚本或人工核对）。
- 样式：遵循 `docs/coding_style_en.md` 与 astyle（Allman/4 空格/pad-oper/header 等）。

## 9. 命名与规范

- 驱动前缀：`s300_<periph>_`；Board 前缀：`Board_`。
- 仅在驱动层包含 `s300.h`；板级与示例禁止直写寄存器。
- 头文件顶部提供“简要 API 说明 + 示例 + 常见坑”。

---

如需落地实现，我方可按本文生成：

1) SoC 原语骨架（RCC/IOMUX）
2) Boards/generic_evb/ 最小 pinmux/clock 组合
3) UART compat shim 与最小示例（不影响现有 Demo）
4) Make 开关（LEGACY_API）与尺寸统计脚本
