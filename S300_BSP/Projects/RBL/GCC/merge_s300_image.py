#!/usr/bin/env python3
"""
S300 Image Merger
合并S300头部和RBL二进制文件
"""

import sys
import os

def merge_s300_image(header_path, rbl_path, output_path):
    """合并S300头部和RBL文件"""
    
    # 检查输入文件
    if not os.path.exists(header_path):
        print(f"Error: Header file not found: {header_path}")
        return False
    
    if not os.path.exists(rbl_path):
        print(f"Error: RBL file not found: {rbl_path}")
        return False
    
    # 读取头部文件
    with open(header_path, 'rb') as f:
        header_data = f.read()
    
    if len(header_data) != 256:
        print(f"Error: Invalid header size: {len(header_data)} (expected 256)")
        return False
    
    # 读取RBL文件
    with open(rbl_path, 'rb') as f:
        rbl_data = f.read()
    
    # 合并文件
    combined_data = header_data + rbl_data
    
    # 写入输出文件
    with open(output_path, 'wb') as f:
        f.write(combined_data)
    
    print(f"S300 image merged successfully:")
    print(f"  Header: {len(header_data)} bytes")
    print(f"  RBL: {len(rbl_data)} bytes")
    print(f"  Total: {len(combined_data)} bytes")
    print(f"  Output: {output_path}")
    
    return True

def main():
    if len(sys.argv) != 4:
        print("Usage: python3 merge_s300_image.py <header_file> <rbl_file> <output_file>")
        print("")
        print("Examples:")
        print("  python3 merge_s300_image.py build/rbl_header.bin build/rbl.bin build/s300_rbl_complete.bin")
        return 1
    
    header_path = sys.argv[1]
    rbl_path = sys.argv[2]
    output_path = sys.argv[3]
    
    # 创建输出目录
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    if merge_s300_image(header_path, rbl_path, output_path):
        return 0
    else:
        return 1

if __name__ == "__main__":
    sys.exit(main())
