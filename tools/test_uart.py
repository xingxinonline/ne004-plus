#!/usr/bin/env python3
"""
简单的 UART 测试脚本，用于诊断 DMA 传输问题
"""

import serial
import struct
import time
import sys

# 配置
SERIAL_PORT = 'COM7'
BAUD_RATE = 115200


def test_uart():
    """测试基本的 UART 通信"""

    print(f"[TEST] 打开串口 {SERIAL_PORT}...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    except Exception as e:
        print(f"[ERROR] 打开失败: {e}")
        return False

    print("[TEST] 串口已打开，等待 2 秒让 MCU 初始化...")
    time.sleep(2)

    # 清空缓冲区
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.1)

    # 读取初始化信息
    print("\n[TEST] 读取 MCU 初始化输出:")
    while ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(data.decode(errors='ignore'), end='')
        time.sleep(0.1)

    # Test 1: 发送命令
    print("\n\n[TEST 1] 发送命令 'T'...")
    ser.write(b'T')
    print("[TEST 1] 命令已发送，等待 ACK...")

    ack = ser.read(1)
    if ack == b'K':
        print("[TEST 1] ✓ 收到 ACK")
    else:
        print(f"[TEST 1] ✗ 收到异常: {ack}")
        return False

    # Test 2: 发送大小 (小文件，只有 1KB)
    print("\n[TEST 2] 发送大小 (1024 bytes)...")
    size = struct.pack('<I', 1024)
    ser.write(size)
    print("[TEST 2] 大小已发送，等待 ACK (Erase 完成)...")

    ack = ser.read(1)
    if ack == b'K':
        print("[TEST 2] ✓ 收到 ACK (Flash 已擦除)")
    else:
        print(f"[TEST 2] ✗ 收到异常: {ack}")
        return False

    # Test 3: 发送数据块
    print("\n[TEST 3] 发送 1KB 数据块...")
    data = bytes(range(256)) * 4  # 重复 256 个字节 4 次 = 1024 字节

    print(f"[TEST 3] 数据大小: {len(data)} bytes")
    print(f"[TEST 3] 数据预览: {data[:20]}...")

    # 添加小延时让 MCU 准备 DMA
    print("[TEST 3] 等待 50ms 让 MCU 准备 DMA...")
    time.sleep(0.05)

    print("[TEST 3] 开始发送数据...")
    ser.write(data)
    print("[TEST 3] 数据已发送，等待 ACK...")

    # 用更长的超时，并逐字节读取
    start_time = time.time()
    ack = b''
    while time.time() - start_time < 3:
        if ser.in_waiting > 0:
            byte = ser.read(1)
            ack += byte
            print(f"[TEST 3] 收到字节: {byte} (ASCII: {byte[0]})")
            if byte == b'K':
                print("[TEST 3] ✓ 收到 ACK (数据已写入 Flash)")
                break
            elif byte == b'E':
                print("[TEST 3] ✗ 收到 NAK (MCU 报错)")
                return False
        time.sleep(0.01)

    if not ack:
        print("[TEST 3] ✗ 超时，未收到任何响应")
        return False
    elif ack != b'K':
        print(f"[TEST 3] ✗ 收到异常: {ack}")


if __name__ == "__main__":
    success = test_uart()
    sys.exit(0 if success else 1)
