#!/usr/bin/env python3
"""
S300 Standard Header Generator
根据S300芯片镜像Header格式规范生成标准256字节头部
参考: S300_Header_Format.md
"""

import sys
import struct
import os
from datetime import datetime

# S300 Header格式常量
HEADER_SIZE = 256

# ROM CRC32 (non-reflected) table, poly=0x04C11DB7
_CRC32_ROM_TABLE = [
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
]


def crc32_rom(data: bytes, init: int = 0) -> int:
    """ROM风格CRC32: 非反射, poly=0x04C11DB7, 初值默认0, 无最终异或。"""
    crc = init & 0xFFFFFFFF
    for b in data:
        if isinstance(b, str):  # 兼容老版本
            b = ord(b)
        idx = ((crc >> 24) ^ b) & 0xFF
        crc = ((crc << 8) & 0xFFFFFFFF) ^ _CRC32_ROM_TABLE[idx]
    return crc & 0xFFFFFFFF


def calculate_crc32(data):
    """兼容旧实现的CRC32（zlib反射版），现已不用于Header与RBL校验。"""
    import zlib
    return zlib.crc32(data) & 0xFFFFFFFF


def calculate_checksum(data):
    """计算校验和"""
    return sum(data) & 0xFFFFFFFF


def generate_s300_standard_header(rbl_bin_path, output_path):
    """
    生成符合S300规范的标准头部

    Header布局 (256字节):
    0x00-0x0F: Cortex-M4段信息 (Pro, Addr, Exe Addr, Len)
    0x10-0x1F: Cortex-M4校验和版本 (Check, Version[12])
    0x20-0x2F: Cortex-M0段信息 (Pro, Addr, Exe Addr, Len)
    0x30-0x3F: Cortex-M0校验和版本 (Check, Version[12])
    0x40-0x4F: DSP/CPT段信息 (Pro, Addr, Exe Addr, Len)
    0x50-0xB3: DSP RAM映射信息 (5个RAM区域 * 20字节)
    0xB4-0xCB: DSP Version[24]
    0xCC:      Other Pro
    0xD0-0xDF: Other 段信息 (Addr, Exe Addr, Len, Check)
    0xE0-0xEB: Other Version[12]
    0xEC:      REFclock
    0xF0:      foutclock
    0xF4:      Clkconfig0
    0xF8:      Clkconfig1
    0xFC:      Header CRC32 (对0x00-0xFB)
    """

    # 读取RBL二进制文件
    if not os.path.exists(rbl_bin_path):
        print(f"Error: RBL binary file not found: {rbl_bin_path}")
        return False

    with open(rbl_bin_path, 'rb') as f:
        rbl_data = f.read()

    rbl_size = len(rbl_data)
    print(f"RBL binary size: {rbl_size} bytes")

    # 计算RBL的CRC32（使用ROM算法以与引导一致）
    rbl_crc32 = crc32_rom(rbl_data, 0)
    print(f"RBL CRC32: 0x{rbl_crc32:08X}")

    # 构建S300标准头部
    header = bytearray(HEADER_SIZE)

    # === Cortex-M4 (控制系统) 段信息 (0x00-0x1F) ===

    # 0x00: Pro字段 - 控制系统程序属性
    # Bit[7:2]=0x02: 控制程序在flash中存储，运行时在ram1(384k)
    # Bit[1:0]=CheckMode: 3=不校验(默认), 1=CRC32 等
    # 说明：默认按用户要求设置为3（不校验）。如需覆盖，可设置环境变量 S300_CHECK_MODE=0|1|2|3。
    check_mode_default = 3
    try:
        check_mode_env = int(
            os.environ.get("S300_CHECK_MODE", str(check_mode_default))
        )
    except Exception:
        check_mode_env = check_mode_default
    check_mode = check_mode_env & 0x3
    cortex_m4_pro_base = 0x00f34008  # 低两位清零的基值
    cortex_m4_pro = cortex_m4_pro_base | check_mode
    struct.pack_into('<I', header, 0x00, cortex_m4_pro)

    # 0x04: Addr - RBL在Flash中的地址 (紧跟Header之后)
    rbl_flash_addr = 0x00000100  # Header后256字节开始
    struct.pack_into('<I', header, 0x04, rbl_flash_addr)

    # 0x08: Exe Addr - SRAM1执行地址 (384K SRAM)
    rbl_sram_addr = 0x20000000   # SRAM1基地址
    struct.pack_into('<I', header, 0x08, rbl_sram_addr)

    # 0x0C: Len - RBL段长度
    struct.pack_into('<I', header, 0x0C, rbl_size)

    # 0x10: Check - RBL CRC32校验值
    struct.pack_into('<I', header, 0x10, 0)

    # 0x14: Version - Cortex-M4版本信息 (12字节)
    version_info = "RBL_v1.0.0".encode('ascii')[:12]
    header[0x14:0x14+len(version_info)] = version_info

    # === Cortex-M0段信息 (0x20-0x3F) - 暂时未使用 ===

    # 0x20: Pro字段 - Cortex-M0程序属性 (设为0表示未使用)
    struct.pack_into('<I', header, 0x20, 0x00000000)

    # 0x24-0x2F: Cortex-M0地址和长度信息 (设为0)
    # Addr, Exe Addr, Len = 0

    # 0x30-0x3F: Cortex-M0校验和版本 (设为0)

    # === CPT段信息 (0x40-0xBF) - 暂时未使用 ===

    # 0x40: CPT Pro字段 (设为0表示未使用)
    struct.pack_into('<I', header, 0x40, 0x00000000)

    # 0x44-0x4F: CPT地址和长度信息 (设为0)

    # 0x50-0xB3: DSP RAM映射信息 (5*20B)。此处保持0表示未使用

    # 0xB4-0xCB: DSP Version[24] - 设为0或按需填充

    # 0xCC: Other 段 Pro (设为0表示未使用)
    struct.pack_into('<I', header, 0xCC, 0x00000000)

    # 0xD0-0xDF: Other 段四元组 - 设为0

    # 0xE0-0xEB: Other Version[12] - 设为0或按需填充

    # 0xEC: REFclock - 参考时钟(晶振频率) 24MHz (Table 15)
    ref_clock = 24000000  # 24MHz晶振
    struct.pack_into('<I', header, 0xEC, ref_clock)

    # 0xF0: foutclock - PLL输出频率 (Table 16)
    # PLL禁用时，输出频率等于参考时钟频率
    fout_clock = ref_clock  # 24MHz (PLL禁用，直接使用参考时钟)
    struct.pack_into('<I', header, 0xF0, fout_clock)

    # 0xF4: Clkconfig0 - PLL配置信息0 (Table 17)
    # PLL禁用时，这些参数不生效，设为0
    clkconfig0 = 0x00000000  # PLL禁用，配置无效
    struct.pack_into('<I', header, 0xF4, clkconfig0)

    # 0xF8: Clkconfig1 - PLL配置信息1 (Table 18)
    en = 0x0          # PLL功能禁用 (非0x3值=不启用PLL)
    refdiv = 1        # 输入频率分频值 (1-63) - 保留设置但不生效
    frac = 0          # PLL浮点分频值 (保留，设为0)

    clkconfig1 = (en << 30) | (refdiv << 24) | frac
    struct.pack_into('<I', header, 0xF8, clkconfig1)

    # 0xFC: Header CRC32 (4字节) - 最后计算
    # ROM规则：将0xFC..0xFF置零，随后对0x00..0xFF计算ROM CRC32，结果小端存放于0xFC
    header_zero = bytearray(header)
    header_zero[0xFC:0x100] = b"\x00\x00\x00\x00"
    header_crc32 = crc32_rom(header_zero, 0)
    struct.pack_into('<I', header, 0xFC, header_crc32)

    # 写入输出文件
    with open(output_path, 'wb') as f:
        f.write(header)

    print(f"S300 standard header generated: {output_path}")
    print(f"Header size: {len(header)} bytes")
    print(f"Header CRC32: 0x{header_crc32:08X}")
    print(f"RBL Flash address: 0x{rbl_flash_addr:08X}")
    print(f"RBL SRAM address: 0x{rbl_sram_addr:08X}")
    print(f"RBL size: {rbl_size} bytes")
    print(
        f"Header CheckMode: {check_mode} "
        f"({'no-check' if check_mode == 3 else 'with-check'})"
    )
    print(f"Reference clock: {ref_clock} Hz ({ref_clock/1000000:.1f} MHz)")
    print(f"Output clock: {fout_clock} Hz ({fout_clock/1000000:.1f} MHz)")
    print(f"PLL enabled: {en == 0x3} (en={en})")
    if en == 0x3:
        print(f"PLL config: refdiv={refdiv}")
    else:
        print("PLL disabled - using reference clock directly")
    print(f"Build timestamp: {datetime.now()}")

    return True


