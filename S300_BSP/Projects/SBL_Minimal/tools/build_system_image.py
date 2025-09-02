#!/usr/bin/env python3
"""
S300 Complete System Image Builder
在现有的Header+RBL镜像基础上，在64KB偏移处添加SBL，生成完整系统镜像
"""

import sys
import os
from datetime import datetime


def build_complete_system(header_rbl_path, sbl_path, output_path):
    """
    构建完整的S300系统镜像

    Args:
        header_rbl_path: Header+RBL镜像路径
        sbl_path: SBL二进制文件路径  
        output_path: 输出完整镜像路径

    Returns:
        bool: 成功返回True，失败返回False
    """
    try:
        # 读取Header+RBL镜像
        with open(header_rbl_path, 'rb') as f:
            header_rbl_data = f.read()

        print(f"📄 Header+RBL文件: {header_rbl_path}")
        print(f"   大小: {len(header_rbl_data)} 字节")

        # 读取SBL文件
        with open(sbl_path, 'rb') as f:
            sbl_data = f.read()

        print(f"📄 SBL文件: {sbl_path}")
        print(f"   大小: {len(sbl_data)} 字节")

        # 计算需要填充到64KB的大小
        SBL_OFFSET = 0x10000  # 64KB

        if len(header_rbl_data) > SBL_OFFSET:
            print(f"❌ Header+RBL大小 ({len(header_rbl_data)}) 超过64KB偏移")
            return False

        # 创建完整镜像
        print(f"\n🔨 构建完整镜像...")

        # 1. 复制Header+RBL数据
        complete_image = bytearray(header_rbl_data)

        # 2. 填充到64KB偏移
        padding_size = SBL_OFFSET - len(header_rbl_data)
        complete_image.extend([0xFF] * padding_size)

        # 3. 添加SBL数据
        complete_image.extend(sbl_data)

        # 写入输出文件
        with open(output_path, 'wb') as f:
            f.write(complete_image)

        print(f"✅ 完整镜像已生成: {output_path}")
        print(f"\n📊 镜像信息:")
        print(
            f"   Header+RBL: 0x000000 - 0x{len(header_rbl_data)-1:06X} ({len(header_rbl_data)} 字节)")
        print(
            f"   填充区域:   0x{len(header_rbl_data):06X} - 0x{SBL_OFFSET-1:06X} ({padding_size} 字节)")
        print(
            f"   SBL:       0x{SBL_OFFSET:06X} - 0x{SBL_OFFSET+len(sbl_data)-1:06X} ({len(sbl_data)} 字节)")
        print(
            f"   总大小:    {len(complete_image)} 字节 ({len(complete_image)/1024:.1f} KB)")

        return True

    except FileNotFoundError as e:
        print(f"❌ 文件不存在: {e}")
        return False
    except Exception as e:
        print(f"❌ 构建过程中出错: {e}")
        return False


def main():
    """主函数"""
    if len(sys.argv) != 4:
        print("Usage: python3 build_complete_system.py <header_rbl_file> <sbl_file> <output_file>")
        print("Example: python3 build_complete_system.py s300_rbl_simple_complete.bin s300_sbl_minimal.bin s300_system_complete.bin")
        sys.exit(1)

    header_rbl_path = sys.argv[1]
    sbl_path = sys.argv[2]
    output_path = sys.argv[3]

    print("🔧 S300 Complete System Image Builder")
    print("=" * 50)

    success = build_complete_system(header_rbl_path, sbl_path, output_path)

    if success:
        print(f"\n✅ 系统镜像构建成功!")
        sys.exit(0)
    else:
        print(f"\n❌ 系统镜像构建失败!")
        sys.exit(1)


if __name__ == "__main__":
    main()
