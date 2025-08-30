#!/bin/bash

# SBL项目构建脚本
# S300二级引导程序自动化构建工具

set -e  # 遇到错误立即退出

#===============================================================================
# 配置变量
#===============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
GCC_DIR="$PROJECT_ROOT/GCC"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

#===============================================================================
# 帮助信息
#===============================================================================
show_help() {
    cat << EOF
SBL项目构建脚本

用法: $0 [选项] [目标]

选项:
    -h, --help          显示此帮助信息
    -v, --verbose       详细输出
    -c, --clean         构建前清理
    -d, --debug         构建调试版本
    -r, --release       构建发布版本
    -t, --test          构建后运行测试
    -f, --flash         构建后下载到目标板

目标:
    build               构建项目 (默认)
    clean               清理构建文件
    test                运行测试
    flash               下载固件
    info                显示项目信息
    verify              验证构建结果

示例:
    $0                  # 构建项目
    $0 -c -d            # 清理后构建调试版本
    $0 -r -f            # 构建发布版本并下载
    $0 test             # 运行测试

EOF
}

#===============================================================================
# 环境检查
#===============================================================================
check_environment() {
    log_info "检查构建环境..."
    
    # 检查工具链
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        log_error "ARM工具链未找到，请安装arm-none-eabi-gcc"
        exit 1
    fi
    
    # 检查Make
    if ! command -v make &> /dev/null; then
        log_error "Make工具未找到"
        exit 1
    fi
    
    # 检查项目文件
    if [ ! -f "$GCC_DIR/Makefile" ]; then
        log_error "Makefile未找到: $GCC_DIR/Makefile"
        exit 1
    fi
    
    # 显示工具链版本
    GCC_VERSION=$(arm-none-eabi-gcc --version | head -1)
    log_success "工具链检查通过: $GCC_VERSION"
}

#===============================================================================
# 构建函数
#===============================================================================
build_project() {
    local build_type="$1"
    local extra_flags="$2"
    
    log_info "开始构建SBL项目..."
    
    cd "$GCC_DIR"
    
    # 构建命令
    local make_cmd="make"
    
    if [ "$build_type" = "debug" ]; then
        make_cmd="$make_cmd debug"
        log_info "构建类型: 调试版本"
    elif [ "$build_type" = "release" ]; then
        make_cmd="$make_cmd release"
        log_info "构建类型: 发布版本"
    else
        log_info "构建类型: 默认"
    fi
    
    if [ "$VERBOSE" = "1" ]; then
        make_cmd="$make_cmd V=1"
    fi
    
    log_info "执行: $make_cmd"
    
    if eval "$make_cmd"; then
        log_success "SBL构建完成"
        return 0
    else
        log_error "SBL构建失败"
        return 1
    fi
}

#===============================================================================
# 清理函数
#===============================================================================
clean_project() {
    log_info "清理构建文件..."
    
    cd "$GCC_DIR"
    
    if make clean; then
        log_success "清理完成"
    else
        log_warning "清理过程中出现警告"
    fi
}

#===============================================================================
# 测试函数
#===============================================================================
run_tests() {
    log_info "运行SBL测试..."
    
    cd "$GCC_DIR"
    
    if make test; then
        log_success "所有测试通过"
        return 0
    else
        log_error "测试失败"
        return 1
    fi
}

#===============================================================================
# 下载函数
#===============================================================================
flash_firmware() {
    log_info "下载固件到目标板..."
    
    cd "$GCC_DIR"
    
    # 检查二进制文件是否存在
    local bin_files=(build/bin/sbl_*.bin)
    if [ ! -f "${bin_files[0]}" ]; then
        log_error "二进制文件未找到，请先构建项目"
        return 1
    fi
    
    local bin_file="${bin_files[0]}"
    log_info "使用固件文件: $bin_file"
    
    if make flash; then
        log_success "固件下载完成"
        return 0
    else
        log_error "固件下载失败"
        return 1
    fi
}

#===============================================================================
# 信息显示函数
#===============================================================================
show_project_info() {
    log_info "显示项目信息..."
    
    cd "$GCC_DIR"
    make info
}

#===============================================================================
# 验证函数
#===============================================================================
verify_build() {
    log_info "验证构建结果..."
    
    cd "$GCC_DIR"
    
    if make verify; then
        log_success "构建验证通过"
        return 0
    else
        log_error "构建验证失败"
        return 1
    fi
}

#===============================================================================
# 主函数
#===============================================================================
main() {
    local clean_first=0
    local build_type=""
    local run_test=0
    local do_flash=0
    local target="build"
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=1
                ;;
            -c|--clean)
                clean_first=1
                ;;
            -d|--debug)
                build_type="debug"
                ;;
            -r|--release)
                build_type="release"
                ;;
            -t|--test)
                run_test=1
                ;;
            -f|--flash)
                do_flash=1
                ;;
            build|clean|test|flash|info|verify)
                target="$1"
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
        shift
    done
    
    # 显示横幅
    echo ""
    echo "========================================"
    echo "    S300 SBL 构建脚本 v1.0"
    echo "========================================"
    echo ""
    
    # 检查环境
    check_environment
    
    # 执行操作
    case "$target" in
        build)
            if [ "$clean_first" = "1" ]; then
                clean_project
            fi
            
            if build_project "$build_type"; then
                if [ "$run_test" = "1" ]; then
                    run_tests
                fi
                
                if [ "$do_flash" = "1" ]; then
                    flash_firmware
                fi
            else
                exit 1
            fi
            ;;
        clean)
            clean_project
            ;;
        test)
            run_tests
            ;;
        flash)
            flash_firmware
            ;;
        info)
            show_project_info
            ;;
        verify)
            verify_build
            ;;
        *)
            log_error "未知目标: $target"
            exit 1
            ;;
    esac
    
    echo ""
    log_success "操作完成"
    echo ""
}

# 设置默认值
VERBOSE=0

# 运行主函数
main "$@"
