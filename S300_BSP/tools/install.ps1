# S300 Tools Windows 安装脚本 (Scoop)
# 自动安装 Python, uv 和项目依赖

param(
    [switch]$Force,
    [switch]$Dev
)

$ErrorActionPreference = "Stop"

Write-Host "🚀 S300 Tools Windows 安装程序" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green

# 检查 PowerShell 版本
if ($PSVersionTable.PSVersion.Major -lt 5) {
    Write-Error "需要 PowerShell 5.0 或更高版本"
    exit 1
}

# 检查是否以管理员身份运行
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning "建议以管理员身份运行以避免权限问题"
}

# 函数：检查命令是否存在
function Test-Command {
    param($Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    } catch {
        return $false
    }
}

# 函数：运行命令并检查结果
function Invoke-SafeCommand {
    param($Command, $Description)
    Write-Host "⏳ $Description..." -ForegroundColor Yellow
    try {
        Invoke-Expression $Command
        Write-Host "✅ $Description 完成" -ForegroundColor Green
    } catch {
        Write-Error "❌ $Description 失败: $_"
        exit 1
    }
}

# 1. 安装 Scoop (如果未安装)
if (-not (Test-Command "scoop")) {
    Write-Host "📦 安装 Scoop 包管理器..." -ForegroundColor Cyan
    
    # 设置执行策略
    if ((Get-ExecutionPolicy) -eq "Restricted") {
        Write-Host "设置执行策略..." -ForegroundColor Yellow
        Set-ExecutionPolicy RemoteSigned -Scope CurrentUser -Force
    }
    
    # 安装 Scoop
    Invoke-SafeCommand "irm get.scoop.sh | iex" "Scoop 安装"
    
    # 刷新环境变量
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "Machine")
} else {
    Write-Host "✅ Scoop 已安装" -ForegroundColor Green
}

# 2. 添加 extras bucket (包含更多软件包)
Write-Host "📦 配置 Scoop buckets..." -ForegroundColor Cyan
try {
    scoop bucket add extras 2>$null
} catch {
    # 忽略已存在的错误
}

# 3. 安装 Python (如果未安装)
if (-not (Test-Command "python")) {
    Invoke-SafeCommand "scoop install python" "Python 安装"
} else {
    $pythonVersion = python --version
    Write-Host "✅ Python 已安装: $pythonVersion" -ForegroundColor Green
}

# 4. 安装 uv (现代 Python 包管理器)
if (-not (Test-Command "uv")) {
    Invoke-SafeCommand "scoop install uv" "uv 安装"
} else {
    $uvVersion = uv --version
    Write-Host "✅ uv 已安装: $uvVersion" -ForegroundColor Green
}

# 5. 安装 Git (如果未安装，用于开发)
if ($Dev -and -not (Test-Command "git")) {
    Invoke-SafeCommand "scoop install git" "Git 安装"
}

# 6. 进入工具目录
$toolsPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $toolsPath

# 7. 安装项目依赖
Write-Host "📦 安装项目依赖..." -ForegroundColor Cyan
if ($Dev) {
    Invoke-SafeCommand "uv sync --dev" "开发依赖安装"
} else {
    Invoke-SafeCommand "uv sync" "项目依赖安装"
}

# 8. 创建快捷命令 (添加到 PATH)
Write-Host "🔗 创建快捷命令..." -ForegroundColor Cyan

$binPath = "$env:USERPROFILE\scoop\shims"
$resetToolPath = "$toolsPath\s300_reset_tool.py"
$otaToolPath = "$toolsPath\s300_ota_tool.py"

# 创建批处理文件
$resetBat = @"
@echo off
uv run --directory "$toolsPath" python "$resetToolPath" %*
"@

$otaBat = @"
@echo off
uv run --directory "$toolsPath" python "$otaToolPath" %*
"@

$resetBat | Out-File -FilePath "$binPath\s300-reset.bat" -Encoding ASCII
$otaBat | Out-File -FilePath "$binPath\s300-ota.bat" -Encoding ASCII

Write-Host "✅ 快捷命令已创建:" -ForegroundColor Green
Write-Host "   s300-reset    - 软件复位工具" -ForegroundColor Cyan
Write-Host "   s300-ota      - OTA 升级工具" -ForegroundColor Cyan

# 9. 验证安装
Write-Host "🔍 验证安装..." -ForegroundColor Cyan

$testCommands = @(
    @{Command = "python --version"; Name = "Python"},
    @{Command = "uv --version"; Name = "uv"},
    @{Command = "uv run python -c 'import serial; print(\"pyserial OK\")'"; Name = "pyserial"},
    @{Command = "uv run python -c 'import requests; print(\"requests OK\")'"; Name = "requests"}
)

$allGood = $true
foreach ($test in $testCommands) {
    try {
        $result = Invoke-Expression $test.Command 2>$null
        Write-Host "✅ $($test.Name): $result" -ForegroundColor Green
    } catch {
        Write-Host "❌ $($test.Name): 失败" -ForegroundColor Red
        $allGood = $false
    }
}

# 10. 显示使用说明
Write-Host ""
Write-Host "🎉 安装完成!" -ForegroundColor Green
Write-Host "===============" -ForegroundColor Green

if ($allGood) {
    Write-Host "✅ 所有组件安装成功" -ForegroundColor Green
} else {
    Write-Host "⚠️  部分组件安装可能有问题，请检查上述错误" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "📖 使用方法:" -ForegroundColor Cyan
Write-Host ""
Write-Host "扫描设备:" -ForegroundColor White
Write-Host "  s300-reset scan" -ForegroundColor Gray
Write-Host ""
Write-Host "触发下载模式:" -ForegroundColor White
Write-Host "  s300-reset serial COM3 --mode download" -ForegroundColor Gray
Write-Host "  s300-reset double-reset COM3" -ForegroundColor Gray
Write-Host ""
Write-Host "OTA 升级:" -ForegroundColor White
Write-Host "  s300-ota ota COM3 firmware.bin" -ForegroundColor Gray
Write-Host "  s300-ota trigger COM3 --method double_reset" -ForegroundColor Gray
Write-Host ""
Write-Host "获取帮助:" -ForegroundColor White
Write-Host "  s300-reset --help" -ForegroundColor Gray
Write-Host "  s300-ota --help" -ForegroundColor Gray
Write-Host ""

Write-Host "💡 提示:" -ForegroundColor Yellow
Write-Host "   - Windows 串口通常是 COM1, COM2, COM3 等" -ForegroundColor White
Write-Host "   - 使用设备管理器查看串口设备" -ForegroundColor White
Write-Host "   - 如果遇到权限问题，请以管理员身份运行" -ForegroundColor White

if ($Dev) {
    Write-Host ""
    Write-Host "🛠️  开发模式已启用:" -ForegroundColor Cyan
    Write-Host "   - Git 已安装" -ForegroundColor White
    Write-Host "   - 开发依赖已安装 (pytest, black, mypy)" -ForegroundColor White
    Write-Host "   - 使用 'uv run black .' 格式化代码" -ForegroundColor White
    Write-Host "   - 使用 'uv run pytest' 运行测试" -ForegroundColor White
}
