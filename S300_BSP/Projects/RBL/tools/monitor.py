#!/usr/bin/env python3
"""
串口监控工具 - 只负责显示串口输出
可以与下载工具分时使用同一个串口
"""

import serial
import sys
import time


def monitor_serial(port='/dev/ttyUSB0', baudrate=115200):
    """监控串口输出"""
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1
        )

        print(f"📟 监控串口: {port} @ {baudrate} baud")
        print("按 Ctrl+C 退出")
        print("-" * 50)

        buffer = b''

        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer += data

                # 按行输出，添加时间戳
                while b'\n' in buffer:
                    line, buffer = buffer.split(b'\n', 1)
                    timestamp = time.strftime("[%H:%M:%S.%f]")[:-3]
                    try:
                        text = line.decode('utf-8', errors='ignore').rstrip()
                        if text:  # 只输出非空行
                            print(f"{timestamp} {text}")
                    except:
                        pass

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n👋 监控已停止")
    except Exception as e:
        print(f"❌ 错误: {e}")
    finally:
        if 'ser' in locals():
            ser.close()


if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
    monitor_serial(port)
