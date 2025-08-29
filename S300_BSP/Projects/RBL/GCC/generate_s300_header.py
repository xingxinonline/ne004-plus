#!/usr/bin/env python3
"""
S300 Header Generator
生成S300 ROMBOOT兼容的256字节头部
"""

import sys
import struct
import os
from datetime import datetime

# S300 Header格式常量
HEADER_SIZE = 256
MAGIC_NUMBER = 0x5350494D  # "SPIM" - S300 PiMCHIP
VERSION = 0x01

def calculate_crc32(data):
    """计算CRC32校验值"""
    import zlib
    return zlib.crc32(data) & 0xFFFFFFFF

def pad_to_size(data, size, pad_byte=0xFF):
    """填充数据到指定大小"""
    if len(data) > size:
        raise ValueError(f"Data too large: {len(data)} > {size}")
    return data + bytes([pad_byte] * (size - len(data)))

def generate_s300_header(rbl_bin_path, output_path):
    """生成S300头部文件"""
    
    # 读取RBL二进制文件
    if not os.path.exists(rbl_bin_path):
        print(f"Error: RBL binary file not found: {rbl_bin_path}")
        return False
    
    with open(rbl_bin_path, 'rb') as f:
        rbl_data = f.read()
    
    rbl_size = len(rbl_data)
    print(f"RBL binary size: {rbl_size} bytes")
    
    if rbl_size > 240 * 1024:  # 预留空间检查
        print(f"Warning: RBL size ({rbl_size}) is large, may not fit in reserved space")
    
    # 计算RBL的CRC32
    rbl_crc32 = calculate_crc32(rbl_data)
    print(f"RBL CRC32: 0x{rbl_crc32:08X}")
    
    # 构建S300头部
    header = bytearray(HEADER_SIZE)
    
    # 偏移 0x00: 魔数 (4字节)
    struct.pack_into('<I', header, 0x00, MAGIC_NUMBER)
    
    # 偏移 0x04: 版本 (1字节)
    header[0x04] = VERSION
    
    # 偏移 0x05: 保留 (3字节)
    header[0x05:0x08] = b'\xFF\xFF\xFF'
    
    # 偏移 0x08: RBL加载地址 (4字节) - SRAM起始
    load_addr = 0x20000000
    struct.pack_into('<I', header, 0x08, load_addr)
    
    # 偏移 0x0C: RBL入口地址 (4字节) - 向量表+1 (Thumb模式)
    entry_addr = load_addr + 1
    struct.pack_into('<I', header, 0x0C, entry_addr)
    
    # 偏移 0x10: RBL大小 (4字节)
    struct.pack_into('<I', header, 0x10, rbl_size)
    
    # 偏移 0x14: RBL CRC32 (4字节)
    struct.pack_into('<I', header, 0x14, rbl_crc32)
    
    # 偏移 0x18: 构建时间戳 (4字节)
    timestamp = int(datetime.now().timestamp())
    struct.pack_into('<I', header, 0x18, timestamp)
    
    # 偏移 0x1C: 特性标志 (4字节)
    features = 0x00000001  # bit 0: 支持串口下载
    struct.pack_into('<I', header, 0x1C, features)
    
    # 偏移 0x20-0xEF: 保留区域 (208字节)
    header[0x20:0xF0] = b'\xFF' * 208
    
    # 偏移 0xF0-0xFF: 头部CRC32区域 (16字节) - 先清零
    header[0xF0:0x100] = b'\x00' * 16
    
    # 计算头部CRC32 (不包括CRC32字段本身)
    header_for_crc = bytes(header[:0xF0])
    header_crc32 = calculate_crc32(header_for_crc)
    
    # 填入头部CRC32
    struct.pack_into('<I', header, 0xF0, header_crc32)
    
    # 写入输出文件
    with open(output_path, 'wb') as f:
        f.write(header)
    
    print(f"S300 header generated: {output_path}")
    print(f"Header size: {len(header)} bytes")
    print(f"Header CRC32: 0x{header_crc32:08X}")
    print(f"Load address: 0x{load_addr:08X}")
    print(f"Entry address: 0x{entry_addr:08X}")
    print(f"Build timestamp: {datetime.fromtimestamp(timestamp)}")
    
    return True

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 generate_s300_header.py <rbl_binary> <output_header>")
        print("")
        print("Examples:")
        print("  python3 generate_s300_header.py build/rbl.bin build/rbl_header.bin")
        return 1
    
    rbl_bin_path = sys.argv[1]
    output_path = sys.argv[2]
    
    # 创建输出目录
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    if generate_s300_header(rbl_bin_path, output_path):
        return 0
    else:
        return 1

if __name__ == "__main__":
    sys.exit(main())
