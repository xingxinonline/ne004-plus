set pagination off
set confirm off
set remotetimeout 20

# Generic SRAM debug init (SRAM execution at 0x20000000)
# Usage example:
#   arm-none-eabi-gdb -q -ex "file <path-to-elf>" -ex "target extended-remote :3333" -x S300_BSP/gdbinit.sram.gdb

# Ensure we operate on Cortex-M4 core
monitor targets ne005.m4

# Halt and soft-reset only the selected core, then load the image into SRAM
monitor halt
monitor soft_reset_halt
# Wait until the target is really halted to avoid register write errors
monitor wait_halt 2000
load

# Vector table is linked at 0x20000000 (SRAM1). Set SP and PC from vector.
set $sp = *(unsigned int*)0x20000000
set $pc = *(unsigned int*)0x20000004

# Set VTOR to SRAM vector base for proper exception handling
set {unsigned int}0xE000ED08 = 0x20000000

echo \n>>> Starting SRAM app...\n
continue
