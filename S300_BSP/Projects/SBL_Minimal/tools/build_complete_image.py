#!/usr/bin/env python3
"""
S300完整镜像合成工具
将Header + RBL + SBL合成为一个完整的Flash镜像文件
"""

import os
import sys
import struct
import zlib
from datetime import datetime


def calculate_crc32(data):
    """计算CRC32校验值"""
    return zlib.crc32(data) & 0xFFFFFFFF


def pad_to_size(data, size, fill_byte=0xFF):
    """将数据填充到指定大小"""
    if len(data) > size:
        raise ValueError(f"Data size {len(data)} exceeds target size {size}")
    return data + bytes([fill_byte] * (size - len(data)))


def align_to_boundary(data, boundary=256):
    """将数据对齐到指定边界"""
    remainder = len(data) % boundary
    if remainder == 0:
        return data
    padding_size = boundary - remainder
    return data + bytes([0xFF] * padding_size)


def create_complete_image(rbl_path, sbl_path, output_path):
    """创建完整的S300镜像"""

    print(f"🔧 S300 Complete Image Builder")
    print(f"=" * 50)

    # 检查输入文件
    if not os.path.exists(rbl_path):
        print(f"❌ RBL文件不存在: {rbl_path}")
        return False

    if not os.path.exists(sbl_path):
        print(f"❌ SBL文件不存在: {sbl_path}")
        return False

    # 读取RBL和SBL文件
    with open(rbl_path, 'rb') as f:
        rbl_data = f.read()

    with open(sbl_path, 'rb') as f:
        sbl_data = f.read()

    print(f"📄 RBL文件: {rbl_path}")
    print(f"   大小: {len(rbl_data)} 字节")

    print(f"📄 SBL文件: {sbl_path}")
    print(f"   大小: {len(sbl_data)} 字节")

    # S300镜像布局定义
    HEADER_SIZE = 256
    RBL_OFFSET = 0x100        # 256字节
    SBL_OFFSET = 0x10000      # 64KB

    # 创建Header (256字节)
    header = bytearray(HEADER_SIZE)

    # Header结构 (参考RBL的header生成)
    # Magic number (4字节)
    header[0:4] = b'S300'

    # RBL信息
    rbl_size = len(rbl_data)
    rbl_crc = calculate_crc32(rbl_data)

    struct.pack_into('<I', header, 4, rbl_size)       # RBL大小
    struct.pack_into('<I', header, 8, rbl_crc)        # RBL CRC32
    struct.pack_into('<I', header, 12, RBL_OFFSET)    # RBL Flash偏移
    struct.pack_into('<I', header, 16, 0x20000000)    # RBL SRAM地址

    # SBL信息
    sbl_size = len(sbl_data)
    sbl_crc = calculate_crc32(sbl_data)

    struct.pack_into('<I', header, 20, sbl_size)      # SBL大小
    struct.pack_into('<I', header, 24, sbl_crc)       # SBL CRC32
    struct.pack_into('<I', header, 28, SBL_OFFSET)    # SBL Flash偏移
    struct.pack_into('<I', header, 32, 0x08010000)    # SBL Flash地址(XIP)

    # 时钟配置
    struct.pack_into('<I', header, 36, 24000000)      # 参考时钟 24MHz
    struct.pack_into('<I', header, 40, 24000000)      # 输出时钟 24MHz
    struct.pack_into('<I', header, 44, 0)             # PLL禁用

    # 版本和时间戳
    timestamp = int(datetime.now().timestamp())
    struct.pack_into('<I', header, 48, timestamp)     # 构建时间戳
    struct.pack_into('<I', header, 52, 0x01000000)    # 版本号 v1.0.0.0

    # 预留字段填充为0xFF
    for i in range(56, HEADER_SIZE - 4):
        header[i] = 0xFF

    # 计算Header CRC32 (最后4字节)
    header_crc = calculate_crc32(header[:-4])
    struct.pack_into('<I', header, HEADER_SIZE - 4, header_crc)

    # 构建完整镜像
    print(f"\n🔨 构建完整镜像...")

    # 创建镜像缓冲区
    total_size = SBL_OFFSET + len(sbl_data)
    image = bytearray(total_size)

    # 填充默认值
    for i in range(total_size):
        image[i] = 0xFF

    # 写入Header (0x000000 - 0x0000FF)
    image[0:HEADER_SIZE] = header

    # 写入RBL (0x000100 - 0x00FFFF)
    image[RBL_OFFSET:RBL_OFFSET + len(rbl_data)] = rbl_data

    # 写入SBL (0x010000 - ...)
    image[SBL_OFFSET:SBL_OFFSET + len(sbl_data)] = sbl_data

    # 写入完整镜像文件
    with open(output_path, 'wb') as f:
        f.write(image)

    print(f"✅ 完整镜像已生成: {output_path}")
    print(f"\n📊 镜像信息:")
    print(f"   Header:  0x{0:06X} - 0x{HEADER_SIZE-1:06X} ({HEADER_SIZE} 字节)")
    print(
        f"   RBL:     0x{RBL_OFFSET:06X} - 0x{RBL_OFFSET + len(rbl_data)-1:06X} ({len(rbl_data)} 字节)")
    print(
        f"   SBL:     0x{SBL_OFFSET:06X} - 0x{SBL_OFFSET + len(sbl_data)-1:06X} ({len(sbl_data)} 字节)")
    print(f"   总大小:  {len(image)} 字节 ({len(image)/1024:.1f} KB)")

    print(f"\n🔍 校验信息:")
    print(f"   Header CRC32: 0x{header_crc:08X}")
    print(f"   RBL CRC32:    0x{rbl_crc:08X}")
    print(f"   SBL CRC32:    0x{sbl_crc:08X}")

    return True


