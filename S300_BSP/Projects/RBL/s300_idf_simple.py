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

依赖安装:
  pip install pyserial

版本: 1.0
日期: 2025-08-30
"""

import os
import sys
import time
import argparse
import subprocess
import threading
import signal
import glob
import json
from pathlib import Path
from typing import Optional, List

# 尝试导入串口库
try:
    import serial
    import serial.tools.list_ports
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

# 配置常量
DEFAULT_BAUD_RATE = 115200
DOWNLOAD_TIMEOUT = 30
RESET_DELAY = 0.1
CONNECT_RETRIES = 3

# 颜色输出类
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def print_colored(text: str, color: str = Colors.END):
    """彩色打印"""
    print(f"{color}{text}{Colors.END}")

def error(text: str):
    print_colored(f"ERROR: {text}", Colors.RED)

def success(text: str):
    print_colored(f"SUCCESS: {text}", Colors.GREEN)

def info(text: str):
    print_colored(f"INFO: {text}", Colors.BLUE)

def warning(text: str):
    print_colored(f"WARNING: {text}", Colors.YELLOW)

class AutoDownloader:
    """自动下载器 - 实现ESP32风格的自动下载"""
    
    def __init__(self, port: Optional[str] = None, baud_rate: int = DEFAULT_BAUD_RATE):
        self.port = port
        self.baud_rate = baud_rate
        self.serial_conn = None
        
    def find_ports(self) -> List[str]:
        """查找串口"""
        if not SERIAL_AVAILABLE:
            # 回退到系统方法
            ports = []
            for dev in ['/dev/ttyUSB*', '/dev/ttyACM*', '/dev/cu.usbserial*']:
                ports.extend(glob.glob(dev))
            return sorted(ports)
        
        ports = []
        try:
            for port in serial.tools.list_ports.comports():
                ports.append(port.device)
        except:
            pass
        return sorted(ports)
    
    def auto_detect_port(self) -> Optional[str]:
        """自动检测串口"""
        ports = self.find_ports()
        if not ports:
            error("No serial ports found")
            return None
        
        info(f"Found ports: {', '.join(ports)}")
        
        if len(ports) == 1:
            return ports[0]
        
        # 优先选择USB串口
        for port in ports:
            if 'USB' in port.upper() or 'ACM' in port.upper():
                return port
        
        return ports[0]
    
    def connect(self) -> bool:
        """连接串口"""
        if not self.port:
            self.port = self.auto_detect_port()
        
        if not self.port:
            return False
        
        info(f"Connecting to {self.port} at {self.baud_rate}")
        
        if not SERIAL_AVAILABLE:
            warning("pyserial not available, using fallback method")
            return True
        
        try:
            self.serial_conn = serial.Serial(
                self.port, 
                self.baud_rate, 
                timeout=1,
                rtscts=False,
                dsrdtr=False
            )
            success(f"Connected to {self.port}")
            return True
        except Exception as e:
            error(f"Connection failed: {e}")
            return False
    
    def send_download_trigger(self) -> bool:
        """发送下载触发信号"""
        if not SERIAL_AVAILABLE or not self.serial_conn:
            # 使用外部工具作为回退
            return self._trigger_with_external_tool()
        
        info("Triggering download mode...")
        
        try:
            # 方法1: 软件触发
            self.serial_conn.write(b"DOWNLOAD\r\n")
            time.sleep(0.5)
            
            # 检查响应
            response = self._read_response(2)
            if "download" in response.lower():
                success("Software trigger successful")
                return True
            
            # 方法2: 模拟双重复位
            info("Trying double reset...")
            for _ in range(2):
                self.serial_conn.dtr = True
                time.sleep(0.1)
                self.serial_conn.dtr = False
                time.sleep(0.3)
            
            response = self._read_response(3)
            if "download" in response.lower():
                success("Double reset successful")
                return True
            
            # 方法3: 串口窗口触发
            info("Trying serial window...")
            self.serial_conn.write(b" \r\n")
            time.sleep(0.1)
            
            response = self._read_response(2)
            if "download" in response.lower():
                success("Serial window trigger successful")
                return True
            
            warning("Auto trigger failed, please manually enter download mode")
            return False
            
        except Exception as e:
            error(f"Trigger failed: {e}")
            return False
    
    def _read_response(self, timeout: float) -> str:
        """读取串口响应"""
        if not self.serial_conn:
            return ""
        
        start = time.time()
        data = ""
        
        while time.time() - start < timeout:
            if self.serial_conn.in_waiting > 0:
                try:
                    chunk = self.serial_conn.read(self.serial_conn.in_waiting)
                    text = chunk.decode('utf-8', errors='ignore')
                    data += text
                    print(text, end='', flush=True)
                except:
                    pass
            time.sleep(0.01)
        
        return data
    
    def _trigger_with_external_tool(self) -> bool:
        """使用外部工具触发下载模式"""
        info("Using external tool for download trigger...")
        
        # 这里可以集成其他下载工具
        warning("External trigger not implemented yet")
        warning("Please manually enter download mode:")
        warning("1. Reset device twice within 3 seconds, OR")
        warning("2. Press any key in 5-second window, OR") 
        warning("3. Send 'DOWNLOAD' command via serial")
        
        input("Press Enter when device is in download mode...")
        return True
    
    def download_file(self, firmware_path: str) -> bool:
        """下载文件"""
        if not os.path.exists(firmware_path):
            error(f"Firmware file not found: {firmware_path}")
            return False
        
        info(f"Downloading {firmware_path}")
        
        # 使用sz/rz进行YMODEM传输
        if self._download_with_sz(firmware_path):
            return True
        
        # 回退方法
        return self._download_with_alternative(firmware_path)
    
    def _download_with_sz(self, firmware_path: str) -> bool:
        """使用sz命令下载"""
        try:
            cmd = ["sz", "--ymodem", "--1k", firmware_path]
            
            if SERIAL_AVAILABLE and self.serial_conn:
                # 直接通过串口连接
                info("Starting YMODEM transfer with sz...")
                
                # 这里需要特殊处理，因为sz需要直接访问串口设备
                # 关闭当前连接，让sz直接使用
                self.serial_conn.close()
                
                # 使用系统调用
                env = os.environ.copy()
                result = subprocess.run(
                    ["sz", "--ymodem", "--1k", firmware_path],
                    input=f"redirect-device {self.port}\n".encode(),
                    capture_output=True
                )
                
                if result.returncode == 0:
                    success("Download completed with sz")
                    return True
            else:
                info("sz command not available or no serial connection")
                
        except FileNotFoundError:
            info("sz command not found")
        except Exception as e:
            warning(f"sz transfer failed: {e}")
        
        return False
    
    def _download_with_alternative(self, firmware_path: str) -> bool:
        """替代下载方法"""
        info("Using alternative download method...")
        
        # 检查是否有项目特定的下载脚本
        project_scripts = [
            "flash_program.sh",
            "flash_programmer.py", 
            "../flash_program.sh"
        ]
        
        for script in project_scripts:
            if os.path.exists(script):
                info(f"Using project script: {script}")
                try:
                    result = subprocess.run([script, "program"], 
                                          capture_output=True, text=True)
                    if result.returncode == 0:
                        success("Download completed with project script")
                        return True
                    else:
                        warning(f"Script failed: {result.stderr}")
                except:
                    pass
        
        # 手动指导
        warning("No automated download method available")
        print(f"\nPlease manually download the firmware:")
        print(f"1. Ensure device is in download mode")
        print(f"2. Use your preferred tool to send: {firmware_path}")
        print(f"3. Protocol: YMODEM")
        print(f"4. Port: {self.port}")
        print(f"5. Baud rate: {self.baud_rate}")
        
        response = input("\nDownload completed successfully? (y/N): ")
        return response.lower().startswith('y')
    
    def reset_device(self):
        """复位设备"""
        if SERIAL_AVAILABLE and self.serial_conn:
            info("Resetting device...")
            self.serial_conn.dtr = True
            time.sleep(RESET_DELAY)
            self.serial_conn.dtr = False
            time.sleep(0.5)
        else:
            info("Please manually reset the device")
    
    def close(self):
        """关闭连接"""
        if self.serial_conn:
            self.serial_conn.close()

class S300IDF:
    """S300 IDF主类"""
    
    def __init__(self):
        self.project_dir = Path.cwd()
        self.build_dir = self.project_dir / "GCC" / "build"
        self.config = self._load_config()
    
    def _load_config(self) -> dict:
        """加载配置"""
        return {
            "port": None,
            "baud_rate": DEFAULT_BAUD_RATE,
            "firmware_name": "rbl.bin",
            "build_cmd": ["make", "-C", "GCC"],
            "clean_cmd": ["make", "-C", "GCC", "clean"]
        }
    
    def find_firmware(self) -> Optional[str]:
        """查找固件文件"""
        candidates = [
            self.build_dir / self.config["firmware_name"],
            self.build_dir / "*.bin",
            self.project_dir / "*.bin"
        ]
        
        for path in candidates:
            if "*" in str(path):
                files = glob.glob(str(path))
                if files:
                    return files[0]
            elif path.exists():
                return str(path)
        
        return None
    
    def build(self) -> bool:
        """构建固件"""
        info("Building firmware...")
        
        try:
            result = subprocess.run(
                self.config["build_cmd"],
                cwd=self.project_dir,
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                success("Build completed")
                if result.stdout:
                    print(result.stdout)
                return True
            else:
                error("Build failed")
                if result.stderr:
                    print(result.stderr)
                return False
                
        except Exception as e:
            error(f"Build error: {e}")
            return False
    
    def clean(self) -> bool:
        """清理构建"""
        info("Cleaning...")
        
        try:
            subprocess.run(self.config["clean_cmd"], cwd=self.project_dir)
            success("Clean completed")
            return True
        except Exception as e:
            error(f"Clean error: {e}")
            return False
    
    def flash(self, port: Optional[str] = None) -> bool:
        """烧录固件"""
        # 查找固件
        firmware = self.find_firmware()
        if not firmware:
            error("No firmware file found. Please build first.")
            return False
        
        info(f"Found firmware: {firmware}")
        
        # 创建下载器
        downloader = AutoDownloader(port or self.config["port"], self.config["baud_rate"])
        
        try:
            # 连接
            if not downloader.connect():
                return False
            
            # 触发下载模式
            if not downloader.send_download_trigger():
                error("Failed to enter download mode")
                return False
            
            # 下载固件
            if not downloader.download_file(firmware):
                return False
            
            # 复位
            downloader.reset_device()
            
            success("Flash completed successfully!")
            return True
            
        except KeyboardInterrupt:
            warning("Flash cancelled by user")
            return False
        except Exception as e:
            error(f"Flash failed: {e}")
            return False
        finally:
            downloader.close()
    
    def monitor(self, port: Optional[str] = None):
        """串口监控"""
        target_port = port or self.config["port"]
        
        if not target_port:
            downloader = AutoDownloader()
            target_port = downloader.auto_detect_port()
        
        if not target_port:
            error("No port available for monitoring")
            return
        
        info(f"Starting monitor on {target_port}")
        info("Press Ctrl+C to exit")
        
        try:
            if SERIAL_AVAILABLE:
                with serial.Serial(target_port, self.config["baud_rate"], timeout=1) as ser:
                    while True:
                        if ser.in_waiting > 0:
                            data = ser.read(ser.in_waiting)
                            print(data.decode('utf-8', errors='ignore'), end='', flush=True)
                        time.sleep(0.01)
            else:
                # 使用系统工具
                try:
                    subprocess.run(["screen", target_port, str(self.config["baud_rate"])])
                except:
                    try:
                        subprocess.run(["minicom", "-D", target_port, "-b", str(self.config["baud_rate"])])
                    except:
                        error("No serial monitor tool available. Please install screen or minicom.")
                        
        except KeyboardInterrupt:
            info("\nMonitor stopped")
        except Exception as e:
            error(f"Monitor error: {e}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="S300 IDF - ESP32-style development tool",
        epilog="""
