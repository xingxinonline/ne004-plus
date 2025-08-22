#!/usr/bin/env python3
import argparse
import binascii
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    serial = None

APP_FLASH_BASE = 0x08010000


def open_port(port, baud):
    if serial is None:
        print("pyserial 未安装，请先安装: pip install pyserial", file=sys.stderr)
        sys.exit(2)
    ser = serial.Serial(port, baudrate=baud, timeout=0.5)
    return ser


def send_cmd(ser, cmd):
    ser.write((cmd + "\n").encode('ascii'))
    ser.flush()
    # read one line
    line = ser.readline().decode('ascii', errors='ignore').strip()
    return line


def crc32_file(path):
    crc = 0xFFFFFFFF
    with open(path, 'rb') as f:
        while True:
            chunk = f.read(4096)
            if not chunk:
                break
            for b in chunk:
                crc ^= (b & 0xFF) << 24
                for _ in range(8):
                    if (crc & 0x80000000) != 0:
                        crc = ((crc << 1) & 0xFFFFFFFF) ^ 0x04C11DB7
                    else:
                        crc = (crc << 1) & 0xFFFFFFFF
    return crc & 0xFFFFFFFF


def do_flash(ser, path, addr):
    size = Path(path).stat().st_size
    # erase
    print(f"Erase 0x{addr:08X} +0x{size:X} ...")
    r = send_cmd(ser, f"erase {addr:X} {size:X}")
    print("<-", r)
    if r != 'OK':
        print("擦除未实现或失败，跳过后续写入。")
        return 1
    # write in chunks
    with open(path, 'rb') as f:
        off = 0
        while off < size:
            chunk = f.read(1024)
            if not chunk:
                break
            clen = len(chunk)
            print(f"Write 0x{addr+off:08X} +{clen} ...", end=" ")
            r = send_cmd(ser, f"write {addr+off:X} {clen:X}")
            if r != 'READY':
                print("ERR (not READY)")
                return 2
            ser.write(chunk)
            ser.flush()
            ack = ser.readline().decode('ascii', errors='ignore').strip()
            print(ack)
            if ack != 'OK':
                return 3
            off += clen
    # verify
    crc = crc32_file(path)
    print(f"Verify CRC32=0x{crc:08X} ...", end=" ")
    r = send_cmd(ser, f"verify {addr:X} {size:X} {crc:X}")
    print(r)
    if r != 'OK':
        return 4
    # boot
    print("Booting...")
    send_cmd(ser, "boot")
    return 0


def auto_reset(ser):
    # Try toggle DTR/RTS to reset into bootloader if hardware supports it
    try:
        ser.dtr = False
        ser.rts = True
        time.sleep(0.05)
        ser.dtr = True
        ser.rts = False
        time.sleep(0.05)
        ser.dtr = False
        ser.rts = False
    except Exception:
        pass


def main():
    ap = argparse.ArgumentParser(description='S300 UART bootloader host tool')
    ap.add_argument('-p', '--port', required=True,
                    help='Serial port, e.g. /dev/ttyUSB0')
    ap.add_argument('-b', '--baud', type=int, default=115200, help='Baudrate')
    sub = ap.add_subparsers(dest='cmd')
    sub.add_parser('ping')
    sub.add_parser('info')
    sp_v = sub.add_parser('verify')
    sp_v.add_argument('file')
    sp_v.add_argument('--addr', type=lambda x: int(x, 0),
                      default=APP_FLASH_BASE)
    sp_f = sub.add_parser('flash')
    sp_f.add_argument('file')
    sp_f.add_argument('--addr', type=lambda x: int(x, 0),
                      default=APP_FLASH_BASE)
    sub.add_parser('monitor')
    args = ap.parse_args()

    ser = open_port(args.port, args.baud)
    auto_reset(ser)
    time.sleep(0.1)

    if args.cmd == 'ping':
        print(send_cmd(ser, 'ping'))
    elif args.cmd == 'info':
        print(send_cmd(ser, 'info'))
    elif args.cmd == 'verify':
        size = Path(args.file).stat().st_size
        crc = crc32_file(args.file)
        print(send_cmd(ser, f'verify {args.addr:X} {size:X} {crc:X}'))
    elif args.cmd == 'flash':
        rc = do_flash(ser, args.file, args.addr)
        sys.exit(rc)
    elif args.cmd == 'monitor':
        print('进入监视器 (Ctrl-C 退出)')
        try:
            while True:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    sys.stdout.write(data.decode('ascii', errors='ignore'))
                    sys.stdout.flush()
        except KeyboardInterrupt:
            pass
    else:
        ap.print_help()
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
