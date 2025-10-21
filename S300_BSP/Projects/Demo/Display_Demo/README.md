# Display_Demo + LVGL v9.4 集成

本 demo 将 LVGL v9.4 集成到 S300 BSP 的显示路径，使用 MM 子系统作为渲染输出，双缓冲 RGB565 全屏渲染。

## 获取 LVGL 源码（离线可选）

默认通过 CMake FetchContent 在线拉取 `https://github.com/lvgl/lvgl.git` `v9.4.0` 标签。如果网络不可用，有两种离线方式：

1. 供应本地路径：
   - 将 LVGL 源码克隆到本地：
     - `git clone --depth=1 --branch v9.4.0 https://github.com/lvgl/lvgl.git /path/to/lvgl`
   - 配置时指定：`-DLVGL_LOCAL_PATH=/path/to/lvgl`

2. vendoring（把源码放到仓库目录）：
   - 将 LVGL 源码放入：`Projects/Demo/Display_Demo/third_party/lvgl/lvgl/`，该目录包含官方 `CMakeLists.txt`。

如果你选择方式 (2)，则无需设置 `LVGL_LOCAL_PATH`，CMake 会优先使用 vendor 目录。

## 配置选项

- `LV_CONF_PATH`：默认已指向 `third_party/lvgl/lv_conf.h`。
- `LVGL_LOCAL_PATH`：离线本地 LVGL 路径（可选）。

## 运行

- 构建：
  - `cmake -S S300_BSP -B S300_BSP/build`
  - `cmake --build S300_BSP/build --target s300_display_demo -- -j`
- 烧录与调试可参见仓库根部文档与 `dbg_display` 目标。

## 说明

- SysTick 以 1ms 调用 `lv_tick_inc(1)`。
- LVGL 使用双缓冲，缓冲地址直接映射到硬件帧缓存；`flush_cb` 将对应缓冲标记为 Ready 供显示控制器翻转。
- 默认分辨率 128x160，可按硬件调整 `video.h` 的 `DISP_IMAGE_WIDTH/HEIGHT`。
