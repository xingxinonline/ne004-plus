#!/bin/bash

# S300 芯片检测工具
# 用于自动检测和识别连接的芯片类型
# 版本: 1.0
# 日期: 2025-08-30

set -e

# 配置
DEFAULT_BAUD="115200"
TIMEOUT="3"
VERBOSE=false

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# 输出函数
error() { echo -e "${RED}ERROR: $1${NC}" >&2; }
success() { echo -e "${GREEN}SUCCESS: $1${NC}"; }
info() { echo -e "${BLUE}INFO: $1${NC}"; }
warning() { echo -e "${YELLOW}WARNING: $1${NC}"; }
header() { echo -e "${CYAN}${BOLD}$1${NC}"; }

# 帮助信息
show_help() {
    cat << EOF
S300 芯片检测工具 v1.0

使用方法:
    $0 [选项] [串口设备]

选项:
    --port, -p PORT        指定串口设备
    --baud, -b BAUD        指定波特率 (默认: $DEFAULT_BAUD)
    --timeout, -t SEC      检测超时时间 (默认: $TIMEOUT 秒)
    --verbose, -v          详细输出
    --list, -l             列出所有串口设备
    --help, -h             显示帮助信息

示例:
    $0                              # 自动检测所有串口
    $0 /dev/ttyUSB0                # 检测指定串口
    $0 --list                      # 列出串口设备
    $0 --verbose /dev/ttyUSB0      # 详细检测过程
    $0 --baud 921600 /dev/ttyUSB0  # 指定波特率检测

支持的芯片类型:
    - PiMCHIP S300
    - ESP32 系列 (ESP32, ESP32-C3, ESP32-S3)
    - STM32 系列 (STM32F4, STM32H7)
    - GD32 系列
    - CH32 系列
    - 通用 RISC-V

检测方法:
    1. 串口命令检测 (INFO, VERSION, STATUS等)
    2. USB描述符检测 (lsusb, udevadm)
    3. 复位序列检测 (DTR复位 + 启动信息)
    4. 特征码检测 (通用探测命令)
EOF
}

# 查找串口设备
find_serial_ports() {
    local ports=()
    
    # USB串口
    for pattern in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.usbmodem*; do
        if ls $pattern >/dev/null 2>&1; then
            ports+=($(ls $pattern))
        fi
    done
    
    printf '%s\n' "${ports[@]}" | sort
}

# 列出串口设备详细信息
list_serial_devices() {
    header "=== 串口设备列表 ==="
    
    local ports=($(find_serial_ports))
    
    if [[ ${#ports[@]} -eq 0 ]]; then
        warning "未找到串口设备"
        return 1
    fi
    
    echo "找到 ${#ports[@]} 个串口设备:"
    echo ""
    
    for port in "${ports[@]}"; do
        echo "设备: $port"
        
        # 检查设备权限
        if [[ -r "$port" && -w "$port" ]]; then
            echo "  权限: ✓ 可读写"
        else
            echo "  权限: ✗ 权限不足 (需要 sudo 或将用户加入 dialout 组)"
        fi
        
        # USB设备信息
        if command -v udevadm >/dev/null; then
            local vendor=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_VENDOR=" | cut -d'=' -f2 || echo "Unknown")
            local model=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_MODEL=" | cut -d'=' -f2 || echo "Unknown")
            local vendor_id=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_VENDOR_ID=" | cut -d'=' -f2 || echo "Unknown")
            local product_id=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_PRODUCT_ID=" | cut -d'=' -f2 || echo "Unknown")
            
            echo "  厂商: $vendor"
            echo "  型号: $model"
            echo "  VID:PID: $vendor_id:$product_id"
            
            # 根据厂商ID推测芯片类型
            case "$vendor_id" in
                "1a86") echo "  推测: 可能是使用CH340的设备 (常见于S300等国产芯片)" ;;
                "10c4") echo "  推测: 可能是使用CP210x的设备 (常见于ESP32)" ;;
                "0403") echo "  推测: 可能是使用FTDI的设备" ;;
                "2341") echo "  推测: 可能是Arduino设备" ;;
                *) echo "  推测: 未知设备类型" ;;
            esac
        fi
        echo ""
    done
}

# 测试串口连接
test_serial_connection() {
    local port="$1"
    local baud="$2"
    
    if [[ ! -e "$port" ]]; then
        return 1
    fi
    
    # 检查权限
    if [[ ! -r "$port" || ! -w "$port" ]]; then
        return 1
    fi
    
    # 尝试配置串口
    if command -v stty >/dev/null; then
        if stty -F "$port" "$baud" raw -echo >/dev/null 2>&1; then
            return 0
        fi
    fi
    
    return 1
}

# 发送命令并读取响应
send_serial_command() {
    local port="$1"
    local baud="$2"
    local command="$3"
    local timeout="${4:-$TIMEOUT}"
    
    if ! test_serial_connection "$port" "$baud"; then
        return 1
    fi
    
    # 配置串口
    stty -F "$port" "$baud" raw -echo -echoe -echok 2>/dev/null || return 1
    
    # 发送命令
    echo -ne "$command" > "$port" 2>/dev/null || return 1
    
    # 读取响应
    timeout "$timeout" cat "$port" 2>/dev/null || true
}

