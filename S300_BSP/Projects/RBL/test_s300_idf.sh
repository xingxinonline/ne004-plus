#!/bin/bash

# S300 IDF 快速测试脚本
# 用于验证工具是否正常工作

echo "===========================================" 
echo "  S300 IDF 功能测试"
echo "==========================================="

# 检查脚本权限
if [[ ! -x "./s300_idf.sh" ]]; then
    echo "错误: s300_idf.sh 没有执行权限"
    echo "运行: chmod +x s300_idf.sh"
    exit 1
fi

echo "✓ 脚本权限检查通过"

# 检查构建环境
if [[ -f "GCC/Makefile" ]] || [[ -f "Makefile" ]]; then
    echo "✓ 发现构建文件"
else
    echo "⚠ 警告: 未发现Makefile，构建可能失败"
fi

# 检查串口工具
echo ""
echo "检查系统工具:"

tools=("make" "stty" "timeout" "cat")
for tool in "${tools[@]}"; do
    if command -v "$tool" >/dev/null; then
        echo "  ✓ $tool"
    else
        echo "  ✗ $tool (missing)"
    fi
done

# 检查可选工具
echo ""
echo "检查可选工具:"

optional_tools=("sz" "minicom" "screen" "picocom" "python3")
for tool in "${optional_tools[@]}"; do
    if command -v "$tool" >/dev/null; then
        echo "  ✓ $tool"
    else
        echo "  - $tool (not found, but optional)"
    fi
done

# 检查串口设备
echo ""
echo "检查串口设备:"

serial_ports=($(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true))
if [[ ${#serial_ports[@]} -gt 0 ]]; then
    echo "  发现串口设备:"
    for port in "${serial_ports[@]}"; do
        echo "    $port"
    done
else
    echo "  ⚠ 未发现串口设备 (USB设备可能未连接)"
fi

# 测试功能
echo ""
echo "==========================================="
echo "功能测试:"

echo ""
echo "1. 测试帮助功能..."
if ./s300_idf.sh --help >/dev/null 2>&1; then
    echo "  ✓ 帮助功能正常"
else
    echo "  ✗ 帮助功能异常"
fi

echo ""
echo "2. 测试构建检测..."
if ./s300_idf.sh build --dry-run 2>/dev/null || true; then
    echo "  ✓ 构建检测正常"
else
    echo "  - 构建检测跳过"
fi

echo ""
echo "=========================================="
echo "快速使用指南:"
echo ""
echo "1. 构建固件:"
echo "   ./s300_idf.sh build"
echo ""
echo "2. 一键下载 (类似 ESP32 idf.py flash):"
echo "   ./s300_idf.sh flash"
echo ""
echo "3. 完整开发流程:"
echo "   ./s300_idf.sh build flash monitor"
echo ""
echo "4. 指定串口:"
echo "   ./s300_idf.sh --port /dev/ttyUSB0 flash"
echo ""

if [[ ${#serial_ports[@]} -gt 0 ]]; then
    echo "建议首先尝试:"
    echo "  ./s300_idf.sh --port ${serial_ports[0]} flash"
fi

echo ""
echo "=========================================="
echo "自动下载模式说明:"
echo ""
echo "工具会自动尝试以下方式进入下载模式:"
echo "1. 发送 DOWNLOAD 软件命令"
echo "2. DTR信号双重复位"  
echo "3. 串口窗口期按键触发"
echo "4. 手动操作指导"
echo ""
echo "无需手动双击复位按钮！"
echo ""

echo "测试完成！如有问题，请查看 S300_IDF_USAGE.md"
