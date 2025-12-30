#!/usr/bin/env python3
import serial
import struct
import time
import sys
import os


def send_file(port, baudrate, file_path):
    if not os.path.exists(file_path):
        print(f"Error: File {file_path} not found.")
        return

    file_size = os.path.getsize(file_path)
    print(f"Opening serial port {port} at {baudrate}...")

    try:
        ser = serial.Serial(port, baudrate, timeout=5)
    except Exception as e:
        print(f"Error opening serial port: {e}")
        return

    print("Waiting for device...")
    # Wait a bit
    time.sleep(1)

    # 0. Send 'load' command
    print("Sending 'load' command...")
    ser.write(b'load\r')
    time.sleep(0.5)

    # Flush input buffer
    ser.reset_input_buffer()

    # 1. Send 'T'
    print("Sending 'T' command...")
    ser.write(b'T')

    # 2. Wait for 'K'
    ack = ser.read(1)
    if ack != b'K':
        print(f"Error: Expected 'K' ack, got {ack}")
        return
    print("Received ACK for command.")

    # 3. Send Size (4 bytes, little endian)
    print(f"Sending file size: {file_size} bytes")
    ser.write(struct.pack('<I', file_size))

    # 4. Wait for 'K'
    ack = ser.read(1)
    if ack != b'K':
        print(f"Error: Expected 'K' ack for size, got {ack}")
        return
    print("Received ACK for size. Starting transfer...")

    # 5. Send Data
    with open(file_path, 'rb') as f:
        sent = 0
        while sent < file_size:
            chunk = f.read(4096)
            if not chunk:
                break
            ser.write(chunk)
            sent += len(chunk)
            print(
                f"\rProgress: {sent}/{file_size} bytes ({(sent/file_size)*100:.1f}%)", end='')
            # Optional: small delay to prevent buffer overflow if flow control is poor
            # time.sleep(0.001)

    print("\nTransfer finished. Waiting for final ACK...")

    # 6. Wait for final 'K'
    ack = ser.read(1)
    if ack == b'K':
        print("Success: File transfer complete and acknowledged.")
    else:
        print(f"Warning: Final ACK not received (got {ack}).")

    ser.close()


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(
            "Usage: python dsp_file_transfer.py <serial_port> <file_path> [baudrate]")
        print("Example: python dsp_file_transfer.py /dev/ttyUSB0 model.bin")
        sys.exit(1)

    port = sys.argv[1]
    file_path = sys.argv[2]
    baud = 115200
    if len(sys.argv) > 3:
        baud = int(sys.argv[3])

    send_file(port, baud, file_path)
