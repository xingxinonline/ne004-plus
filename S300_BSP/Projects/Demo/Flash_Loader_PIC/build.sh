#!/bin/bash
# Build script for Flash Loader PIC demo
# Combines manual Makefile build with CMake integration

set -e

echo "================================"
echo " Flash Loader PIC Build Script"
echo "================================"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Step 1: Build PIC assembly code
echo -e "${YELLOW}Step 1: Building PIC flash operations...${NC}"
make clean
make flash_ops.inc

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ PIC code built successfully${NC}"
    ls -lh flash_ops.bin flash_ops.inc
else
    echo -e "${RED}✗ PIC code build failed${NC}"
    exit 1
fi

# Step 2: Verify the generated code
echo ""
echo -e "${YELLOW}Step 2: Verifying generated code...${NC}"
if [ -f flash_ops.inc ]; then
    SIZE=$(stat -c%s flash_ops.bin 2>/dev/null || stat -f%z flash_ops.bin)
    echo "  Binary size: $SIZE bytes"
    echo "  Disassembly preview:"
    head -20 flash_ops.lst
    echo -e "${GREEN}✓ Code verification passed${NC}"
else
    echo -e "${RED}✗ flash_ops.inc not found${NC}"
    exit 1
fi

# Step 3: (Optional) Build with CMake
echo ""
echo -e "${YELLOW}Step 3: Building main program...${NC}"
read -p "Build main program with CMake? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    mkdir -p build
    cd build
    cmake .. -G "Ninja"
    ninja
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Main program built successfully${NC}"
        ls -lh *.elf *.bin
    else
        echo -e "${RED}✗ Main program build failed${NC}"
        exit 1
    fi
    cd ..
fi

echo ""
echo -e "${GREEN}================================${NC}"
echo -e "${GREEN} Build Complete!${NC}"
echo -e "${GREEN}================================${NC}"
echo ""
echo "Generated files:"
echo "  flash_ops.bin - Raw binary (loadable to SRAM0)"
echo "  flash_ops.inc - C header for embedding"
echo "  flash_ops.lst - Assembly listing"
echo "  flash_ops.dis - Disassembly"
echo ""
echo "Usage:"
echo "  1. Include flash_ops.inc in your C program"
echo "  2. Copy flash_ops_code[] to SRAM0"
echo "  3. Call functions with proper parameters"
