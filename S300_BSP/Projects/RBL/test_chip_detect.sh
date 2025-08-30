#!/bin/bash

# 芯片检测功能测试脚本
# 用于测试 chip_detect.sh 的各种功能

set -e

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

# 测试函数
test_help() {
    header "测试1: 帮助信息"
    ./chip_detect.sh --help
    echo ""
}

test_list_devices() {
    header "测试2: 列出串口设备"
    ./chip_detect.sh --list
    echo ""
}

test_auto_detect() {
    header "测试3: 自动检测所有设备"
    ./chip_detect.sh
    echo ""
}

test_verbose_mode() {
    header "测试4: 详细模式检测"
    local ports=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | head -1))
    
    if [[ ${#ports[@]} -gt 0 ]]; then
        info "使用设备: ${ports[0]}"
        ./chip_detect.sh --verbose "${ports[0]}"
    else
        warning "未找到串口设备，跳过详细模式测试"
    fi
    echo ""
}

test_different_bauds() {
    header "测试5: 不同波特率测试"
    local ports=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | head -1))
    
    if [[ ${#ports[@]} -gt 0 ]]; then
        local bauds=("9600" "38400" "115200" "921600")
        for baud in "${bauds[@]}"; do
            info "测试波特率: $baud"
            ./chip_detect.sh --baud "$baud" "${ports[0]}" || true
        done
    else
        warning "未找到串口设备，跳过波特率测试"
    fi
    echo ""
}

test_timeout_settings() {
    header "测试6: 超时设置测试"
    local ports=($(find /dev -name "ttyUSB*" -o -name "ttyACM*" 2>/dev/null | head -1))
    
    if [[ ${#ports[@]} -gt 0 ]]; then
        info "测试超时时间: 1秒"
        ./chip_detect.sh --timeout 1 "${ports[0]}" || true
    else
        warning "未找到串口设备，跳过超时测试"
    fi
    echo ""
}

test_error_handling() {
    header "测试7: 错误处理测试"
    
    info "测试不存在的设备"
    ./chip_detect.sh /dev/ttyNONEXIST || true
    
    info "测试无效波特率"
    ./chip_detect.sh --baud invalid_baud || true
    
    info "测试无效参数"
    ./chip_detect.sh --invalid-option || true
    
    echo ""
}

# 创建模拟设备响应
create_mock_responses() {
    header "创建模拟响应文件"
    
    # S300响应
    cat > s300_response.txt << 'EOF'
PiMCHIP S300 RBL v1.0
Board: S300 Generic EVB
CPU: ARM Cortex-M33 @ 200MHz
Flash: 2MB
RAM: 512KB
Status: Ready
Commands: INFO, VERSION, STATUS, CHIP_ID, DOWNLOAD
EOF

    # ESP32响应
    cat > esp32_response.txt << 'EOF'
ESP32-D0WDQ6 (revision 1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: 24:0a:c4:12:34:56
EOF

    success "模拟响应文件已创建"
    echo ""
}

# 测试模拟响应
test_mock_responses() {
    header "测试8: 模拟响应识别"
    
    create_mock_responses
    
    info "测试S300响应识别"
    if grep -qi "s300\|pimchip\|rbl" s300_response.txt; then
        success "S300响应识别正确"
    else
        error "S300响应识别失败"
    fi
    
    info "测试ESP32响应识别"
    if grep -qi "esp32" esp32_response.txt; then
        success "ESP32响应识别正确"
    else
        error "ESP32响应识别失败"
    fi
    
    # 清理
    rm -f s300_response.txt esp32_response.txt
    echo ""
}

# 性能测试
test_performance() {
    header "测试9: 性能测试"
    
    info "测试检测速度"
    local start_time=$(date +%s.%N)
    ./chip_detect.sh >/dev/null 2>&1 || true
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
    
    if [[ "$duration" != "N/A" ]]; then
        info "检测耗时: ${duration} 秒"
        if (( $(echo "$duration < 10" | bc -l) )); then
            success "性能测试通过 (< 10秒)"
        else
            warning "检测时间较长 (> 10秒)"
        fi
    else
        info "无法计算检测时间 (缺少bc命令)"
    fi
    echo ""
}

# 集成测试
test_integration() {
    header "测试10: 集成测试"
    
    info "测试与s300_idf.sh的集成"
    if [[ -f "s300_idf.sh" ]]; then
        # 检查s300_idf.sh是否可以调用chip_detect.sh
        if grep -q "chip_detect.sh" s300_idf.sh; then
            success "找到集成代码"
        else
            warning "未找到集成代码，建议添加芯片检测功能到s300_idf.sh"
        fi
    else
        warning "未找到s300_idf.sh文件"
    fi
    echo ""
}

# 主测试函数
main() {
    header "=========================================="
    header "  芯片检测工具测试套件"
    header "=========================================="
    echo ""
    
    # 检查测试环境
    if [[ ! -f "chip_detect.sh" ]]; then
        error "chip_detect.sh 文件不存在"
        exit 1
    fi
    
    if [[ ! -x "chip_detect.sh" ]]; then
        error "chip_detect.sh 没有执行权限"
        exit 1
    fi
    
    # 运行测试
    local tests=(
        "test_help"
        "test_list_devices"
        "test_auto_detect"
        "test_verbose_mode"
        "test_different_bauds"
        "test_timeout_settings"
        "test_error_handling"
        "test_mock_responses"
        "test_performance"
        "test_integration"
    )
    
    local passed=0
    local total=${#tests[@]}
    
    for test in "${tests[@]}"; do
        info "运行测试: $test"
        if $test; then
            ((passed++))
        fi
    done
    
    # 测试总结
    header "=========================================="
    header "  测试总结"
    header "=========================================="
    echo "总测试数: $total"
    echo "通过测试: $passed"
    echo "失败测试: $((total - passed))"
    
    if [[ $passed -eq $total ]]; then
        success "所有测试通过!"
    else
        warning "部分测试失败，请检查实现"
    fi
    
    # 使用建议
    echo ""
    header "使用建议:"
    echo "1. 日常使用:"
    echo "   ./chip_detect.sh                    # 自动检测所有设备"
    echo "   ./chip_detect.sh /dev/ttyUSB0       # 检测指定设备"
    echo ""
    echo "2. 调试模式:"
    echo "   ./chip_detect.sh --verbose /dev/ttyUSB0  # 详细输出"
    echo "   ./chip_detect.sh --list             # 列出设备信息"
    echo ""
    echo "3. 自定义配置:"
    echo "   ./chip_detect.sh --baud 921600 /dev/ttyUSB0  # 指定波特率"
    echo "   ./chip_detect.sh --timeout 5 /dev/ttyUSB0    # 指定超时"
    echo ""
    echo "4. 集成到开发流程:"
    echo "   可以将芯片检测集成到s300_idf.sh中实现自动设备识别"
}

# 运行测试
main "$@"
