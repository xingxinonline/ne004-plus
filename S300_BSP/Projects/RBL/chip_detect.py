#!/usr/bin/env python3
"""
S300 芯片检测工具 (Python版本)
用于自动检测和识别连接的芯片类型
版本: 1.0
日期: 2025-08-30
"""

import sys
import os
import time
import re
import json
import argparse
import glob
import subprocess
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass
from enum import Enum

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

# 颜色定义
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[0;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    BOLD = '\033[1m'
    NC = '\033[0m'

class ChipType(Enum):
    S300 = "PiMCHIP S300"
    ESP32 = "ESP32"
    ESP32_C3 = "ESP32-C3"
    ESP32_S3 = "ESP32-S3"
    STM32 = "STM32"
    GD32 = "GD32"
    CH32 = "CH32"
    UNKNOWN = "未知"

@dataclass
class ChipInfo:
    chip_type: ChipType
    version: str = ""
    features: List[str] = None
    flash_size: str = ""
    ram_size: str = ""
    cpu_freq: str = ""
    mode: str = ""
    confidence: float = 0.0
    
    def __post_init__(self):
        if self.features is None:
            self.features = []

@dataclass
class SerialDevice:
    port: str
    description: str = ""
    vendor_id: str = ""
    product_id: str = ""
    vendor: str = ""
    model: str = ""

class Logger:
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
    
    def error(self, msg: str):
        print(f"{Colors.RED}ERROR: {msg}{Colors.NC}", file=sys.stderr)
    
    def success(self, msg: str):
        print(f"{Colors.GREEN}SUCCESS: {msg}{Colors.NC}")
    
    def info(self, msg: str):
        print(f"{Colors.BLUE}INFO: {msg}{Colors.NC}")
    
    def warning(self, msg: str):
        print(f"{Colors.YELLOW}WARNING: {msg}{Colors.NC}")
    
    def header(self, msg: str):
        print(f"{Colors.CYAN}{Colors.BOLD}{msg}{Colors.NC}")
    
    def debug(self, msg: str):
        if self.verbose:
            print(f"DEBUG: {msg}")

class ChipDetector:
    def __init__(self, logger: Logger, timeout: int = 3):
        self.logger = logger
        self.timeout = timeout
        self.detection_patterns = {
            ChipType.S300: [
                r's300',
                r'pimchip',
                r'rbl',
                r'cortex-m33'
            ],
            ChipType.ESP32: [
                r'esp32-d0wdq6',
                r'esp32.*wifi.*bt'
            ],
            ChipType.ESP32_C3: [
                r'esp32-c3'
            ],
            ChipType.ESP32_S3: [
                r'esp32-s3'
            ],
            ChipType.STM32: [
                r'stm32f[0-9]',
                r'stm32h[0-9]',
                r'stm32l[0-9]'
            ],
            ChipType.GD32: [
                r'gd32f[0-9]',
                r'gd32e[0-9]'
            ],
            ChipType.CH32: [
                r'ch32f[0-9]',
                r'ch32v[0-9]'
            ]
        }
        
        self.usb_vendor_mapping = {
            "1a86": "可能是S300系列 (CH340串口芯片)",
            "10c4": "可能是ESP32系列 (CP210x串口芯片)",
            "0403": "使用FTDI串口芯片的设备",
            "2341": "可能是Arduino设备"
        }
    
    def find_serial_ports(self) -> List[SerialDevice]:
        """查找所有串口设备"""
        devices = []
        
        # Windows
        if sys.platform.startswith('win'):
            patterns = ['COM*']
        # macOS
        elif sys.platform.startswith('darwin'):
            patterns = ['/dev/cu.usbserial*', '/dev/cu.usbmodem*']
        # Linux
        else:
            patterns = ['/dev/ttyUSB*', '/dev/ttyACM*']
        
        # 查找设备
        for pattern in patterns:
            for port in glob.glob(pattern):
                device = SerialDevice(port=port)
                self._get_device_info(device)
                devices.append(device)
        
        return sorted(devices, key=lambda x: x.port)
    
    def _get_device_info(self, device: SerialDevice):
        """获取设备详细信息"""
        try:
            if sys.platform.startswith('linux'):
                # 使用udevadm获取设备信息
                result = subprocess.run([
                    'udevadm', 'info', '--name', device.port, '--query', 'property'
                ], capture_output=True, text=True, timeout=5)
                
                if result.returncode == 0:
                    output = result.stdout
                    for line in output.split('\n'):
                        if '=' in line:
                            key, value = line.split('=', 1)
                            if key == 'ID_VENDOR_ID':
                                device.vendor_id = value
                            elif key == 'ID_PRODUCT_ID':
                                device.product_id = value
                            elif key == 'ID_VENDOR':
                                device.vendor = value
                            elif key == 'ID_MODEL':
                                device.model = value
        except Exception as e:
            self.logger.debug(f"获取设备信息失败: {e}")
    
    def detect_by_uart_commands(self, port: str, baud: int = 115200) -> Optional[ChipInfo]:
        """通过串口命令检测芯片"""
        if not SERIAL_AVAILABLE:
            self.logger.warning("pyserial未安装，跳过串口检测")
            return None
        
        self.logger.debug(f"尝试串口命令检测: {port}")
        
        commands = [
            b"INFO\r\n",
            b"VERSION\r\n", 
            b"STATUS\r\n",
            b"CHIP_ID\r\n",
            b"\r\n",
            b"?\r\n",
            b"help\r\n"
        ]
        
        try:
            with serial.Serial(port, baud, timeout=self.timeout) as ser:
                time.sleep(0.1)  # 等待串口稳定
                
                for cmd in commands:
                    self.logger.debug(f"发送命令: {cmd}")
                    ser.write(cmd)
                    time.sleep(0.2)
                    
                    # 读取响应
                    response = b""
                    start_time = time.time()
                    while time.time() - start_time < 1:
                        if ser.in_waiting > 0:
                            chunk = ser.read(ser.in_waiting)
                            response += chunk
                        time.sleep(0.01)
                    
                    if len(response) > 5:
                        response_str = response.decode('utf-8', errors='ignore').lower()
                        self.logger.debug(f"收到响应: {response_str[:100]}")
                        
                        # 分析响应
                        chip_info = self._analyze_response(response_str)
                        if chip_info.chip_type != ChipType.UNKNOWN:
                            return chip_info
        
        except Exception as e:
            self.logger.debug(f"串口检测失败: {e}")
        
        return None
    
    def detect_by_usb_info(self, device: SerialDevice) -> Optional[ChipInfo]:
        """通过USB描述符检测芯片"""
        self.logger.debug(f"尝试USB描述符检测: {device.port}")
        
        if device.vendor_id in self.usb_vendor_mapping:
            description = self.usb_vendor_mapping[device.vendor_id]
            self.logger.debug(f"USB信息: {description}")
            
            # 根据厂商ID推测
            if device.vendor_id == "1a86":
                return ChipInfo(ChipType.S300, confidence=0.6)
            elif device.vendor_id == "10c4":
                return ChipInfo(ChipType.ESP32, confidence=0.6)
        
        # 根据产品描述判断
        if device.model:
            model_lower = device.model.lower()
            if "ch340" in model_lower:
                return ChipInfo(ChipType.S300, confidence=0.5)
            elif "cp210" in model_lower:
                return ChipInfo(ChipType.ESP32, confidence=0.5)
        
        return None
    
    def detect_by_reset_sequence(self, port: str, baud: int = 115200) -> Optional[ChipInfo]:
        """通过复位序列检测芯片"""
        if not SERIAL_AVAILABLE:
            return None
        
        self.logger.debug(f"尝试复位序列检测: {port}")
        
        try:
            with serial.Serial(port, baud, timeout=self.timeout) as ser:
                # DTR复位序列
                ser.dtr = True
                time.sleep(0.1)
                ser.dtr = False
                time.sleep(0.5)
                
                # 读取启动信息
                response = b""
                start_time = time.time()
                while time.time() - start_time < 3:
                    if ser.in_waiting > 0:
                        chunk = ser.read(ser.in_waiting())
                        response += chunk
                    time.sleep(0.01)
                
                if len(response) > 10:
                    response_str = response.decode('utf-8', errors='ignore').lower()
                    self.logger.debug(f"复位响应: {response_str[:100]}")
                    
                    chip_info = self._analyze_response(response_str)
                    if chip_info.chip_type != ChipType.UNKNOWN:
                        chip_info.confidence = min(chip_info.confidence + 0.2, 1.0)
                        return chip_info
        
        except Exception as e:
            self.logger.debug(f"复位检测失败: {e}")
        
        return None
    
    def _analyze_response(self, response: str) -> ChipInfo:
        """分析串口响应"""
        response_lower = response.lower()
        
        for chip_type, patterns in self.detection_patterns.items():
            for pattern in patterns:
                if re.search(pattern, response_lower):
                    chip_info = ChipInfo(chip_type, confidence=0.8)
                    
                    # 提取版本信息
                    version_match = re.search(r'v(\d+\.\d+)', response_lower)
                    if version_match:
                        chip_info.version = version_match.group(1)
                    
                    # 提取特征信息
                    if chip_type == ChipType.S300:
                        if "rbl" in response_lower:
                            chip_info.mode = "Bootloader (RBL)"
                        else:
                            chip_info.mode = "Application"
                        
                        # 提取内存信息
                        flash_match = re.search(r'flash:\s*(\d+\w+)', response_lower)
                        if flash_match:
                            chip_info.flash_size = flash_match.group(1)
                        
                        ram_match = re.search(r'ram:\s*(\d+\w+)', response_lower)
                        if ram_match:
                            chip_info.ram_size = ram_match.group(1)
                    
                    elif chip_type in [ChipType.ESP32, ChipType.ESP32_C3, ChipType.ESP32_S3]:
                        if "wifi" in response_lower:
                            chip_info.features.append("WiFi")
                        if "bt" in response_lower or "bluetooth" in response_lower:
                            chip_info.features.append("Bluetooth")
                    
                    return chip_info
        
        return ChipInfo(ChipType.UNKNOWN)
    
    def detect_device(self, device: SerialDevice, baud: int = 115200) -> Optional[ChipInfo]:
        """综合检测单个设备"""
        best_result = None
        best_confidence = 0.0
        
        # 方法1: 串口命令检测
        result = self.detect_by_uart_commands(device.port, baud)
        if result and result.confidence > best_confidence:
            best_result = result
            best_confidence = result.confidence
        
        # 方法2: USB描述符检测
        result = self.detect_by_usb_info(device)
        if result and result.confidence > best_confidence:
            best_result = result
            best_confidence = result.confidence
        
        # 方法3: 复位序列检测
        result = self.detect_by_reset_sequence(device.port, baud)
        if result and result.confidence > best_confidence:
            best_result = result
            best_confidence = result.confidence
        
        return best_result

def print_device_info(device: SerialDevice, chip_info: Optional[ChipInfo], logger: Logger):
    """打印设备信息"""
    logger.header(f"设备: {device.port}")
    
    # 检查权限
    try:
        if os.access(device.port, os.R_OK | os.W_OK):
            print("  权限: ✓ 可读写")
        else:
            print("  权限: ✗ 权限不足")
    except:
        print("  权限: ✗ 无法检查")
    
    # USB信息
    if device.vendor:
        print(f"  厂商: {device.vendor}")
    if device.model:
        print(f"  型号: {device.model}")
    if device.vendor_id and device.product_id:
        print(f"  VID:PID: {device.vendor_id}:{device.product_id}")
    
    # 芯片信息
    if chip_info:
        print(f"  芯片类型: {chip_info.chip_type.value}")
        if chip_info.version:
            print(f"  版本: {chip_info.version}")
        if chip_info.mode:
            print(f"  模式: {chip_info.mode}")
        if chip_info.features:
            print(f"  特性: {', '.join(chip_info.features)}")
        if chip_info.flash_size:
            print(f"  Flash: {chip_info.flash_size}")
        if chip_info.ram_size:
            print(f"  RAM: {chip_info.ram_size}")
        print(f"  置信度: {chip_info.confidence:.1%}")
        
        # 推荐配置
        print("\n推荐配置:")
        if chip_info.chip_type == ChipType.S300:
            print("  下载工具: ./s300_idf.sh")
            print("  波特率: 115200")
            print("  协议: YMODEM")
        elif chip_info.chip_type in [ChipType.ESP32, ChipType.ESP32_C3, ChipType.ESP32_S3]:
            print("  下载工具: esptool.py 或 idf.py")
            print("  波特率: 115200")
            print("  协议: ESP32 Serial Protocol")
        elif chip_info.chip_type == ChipType.STM32:
            print("  下载工具: stm32flash 或 STM32CubeProgrammer")
            print("  波特率: 115200")
            print("  协议: STM32 Serial Protocol")
    else:
        print("  芯片类型: 未识别")

def main():
    parser = argparse.ArgumentParser(
        description="S300 芯片检测工具 (Python版本)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                              # 自动检测所有串口
  %(prog)s /dev/ttyUSB0                # 检测指定串口
  %(prog)s --list                      # 列出串口设备
  %(prog)s --verbose /dev/ttyUSB0      # 详细检测过程
  %(prog)s --baud 921600 /dev/ttyUSB0  # 指定波特率检测
        """
    )
    
    parser.add_argument('port', nargs='?', help='串口设备路径')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('--timeout', '-t', type=int, default=3, help='超时时间 (默认: 3秒)')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细输出')
    parser.add_argument('--list', '-l', action='store_true', help='列出所有串口设备')
    parser.add_argument('--json', action='store_true', help='JSON格式输出')
    
    args = parser.parse_args()
    
    logger = Logger(args.verbose)
    detector = ChipDetector(logger, args.timeout)
    
    if not args.json:
        logger.header("==========================================")
        logger.header("  S300 芯片检测工具 v1.0 (Python版本)")
        logger.header("==========================================")
        print()
    
    # 检查pyserial
    if not SERIAL_AVAILABLE:
        logger.warning("pyserial未安装，部分功能不可用")
        logger.info("安装方法: pip install pyserial")
        print()
    
    # 查找设备
    devices = detector.find_serial_ports()
    
    if args.list:
        if not args.json:
            logger.header("=== 串口设备列表 ===")
        
        if not devices:
            if not args.json:
                logger.warning("未找到串口设备")
            else:
                print(json.dumps({"devices": [], "count": 0}))
            return 1
        
        if args.json:
            device_list = []
            for device in devices:
                device_list.append({
                    "port": device.port,
                    "vendor": device.vendor,
                    "model": device.model,
                    "vendor_id": device.vendor_id,
                    "product_id": device.product_id
                })
            print(json.dumps({"devices": device_list, "count": len(device_list)}, indent=2))
        else:
            print(f"找到 {len(devices)} 个串口设备:\n")
            for device in devices:
                print_device_info(device, None, logger)
                print()
        return 0
    
    # 检测设备
    if args.port:
        # 检测指定设备
        target_device = None
        for device in devices:
            if device.port == args.port:
                target_device = device
                break
        
        if not target_device:
            target_device = SerialDevice(port=args.port)
            detector._get_device_info(target_device)
        
        chip_info = detector.detect_device(target_device, args.baud)
        
        if args.json:
            result = {
                "port": target_device.port,
                "detected": chip_info is not None,
                "chip_type": chip_info.chip_type.value if chip_info else "未知",
                "confidence": chip_info.confidence if chip_info else 0.0
            }
            if chip_info:
                result.update({
                    "version": chip_info.version,
                    "mode": chip_info.mode,
                    "features": chip_info.features,
                    "flash_size": chip_info.flash_size,
                    "ram_size": chip_info.ram_size
                })
            print(json.dumps(result, indent=2))
        else:
            print_device_info(target_device, chip_info, logger)
    else:
        # 自动检测所有设备
        if not devices:
            if not args.json:
                logger.warning("未找到串口设备")
            else:
                print(json.dumps({"devices": [], "detected_count": 0}))
            return 1
        
        if not args.json:
            logger.header("=== 自动检测所有串口设备 ===")
            print(f"找到 {len(devices)} 个串口设备，开始检测...\n")
        
        detected_devices = []
        detected_count = 0
        
        for device in devices:
            chip_info = detector.detect_device(device, args.baud)
            if chip_info and chip_info.chip_type != ChipType.UNKNOWN:
                detected_count += 1
            
            if args.json:
                device_result = {
                    "port": device.port,
                    "detected": chip_info is not None and chip_info.chip_type != ChipType.UNKNOWN,
                    "chip_type": chip_info.chip_type.value if chip_info else "未知",
                    "confidence": chip_info.confidence if chip_info else 0.0
                }
                if chip_info:
                    device_result.update({
                        "version": chip_info.version,
                        "mode": chip_info.mode,
                        "features": chip_info.features
                    })
                detected_devices.append(device_result)
            else:
                print_device_info(device, chip_info, logger)
                print()
        
        if args.json:
            print(json.dumps({
                "devices": detected_devices,
                "total_count": len(devices),
                "detected_count": detected_count
            }, indent=2))
        else:
            logger.header("=== 检测完成 ===")
            if detected_count > 0:
                logger.success(f"成功识别 {detected_count} 个设备")
            else:
                logger.warning("未识别到任何设备")
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}检测被用户中断{Colors.NC}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.RED}ERROR: {e}{Colors.NC}", file=sys.stderr)
        sys.exit(1)
