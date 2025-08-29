#!/bin/bash

# S300 RBL Flash 烧录脚本
# 使用OpenOCD通过JTAG/SWD直接烧录RBL到QSPI Flash

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 配置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENOCD_CFG="$SCRIPT_DIR/s300_flash.cfg"
RBL_BIN="$SCRIPT_DIR/s300_rbl_complete.bin"
INTERFACE="stlink"  # 默认使用ST-Link

# 显示帮助信息
show_help() {
    echo "S300 RBL Flash 烧录工具"
    echo
    echo "用法: $0 [选项] [命令]"
    echo
    echo "选项:"
    echo "  -i, --interface <name>   指定调试器接口 (stlink|jlink|daplink)"
    echo "  -f, --file <path>        指定RBL二进制文件路径"
    echo "  -h, --help              显示此帮助信息"
    echo
    echo "命令:"
    echo "  info                    显示Flash信息"
    echo "  program                 烧录RBL文件"
    echo "  disable-wp              禁用Flash写保护"
    echo "  enable-wp               启用Flash写保护"
    echo "  build-and-program       构建并烧录RBL"
    echo
    echo "示例:"
    echo "  $0 info                                    # 显示Flash信息"
    echo "  $0 program                                 # 烧录默认RBL文件"
    echo "  $0 -f custom_rbl.bin program              # 烧录指定文件"
    echo "  $0 -i jlink build-and-program             # 使用J-Link构建并烧录"
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖工具..."
    
    if ! command -v openocd &> /dev/null; then
        print_error "OpenOCD 未安装!"
        print_info "Ubuntu/Debian: sudo apt install openocd"
        print_info "macOS: brew install openocd"
        exit 1
    fi
    
    print_success "依赖检查完成"
}

# 检查文件
check_files() {
    if [[ ! -f "$OPENOCD_CFG" ]]; then
        print_error "OpenOCD配置文件不存在: $OPENOCD_CFG"
        exit 1
    fi
    
    if [[ "$1" == "program" || "$1" == "build-and-program" ]]; then
        if [[ ! -f "$RBL_BIN" ]]; then
            print_error "RBL二进制文件不存在: $RBL_BIN"
            print_info "请先运行 './build.sh build' 构建RBL"
            exit 1
        fi
        
        local size=$(stat -c%s "$RBL_BIN")
        if [[ $size -gt 65536 ]]; then
            print_error "RBL文件太大: $size bytes > 64KB"
            exit 1
        fi
        
        print_info "RBL文件: $RBL_BIN ($(echo "scale=1; $size/1024" | bc)KB)"
    fi
}

# 创建临时OpenOCD配置
create_openocd_config() {
    local temp_cfg=$(mktemp)
    
    # 根据接口类型选择配置
    case "$INTERFACE" in
        "stlink")
            echo "source [find interface/stlink.cfg]" > "$temp_cfg"
            ;;
        "jlink")
            echo "source [find interface/jlink.cfg]" > "$temp_cfg"
            ;;
        "daplink")
            echo "source [find interface/cmsis-dap.cfg]" > "$temp_cfg"
            ;;
        *)
            print_error "不支持的接口: $INTERFACE"
            exit 1
            ;;
    esac
    
    # 添加主配置
    echo "source \"$OPENOCD_CFG\"" >> "$temp_cfg"
    
    echo "$temp_cfg"
}

# 执行OpenOCD命令
run_openocd_command() {
    local cmd="$1"
    local temp_cfg=$(create_openocd_config)
    
    print_info "执行OpenOCD命令: $cmd"
    print_info "使用接口: $INTERFACE"
    
    # 运行OpenOCD
    if timeout 30s openocd -f "$temp_cfg" -c "$cmd; exit" 2>&1; then
        print_success "命令执行成功"
    else
        print_error "命令执行失败"
        rm -f "$temp_cfg"
        exit 1
    fi
    
    rm -f "$temp_cfg"
}

# 显示Flash信息
flash_info() {
    print_info "获取S300 Flash信息..."
    run_openocd_command "init; rbl_info"
}

# 禁用写保护
disable_write_protection() {
    print_info "禁用Flash写保护..."
    run_openocd_command "init; rbl_disable_wp"
}

# 启用写保护
enable_write_protection() {
    print_info "启用Flash写保护..."
    run_openocd_command "init; rbl_enable_wp"
}

# 烧录RBL
program_rbl() {
    print_info "开始烧录RBL到Flash..."
    print_warning "⚠️  请确保目标设备已正确连接"
    print_warning "⚠️  烧录过程中请勿断开连接"
    
    # 询问用户确认
    echo
    read -p "确认开始烧录? [y/N]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "用户取消烧录"
        exit 0
    fi
    
    run_openocd_command "init; rbl_program \"$RBL_BIN\""
    
    print_success "RBL烧录完成!"
    print_info "请重启目标设备验证启动"
}

# 构建并烧录
build_and_program() {
    print_info "构建并烧录RBL..."
    
    # 先构建RBL
    if ! "$SCRIPT_DIR/build.sh" build; then
        print_error "RBL构建失败"
        exit 1
    fi
    
    # 检查生成的文件
    check_files "program"
    
    # 烧录
    program_rbl
}

# 主函数
main() {
    echo
    print_info "S300 RBL Flash 烧录工具"
    echo "=========================================="
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -i|--interface)
                INTERFACE="$2"
                shift 2
                ;;
            -f|--file)
                RBL_BIN="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            info)
                CMD="info"
                shift
                ;;
            program)
                CMD="program"
                shift
                ;;
            disable-wp)
                CMD="disable-wp"
                shift
                ;;
            enable-wp)
                CMD="enable-wp"
                shift
                ;;
            build-and-program)
                CMD="build-and-program"
                shift
                ;;
            *)
                print_error "未知参数: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 如果没有指定命令，显示帮助
    if [[ -z "${CMD:-}" ]]; then
        show_help
        exit 1
    fi
    
    # 检查依赖
    check_dependencies
    
    # 检查文件
    check_files "$CMD"
    
    # 执行命令
    case "$CMD" in
        "info")
            flash_info
            ;;
        "program")
            program_rbl
            ;;
        "disable-wp")
            disable_write_protection
            ;;
        "enable-wp")
            enable_write_protection
            ;;
        "build-and-program")
            build_and_program
            ;;
        *)
            print_error "未知命令: $CMD"
            exit 1
            ;;
    esac
    
    echo
    print_success "操作完成!"
}

# 错误处理
trap 'print_error "脚本异常退出"' ERR

# 执行主函数
main "$@"
