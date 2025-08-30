#!/bin/bash
# S300 Tools Linux/macOS 安装脚本
# 自动安装 Python, uv 和项目依赖

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 参数解析
FORCE=false
DEV=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --force)
            FORCE=true
            shift
            ;;
        --dev)
            DEV=true
            shift
            ;;
        -h|--help)
            echo "使用方法: $0 [--force] [--dev]"
            echo "  --force  强制重新安装"
            echo "  --dev    安装开发依赖"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}🚀 S300 Tools Linux/macOS 安装程序${NC}"
echo -e "${GREEN}=====================================${NC}"

# 函数：检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 函数：运行命令并检查结果
run_command() {
    local cmd="$1"
    local desc="$2"
    echo -e "${YELLOW}⏳ $desc...${NC}"
    if eval "$cmd"; then
        echo -e "${GREEN}✅ $desc 完成${NC}"
    else
        echo -e "${RED}❌ $desc 失败${NC}"
        exit 1
    fi
}

# 函数：检测操作系统
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
echo -e "${CYAN}📱 检测到操作系统: $OS${NC}"

# 1. 检查和安装 Python
if ! command_exists python3; then
    echo -e "${CYAN}📦 安装 Python...${NC}"
    case $OS in
        linux)
            if command_exists apt-get; then
                run_command "sudo apt-get update && sudo apt-get install -y python3 python3-pip python3-venv" "Python 安装 (apt)"
            elif command_exists yum; then
                run_command "sudo yum install -y python3 python3-pip" "Python 安装 (yum)"
            elif command_exists dnf; then
                run_command "sudo dnf install -y python3 python3-pip" "Python 安装 (dnf)"
            elif command_exists pacman; then
                run_command "sudo pacman -S --noconfirm python python-pip" "Python 安装 (pacman)"
            else
                echo -e "${RED}❌ 不支持的 Linux 发行版，请手动安装 Python3${NC}"
                exit 1
            fi
            ;;
        macos)
            if command_exists brew; then
                run_command "brew install python3" "Python 安装 (Homebrew)"
            else
                echo -e "${YELLOW}⚠️  请先安装 Homebrew 或手动安装 Python3${NC}"
                echo "Homebrew 安装: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            ;;
        *)
            echo -e "${RED}❌ 不支持的操作系统${NC}"
            exit 1
            ;;
    esac
else
    PYTHON_VERSION=$(python3 --version)
    echo -e "${GREEN}✅ Python 已安装: $PYTHON_VERSION${NC}"
fi

# 2. 安装 uv (现代 Python 包管理器)
if ! command_exists uv || [[ "$FORCE" == true ]]; then
    echo -e "${CYAN}📦 安装 uv...${NC}"
    run_command "curl -LsSf https://astral.sh/uv/install.sh | sh" "uv 安装"
    
    # 刷新环境变量
    export PATH="$HOME/.cargo/bin:$PATH"
    
    # 检查安装
    if ! command_exists uv; then
        echo -e "${YELLOW}⚠️  uv 未添加到 PATH，尝试手动添加...${NC}"
        echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> ~/.bashrc
        echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> ~/.zshrc 2>/dev/null || true
        export PATH="$HOME/.cargo/bin:$PATH"
    fi
else
    UV_VERSION=$(uv --version)
    echo -e "${GREEN}✅ uv 已安装: $UV_VERSION${NC}"
fi

# 3. 安装开发工具 (如果需要)
if [[ "$DEV" == true ]]; then
    echo -e "${CYAN}📦 安装开发工具...${NC}"
    case $OS in
        linux)
            if command_exists apt-get; then
                run_command "sudo apt-get install -y git build-essential" "开发工具安装 (apt)"
            elif command_exists yum; then
                run_command "sudo yum groupinstall -y 'Development Tools' && sudo yum install -y git" "开发工具安装 (yum)"
            elif command_exists dnf; then
                run_command "sudo dnf groupinstall -y 'Development Tools' && sudo dnf install -y git" "开发工具安装 (dnf)"
            elif command_exists pacman; then
                run_command "sudo pacman -S --noconfirm git base-devel" "开发工具安装 (pacman)"
            fi
            ;;
        macos)
            if ! command_exists git; then
                run_command "xcode-select --install" "Xcode 命令行工具安装"
            fi
            ;;
    esac
fi

# 4. 进入工具目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 5. 安装项目依赖
echo -e "${CYAN}📦 安装项目依赖...${NC}"
if [[ "$DEV" == true ]]; then
    run_command "uv sync --dev" "开发依赖安装"
else
    run_command "uv sync" "项目依赖安装"
fi

# 6. 创建快捷命令 (添加到 PATH)
echo -e "${CYAN}🔗 创建快捷命令...${NC}"

# 创建 ~/.local/bin 目录 (如果不存在)
mkdir -p ~/.local/bin

# 创建快捷脚本
cat > ~/.local/bin/s300-reset << EOF
#!/bin/bash
cd "$SCRIPT_DIR"
uv run python s300_reset_tool.py "\$@"
EOF