# 串口命令检测
detect_by_uart_commands() {
    local port="$1"
    local baud="$2"
    
    info "尝试串口命令检测..."
    
    # S300/RBL检测命令
    local commands=(
        "INFO\r\n"
        "VERSION\r\n"
        "STATUS\r\n"
        "CHIP_ID\r\n"
        "\r\n"
        "?\r\n"
        "help\r\n"
    )
    
    for cmd in "${commands[@]}"; do
        if [[ $VERBOSE == true ]]; then
            info "发送命令: $(echo -ne "$cmd" | cat -v)"
        fi
        
        local response=$(send_serial_command "$port" "$baud" "$cmd" 2)
        
        if [[ -n "$response" && ${#response} -gt 5 ]]; then
            if [[ $VERBOSE == true ]]; then
                info "收到响应: $(echo "$response" | head -3 | tr '\n' ' ')"
            fi
            
            # 分析响应内容
            if echo "$response" | grep -qi "s300\|pimchip\|rbl"; then
                echo "PiMCHIP S300"
                if echo "$response" | grep -qi "rbl\|bootloader"; then
                    echo "  模式: Bootloader (RBL)"
                else
                    echo "  模式: Application"
                fi
                return 0
            elif echo "$response" | grep -qi "esp32"; then
                if echo "$response" | grep -qi "esp32-c3"; then
                    echo "ESP32-C3"
                elif echo "$response" | grep -qi "esp32-s3"; then
                    echo "ESP32-S3"
                else
                    echo "ESP32"
                fi
                return 0
            elif echo "$response" | grep -qi "stm32"; then
                echo "STM32"
                return 0
            elif echo "$response" | grep -qi "gd32"; then
                echo "GD32"
                return 0
            elif echo "$response" | grep -qi "ch32"; then
                echo "CH32"
                return 0
            else
                echo "未知设备 (有响应)"
                if [[ $VERBOSE == true ]]; then
                    echo "  响应内容: $(echo "$response" | head -2)"
                fi
                return 0
            fi
        fi
    done
    
    return 1
}

# USB描述符检测
detect_by_usb_info() {
    local port="$1"
    
    info "尝试USB描述符检测..."
    
    if command -v udevadm >/dev/null; then
        local vendor_id=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_VENDOR_ID=" | cut -d'=' -f2)
        local product_id=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_PRODUCT_ID=" | cut -d'=' -f2)
        local vendor=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_VENDOR=" | cut -d'=' -f2)
        local model=$(udevadm info --name="$port" --query=property 2>/dev/null | grep "ID_MODEL=" | cut -d'=' -f2)
        
        if [[ $VERBOSE == true ]]; then
            info "USB信息: VID=$vendor_id, PID=$product_id, 厂商=$vendor, 型号=$model"
        fi
        
        # 根据厂商ID判断
        case "$vendor_id" in
            "1a86")
                echo "可能是S300系列 (CH340串口芯片)"
                return 0
                ;;
            "10c4")
                echo "可能是ESP32系列 (CP210x串口芯片)"
                return 0
                ;;
            "0403")
                echo "使用FTDI串口芯片的设备"
                return 0
                ;;
            "2341")
                echo "可能是Arduino设备"
                return 0
                ;;
        esac
        
        # 根据产品描述判断
        if echo "$model" | grep -qi "ch340"; then
            echo "可能是S300系列 (CH340)"
            return 0
        elif echo "$model" | grep -qi "cp210"; then
            echo "可能是ESP32系列 (CP210x)"
            return 0
        fi
    fi
    
    return 1
}

