#!/bin/bash
# S300 RBL 自动下载脚本

set -e

# 配置参数
SERIAL_PORT="/dev/ttyUSB0"  # 根据你的串口设备调整
FIRMWARE_FILE="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/rbl_download.py"

# 检查参数
if [ $# -ne 1 ]; then
    echo "用法: $0 <固件文件>"
    echo "示例: $0 ../GCC/build/s300_rbl_simple.bin"
    exit 1
fi

# 检查文件是否存在
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "❌ 固件文件不存在: $FIRMWARE_FILE"
    exit 1
fi

# 检查串口是否存在
if [ ! -e "$SERIAL_PORT" ]; then
    echo "❌ 串口设备不存在: $SERIAL_PORT"
    echo "请检查设备连接或修改脚本中的SERIAL_PORT变量"
    exit 1
fi

# 检查Python脚本
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "❌ Python下载脚本不存在: $PYTHON_SCRIPT"
    exit 1
fi

# 显示信息
echo "🚀 S300 RBL 自动下载工具"
echo "======================="
echo "串口设备: $SERIAL_PORT"
echo "固件文件: $FIRMWARE_FILE"
echo "文件大小: $(stat -c%s "$FIRMWARE_FILE") 字节"
echo ""

# 检查Python依赖
if ! python3 -c "import serial" 2>/dev/null; then
    echo "❌ 缺少pyserial库，请安装:"
    echo "   sudo apt install python3-serial"
    echo "   或者: pip3 install pyserial"
    exit 1
fi

# 运行下载脚本
echo "📱 请复位你的设备，然后按任意键开始监控..."
read -n 1

echo "🔄 开始监控串口并等待下载模式..."
python3 "$PYTHON_SCRIPT" "$SERIAL_PORT" "$FIRMWARE_FILE"