cat > ~/.local/bin/s300-ota << EOF
#!/bin/bash
cd "$SCRIPT_DIR"
uv run python s300_ota_tool.py "\$@"
EOF

# 添加执行权限
chmod +x ~/.local/bin/s300-reset
chmod +x ~/.local/bin/s300-ota

# 检查 ~/.local/bin 是否在 PATH 中
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo -e "${YELLOW}⚠️  ~/.local/bin 不在 PATH 中，添加到 shell 配置...${NC}"
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc 2>/dev/null || true
    export PATH="$HOME/.local/bin:$PATH"
fi

echo -e "${GREEN}✅ 快捷命令已创建:${NC}"
echo -e "${CYAN}   s300-reset    - 软件复位工具${NC}"
echo -e "${CYAN}   s300-ota      - OTA 升级工具${NC}"

# 7. 设置串口权限 (Linux)
if [[ "$OS" == "linux" ]]; then
    echo -e "${CYAN}🔧 设置串口权限...${NC}"
    if groups | grep -q dialout; then
        echo -e "${GREEN}✅ 用户已在 dialout 组中${NC}"
    else
        echo -e "${YELLOW}⚠️  添加用户到 dialout 组...${NC}"
        sudo usermod -a -G dialout "$USER"
        echo -e "${YELLOW}⚠️  请重新登录以使组权限生效${NC}"
    fi
fi

# 8. 验证安装
echo -e "${CYAN}🔍 验证安装...${NC}"

ALL_GOOD=true

# 测试命令列表
declare -a TEST_COMMANDS=(
    "python3 --version:Python"
    "uv --version:uv"
    "uv run python -c 'import serial; print(\"pyserial OK\")':pyserial"
    "uv run python -c 'import requests; print(\"requests OK\")':requests"
)

for test in "${TEST_COMMANDS[@]}"; do
    IFS=':' read -r cmd name <<< "$test"
    if result=$(eval "$cmd" 2>/dev/null); then
        echo -e "${GREEN}✅ $name: $result${NC}"
    else
        echo -e "${RED}❌ $name: 失败${NC}"
        ALL_GOOD=false
    fi
done

# 9. 显示使用说明
echo ""
echo -e "${GREEN}🎉 安装完成!${NC}"
echo -e "${GREEN}===============${NC}"

if [[ "$ALL_GOOD" == true ]]; then
    echo -e "${GREEN}✅ 所有组件安装成功${NC}"
else
    echo -e "${YELLOW}⚠️  部分组件安装可能有问题，请检查上述错误${NC}"
fi

echo ""
echo -e "${CYAN}📖 使用方法:${NC}"
echo ""
echo -e "${NC}扫描设备:${NC}"
echo -e "${YELLOW}  s300-reset scan${NC}"
echo ""
echo -e "${NC}触发下载模式:${NC}"
echo -e "${YELLOW}  s300-reset serial /dev/ttyUSB0 --mode download${NC}"
echo -e "${YELLOW}  s300-reset double-reset /dev/ttyUSB0${NC}"
echo ""
echo -e "${NC}OTA 升级:${NC}"
echo -e "${YELLOW}  s300-ota ota /dev/ttyUSB0 firmware.bin${NC}"
echo -e "${YELLOW}  s300-ota trigger /dev/ttyUSB0 --method double_reset${NC}"
echo ""
echo -e "${NC}获取帮助:${NC}"
echo -e "${YELLOW}  s300-reset --help${NC}"
echo -e "${YELLOW}  s300-ota --help${NC}"
echo ""

echo -e "${YELLOW}💡 提示:${NC}"
echo -e "${NC}   - Linux 串口通常是 /dev/ttyUSB0, /dev/ttyACM0 等${NC}"
echo -e "${NC}   - macOS 串口通常是 /dev/cu.usbserial-* 等${NC}"
echo -e "${NC}   - 使用 'ls /dev/tty*' 查看可用串口${NC}"
if [[ "$OS" == "linux" ]]; then
    echo -e "${NC}   - 如果串口权限问题，请重新登录或运行: sudo chmod 666 /dev/ttyUSB0${NC}"
fi

if [[ "$DEV" == true ]]; then
    echo ""
    echo -e "${CYAN}🛠️  开发模式已启用:${NC}"
    echo -e "${NC}   - Git 和构建工具已安装${NC}"
    echo -e "${NC}   - 开发依赖已安装 (pytest, black, mypy)${NC}"
    echo -e "${NC}   - 使用 'uv run black .' 格式化代码${NC}"
    echo -e "${NC}   - 使用 'uv run pytest' 运行测试${NC}"
fi

# 检查是否需要重新登录
if [[ "$OS" == "linux" ]] && ! groups | grep -q dialout; then
    echo ""
    echo -e "${YELLOW}⚠️  重要: 请重新登录以使串口权限生效${NC}"
fi
