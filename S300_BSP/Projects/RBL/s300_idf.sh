#!/bin/bash

# S300 IDF - ESP32风格的一键下载工具
# 版本: 1.0
# 日期: 2025-08-30
# 
# 使用方法:
#   ./s300_idf.sh build                     # 构建固件
#   ./s300_idf.sh flash                     # 自动下载固件
#   ./s300_idf.sh monitor                   # 监控串口
#   ./s300_idf.sh build flash monitor       # 完整开发流程
#   ./s300_idf.sh --port /dev/ttyUSB0 flash # 指定串口下载

set -e

# 配置常量
DEFAULT_PORT=""
DEFAULT_BAUD="115200"
BUILD_DIR="GCC/build"
FIRMWARE_NAME="rbl.bin"
CONFIG_FILE=".s300_idf_config"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# 输出函数
error() {
    echo -e "${RED}ERROR: $1${NC}" >&2
}

success() {
    echo -e "${GREEN}SUCCESS: $1${NC}"
}

info() {
    echo -e "${BLUE}INFO: $1${NC}"
}

warning() {
    echo -e "${YELLOW}WARNING: $1${NC}"
}

header() {
    echo -e "${CYAN}$1${NC}"
}

# 帮助信息
show_help() {
    cat << EOF
S300 IDF - ESP32风格开发工具 v1.0

使用方法:
    $0 [选项] <命令> [命令...]

命令:
    build                   构建固件
    flash                   烧录固件到设备  
    monitor                 监控串口输出
    clean                   清理构建文件
    
    组合使用:
    build flash             构建并烧录
    build flash monitor     完整开发流程
    flash monitor           烧录并监控

选项:
    --port, -p PORT        指定串口设备
    --baud, -b BAUD        指定波特率 (默认: $DEFAULT_BAUD)
    --help, -h             显示帮助信息

示例:
    $0 build                              # 构建固件
    $0 flash                              # 自动检测串口并烧录
    $0 --port /dev/ttyUSB0 flash         # 指定串口烧录
    $0 build flash monitor               # 完整开发流程
    $0 monitor                           # 仅监控串口

注意:
    - 首次使用会自动检测并保存串口配置
    - 支持自动触发下载模式，无需手动操作
    - 兼容多种串口工具：sz/rz, minicom, screen等
EOF
}

# 加载配置
load_config() {
    if [[ -f "$CONFIG_FILE" ]]; then
        source "$CONFIG_FILE"
        if [[ -n "$SAVED_PORT" ]]; then
            DEFAULT_PORT="$SAVED_PORT"
        fi
    fi
}

# 保存配置
save_config() {
    cat > "$CONFIG_FILE" << EOF
# S300 IDF 配置文件
SAVED_PORT="$1"
SAVED_BAUD="$2"
EOF
    info "Configuration saved: port=$1, baud=$2"
}

# 查找串口设备
find_serial_ports() {
    local ports=()
    
    # 查找USB串口设备
    for pattern in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.usbmodem*; do
        if ls $pattern >/dev/null 2>&1; then
            ports+=($(ls $pattern))
        fi
    done
    
    printf '%s\n' "${ports[@]}"
}