# 复位序列检测
detect_by_reset_sequence() {
    local port="$1"
    local baud="$2"
    
    info "尝试复位序列检测..."
    
    # 检查是否支持DTR控制
    if ! command -v python3 >/dev/null; then
        if [[ $VERBOSE == true ]]; then
            warning "未安装python3，跳过DTR复位"
        fi
        return 1
    fi
    
    # 使用Python进行DTR复位
    local reset_result=$(python3 -c "
import sys
try:
    import serial
    import time
    
    ser = serial.Serial('$port', $baud, timeout=1)
    
    # DTR复位序列
    ser.dtr = True
    time.sleep(0.1)
    ser.dtr = False
    time.sleep(0.5)
    
    # 读取启动信息
    data = ''
    start_time = time.time()
    while time.time() - start_time < 3:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting()).decode('utf-8', errors='ignore')
            data += chunk
        time.sleep(0.01)
    
    ser.close()
    print(data)
except ImportError:
    sys.exit(1)
except Exception as e:
    print(f'Error: {e}', file=sys.stderr)
    sys.exit(1)
" 2>/dev/null)
    
    if [[ -n "$reset_result" && ${#reset_result} -gt 10 ]]; then
        if [[ $VERBOSE == true ]]; then
            info "复位响应: $(echo "$reset_result" | head -3 | tr '\n' ' ')"
        fi
        
        # 分析复位响应
        if echo "$reset_result" | grep -qi "s300\|pimchip\|rbl"; then
            echo "PiMCHIP S300"
            if echo "$reset_result" | grep -qi "rbl"; then
                echo "  启动模式: RBL Bootloader"
            fi
            return 0
        elif echo "$reset_result" | grep -qi "esp32"; then
            echo "ESP32系列"
            return 0
        elif echo "$reset_result" | grep -qi "stm32"; then
            echo "STM32系列"
            return 0
        else
            echo "未知设备 (有复位响应)"
            return 0
        fi
    fi
    
    return 1
}

# 综合检测单个设备
detect_single_device() {
    local port="$1"
    local baud="$2"
    
    header "检测设备: $port (波特率: $baud)"
    
    # 检查设备是否存在
    if [[ ! -e "$port" ]]; then
        error "设备不存在"
        return 1
    fi
    
    # 检查权限
    if [[ ! -r "$port" || ! -w "$port" ]]; then
        error "权限不足，请运行: sudo chmod 666 $port 或将用户加入 dialout 组"
        return 1
    fi
    
    local detected=false
    local chip_type="未知"
    
    # 方法1: 串口命令检测
    if chip_type=$(detect_by_uart_commands "$port" "$baud"); then
        success "串口命令检测成功"
        echo "  芯片类型: $chip_type"
        detected=true
    fi
    
    # 方法2: USB描述符检测
    if [[ $detected == false ]]; then
        if chip_type=$(detect_by_usb_info "$port"); then
            success "USB描述符检测成功"
            echo "  芯片类型: $chip_type"
            detected=true
        fi
    fi
    
    # 方法3: 复位序列检测
    if [[ $detected == false ]]; then
        if chip_type=$(detect_by_reset_sequence "$port" "$baud"); then
            success "复位序列检测成功"
            echo "  芯片类型: $chip_type"
            detected=true
        fi
    fi
    
    if [[ $detected == false ]]; then
        warning "无法识别芯片类型"
        echo "  建议:"
        echo "    1. 检查设备是否正确连接"
        echo "    2. 尝试不同的波特率"
        echo "    3. 确认设备处于正确的工作模式"
        return 1
    fi
    
    # 输出推荐配置
    echo ""
    echo "推荐配置:"
    case "$chip_type" in
        *"S300"*)
            echo "  下载工具: ./s300_idf.sh"
            echo "  波特率: 115200"
            echo "  协议: YMODEM"
            ;;
        *"ESP32"*)
            echo "  下载工具: esptool.py 或 idf.py"
            echo "  波特率: 115200"
            echo "  协议: ESP32 Serial Protocol"
            ;;
        *"STM32"*)
            echo "  下载工具: stm32flash 或 STM32CubeProgrammer"
            echo "  波特率: 115200"
            echo "  协议: STM32 Serial Protocol"
            ;;
    esac
    
    return 0
}

# 自动检测所有设备
detect_all_devices() {
    local baud="$1"
    
    header "=== 自动检测所有串口设备 ==="
    
    local ports=($(find_serial_ports))
    
    if [[ ${#ports[@]} -eq 0 ]]; then
        warning "未找到串口设备"
        return 1
    fi
    
    info "找到 ${#ports[@]} 个串口设备，开始检测..."
    echo ""
    
    local detected_count=0
    for port in "${ports[@]}"; do
        if detect_single_device "$port" "$baud"; then
            ((detected_count++))
        fi
        echo ""
    done
    
    header "=== 检测完成 ==="
    if [[ $detected_count -gt 0 ]]; then
        success "成功识别 $detected_count 个设备"
    else
        warning "未识别到任何设备"
    fi
}

# 主函数
main() {
    local port=""
    local baud="$DEFAULT_BAUD"
    local list_only=false
    
    # 解析参数
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
            --timeout|-t)
                TIMEOUT="$2"
                shift 2
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --list|-l)
                list_only=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            /dev/*)
                port="$1"
                shift
                ;;
            *)
                error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 打印标题
    header "=========================================="
    header "  S300 芯片检测工具 v1.0"
    header "=========================================="
    
    # 仅列出设备
    if [[ $list_only == true ]]; then
        list_serial_devices
        exit 0
    fi
    
    # 检查依赖
    local missing_tools=()
    for tool in stty timeout cat; do
        if ! command -v "$tool" >/dev/null; then
            missing_tools+=("$tool")
        fi
    done
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        error "缺少必要工具: ${missing_tools[*]}"
        exit 1
    fi
    
    # 检查可选工具
    if ! command -v udevadm >/dev/null; then
        warning "udevadm 未安装，USB检测功能受限"
    fi
    
    if ! command -v python3 >/dev/null; then
        warning "python3 未安装，DTR复位功能不可用"
    fi
    
    echo ""
    
    # 执行检测
    if [[ -n "$port" ]]; then
        # 检测指定设备
        detect_single_device "$port" "$baud"
    else
        # 自动检测所有设备
        detect_all_devices "$baud"
    fi
}

# 信号处理
trap 'echo -e "\n${YELLOW}检测被用户中断${NC}"; exit 1' INT TERM

# 运行主函数
main "$@"
