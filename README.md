# NE004-Plus 项目

PiMCHIP S300 系列芯片的 BSP 和应用开发框架

## 📁 项目结构

```
ne004-plus/
├── S300_BSP/              # S300 板级支持包
│   ├── CMSIS/            # ARM CMSIS 标准库
│   ├── Drivers/           # 外设驱动
│   └── Projects/          # 示例项目
├── docs/                  # 项目文档
├── CONTRIBUTING.md        # 贡献指南
└── README.md             # 项目说明
```

## 🚀 快速开始

### 环境要求

- ARM GNU Toolchain (arm-none-eabi-gcc)
- CMake + Ninja
- Git

### 构建示例

```bash
# 克隆项目
git clone https://github.com/xingxinonline/ne004-plus.git
cd ne004-plus

# 构建 BSP
cd S300_BSP
mkdir build && cd build
cmake -G Ninja ..
ninja s300_rbl_minimal
```

## 📚 文档

- [S300 BSP 文档](S300_BSP/README.md)
- [贡献指南](CONTRIBUTING.md)
- [API 文档](docs/)

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

请阅读 [贡献指南](CONTRIBUTING.md) 了解详细信息。

## 📄 许可证

本项目采用 MIT 许可证。