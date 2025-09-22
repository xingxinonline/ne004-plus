#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

usage() {
  cat <<'EOF'
Usage: ./build.sh [build|clean|size|s300_image|dis|info|debug]

Commands:
  build       Build all artifacts (.elf .bin .hex .dis)
  clean       Remove build directory
  size        Print size of ELF
  s300_image  Generate header.bin and complete.bin
  dis         Open disassembly in less
  info        Print toolchain & targets
  debug       Run GDB with common init (requires debug server at :3333)
EOF
}

cmd=${1:-build}

case "$cmd" in
  build)
    make -C . all
    ;;
  clean)
    make -C . clean
    ;;
  size)
    make -C . all >/dev/null
    arm-none-eabi-size build/s300_rbl_minimal.elf || true
    ;;
  s300_image)
    make -C . s300_image
    ;;
  dis)
    make -C . all >/dev/null
    ${PAGER:-less} build/s300_rbl_minimal.dis
    ;;
  info)
    echo "Toolchain:"; arm-none-eabi-gcc --version | head -n1 || true
    echo "Makefile target: s300_rbl_minimal"
    echo "Output dir: $(pwd)/build"
    ;;
  debug)
    make -C . dbg
    ;;
  *)
    usage
    exit 1
    ;;
 esac