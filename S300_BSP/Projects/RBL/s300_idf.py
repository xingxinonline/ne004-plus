#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
S300 IDF - ESP32风格的一键下载工具
类似于 idf.py flash 的功能，支持自动复位和下载

使用方法:
  ./s300_idf.py flash                    # 自动下载当前固件
  ./s300_idf.py flash --port /dev/ttyUSB0 # 指定串口
  ./s300_idf.py monitor                  # 串口监控
  ./s300_idf.py flash monitor            # 下载后监控
  ./s300_idf.py build                    # 构建固件
  ./s300_idf.py build flash monitor      # 完整流程

版本: 1.0
日期: 2025-08-30
"""

import os
import sys
import time
import argparse
import subprocess
import threading
import queue
import signal
import glob
from pathlib import Path
from typing import Optional, List
import json

try:
    import serial
    import serial.tools.list_ports
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("Warning: pyserial not installed. Run: pip install pyserial")

# 配置常量
DEFAULT_BAUD_RATE = 115200
DOWNLOAD_TIMEOUT = 30  # 下载超时时间（秒）
RESET_DELAY = 0.1      # 复位延时（秒）
CONNECT_RETRIES = 3    # 连接重试次数

# 颜色输出
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

def colored_print(text, color=Colors.WHITE):
    """彩色打印"""
    print(f"{color}{text}{Colors.END}")

def error_print(text):
    """错误信息打印"""
    colored_print(f"ERROR: {text}", Colors.RED)

def success_print(text):
    """成功信息打印"""
    colored_print(f"SUCCESS: {text}", Colors.GREEN)

def info_print(text):
    """信息打印"""
    colored_print(f"INFO: {text}", Colors.BLUE)

def warning_print(text):
    """警告信息打印"""
    colored_print(f"WARNING: {text}", Colors.YELLOW)

class S300Flasher:
    """S300自动下载器"""
    
    def __init__(self, port: Optional[str] = None, baud_rate: int = DEFAULT_BAUD_RATE):
        self.port = port
        self.baud_rate = baud_rate
        self.serial_conn = None
        self.download_thread = None
        self.stop_flag = threading.Event()
        
    def find_serial_ports(self) -> List[str]:
        """查找可用串口"""
        if not SERIAL_AVAILABLE:
            return []
            
        ports = []
        for port in serial.tools.list_ports.comports():
            # 优先选择USB串口
            if 'USB' in port.description or 'CH340' in port.description or 'CP210' in port.description:
                ports.insert(0, port.device)
            else:
                ports.append(port.device)
        return ports
    
    def auto_detect_port(self) -> Optional[str]:
        """自动检测串口"""
        ports = self.find_serial_ports()
        if not ports:
            return None
            
        info_print(f"Found {len(ports)} serial port(s): {', '.join(ports)}")
        
        # 如果只有一个串口，直接使用
        if len(ports) == 1:
            return ports[0]
            
        # 尝试连接每个串口
        for port in ports:
            if self.test_port_connection(port):
                return port
                
        return ports[0]  # 返回第一个作为默认选择
    
    def test_port_connection(self, port: str) -> bool:
        """测试串口连接"""
        try:
            with serial.Serial(port, self.baud_rate, timeout=1) as ser:
                # 发送简单命令测试
                ser.write(b'\r\n')
                time.sleep(0.1)
                return True
        except:
            return False
    
    def connect_serial(self) -> bool:
        """连接串口"""
        if not SERIAL_AVAILABLE:
            error_print("pyserial not available. Cannot connect to device.")
            return False
            
        # 自动检测端口
        if not self.port:
            self.port = self.auto_detect_port()
            
        if not self.port:
            error_print("No serial port found")
            return False
            
        info_print(f"Connecting to {self.port} at {self.baud_rate} baud...")
        
        try:
            self.serial_conn = serial.Serial(
                self.port, 
                self.baud_rate, 
                timeout=1,
                rtscts=False,
                dsrdtr=False
            )
            success_print(f"Connected to {self.port}")
            return True
        except Exception as e:
            error_print(f"Failed to connect to {self.port}: {e}")
            return False
    
    def trigger_download_mode(self) -> bool:
        """触发下载模式"""
        if not self.serial_conn:
            return False
            
        info_print("Triggering download mode...")
        
        try:
            # 方法1: 发送软件下载命令
            info_print("Attempting software download trigger...")
            self.serial_conn.write(b"DOWNLOAD\r\n")
            time.sleep(0.5)
            
            # 检查响应
            response = self.read_serial_data(timeout=2)
            if "download mode" in response.lower() or "ymodem" in response.lower():
                success_print("Software download mode activated")
                return True
            
            # 方法2: 模拟双重复位
            info_print("Attempting double reset simulation...")
            for i in range(2):
                # 使用DTR信号模拟复位
                self.serial_conn.dtr = True
                time.sleep(0.1)
                self.serial_conn.dtr = False
                time.sleep(0.5 if i == 0 else 0.1)
                
            # 检查是否进入下载模式
            response = self.read_serial_data(timeout=3)
            if "download mode" in response.lower():
                success_print("Double reset download mode activated")
                return True
            
            # 方法3: 在启动窗口期发送按键
            info_print("Attempting serial window activation...")
            self.serial_conn.write(b" ")  # 发送空格键
            time.sleep(0.1)
            
            response = self.read_serial_data(timeout=2)
            if "download" in response.lower():
                success_print("Serial window download mode activated")
                return True
                
            warning_print("Could not trigger download mode automatically")
            return False
            
        except Exception as e:
            error_print(f"Error triggering download mode: {e}")
            return False
    
    def read_serial_data(self, timeout: float = 1.0) -> str:
        """读取串口数据"""
        if not self.serial_conn:
            return ""
            
        start_time = time.time()
        data = ""
        
        while time.time() - start_time < timeout:
            if self.serial_conn.in_waiting > 0:
                try:
                    chunk = self.serial_conn.read(self.serial_conn.in_waiting).decode('utf-8', errors='ignore')
                    data += chunk
                    print(chunk, end='', flush=True)
                except:
                    pass
            time.sleep(0.01)
            
        return data
    
    def wait_for_download_ready(self) -> bool:
        """等待设备准备好接收下载"""
        info_print("Waiting for download ready signal...")
        
        start_time = time.time()
        while time.time() - start_time < DOWNLOAD_TIMEOUT:
            data = self.read_serial_data(timeout=1)
            
            # 检查下载就绪信号
            if any(keyword in data.lower() for keyword in [
                "ymodem", "download mode", "ready for download", 
                "send file", "press any key"
            ]):
                success_print("Device ready for download")
                return True
                
            if "normal boot" in data.lower() or "application" in data.lower():
                warning_print("Device entered normal boot mode")
                return False
        
        error_print("Timeout waiting for download ready signal")
        return False
    
    def download_firmware(self, firmware_path: str) -> bool:
        """下载固件"""
        if not os.path.exists(firmware_path):
            error_print(f"Firmware file not found: {firmware_path}")
            return False
            
        info_print(f"Downloading firmware: {firmware_path}")
        
        # 使用sz命令进行YMODEM传输
        try:
            # 构建sz命令
            cmd = [
                "sz", 
                "--ymodem",
                "--1k",
                firmware_path
            ]
            
            info_print("Starting YMODEM transfer...")
            
            # 启动传输进程
            process = subprocess.Popen(
                cmd,
                stdin=self.serial_conn,
                stdout=self.serial_conn,
                stderr=subprocess.PIPE
            )
            
            # 等待传输完成
            stdout, stderr = process.communicate(timeout=DOWNLOAD_TIMEOUT)
            
            if process.returncode == 0:
                success_print("Firmware download completed")
                return True
            else:
                error_print(f"Download failed: {stderr.decode()}")
                return False
                
        except subprocess.TimeoutExpired:
            error_print("Download timeout")
            process.kill()
            return False
        except FileNotFoundError:
            error_print("sz command not found. Please install lrzsz package.")
            return False
        except Exception as e:
            error_print(f"Download error: {e}")
            return False
    
    def reset_device(self):
        """复位设备"""
        if self.serial_conn:
            info_print("Resetting device...")
            self.serial_conn.dtr = True
            time.sleep(RESET_DELAY)
            self.serial_conn.dtr = False
            time.sleep(0.5)
    
    def close(self):
        """关闭连接"""
        if self.serial_conn:
            self.serial_conn.close()
            self.serial_conn = None

class S300IDF:
    """S300 IDF主类"""
    
    def __init__(self):
        self.project_dir = Path.cwd()
        self.build_dir = self.project_dir / "GCC" / "build"
        self.config_file = self.project_dir / "s300_idf_config.json"
        self.config = self.load_config()
        
    def load_config(self) -> dict:
        """加载配置"""
        default_config = {
            "port": None,
            "baud_rate": DEFAULT_BAUD_RATE,
            "firmware_name": "rbl.bin",
            "build_command": ["make", "-C", "GCC"],
            "clean_command": ["make", "-C", "GCC", "clean"]
        }
        
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                default_config.update(config)
            except:
                pass
                
        return default_config
    
    def save_config(self):
        """保存配置"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
        except:
            pass
    
    def find_firmware_file(self) -> Optional[str]:
        """查找固件文件"""
        # 查找顺序
        search_paths = [
            self.build_dir / self.config["firmware_name"],
            self.build_dir / "*.bin",
            self.project_dir / "*.bin",
            self.project_dir / "GCC" / "*.bin"
        ]
        
        for path in search_paths:
            if "*" in str(path):
                files = glob.glob(str(path))
                if files:
                    return files[0]
            elif path.exists():
                return str(path)
                
        return None
    
    def build_firmware(self) -> bool:
        """构建固件"""
        info_print("Building firmware...")
        
        try:
            result = subprocess.run(
                self.config["build_command"],
                cwd=self.project_dir,
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                success_print("Build completed successfully")
                print(result.stdout)
                return True
            else:
                error_print("Build failed")
                print(result.stderr)
                return False
                
        except Exception as e:
            error_print(f"Build error: {e}")
            return False
    
    def clean_build(self) -> bool:
        """清理构建"""
        info_print("Cleaning build...")
        
        try:
            result = subprocess.run(
                self.config["clean_command"],
                cwd=self.project_dir,
                capture_output=True,
                text=True
            )
            
            success_print("Clean completed")
            return result.returncode == 0
            
        except Exception as e:
            error_print(f"Clean error: {e}")
            return False
    
    def flash_firmware(self, port: Optional[str] = None) -> bool:
        """烧录固件"""
        # 查找固件文件
        firmware_file = self.find_firmware_file()
        if not firmware_file:
            error_print("No firmware file found")
            return False
            
        info_print(f"Found firmware: {firmware_file}")
        
        # 创建下载器
        flasher = S300Flasher(port or self.config["port"], self.config["baud_rate"])
        
        try:
            # 连接设备
            if not flasher.connect_serial():
                return False
            
            # 更新配置中的端口
            if flasher.port != self.config["port"]:
                self.config["port"] = flasher.port
                self.save_config()
            
            # 触发下载模式
            if not flasher.trigger_download_mode():
                error_print("Failed to enter download mode")
                warning_print("Please manually reset device and try again")
                return False
            
            # 等待下载就绪
            if not flasher.wait_for_download_ready():
                return False
            
            # 下载固件
            if not flasher.download_firmware(firmware_file):
                return False
            
            # 复位设备
            flasher.reset_device()
            
            success_print("Flash operation completed successfully!")
            return True
            
        except KeyboardInterrupt:
            warning_print("Flash operation cancelled by user")
            return False
        except Exception as e:
            error_print(f"Flash operation failed: {e}")
            return False
        finally:
            flasher.close()
    
    def monitor_serial(self, port: Optional[str] = None):
        """串口监控"""
        target_port = port or self.config["port"]
        
        if not target_port:
            flasher = S300Flasher()
            target_port = flasher.auto_detect_port()
            
        if not target_port:
            error_print("No serial port available for monitoring")
            return
            
        info_print(f"Starting serial monitor on {target_port}")
        info_print("Press Ctrl+C to exit")
        
        try:
            if SERIAL_AVAILABLE:
                with serial.Serial(target_port, self.config["baud_rate"], timeout=1) as ser:
                    while True:
                        if ser.in_waiting > 0:
                            data = ser.read(ser.in_waiting)
                            print(data.decode('utf-8', errors='ignore'), end='', flush=True)
                        time.sleep(0.01)
            else:
                # 回退到系统工具
                subprocess.run(["screen", target_port, str(self.config["baud_rate"])])
                
        except KeyboardInterrupt:
            info_print("\nSerial monitor stopped")
        except Exception as e:
            error_print(f"Monitor error: {e}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="S300 IDF - ESP32-style development tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s build                     Build the firmware
  %(prog)s flash                     Flash firmware to device
  %(prog)s monitor                   Monitor serial output
  %(prog)s build flash monitor       Complete development cycle
  %(prog)s clean                     Clean build files
  %(prog)s --port /dev/ttyUSB0 flash Flash using specific port
        """
    )
    
    parser.add_argument(
        'commands', 
        nargs='+', 
        choices=['build', 'flash', 'monitor', 'clean'],
        help='Commands to execute'
    )
    
    parser.add_argument(
        '--port', '-p',
        help='Serial port to use'
    )
    
    parser.add_argument(
        '--baud-rate', '-b',
        type=int,
        default=DEFAULT_BAUD_RATE,
        help=f'Baud rate (default: {DEFAULT_BAUD_RATE})'
    )
    
    args = parser.parse_args()
    
    # 创建IDF实例
    idf = S300IDF()
    if args.port:
        idf.config["port"] = args.port
    if args.baud_rate != DEFAULT_BAUD_RATE:
        idf.config["baud_rate"] = args.baud_rate
    
    # 打印banner
    colored_print("=" * 60, Colors.CYAN)
    colored_print("  S300 IDF - ESP32-style Development Tool v1.0", Colors.CYAN)
    colored_print("=" * 60, Colors.CYAN)
    
    # 执行命令
    success = True
    for command in args.commands:
        if not success:
            break
            
        if command == 'clean':
            success = idf.clean_build()
        elif command == 'build':
            success = idf.build_firmware()
        elif command == 'flash':
            success = idf.flash_firmware(args.port)
        elif command == 'monitor':
            idf.monitor_serial(args.port)
            # monitor不影响后续命令
            
    if success and 'monitor' not in args.commands:
        success_print("All operations completed successfully!")
    elif not success:
        error_print("Operation failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
