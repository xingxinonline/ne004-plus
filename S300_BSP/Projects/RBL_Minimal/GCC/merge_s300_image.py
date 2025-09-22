#!/usr/bin/env python3
"""
S300 Image Merge Tool
合并S300 header和RBL binary生成完整的S300镜像
"""

import sys
from datetime import datetime


def merge_s300_image(header_path, rbl_path, output_path):
    """
    合并S300 header和RBL binary

    Args:
        header_path: S300 header文件路径 (256字节)
        rbl_path: RBL binary文件路径
        output_path: 输出完整镜像路径

    Returns:
        bool: 成功返回True，失败返回False
    """
    try:
        # 读取header文件
        with open(header_path, 'rb') as f:
            header_data = f.read()

        if len(header_data) != 256:
            print(f"❌ Header size error: {len(header_data)} bytes "
                  f"(expected 256)")
            return False

        # 读取RBL binary文件
        with open(rbl_path, 'rb') as f:
            rbl_data = f.read()

        if len(rbl_data) == 0:
            print("❌ RBL binary is empty")
            return False

        # 合并数据
        complete_image = header_data + rbl_data

        # 写入输出文件
        with open(output_path, 'wb') as f:
            f.write(complete_image)

        print("S300 image merged successfully:")
        print(f"  Header: {len(header_data)} bytes")
        print(f"  RBL: {len(rbl_data)} bytes")
        print(f"  Total: {len(complete_image)} bytes")
        print(f"  Output: {output_path}")

        return True

    except FileNotFoundError as e:
        print(f"❌ File not found: {e}")
        return False
    except Exception as e:
        print(f"❌ Error during merge: {e}")
        return False


def main():
    """主函数"""
    if len(sys.argv) != 4:
        print("Usage: python3 merge_s300_image.py "
              "<header_file> <rbl_file> <output_file>")
        print("Example: python3 merge_s300_image.py "
              "header.bin rbl.bin complete.bin")
        sys.exit(1)

    header_path = sys.argv[1]
    rbl_path = sys.argv[2]
    output_path = sys.argv[3]

    print("=== S300 Image Merge Tool ===")
    print(f"Header file: {header_path}")
    print(f"RBL file: {rbl_path}")
    print(f"Output file: {output_path}")
    print(f"Timestamp: {datetime.now()}")
    print()

    success = merge_s300_image(header_path, rbl_path, output_path)

    if success:
        print("✅ S300 image merge completed successfully!")
        sys.exit(0)
    else:
        print("❌ S300 image merge failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
