#!/bin/sh
# Convert binary to C character array
# Based on OpenOCD helper script

echo "/* Auto-generated from binary file */"
echo "static const uint8_t flash_ops_code[] = {"

hexdump -v -e '16/1 "0x%02x, " "\n"' "$1" | sed 's/, $//'

echo "};"
echo ""
echo "/* Size: $(stat -c%s "$1") bytes */"