def analyze_image(image_path):
    """分析镜像文件结构"""

    if not os.path.exists(image_path):
        print(f"❌ 镜像文件不存在: {image_path}")
        return False

    with open(image_path, 'rb') as f:
        image_data = f.read()

    print(f"🔍 S300 Image Analyzer")
    print(f"=" * 50)
    print(f"文件: {image_path}")
    print(f"大小: {len(image_data)} 字节 ({len(image_data)/1024:.1f} KB)")

    if len(image_data) < 256:
        print("❌ 文件太小，不是有效的S300镜像")
        return False

    # 解析Header
    header = image_data[:256]

    magic = header[0:4]
    if magic != b'S300':
        print(f"❌ 魔数错误: {magic} (应该是 b'S300')")
        return False

    print(f"✅ 魔数正确: {magic}")

    # 解析字段
    rbl_size = struct.unpack_from('<I', header, 4)[0]
    rbl_crc = struct.unpack_from('<I', header, 8)[0]
    rbl_offset = struct.unpack_from('<I', header, 12)[0]
    rbl_sram = struct.unpack_from('<I', header, 16)[0]

    sbl_size = struct.unpack_from('<I', header, 20)[0]
    sbl_crc = struct.unpack_from('<I', header, 24)[0]
    sbl_offset = struct.unpack_from('<I', header, 28)[0]
    sbl_flash = struct.unpack_from('<I', header, 32)[0]

    ref_clock = struct.unpack_from('<I', header, 36)[0]
    out_clock = struct.unpack_from('<I', header, 40)[0]
    pll_config = struct.unpack_from('<I', header, 44)[0]

    timestamp = struct.unpack_from('<I', header, 48)[0]
    version = struct.unpack_from('<I', header, 52)[0]

    header_crc = struct.unpack_from('<I', header, 252)[0]

    print(f"\n📋 Header信息:")
    print(f"   RBL大小:     {rbl_size} 字节")
    print(f"   RBL CRC32:   0x{rbl_crc:08X}")
    print(f"   RBL偏移:     0x{rbl_offset:06X}")
    print(f"   RBL SRAM:    0x{rbl_sram:08X}")

    print(f"   SBL大小:     {sbl_size} 字节")
    print(f"   SBL CRC32:   0x{sbl_crc:08X}")
    print(f"   SBL偏移:     0x{sbl_offset:06X}")
    print(f"   SBL Flash:   0x{sbl_flash:08X}")

    print(f"   参考时钟:    {ref_clock} Hz")
    print(f"   输出时钟:    {out_clock} Hz")
    print(f"   PLL配置:     0x{pll_config:08X}")

    build_time = datetime.fromtimestamp(timestamp)
    print(f"   构建时间:    {build_time}")
    print(
        f"   版本号:      v{(version>>24)&0xFF}.{(version>>16)&0xFF}.{(version>>8)&0xFF}.{version&0xFF}")
    print(f"   Header CRC:  0x{header_crc:08X}")

    # 验证Header CRC
    calc_header_crc = calculate_crc32(header[:-4])
    if calc_header_crc == header_crc:
        print(f"   ✅ Header CRC校验通过")
    else:
        print(f"   ❌ Header CRC校验失败 (计算值: 0x{calc_header_crc:08X})")

    # 验证RBL CRC
    if rbl_offset + rbl_size <= len(image_data):
        rbl_data = image_data[rbl_offset:rbl_offset + rbl_size]
        calc_rbl_crc = calculate_crc32(rbl_data)
        if calc_rbl_crc == rbl_crc:
            print(f"   ✅ RBL CRC校验通过")
        else:
            print(f"   ❌ RBL CRC校验失败 (计算值: 0x{calc_rbl_crc:08X})")
    else:
        print(f"   ❌ RBL数据超出文件范围")

    # 验证SBL CRC
    if sbl_offset + sbl_size <= len(image_data):
        sbl_data = image_data[sbl_offset:sbl_offset + sbl_size]
        calc_sbl_crc = calculate_crc32(sbl_data)
        if calc_sbl_crc == sbl_crc:
            print(f"   ✅ SBL CRC校验通过")
        else:
            print(f"   ❌ SBL CRC校验失败 (计算值: 0x{calc_sbl_crc:08X})")
    else:
        print(f"   ❌ SBL数据超出文件范围")

    return True


def main():
    """主函数"""

    if len(sys.argv) < 2:
        print(f"用法:")
        print(f"  {sys.argv[0]} build <rbl_file> <sbl_file> <output_file>")
        print(f"  {sys.argv[0]} analyze <image_file>")
        print(f"")
        print(f"示例:")
        print(
            f"  {sys.argv[0]} build ../RBL/GCC/build/s300_rbl_simple.bin build/s300_sbl_minimal.bin build/s300_complete.bin")
        print(f"  {sys.argv[0]} analyze build/s300_complete.bin")
        return 1

    command = sys.argv[1]

    if command == "build":
        if len(sys.argv) != 5:
            print("❌ build命令需要3个参数: <rbl_file> <sbl_file> <output_file>")
            return 1

        rbl_file = sys.argv[2]
        sbl_file = sys.argv[3]
        output_file = sys.argv[4]

        if create_complete_image(rbl_file, sbl_file, output_file):
            return 0
        else:
            return 1

    elif command == "analyze":
        if len(sys.argv) != 3:
            print("❌ analyze命令需要1个参数: <image_file>")
            return 1

        image_file = sys.argv[2]

        if analyze_image(image_file):
            return 0
        else:
            return 1

    else:
        print(f"❌ 未知命令: {command}")
        print(f"支持的命令: build, analyze")
        return 1


if __name__ == "__main__":
    sys.exit(main())
