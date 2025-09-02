#!/usr/bin/env python3
"""
Print brief S300 header info for Cortex-M4 section in a compact format.
Accepts either a 256-byte header.bin or a complete image
(will use first 256 bytes).

Output example:
addr         :0x00000100
exeaddr      :0x20000000
len          :20416
check        :0x824447E2
pro          :0x00F34009
version      :CM4 Core
verify       :CRC32
"""
import sys
import os
import struct


def read_header256(path: str) -> bytes:
    with open(path, 'rb') as f:
        data = f.read()
    if len(data) < 256:
        raise ValueError(f"file too small: {len(data)} bytes (<256)")
    return data[:256]


def parse_and_print(header: bytes) -> None:
    pro = struct.unpack_from('<I', header, 0x00)[0]
    addr = struct.unpack_from('<I', header, 0x04)[0]
    exe = struct.unpack_from('<I', header, 0x08)[0]
    length = struct.unpack_from('<I', header, 0x0C)[0]
    check = struct.unpack_from('<I', header, 0x10)[0]
    ver_b = header[0x14:0x20]
    version = (
        ver_b.split(b'\x00', 1)[0]
        .decode('ascii', errors='ignore')
        .strip()
    )

    check_mode = pro & 0x3
    if check_mode == 1:
        verify = 'CRC32'
    elif check_mode == 0:
        verify = 'Checksum'
    else:
        verify = 'Unknown'

    def hx(v: int) -> str:
        return f"0x{v:08X}"

    # Keep labels left-aligned to width 13 to mimic the sample style
    print(f"{'addr':<13}:{hx(addr)}")
    print(f"{'exeaddr':<13}:{hx(exe)}")
    print(f"{'len':<13}:{length}")
    print(f"{'check':<13}:{hx(check)}")
    print(f"{'pro':<13}:{hx(pro)}")
    print(f"{'version':<13}:{version}")
    print(f"{'verify':<13}:{verify}")


def main():
    if len(sys.argv) != 2:
        print(
            "Usage: python3 print_s300_header_brief.py "
            "<header_or_complete_file>"
        )
        return 1
    p = sys.argv[1]
    if not os.path.exists(p):
        print(f"file not found: {p}")
        return 1
    try:
        header = read_header256(p)
        parse_and_print(header)
        return 0
    except Exception as e:
        print(f"error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
