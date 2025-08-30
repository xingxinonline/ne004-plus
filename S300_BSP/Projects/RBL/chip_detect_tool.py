#!/usr/bin/env python3
"""
S300 芯片检测工具 - ESP32兼容版本
类似 esptool.py 的芯片检测功能

用法:
    python3 chip_detect_tool.py                    # 自动检测
    python3 chip_detect_tool.py --port /dev/ttyUSB0 # 检测指定端口
    python3 chip_detect_tool.py --list             # 列出所有端口
"""

import sys
import time
import serial
import serial.tools.list_ports
import struct
import argparse
from typing import Optional, Dict, List, Tuple

class ChipDetector:
    """芯片检测器类"""
    
    # ESP32 命令定义
    ESP_SYNC = b'\x00\x08\x24\x00\x00\x00\x00\x00\x07\x07\x12\x20'
    ESP_FLASH_BEGIN = b'\x00\x02\x24\x00\x00\x00\x00\x00\x00\x00\x00\x00'
    ESP_READ_REG = b'\x00\x0a\x24\x00\x00\x00\x00\x00'
    
    # 常用波特率
    BAUD_RATES = [115200, 921600, 460800, 230400, 74880, 57600, 38400, 19200, 9600]
    
    # 芯片类型识别
    CHIP_DETECT_MAGIC = {
        0x00f01d83: "ESP32",
        0x6921506f: "ESP32-C3", 
        0x1b31506f: "ESP32-S3",
        0x2ce0806f: "ESP32-C6",
        0x530001:   "PiMCHIP S300",
    }
    
    def __init__(self):
        self.debug = False
    
    def log(self, msg: str, level: str = "INFO"):
        """日志输出"""
        if self.debug or level in ["INFO", "ERROR"]:
            print(f"[{level}] {msg}")
    
    def list_serial_ports(self) -> List[str]:
        """列出所有可用串口"""
        ports = []
        for port in serial.tools.list_ports.comports():
            ports.append(port.device)
        return sorted(ports)
    
    def open_serial_port(self, port: str, baud_rate: int = 115200) -> Optional[serial.Serial]:
        """打开串口"""
        try:
            ser = serial.Serial(
                port=port,
                baudrate=baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=3.0,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False
            )
            return ser
        except Exception as e:
            self.log(f"Failed to open {port}: {e}", "ERROR")
            return None
    
    def esp32_reset_sequence(self, ser: serial.Serial):
        """ESP32进入下载模式的复位序列"""
        # DTR=False, RTS=True
        ser.setDTR(False)
        ser.setRTS(True)
        time.sleep(0.1)
        
        # DTR=True, RTS=False  
        ser.setDTR(True)
        ser.setRTS(False)
        time.sleep(0.05)
        
        # DTR=False, RTS=False
        ser.setDTR(False)
        ser.setRTS(False)
        time.sleep(0.05)
    
    def send_command(self, ser: serial.Serial, command: bytes, timeout: float = 1.0) -> bytes:
        """发送命令并接收响应"""
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        ser.write(command)
        ser.flush()
        
        response = b''
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            if ser.in_waiting:
                chunk = ser.read(ser.in_waiting)
                response += chunk
                
            if len(response) > 0:
                time.sleep(0.01)  # 等待更多数据
                if ser.in_waiting == 0:
                    break
        
        return response
    
    def detect_esp32_chip(self, ser: serial.Serial) -> Optional[Dict]:
        """检测ESP32芯片"""
        self.log("Trying ESP32 detection...")
        
        # 执行ESP32复位序列
        self.esp32_reset_sequence(ser)
        time.sleep(0.1)
        
        # 发送SYNC命令
        response = self.send_command(ser, self.ESP_SYNC, 1.0)
        
        if b'OHAI' in response:
            self.log("ESP32 SYNC response received")
            
            # 读取芯片ID寄存器 (0x40001000)
            read_reg_cmd = self.ESP_READ_REG + struct.pack('<I', 0x40001000)
            reg_response = self.send_command(ser, read_reg_cmd, 1.0)
            
            chip_info = {
                'type': 'ESP32',
                'name': 'ESP32',
                'family': 'ESP32',
                'bootloader_mode': True,
                'raw_response': response.hex()
            }
            
            # 尝试解析芯片ID
            if len(reg_response) >= 8:
                try:
                    chip_id = struct.unpack('<I', reg_response[4:8])[0]
                    chip_info['chip_id'] = f"0x{chip_id:08x}"
                    
                    if chip_id in self.CHIP_DETECT_MAGIC:
                        chip_info['name'] = self.CHIP_DETECT_MAGIC[chip_id]
                        chip_info['type'] = self.CHIP_DETECT_MAGIC[chip_id]
                except:
                    pass
            
            return chip_info
        
        return None
    
    def detect_at_chip(self, ser: serial.Serial) -> Optional[Dict]:
        """检测AT命令芯片 (ESP8266等)"""
        self.log("Trying AT command detection...")
        
        at_commands = [
            b'AT\\r\\n',
            b'AT+GMR\\r\\n',     # 版本信息
            b'AT+CHIPID\\r\\n',  # 芯片ID
        ]
        
        for cmd in at_commands:
            response = self.send_command(ser, cmd, 1.0)
            response_str = response.decode('utf-8', errors='ignore')
            
            if 'OK' in response_str or 'ESP' in response_str:
                chip_info = {
                    'type': 'AT Compatible',
                    'name': 'AT Compatible Device',
                    'family': 'Unknown',
                    'bootloader_mode': False,
                    'raw_response': response_str
                }
                
                # 解析具体类型
                if 'ESP8266' in response_str:
                    chip_info.update({
                        'type': 'ESP8266',
                        'name': 'ESP8266',
                        'family': 'ESP8266'
                    })
                elif 'ESP32' in response_str:
                    chip_info.update({
                        'type': 'ESP32',
                        'name': 'ESP32 (AT Mode)',
                        'family': 'ESP32'
                    })
                
                return chip_info
        
        return None
    
    def detect_s300_chip(self, ser: serial.Serial) -> Optional[Dict]:
        """检测S300芯片"""
        self.log("Trying S300 detection...")
        
        s300_commands = [
            b'INFO\\r\\n',
            b'VERSION\\r\\n', 
            b'CHIPID\\r\\n',
            b'HELP\\r\\n'
        ]
        
        for cmd in s300_commands:
            response = self.send_command(ser, cmd, 1.0)
            response_str = response.decode('utf-8', errors='ignore')
            
            if 'S300' in response_str or 'PiMCHIP' in response_str or 'RBL' in response_str:
                return {
                    'type': 'PiMCHIP S300',
                    'name': 'PiMCHIP S300 (Cortex-M4)',
                    'family': 'PiMCHIP',
                    'chip_id': '0x530001',
                    'flash_size': '16MB',
                    'ram_size': '384KB',
                    'cpu_freq': '200MHz',
                    'bootloader_mode': True,
                    'raw_response': response_str
                }
        
        return None
    
    def detect_chip_on_port(self, port: str, baud_rate: int = 0) -> Optional[Dict]:
        """在指定端口检测芯片"""
        baud_rates = [baud_rate] if baud_rate > 0 else self.BAUD_RATES
        
        for baud in baud_rates:
            self.log(f"Trying {port} @ {baud} baud...")
            
            ser = self.open_serial_port(port, baud)
            if not ser:
                continue
            
            try:
                # 尝试各种检测方法
                detection_methods = [
                    self.detect_esp32_chip,
                    self.detect_at_chip, 
                    self.detect_s300_chip
                ]
                
                for method in detection_methods:
                    result = method(ser)
                    if result:
                        result.update({
                            'port': port,
                            'baud_rate': baud,
                            'detection_method': method.__name__
                        })
                        return result
                
            except Exception as e:
                self.log(f"Error during detection: {e}", "ERROR")
            finally:
                ser.close()
        
        return None
    
    def auto_detect(self) -> List[Dict]:
        """自动检测所有端口"""
        ports = self.list_serial_ports()
        if not ports:
            self.log("No serial ports found", "ERROR")
            return []
        
        self.log(f"Found {len(ports)} serial ports: {', '.join(ports)}")
        
        detected_chips = []
        for port in ports:
            self.log(f"\\nScanning {port}...")
            result = self.detect_chip_on_port(port)
            if result:
                detected_chips.append(result)
                self.log(f"✓ Found {result['name']} on {port}")
            else:
                self.log(f"✗ No chip detected on {port}")
        
        return detected_chips
    
    def print_detection_result(self, result: Dict):
        """打印检测结果 (ESP32兼容格式)"""
        print("\\n" + "="*50)
        print("Chip Detection Result")
        print("="*50)
        print(f"Port: {result['port']} @ {result['baud_rate']} baud")
        print(f"Chip Type: {result['type']}")
        print(f"Chip Name: {result['name']}")
        print(f"Chip Family: {result['family']}")
        
        if 'chip_id' in result:
            print(f"Chip ID: {result['chip_id']}")
        if 'flash_size' in result:
            print(f"Flash Size: {result['flash_size']}")
        if 'ram_size' in result:
            print(f"RAM Size: {result['ram_size']}")
        if 'cpu_freq' in result:
            print(f"CPU Frequency: {result['cpu_freq']}")
        
        print(f"Bootloader Mode: {'Yes' if result.get('bootloader_mode') else 'No'}")
        print(f"Detection Method: {result.get('detection_method', 'Unknown')}")
        
        if result.get('raw_response') and self.debug:
            print(f"\\nRaw Response: {result['raw_response']}")
        
        print("="*50)

