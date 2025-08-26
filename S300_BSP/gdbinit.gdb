############################################################
# Top-level gdbinit for backward compatibility
# Prefer using: make dbg [PROJECT=<name>]
############################################################

# Default to Minimal ELF if user runs: arm-none-eabi-gdb -x S300_BSP/gdbinit.gdb
file Projects/Minimal/GCC/build/s300_minimal.elf

source gdbinit.common.gdb
