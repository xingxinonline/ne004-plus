# Simple Display Demo

不使用LVGL的简单显示Demo，通过直接操作显存实现画框功能。

## 功能特性

1. **图层初始化**
   - 图层颜色：全绿色 (RGB565: 0x07E0)
   - 透明度：0 (完全透明，显示底层摄像头画面)

2. **画框功能**
   - 通过改变特定区域的透明度实现矩形框绘制
   - 支持自定义边框宽度和透明度

3. **数据接收**
   - 支持通过邮箱接收DSP发送的人脸坐标
   - 支持串口命令控制

## 硬件限制

**重要：PSRAM仅支持16bit读写，不支持8bit读写**

因此Alpha通道的操作需要使用16bit读改写方式：
- 每16bit存储两个相邻像素的透明度值
- 修改单个像素需要先读取16bit，修改对应字节，再写回

## 串口命令

| 命令                    | 说明                                             | 示例                      |
| ----------------------- | ------------------------------------------------ | ------------------------- |
| `RECT x,y,w,h`          | 绘制矩形框（默认边框2像素，透明度0xC0）          | `RECT 50,50,80,100`       |
| `RECT x,y,w,h,bw,alpha` | 绘制矩形框（指定边框宽度和透明度）               | `RECT 50,50,80,100,3,200` |
| `CLEAR`                 | 清除所有矩形框                                   | `CLEAR`                   |
| `COLOR name`            | 设置图层颜色 (green/red/blue/white/black/yellow) | `COLOR red`               |
| `COLOR 0xXXXX`          | 设置图层颜色（RGB565十六进制）                   | `COLOR 0xF800`            |
| `ALPHA value`           | 设置全局透明度 (0-255)                           | `ALPHA 128`               |
| `REFRESH`               | 刷新显示                                         | `REFRESH`                 |
| `HELP`                  | 显示帮助信息                                     | `HELP`                    |

## 内存布局

```
PSRAM地址分布 (基于video.h定义):
- DISP_WFRAME0_ADDR: 0x80000000  (摄像头写入帧0)
- DISP_WFRAME1_ADDR: 0x80000000  (摄像头写入帧1)
- DISP_RFRAME0_ADDR: 0x80000000 + 2*W*H  (显示读取帧0 - 图层颜色)
- DISP_RFRAME1_ADDR: 0x80000000 + 4*W*H  (显示读取帧1 - 图层颜色)
- DISP_RALPHA0_ADDR: 0x80000000 + 6*W*H  (Alpha通道0)
- DISP_RALPHA1_ADDR: 0x80000000 + 6*W*H  (Alpha通道1)

其中 W=240, H=320 (屏幕分辨率)
```

## 编译方法

```bash
# 在S300_BSP目录下
cd /path/to/S300_BSP
mkdir -p build && cd build
cmake ..
make s300_simple_display_demo
```

## 调试方法

```bash
# 使用GDB调试
make dbg_simple_display

# 带DSP镜像调试
make dbg_simple_display_dsp
```

## 文件结构

```
Simple_Display_Demo/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 本文档
├── Inc/
│   ├── simple_display.h    # 显示模块头文件
│   ├── simple_uart_cmd.h   # 串口命令头文件
│   └── simple_mailbox_handler.h  # 邮箱处理头文件
├── Src/
│   ├── main.c              # 主程序
│   ├── simple_display.c    # 显示模块实现
│   ├── simple_uart_cmd.c   # 串口命令实现
│   └── simple_mailbox_handler.c  # 邮箱处理实现
└── ld/
    └── sram_demo.ld        # 链接脚本
```

## 与Display_Demo的区别

| 特性       | Display_Demo     | Simple_Display_Demo |
| ---------- | ---------------- | ------------------- |
| LVGL       | 使用LVGL v9.4    | 不使用              |
| 代码量     | 较大             | 较小                |
| 功能       | 眼睛动画、UI控件 | 仅画框              |
| 内存占用   | 较大             | 较小                |
| 编译时间   | 较长             | 较短                |
| 自定义难度 | 需了解LVGL       | 直接操作显存        |
