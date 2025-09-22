#!/usr/bin/env python3
"""
S300 RBL YMODEM下载脚本
支持自动检测RBL下载模式并上传固件
"""

import serial
import time
import sys
import os
from pathlib import Path


class YModemDownloader:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None

    def connect(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1
            )
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False

    def disconnect(self):
        """断开串口"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Serial port closed")

    def wait_for_download_mode(self, timeout=10):
        """等待RBL进入下载模式"""
        print("Waiting for RBL to enter download mode...")
        start_time = time.time()

        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                text = data.decode('utf-8', errors='ignore')
                print(text, end='')

                # 检测下载模式标志
                if 'Download mode started successfully' in text:
                    print("\n✅ RBL entered download mode!")
                    return True
                elif 'YMODEM: Start receive' in text:
                    print("\n✅ YMODEM receiver ready!")
                    return True

            time.sleep(0.1)

        print(f"\n❌ Timeout waiting for download mode")
        return False

    def send_ymodem_file(self, file_path):
        """发送文件通过YMODEM协议（简化版）"""
        if not os.path.exists(file_path):
            print(f"❌ File not found: {file_path}")
            return False

        file_size = os.path.getsize(file_path)
        file_name = os.path.basename(file_path)

        print(f"📤 Sending file: {file_name} ({file_size} bytes)")

        # 等待'C'字符（CRC模式请求）
        print("Waiting for CRC request...")
        c_received = False
        for _ in range(30):  # 3秒超时
            if self.ser.in_waiting > 0:
                data = self.ser.read(1)
                if data == b'C':
                    c_received = True
                    break
            time.sleep(0.1)

        if not c_received:
            print("❌ No CRC request received")
            return False

        print("✅ CRC request received, starting transfer...")

        # 发送文件名包（包0）
        self._send_filename_packet(file_name, file_size)

        # 等待ACK
        if not self._wait_ack():
            return False

        # 发送文件数据
        return self._send_file_data(file_path)

    def _send_filename_packet(self, filename, filesize):
        """发送文件名包"""
        # SOH + 包号0 + ~包号0 + 文件名 + 文件大小 + 填充 + CRC
        packet = bytearray()
        packet.append(0x01)  # SOH
        packet.append(0x00)  # 包号0
        packet.append(0xFF)  # ~包号0

        # 文件信息：filename + 空格 + filesize
        file_info = f"{filename} {filesize}".encode('ascii')
        data = bytearray(128)  # 128字节数据区
        data[:len(file_info)] = file_info

        packet.extend(data)

        # 计算CRC16
        crc = self._calc_crc16(data)
        packet.append((crc >> 8) & 0xFF)
        packet.append(crc & 0xFF)

        self.ser.write(packet)
        print(f"📋 Sent filename packet: {filename} ({filesize} bytes)")

    def _send_file_data(self, file_path):
        """发送文件数据"""
        packet_num = 1

        with open(file_path, 'rb') as f:
            while True:
                data = f.read(128)  # 128字节包
                if not data:
                    break

                # 如果不足128字节，用0x1A填充
                if len(data) < 128:
                    data += b'\x1A' * (128 - len(data))

                # 发送数据包
                packet = bytearray()
                packet.append(0x01)  # SOH
                packet.append(packet_num & 0xFF)
                packet.append((~packet_num) & 0xFF)
                packet.extend(data)

                # CRC16
                crc = self._calc_crc16(data)
                packet.append((crc >> 8) & 0xFF)
                packet.append(crc & 0xFF)

                self.ser.write(packet)
                print(f"📦 Sent packet {packet_num}")

                # 等待ACK
                if not self._wait_ack():
                    print(f"❌ No ACK for packet {packet_num}")
                    return False

                packet_num += 1

        # 发送EOT
        self.ser.write(b'\x04')
        print("📤 Sent EOT")

        # 等待最终ACK
        if self._wait_ack():
            print("✅ File transfer completed!")
            return True
        else:
            print("❌ Final ACK not received")
            return False

    def _wait_ack(self, timeout=5):
        """等待ACK"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(1)
                if data == b'\x06':  # ACK
                    return True
                elif data == b'\x15':  # NAK
                    print("⚠️ Received NAK")
                    return False
            time.sleep(0.01)
        return False

    def _calc_crc16(self, data):
        """计算CRC16 (XMODEM)"""
        crc = 0x0000
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc <<= 1
                crc &= 0xFFFF
        return crc


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 rbl_download.py <serial_port> <firmware_file>")
        print("Example: python3 rbl_download.py /dev/ttyUSB0 firmware.bin")
        return

    port = sys.argv[1]
    firmware = sys.argv[2]

    if not os.path.exists(firmware):
        print(f"❌ Firmware file not found: {firmware}")
        return

    downloader = YModemDownloader(port)

    try:
        if not downloader.connect():
            return

        print("🔄 Reset your device now...")
        print("Monitoring serial output...")

        # 监控串口输出并等待下载模式
        if downloader.wait_for_download_mode(timeout=30):
            # 短暂延时确保RBL准备好
            time.sleep(0.5)

            # 开始YMODEM传输
            if downloader.send_ymodem_file(firmware):
                print("🎉 Firmware download completed successfully!")
            else:
                print("❌ Firmware download failed!")
        else:
            print("❌ RBL did not enter download mode")

    except KeyboardInterrupt:
        print("\n🛑 User interrupted")
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        downloader.disconnect()


if __name__ == "__main__":
    main()
