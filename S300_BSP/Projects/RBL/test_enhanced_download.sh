#!/bin/bash

# S300 RBL 增强下载功能测试脚本
# 版本: 1.0
# 日期: 2025-08-30

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
SERIAL_PORT="/dev/ttyUSB0"
BAUD_RATE="115200"
BUILD_DIR="GCC/build"
BINARY_NAME="rbl.bin"

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  S300 RBL 增强下载功能测试工具${NC}"
echo -e "${BLUE}======================================${NC}"

# 函数定义

print_usage() {
    echo "使用方法: $0 [选项] [命令]"
    echo ""
    echo "命令:"
    echo "  build          - 构建RBL固件"
    echo "  test-serial    - 测试串口下载窗口"
    echo "  test-software  - 测试软件触发下载"
    echo "  test-double    - 测试双重启下载"
    echo "  flash          - 烧录固件"
    echo "  monitor        - 监控串口输出"
    echo ""
    echo "选项:"
    echo "  -p PORT        - 指定串口设备 (默认: $SERIAL_PORT)"
    echo "  -b BAUD        - 指定波特率 (默认: $BAUD_RATE)"
    echo "  -h             - 显示帮助信息"
}

build_firmware() {
    echo -e "${YELLOW}[INFO] 构建RBL固件...${NC}"
    
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    cd GCC
    make clean
    make -j$(nproc)
    
    if [ -f "$BUILD_DIR/$BINARY_NAME" ]; then
        echo -e "${GREEN}[SUCCESS] 固件构建成功: $BUILD_DIR/$BINARY_NAME${NC}"
        ls -la "$BUILD_DIR/$BINARY_NAME"
    else
        echo -e "${RED}[ERROR] 固件构建失败${NC}"
        exit 1
    fi
}

test_serial_download() {
    echo -e "${YELLOW}[INFO] 测试串口下载窗口...${NC}"
    echo "请按以下步骤操作:"
    echo "1. 重启设备"
    echo "2. 在5秒倒计时内按任意键"
    echo "3. 观察是否进入下载模式"
    echo ""
    echo "按回车开始监控串口..."
    read
    
    monitor_serial
}

test_software_download() {
    echo -e "${YELLOW}[INFO] 测试软件触发下载...${NC}"
    echo "请在应用程序中发送以下命令之一:"
    echo "  DOWNLOAD  - 触发用户下载模式"
    echo "  RECOVERY  - 触发恢复下载模式"
    echo "  CRASH     - 模拟崩溃触发下载"
    echo ""
    echo "按回车开始监控串口..."
    read
    
    monitor_serial
}

test_double_reset() {
    echo -e "${YELLOW}[INFO] 测试双重启下载...${NC}"
    echo "请按以下步骤操作:"
    echo "1. 按一次复位按钮"
    echo "2. 在3秒内再次按复位按钮"
    echo "3. 观察是否进入下载模式"
    echo ""
    echo "按回车开始监控串口..."
    read
    
    monitor_serial
}

flash_firmware() {
    echo -e "${YELLOW}[INFO] 烧录固件到设备...${NC}"
    
    if [ ! -f "$BUILD_DIR/$BINARY_NAME" ]; then
        echo -e "${RED}[ERROR] 固件文件不存在，请先构建${NC}"
        exit 1
    fi
    
    # 检查是否有烧录脚本
    if [ -f "./flash_program.sh" ]; then
        echo "使用项目烧录脚本..."
        ./flash_program.sh program
    else
        echo "使用openocd烧录..."
        # 这里需要根据实际硬件调整
        echo -e "${YELLOW}[WARNING] 请手动烧录固件: $BUILD_DIR/$BINARY_NAME${NC}"
    fi
}

monitor_serial() {
    echo -e "${YELLOW}[INFO] 监控串口: $SERIAL_PORT @ $BAUD_RATE${NC}"
    echo "按 Ctrl+C 退出监控"
    echo ""
    
    # 检查串口是否存在
    if [ ! -e "$SERIAL_PORT" ]; then
        echo -e "${RED}[ERROR] 串口设备不存在: $SERIAL_PORT${NC}"
        echo "可用串口设备:"
        ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "没有找到串口设备"
        exit 1
    fi
    
    # 使用minicom或其他工具监控
    if command -v minicom >/dev/null 2>&1; then
        minicom -D "$SERIAL_PORT" -b "$BAUD_RATE"
    elif command -v screen >/dev/null 2>&1; then
        screen "$SERIAL_PORT" "$BAUD_RATE"
    elif command -v picocom >/dev/null 2>&1; then
        picocom "$SERIAL_PORT" -b "$BAUD_RATE"
    else
        echo -e "${RED}[ERROR] 需要安装 minicom, screen 或 picocom${NC}"
        exit 1
    fi
}

send_test_commands() {
    echo -e "${YELLOW}[INFO] 发送测试命令到设备...${NC}"
    
    if ! command -v echo >/dev/null 2>&1; then
        echo -e "${RED}[ERROR] 系统命令不可用${NC}"
        exit 1
    fi
    
    echo "发送命令: DOWNLOAD"
    echo "DOWNLOAD" > "$SERIAL_PORT"
    sleep 2
    
    echo "发送命令: STATUS"
    echo "STATUS" > "$SERIAL_PORT"
    sleep 1
}

run_comprehensive_test() {
    echo -e "${YELLOW}[INFO] 运行综合测试...${NC}"
    
    echo "1. 构建固件..."
    build_firmware
    
    echo "2. 烧录固件..."
    flash_firmware
    
    echo "3. 等待设备启动..."
    sleep 3
    
    echo "4. 开始测试序列..."
    echo "请按提示进行各项测试"
    
    test_serial_download
    test_software_download
    test_double_reset
    
    echo -e "${GREEN}[SUCCESS] 综合测试完成${NC}"
}

# 主程序

# 解析命令行参数
while getopts "p:b:h" opt; do
    case $opt in
        p)
            SERIAL_PORT="$OPTARG"
            ;;
        b)
            BAUD_RATE="$OPTARG"
            ;;
        h)
            print_usage
            exit 0
            ;;
        \?)
            echo -e "${RED}[ERROR] 无效选项: -$OPTARG${NC}" >&2
            print_usage
            exit 1
            ;;
    esac
done

shift $((OPTIND-1))

# 检查命令
if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

COMMAND="$1"

echo "配置:"
echo "  串口: $SERIAL_PORT"
echo "  波特率: $BAUD_RATE"
echo "  命令: $COMMAND"
echo ""

# 执行命令
case "$COMMAND" in
    build)
        build_firmware
        ;;
    test-serial)
        test_serial_download
        ;;
    test-software)
        test_software_download
        ;;
    test-double)
        test_double_reset
        ;;
    flash)
        flash_firmware
        ;;
    monitor)
        monitor_serial
        ;;
    test-all)
        run_comprehensive_test
        ;;
    send-commands)
        send_test_commands
        ;;
    *)
        echo -e "${RED}[ERROR] 未知命令: $COMMAND${NC}"
        print_usage
        exit 1
        ;;
esac

echo -e "${GREEN}[INFO] 操作完成${NC}"