Examples:
  %(prog)s build                     Build firmware
  %(prog)s flash                     Flash firmware
  %(prog)s monitor                   Monitor serial output
  %(prog)s build flash monitor       Complete cycle
  %(prog)s --port /dev/ttyUSB0 flash Flash with specific port
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
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
    
    # 检查依赖
    if not SERIAL_AVAILABLE and ('flash' in args.commands or 'monitor' in args.commands):
        warning("pyserial not installed. Some features may be limited.")
        warning("Install with: pip install pyserial")
    
    # 创建IDF实例
    idf = S300IDF()
    if args.port:
        idf.config["port"] = args.port
    if args.baud_rate != DEFAULT_BAUD_RATE:
        idf.config["baud_rate"] = args.baud_rate
    
    # 打印标题
    print_colored("=" * 50, Colors.CYAN)
    print_colored("  S300 IDF - ESP32-style Tool v1.0", Colors.CYAN)
    print_colored("=" * 50, Colors.CYAN)
    
    # 执行命令
    success_flag = True
    for command in args.commands:
        if not success_flag:
            break
        
        if command == 'clean':
            success_flag = idf.clean()
        elif command == 'build':
            success_flag = idf.build()
        elif command == 'flash':
            success_flag = idf.flash(args.port)
        elif command == 'monitor':
            idf.monitor(args.port)
    
    if success_flag and 'monitor' not in args.commands:
        success("All operations completed!")
    elif not success_flag:
        error("Some operations failed!")
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_colored("\nOperation cancelled by user", Colors.YELLOW)
        sys.exit(1)
