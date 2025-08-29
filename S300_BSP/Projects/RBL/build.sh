#!/bin/bash

# S300 RBL 快速构建脚本
# Quick build script for S300 RBL project

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查必要的工具
check_tools() {
    print_info "检查构建工具..."
    
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        print_error "arm-none-eabi-gcc 未找到!"
        print_info "请安装ARM GCC工具链"
        exit 1
    fi
    
    if ! command -v python3 &> /dev/null; then
        print_error "python3 未找到!"
        exit 1
    fi
    
    print_success "构建工具检查完成"
}

# 显示项目信息
show_project_info() {
    print_info "S300 RBL 项目信息"
    echo "=================================="
    echo "目标芯片: S300 PiMCHIP (Cortex-M4)"
    echo "功能特性: Ymodem协议下载支持"
    echo "Flash大小: 16MB (W25Q128)"
    echo "SRAM大小: 256KB"
    echo "串口通信: UART3 @ 115200bps"
    echo "=================================="
}

# 清理构建文件
clean_build() {
    print_info "清理构建文件..."
    cd /home/xinhao/work/ne004-plus/S300_BSP/Projects/RBL
    make clean > /dev/null 2>&1
    print_success "构建文件已清理"
}

# 执行构建
do_build() {
    print_info "开始构建 S300 RBL..."
    cd /home/xinhao/work/ne004-plus/S300_BSP/Projects/RBL
    
    # 执行构建
    if make s300_image; then
        print_success "构建成功!"
        
        # 显示构建结果
        if [ -f "GCC/rbl.bin" ]; then
            size=$(stat -c%s "GCC/rbl.bin")
            print_info "RBL二进制文件大小: ${size} bytes ($(echo "scale=1; $size/1024" | bc)KB)"
        fi
        
        if [ -f "s300_rbl_complete.bin" ]; then
            size=$(stat -c%s "s300_rbl_complete.bin")
            print_info "完整镜像大小: ${size} bytes ($(echo "scale=1; $size/1024" | bc)KB)"
        fi
        
        return 0
    else
        print_error "构建失败!"
        return 1
    fi
}

# 显示使用方法
show_usage() {
    print_info "S300 RBL 使用方法"
    echo "=================================="
    echo "1. 通过串口连接S300开发板"
    echo "2. 设置串口参数: 115200,8,N,1"
    echo "3. 复位芯片并在3秒内按任意键进入下载模式"
    echo "4. 使用以下命令:"
    echo "   INFO     - 显示芯片信息"
    echo "   YMODEM   - 启动Ymodem文件接收"
    echo "   ERASE    - 擦除应用程序区域"
    echo "   QUIT     - 退出下载模式"
    echo "=================================="
    echo "Ymodem下载步骤:"
    echo "1. 发送 'YMODEM' 命令"
    echo "2. 在串口工具中选择 '发送文件 - Ymodem'"
    echo "3. 选择要下载的.bin文件"
    echo "4. 等待传输完成"
    echo "=================================="
}

# 检查Python依赖
check_python_deps() {
    print_info "检查Python依赖..."
    
    if ! python3 -c "import serial" &> /dev/null; then
        print_warning "pyserial 未安装"
        print_info "安装命令: pip3 install pyserial"
    else
        print_success "Python依赖检查完成"
    fi
}

# 主函数
main() {
    echo
    print_info "S300 RBL 快速构建脚本"
    echo "======================================"
    
    # 解析命令行参数
    case "${1:-build}" in
        "clean")
            clean_build
            ;;
        "build")
            check_tools
            show_project_info
            clean_build
            do_build
            show_usage
            ;;
        "info")
            show_project_info
            show_usage
            ;;
        "deps")
            check_tools
            check_python_deps
            ;;
        "test")
            check_python_deps
            print_info "运行Ymodem测试脚本..."
            print_info "用法: python3 ymodem_test.py <串口设备>"
            print_info "示例: python3 ymodem_test.py /dev/ttyUSB0"
            ;;
        *)
            echo "用法: $0 [命令]"
            echo "命令:"
            echo "  build  - 构建项目 (默认)"
            echo "  clean  - 清理构建文件"
            echo "  info   - 显示项目信息"
            echo "  deps   - 检查依赖"
            echo "  test   - 测试说明"
            exit 1
            ;;
    esac
    
    echo
    print_success "操作完成!"
}

# 执行主函数
main "$@"
