# 贡献指南

## 开发环境设置

### 1. 工具链安装

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake ninja-build gcc-arm-none-eabi

# macOS (使用 Homebrew)
brew install cmake ninja arm-none-eabi-gcc

# Windows (使用 MSYS2 或 WSL)
# 安装 MSYS2，然后：
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja arm-none-eabi-gcc
```

### 2. 克隆仓库

```bash
git clone https://github.com/xingxinonline/ne004-plus.git
cd ne004-plus
```

### 3. 构建项目

```bash
# CMake 方式 (推荐)
mkdir build && cd build
cmake -G Ninja ..
ninja

# 或使用 GNU Make
cd S300_BSP/Projects/RBL_Minimal/GCC
make all
```

## 提交规范

本项目遵循 **Angular 提交规范 + 中文 + Emoji** 格式：

```
✨ feat(rbl_minimal): 新增 QSPI 初始化功能

1. 背景
- 实现 QSPI Flash 访问功能
- 支持 JEDEC ID 读取

2. 方案与实现
- 新增 rbl_qspi.h/.c 模块
- 实现初始化和 ID 读取接口

3. 兼容性与迁移
- 向后兼容，不影响现有功能

4. 验证
- 构建通过，串口输出正确
```

### 提交类型

- ✨ feat: 新功能
- 🐛 fix: 修复缺陷
- 📝 docs: 文档更新
- 🎨 style: 代码风格
- ♻️ refactor: 重构
- ⚡️ perf: 性能优化
- ✅ test: 测试相关
- 🏗️ build: 构建系统
- 🤖 ci: CI/CD 配置
- 🧹 chore: 杂项维护

## 代码风格

- 使用 4 空格缩进
- 函数和变量使用 snake_case
- 宏定义使用 UPPER_SNAKE_CASE
- 注释使用中文，简洁明了
- 提交前运行格式化工具

## 测试要求

- 所有新功能需要单元测试
- 修改现有功能需要回归测试
- 提交前确保构建通过
- 关键功能需要手动验证

## 分支管理

- `main`: 主分支，稳定版本
- `develop`: 开发分支
- `feat/*`: 功能分支
- `fix/*`: 修复分支
- `docs/*`: 文档分支

## 问题反馈

- 使用 GitHub Issues 报告问题
- 提供详细的复现步骤
- 包含环境信息和错误日志