# 自动检测串口
auto_detect_port() {
    local ports=($(find_serial_ports))
    
    if [[ ${#ports[@]} -eq 0 ]]; then
        error "No serial ports found"
        return 1
    fi
    
    info "Found serial port(s): ${ports[*]}"
    
    if [[ ${#ports[@]} -eq 1 ]]; then
        echo "${ports[0]}"
        return 0
    fi
    
    # 多个串口时，优先选择USB串口
    for port in "${ports[@]}"; do
        if [[ "$port" =~ USB|ACM ]]; then
            echo "$port"
            return 0
        fi
    done
    
    # 返回第一个
    echo "${ports[0]}"
}

# 测试串口连接
test_port() {
    local port="$1"
    local baud="$2"
    
    if command -v stty >/dev/null; then
        if stty -F "$port" "$baud" >/dev/null 2>&1; then
            return 0
        fi
    fi
    
    return 1
}

# 发送下载触发信号
trigger_download_mode() {
    local port="$1"
    local baud="$2"
    
    info "Triggering download mode on $port..."
    
    # 方法1: 发送软件命令
    if command -v echo >/dev/null; then
        info "Sending software download command..."
        stty -F "$port" "$baud" raw -echo
        echo -ne "DOWNLOAD\r\n" > "$port"
        sleep 1
        
        # 检查响应
        if timeout 2 cat "$port" | grep -i "download" >/dev/null 2>&1; then
            success "Software download trigger successful"
            return 0
        fi
    fi
    
    # 方法2: 使用DTR信号模拟复位（如果支持）
    if command -v python3 >/dev/null; then
        info "Attempting DTR reset sequence..."
        python3 -c "
import sys
try:
    import serial
    import time
    ser = serial.Serial('$port', $baud, timeout=1)
    # 双重复位序列
    for _ in range(2):
        ser.dtr = True
        time.sleep(0.1)
        ser.dtr = False
        time.sleep(0.3)
    ser.close()
    print('DTR reset sequence sent')
except ImportError:
    sys.exit(1)
except Exception as e:
    print(f'DTR reset failed: {e}')
    sys.exit(1)
" 2>/dev/null && {
            success "DTR reset trigger successful"
            return 0
        }
    fi
    
    # 方法3: 提示用户手动操作
    warning "Automatic trigger failed. Manual intervention required."
    echo ""
    echo "Please choose one of the following methods:"
    echo "1. Double-press reset button within 3 seconds"
    echo "2. Press any key when you see the 5-second countdown"
    echo "3. Send 'DOWNLOAD' command via serial terminal"
    echo ""
    read -p "Press Enter when device is in download mode..." -r
    
    return 0
}

# 等待下载就绪
wait_for_download_ready() {
    local port="$1"
    local timeout=10
    local count=0
    
    info "Waiting for download ready signal..."
    
    while [[ $count -lt $timeout ]]; do
        if timeout 1 cat "$port" | grep -i -E "(ymodem|download mode|ready|send file)" >/dev/null 2>&1; then
            success "Device ready for download"
            return 0
        fi
        
        echo -n "."
        sleep 1
        ((count++))
    done
    
    echo ""
    warning "No download ready signal detected, proceeding anyway..."
    return 0
}

# 使用sz下载文件
download_with_sz() {
    local port="$1"
    local baud="$2" 
    local firmware="$3"
    
    if ! command -v sz >/dev/null; then
        warning "sz command not found. Please install lrzsz package:"
        warning "  Ubuntu/Debian: sudo apt install lrzsz"
        warning "  macOS: brew install lrzsz"
        return 1
    fi
    
    info "Starting YMODEM transfer with sz..."
    
    # 配置串口
    stty -F "$port" "$baud" raw -echo -echoe -echok
    
    # 启动传输
    if sz --ymodem --1k "$firmware" < "$port" > "$port" 2>/dev/null; then
        success "YMODEM transfer completed"
        return 0
    else
        error "YMODEM transfer failed"
        return 1
    fi
}

# 使用项目脚本下载
download_with_project_script() {
    local firmware="$1"
    
    # 查找项目下载脚本
    local scripts=("flash_program.sh" "flash_programmer.py" "../flash_program.sh")
    
    for script in "${scripts[@]}"; do
        if [[ -x "$script" ]]; then
            info "Using project script: $script"
            if "$script" program; then
                success "Download completed with project script"
                return 0
            fi
        fi
    done
    
    return 1
}

# 手动下载指导
manual_download_guide() {
    local port="$1"
    local baud="$2"
    local firmware="$3"
    
    warning "No automatic download method available"
    echo ""
    echo "Manual download instructions:"
    echo "1. Ensure device is in download mode"
    echo "2. Use one of the following tools:"
    echo ""
    echo "   Option A - Using minicom:"
    echo "     minicom -D $port -b $baud"
    echo "     Ctrl+A, S, choose YMODEM, select: $firmware"
    echo ""
    echo "   Option B - Using screen + sz:"
    echo "     In another terminal: sz --ymodem $firmware > $port < $port"
    echo ""
    echo "   Option C - Using your preferred serial tool"
    echo "     Protocol: YMODEM, File: $firmware"
    echo ""
    
    read -p "Download completed successfully? (y/N): " -r response
    if [[ "$response" =~ ^[Yy] ]]; then
        return 0
    else
        return 1
    fi
}

# 构建固件
build_firmware() {
    info "Building firmware..."
    
    local build_cmd="make -C GCC"
    
    if [[ -f "GCC/Makefile" ]]; then
        cd GCC
        if make; then
            cd ..
            success "Build completed successfully"
            return 0
        else
            cd ..
            error "Build failed"
            return 1
        fi
    elif [[ -f "Makefile" ]]; then
        if make; then
            success "Build completed successfully"
            return 0
        else
            error "Build failed"
            return 1
        fi
    else
        error "No Makefile found"
        return 1
    fi
}

# 清理构建
clean_build() {
    info "Cleaning build..."
    
    if [[ -f "GCC/Makefile" ]]; then
        make -C GCC clean
    elif [[ -f "Makefile" ]]; then
        make clean
    fi
    
    success "Clean completed"
}

# 查找固件文件
find_firmware() {
    local candidates=(
        "$BUILD_DIR/$FIRMWARE_NAME"
        "$BUILD_DIR"/*.bin
        ./*.bin
        "GCC"/*.bin
    )
    
    for pattern in "${candidates[@]}"; do
        if ls $pattern >/dev/null 2>&1; then
            echo $(ls $pattern | head -1)
            return 0
        fi
    done
    
    return 1
}

# 烧录固件
flash_firmware() {
    local port="$1"
    local baud="$2"
    
    # 查找固件文件
    local firmware
    if ! firmware=$(find_firmware); then
        error "No firmware file found. Please build first."
        return 1
    fi
    
    info "Found firmware: $firmware"
    
    # 自动检测串口
    if [[ -z "$port" ]]; then
        if ! port=$(auto_detect_port); then
            return 1
        fi
    fi
    
    # 测试串口
    if ! test_port "$port" "$baud"; then
        error "Cannot access port $port"
        return 1
    fi
    
    info "Using port: $port at $baud baud"
    
    # 保存配置
    save_config "$port" "$baud"
    
    # 触发下载模式
    if ! trigger_download_mode "$port" "$baud"; then
        return 1
    fi
    
    # 等待设备就绪
    wait_for_download_ready "$port"
    
    # 尝试各种下载方法
    if download_with_sz "$port" "$baud" "$firmware"; then
        success "Firmware flashed successfully with sz"
    elif download_with_project_script "$firmware"; then
        success "Firmware flashed successfully with project script"
    elif manual_download_guide "$port" "$baud" "$firmware"; then
        success "Firmware flashed successfully (manual)"
    else
        error "Firmware flash failed"
        return 1
    fi
    
    # 复位设备
    info "Resetting device..."
    sleep 1
    
    success "Flash operation completed!"
    return 0
}

# 监控串口
monitor_serial() {
    local port="$1"
    local baud="$2"
    
    # 自动检测串口
    if [[ -z "$port" ]]; then
        if ! port=$(auto_detect_port); then
            return 1
        fi
    fi
    
    info "Starting serial monitor on $port at $baud baud"
    info "Press Ctrl+C to exit"
    
    # 尝试不同的监控工具
    if command -v minicom >/dev/null; then
        minicom -D "$port" -b "$baud"
    elif command -v screen >/dev/null; then
        screen "$port" "$baud"
    elif command -v picocom >/dev/null; then
        picocom "$port" -b "$baud"
    elif command -v cu >/dev/null; then
        cu -l "$port" -s "$baud"
    else
        error "No serial monitor tool found. Please install one of:"
        error "  minicom, screen, picocom, or cu"
        return 1
    fi
}

# 主函数
main() {
    # 加载配置
    load_config
    
    # 解析参数
    local port="$DEFAULT_PORT"
    local baud="$DEFAULT_BAUD"
    local commands=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --port|-p)
                port="$2"
                shift 2
                ;;
            --baud|-b)
                baud="$2"
                shift 2
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            build|flash|monitor|clean)
                commands+=("$1")
                shift
                ;;
            *)
                error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 检查命令
    if [[ ${#commands[@]} -eq 0 ]]; then
        error "No command specified"
        show_help
        exit 1
    fi
    
    # 打印标题
    header "================================================"
    header "  S300 IDF - ESP32-style Development Tool v1.0"
    header "================================================"
    
    # 执行命令
    local success_flag=true
    for command in "${commands[@]}"; do
        if [[ "$success_flag" != true ]]; then
            break
        fi
        
        case "$command" in
            build)
                if ! build_firmware; then
                    success_flag=false
                fi
                ;;
            clean)
                clean_build
                ;;
            flash)
                if ! flash_firmware "$port" "$baud"; then
                    success_flag=false
                fi
                ;;
            monitor)
                monitor_serial "$port" "$baud"
                ;;
        esac
    done
    
    if [[ "$success_flag" == true ]] && [[ ! " ${commands[*]} " =~ " monitor " ]]; then
        success "All operations completed successfully!"
    elif [[ "$success_flag" != true ]]; then
        error "Some operations failed!"
        exit 1
    fi
}

# 信号处理
trap 'echo -e "\n${YELLOW}Operation cancelled by user${NC}"; exit 1' INT TERM

# 运行主函数
main "$@"