def main():
    parser = argparse.ArgumentParser(description='S300 Chip Detection Tool (ESP32 Compatible)')
    parser.add_argument('--port', '-p', help='Serial port to scan')
    parser.add_argument('--baud', '-b', type=int, default=0, help='Baud rate (0=auto)')
    parser.add_argument('--list', '-l', action='store_true', help='List all serial ports')
    parser.add_argument('--debug', '-d', action='store_true', help='Enable debug output')
    
    args = parser.parse_args()
    
    detector = ChipDetector()
    detector.debug = args.debug
    
    if args.list:
        ports = detector.list_serial_ports()
        if ports:
            print("Available serial ports:")
            for i, port in enumerate(ports, 1):
                print(f"  {i}. {port}")
        else:
            print("No serial ports found")
        return
    
    if args.port:
        # 检测指定端口
        print(f"Detecting chip on {args.port}...")
        result = detector.detect_chip_on_port(args.port, args.baud)
        if result:
            detector.print_detection_result(result)
        else:
            print(f"No chip detected on {args.port}")
    else:
        # 自动检测所有端口
        print("Auto-detecting chips on all serial ports...")
        results = detector.auto_detect()
        
        if results:
            print(f"\\n🎉 Found {len(results)} chip(s):")
            for i, result in enumerate(results, 1):
                print(f"\\n{i}. {result['name']} on {result['port']}")
                if args.debug:
                    detector.print_detection_result(result)
        else:
            print("\\n❌ No chips detected on any port")
            print("\\nTroubleshooting:")
            print("1. Check if device is connected and powered")
            print("2. Try different USB cables")
            print("3. Check device drivers (esp. for ESP32/ESP8266)")
            print("4. Use --debug flag for more information")

if __name__ == '__main__':
    main()