def verify_s300_header(header_path):
    """验证S300头部格式是否正确"""

    if not os.path.exists(header_path):
        print(f"Error: Header file not found: {header_path}")
        return False

    with open(header_path, 'rb') as f:
        data = f.read()
    if len(data) < HEADER_SIZE:
        print(f"Error: File too small {len(data)} < {HEADER_SIZE}")
        return False
    header = data[:HEADER_SIZE]

    print("=== S300 Header Verification ===")

    # 检查Cortex-M4段信息
    cortex_m4_pro = struct.unpack_from('<I', header, 0x00)[0]
    cortex_m4_addr = struct.unpack_from('<I', header, 0x04)[0]
    cortex_m4_exe = struct.unpack_from('<I', header, 0x08)[0]
    cortex_m4_len = struct.unpack_from('<I', header, 0x0C)[0]
    cortex_m4_check = struct.unpack_from('<I', header, 0x10)[0]

    print(f"Cortex-M4 Pro: 0x{cortex_m4_pro:08X}")
    print(f"Cortex-M4 Flash Addr: 0x{cortex_m4_addr:08X}")
    print(f"Cortex-M4 XIP Addr: 0x{cortex_m4_exe:08X}")
    print(f"Cortex-M4 Length: {cortex_m4_len} bytes")
    print(f"Cortex-M4 CRC32: 0x{cortex_m4_check:08X}")

    # 检查时钟配置 - 使用正确的偏移地址
    ref_clock = struct.unpack_from('<I', header, 0xEC)[0]    # Table 15
    fout_clock = struct.unpack_from('<I', header, 0xF0)[0]   # Table 16
    clkconfig1 = struct.unpack_from('<I', header, 0xF8)[0]   # Table 18

    # 解析Clkconfig1获取PLL使能状态
    en = (clkconfig1 >> 30) & 0x3

    print(f"Reference Clock: {ref_clock} Hz")
    print(f"Output Clock: {fout_clock} Hz")
    print(f"PLL Enabled: {en == 0x3} (en={en})")

    # 验证Header CRC32
    header_crc32_stored = struct.unpack_from('<I', header, 0xFC)[0]
    # ROM方式：将CRC字段清零后，对0x00..0xFF计算
    header_zero = bytearray(header)
    header_zero[0xFC:0x100] = b"\x00\x00\x00\x00"
    header_crc32_calc = crc32_rom(header_zero, 0)

    print(f"Header CRC32 (stored): 0x{header_crc32_stored:08X}")
    print(f"Header CRC32 (calculated, ROM): 0x{header_crc32_calc:08X}")

    if header_crc32_stored == header_crc32_calc:
        print("✅ Header CRC32 verification PASSED")
        return True
    else:
        print("❌ Header CRC32 verification FAILED")
        return False


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 generate_s300_standard_header.py "
              "<command> <args...>")
        print("")
        print("Commands:")
        print("  generate <rbl_binary> <output_header>  - "
              "Generate S300 standard header")
        print("  verify <header_file>                   - "
              "Verify S300 header format")
        print("")
        print("Examples:")
        print("  python3 generate_s300_standard_header.py generate "
              "build/rbl.bin build/s300_header.bin")
        print("  python3 generate_s300_standard_header.py verify "
              "build/s300_header.bin")
        return 1

    command = sys.argv[1]

    if command == "generate":
        if len(sys.argv) != 4:
            print("Error: generate command requires "
                  "<rbl_binary> <output_header>")
            return 1

        rbl_bin_path = sys.argv[2]
        output_path = sys.argv[3]

        # 创建输出目录（允许直接输出到当前目录）
        out_dir = os.path.dirname(output_path) or "."
        os.makedirs(out_dir, exist_ok=True)

        if generate_s300_standard_header(rbl_bin_path, output_path):
            return 0
        else:
            return 1

    elif command == "verify":
        if len(sys.argv) != 3:
            print("Error: verify command requires <header_file>")
            return 1

        header_path = sys.argv[2]

        if verify_s300_header(header_path):
            return 0
        else:
            return 1

    else:
        print(f"Error: Unknown command '{command}'")
        return 1


if __name__ == "__main__":
    sys.exit(main())
