#!/usr/bin/env python3
"""
S300 RBL Ymodem 测试脚本
用于验证Ymodem协议的正确性和性能
"""

import serial
import time
import sys
import struct
import os
from typing import Optional

class YmodemTester:
    """Ymodem协议测试工具"""
    
    # Ymodem协议常量
    SOH = 0x01  # 128字节包头
    STX = 0x02  # 1024字节包头
    EOT = 0x04  # 传输结束
    ACK = 0x06  # 确认
    NAK = 0x15  # 重传请求
    CAN = 0x18  # 取消传输
    CRC16_INIT = 0x43  # 'C' - 请求CRC模式

    def __init__(self, port: str, baudrate: int = 115200):
        """初始化串口连接"""
        self.port = port
        self.baudrate = baudrate
        self.ser: Optional[serial.Serial] = None
        
    def connect(self) -> bool:
        """连接串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=8,
                parity='N',
                stopbits=1,
                timeout=1
            )
            print(f"✅ 已连接到 {self.port} @ {self.baudrate}bps")
            return True
        except Exception as e:
            print(f"❌ 串口连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开串口连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("📱 串口已断开")
    
    def wait_for_prompt(self, timeout: int = 10) -> bool:
        """等待RBL提示符"""
        start_time = time.time()
        buffer = b""
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                buffer += data
                print(data.decode('utf-8', errors='ignore'), end='')
                
                # 检查是否收到提示符
                if b'[RBL READY]' in buffer or b'> ' in buffer:
                    return True
            time.sleep(0.1)
        
        return False
    
    def send_command(self, cmd: str) -> bool:
        """发送命令到RBL"""
        try:
            self.ser.write(f"{cmd}\r\n".encode())
            return True
        except Exception as e:
            print(f"❌ 发送命令失败: {e}")
            return False
    
    def test_info_command(self) -> bool:
        """测试INFO命令"""
        print("\n🔍 测试INFO命令...")
        self.send_command("INFO")
        time.sleep(2)
        
        # 读取响应
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            print("📋 芯片信息:")
            print(response.decode('utf-8', errors='ignore'))
            return True
        
        return False
    
    def test_erase_command(self) -> bool:
        """测试ERASE命令"""
        print("\n🧹 测试ERASE命令...")
        self.send_command("ERASE")
        time.sleep(5)  # 擦除需要时间
        
        # 读取响应
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            print("🗑️ 擦除结果:")
            print(response.decode('utf-8', errors='ignore'))
            return True
        
        return False
    
    def create_test_file(self, filename: str, size: int = 1024) -> bool:
        """创建测试文件"""
        try:
            with open(filename, 'wb') as f:
                # 创建有规律的测试数据
                pattern = b"S300_TEST_"
                for i in range(size // len(pattern) + 1):
                    f.write(pattern)
                
                # 截断到指定大小
                f.truncate(size)
            
            print(f"📄 已创建测试文件: {filename} ({size} bytes)")
            return True
        except Exception as e:
            print(f"❌ 创建测试文件失败: {e}")
            return False
    
    def test_ymodem_protocol(self, filename: str) -> bool:
        """测试Ymodem协议（模拟）"""
        print(f"\n📡 测试Ymodem协议 - 文件: {filename}")
        
        # 发送YMODEM命令
        self.send_command("YMODEM")
        time.sleep(1)
        
        # 等待RBL准备接收
        start_time = time.time()
        while time.time() - start_time < 10:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                response = data.decode('utf-8', errors='ignore')
                print(response, end='')
                
                if "Please send file using Ymodem protocol" in response:
                    print("✅ RBL已准备接收Ymodem数据")
                    print("📝 注意: 请在串口工具中手动发送文件")
                    print("   建议使用Tera Term或SecureCRT的Ymodem功能")
                    return True
            
            time.sleep(0.1)
        
        print("❌ RBL未进入Ymodem接收模式")
        return False
    
    def monitor_ymodem_transfer(self, timeout: int = 120) -> bool:
        """监控Ymodem传输过程"""
        print("\n📊 监控Ymodem传输...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                response = data.decode('utf-8', errors='ignore')
                print(response, end='')
                
                # 检查传输完成标志
                if "File reception completed" in response:
                    print("\n✅ Ymodem传输完成!")
                    return True
                elif "transmission failed" in response.lower():
                    print("\n❌ Ymodem传输失败!")
                    return False
            
            time.sleep(0.1)
        
        print("\n⏰ 传输监控超时")
        return False
    
    def run_complete_test(self, test_file: str = "test_firmware.bin"):
        """运行完整测试流程"""
        print("🚀 S300 RBL Ymodem 测试开始")
        print("=" * 50)
        
        # 1. 连接串口
        if not self.connect():
            return False
        
        try:
            # 2. 等待RBL提示符
            print("\n⏳ 等待RBL启动...")
            print("💡 提示: 请在3秒内按任意键进入下载模式")
            if not self.wait_for_prompt(15):
                print("❌ 未检测到RBL提示符")
                return False
            
            # 3. 测试INFO命令
            if not self.test_info_command():
                print("❌ INFO命令测试失败")
            
            # 4. 创建测试文件
            if not self.create_test_file(test_file, 2048):
                print("❌ 测试文件创建失败")
                return False
            
            # 5. 测试ERASE命令
            if not self.test_erase_command():
                print("❌ ERASE命令测试失败")
            
            # 6. 测试Ymodem协议
            if not self.test_ymodem_protocol(test_file):
                print("❌ Ymodem协议测试失败")
                return False
            
            # 7. 提示用户手动操作
            print("\n" + "=" * 50)
            print("📌 手动操作指南:")
            print("1. 在串口工具中选择 '发送文件 - Ymodem'")
            print(f"2. 选择测试文件: {os.path.abspath(test_file)}")
            print("3. 开始传输")
            print("4. 观察传输进度和结果")
            print("=" * 50)
            
            # 8. 监控传输过程
            input("\n按Enter键开始监控传输过程...")
            if self.monitor_ymodem_transfer():
                print("🎉 测试成功!")
            else:
                print("😞 测试未完成")
            
            # 9. 退出下载模式
            time.sleep(2)
            self.send_command("QUIT")
            time.sleep(1)
            
            print("\n✅ 测试流程结束")
            
        finally:
            self.disconnect()
            # 清理测试文件
            if os.path.exists(test_file):
                os.remove(test_file)
                print(f"🧹 已清理测试文件: {test_file}")

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: python3 ymodem_test.py <串口设备>")
        print("示例: python3 ymodem_test.py /dev/ttyUSB0")
        print("      python3 ymodem_test.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    tester = YmodemTester(port)
    tester.run_complete_test()

if __name__ == "__main__":
    main()
