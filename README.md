# NE004-Plus 项目

PiMCHIP S300 系列芯片的 BSP 和应用开发框架

## 📁 项目结构

```text
ne004-plus/
├── S300_BSP/              # S300 板级支持包
│   ├── CMSIS/            # ARM CMSIS 标准库
│   ├── Drivers/           # 外设驱动
│   └── Projects/          # 示例项目（含 Display_Demo）
├── docs/                  # 项目文档
├── CONTRIBUTING.md        # 贡献指南
└── README.md             # 项目说明（本文件）
```

## 🧰 跨平台开发环境搭建（Linux / Windows / macOS）

仅推荐“手动搭建”开发环境，以确保对工具链与依赖的可控性与可重复性。

提示：本仓库已启用 Git LFS 跟踪 DSP 镜像（二进制）。首次在本机使用前请执行一次：

```bash
git lfs install
```

### 必备依赖（按操作系统）

- 通用
  - Git、CMake(≥3.22)、Ninja(≥1.10)
  - ARM GNU Toolchain（arm-none-eabi-gcc、arm-none-eabi-gdb、arm-none-eabi-objcopy/objdump）
  - GDB 服务器（任选其一）：OpenOCD 或 J-Link GDB Server
  - Python 3.8+ 与包：`pyserial`, `requests`, `click`, `rich`

- Linux（Debian/Ubuntu 示例）
  - CMake/Ninja：`sudo apt install cmake ninja-build`
  - 工具链：从 Arm 官网安装或包管理安装 `gcc-arm-none-eabi`
  - GDB 服务器：安装 OpenOCD（或使用 JLinkGDBServer）
  - 串口权限：确保用户属于 `dialout` 组

- Windows
  - 安装包：建议使用 Scoop 或官方安装包安装 CMake、Ninja、Python、uv、ARM GNU Toolchain
  - 调试：安装 J-Link（含 J-Link GDB Server）或 OpenOCD

- macOS（Homebrew）
  - `brew install cmake ninja arm-none-eabi-gcc openocd python uv`

> 工具链路径：建议将 `arm-none-eabi-*` 加入 PATH；或在生成构建时通过 CMake 显式指定编译器以覆盖仓库默认路径（见下文）。
>
> 工具脚本（如软件复位/OTA）位于 `S300_BSP/tools/`，可通过 Python/uv 手动运行；不再提供一键安装脚本。

## 🚀 以「显示 Demo」为例：编译、下载与调试

显示 Demo 位于 `S300_BSP/Projects/Demo/Display_Demo/`，已集成 LVGL v9.4（支持在线拉取或离线本地源）。

### 1）准备仓库

```bash
git clone https://github.com/xingxinonline/ne004-plus.git
cd ne004-plus
```

可选（LVGL 离线构建）：

- 方式 A：本地 LVGL 源码路径，配置时添加 `-DLVGL_LOCAL_PATH=/path/to/lvgl`
- 方式 B：将源码放到 `S300_BSP/Projects/Demo/Display_Demo/third_party/lvgl/lvgl/`

更多细节见 `S300_BSP/Projects/Demo/Display_Demo/README.md`。

### 2）配置与构建（CMake + Ninja）

首次配置时，建议确保工具链可用：

- 若 `arm-none-eabi-gcc` 已在 PATH：

```bash
cmake -S S300_BSP -B S300_BSP/build -G Ninja
```

- 若需显式指定编译器（覆盖仓库中默认的主机特定路径）：

```bash
cmake -S S300_BSP -B S300_BSP/build -G Ninja \
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ \
  -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc
```

构建显示 Demo：

```bash
cmake --build S300_BSP/build --target s300_display_demo -- -j
```

产物位置（示例）：`S300_BSP/build/Projects/Demo/Display_Demo/` 下包含 `s300_display_demo.elf/.bin/.hex/.dis`。

### 3）下载/烧录（可选）

如使用串口下载到设备（RBL/SBL 下载模式）：

```bash
# 自动检测串口或手动指定 -p
uv run python S300_BSP/tools/s300_download.py -f S300_BSP/build/Projects/Demo/Display_Demo/s300_display_demo.bin
```

> 如果已在当前目录执行过 `uv sync` 安装工具依赖，也可直接使用 console_scripts：`uv run s300-ota`；否则可使用 `uv run python S300_BSP/tools/s300_ota_tool.py ...`（详见 `S300_BSP/tools/README.md`）。

### 4）调试（GDB）

先启动 GDB 服务器（两种常见方式，端口建议 3333 以匹配仓库 gdbinit）：

- OpenOCD：请使用适配 S300 的配置文件启动，并监听 `:3333`
- J-Link：`JLinkGDBServer -if SWD -device Cortex-M4 -speed 4000 -port 3333`

随后，一键启动调试目标（会自动 `load` 到 SRAM 0x20000000 并设置 VTOR；默认端口 3333，可在 CMake 配置时通过 `-DGDB_PORT=3334` 修改）：

```bash
# 基本调试（不加载 DSP boot 镜像）
ninja -C S300_BSP/build dbg_display

# 含 DSP boot 镜像预加载（需要准备 Display_Demo/DSP_Images/ 下的 bin）
ninja -C S300_BSP/build dbg_display_dsp
```

调试脚本位于：

- `S300_BSP/gdbinit.display.gdb`
- `S300_BSP/gdbinit.display.dsp.gdb`

脚本会：

- 连接到 `:3333` 的 GDB 服务器并复位/停机 Cortex-M4 内核
- `load` ELF 至 SRAM（0x20000000），设置 SP/PC 与 VTOR
- 可选加载 DSP 引导镜像（`Projects/Demo/Display_Demo/DSP_Images/`）

> DSP 引导镜像的放置与分发方式详见 `S300_BSP/Projects/Demo/Display_Demo/DSP_Images/README.md`（推荐使用 Git LFS 或压缩包离线分发）。

### 常见问题（FAQ）

- CMake 找不到编译器：使用上面的显式编译器参数覆盖，或将 `arm-none-eabi-gcc` 加入 PATH。
- LVGL 无法联网拉取：按上文使用 `-DLVGL_LOCAL_PATH=...` 或将源码放入 vendor 目录。
- Linux 串口权限：将用户加入 `dialout` 组或使用 `sudo`，安装脚本已提供指引。
- GDB 无法连接：确认 GDB 服务器已运行并监听 `:3333`，线路（SWD/JTAG）与复位策略配置正确。

## 📚 文档

- [S300 BSP 文档（子仓库说明）](S300_BSP/README.md)
- [Display_Demo 说明（LVGL 离线等）](S300_BSP/Projects/Demo/Display_Demo/README.md)
- [贡献指南](CONTRIBUTING.md)
- 其他架构/指南见 `docs/`

## 🎬 效果视频

- 演示/效果视频位于 `S300_BSP/docs/media/`
- 例如： [演示视频 2025-10-22](S300_BSP/docs/media/41fe888859011a12e97288f477049cbd.mp4)

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！提交前请阅读 [贡献指南](CONTRIBUTING.md)。

## 📄 许可证

本项目采用 MIT 许可证。
