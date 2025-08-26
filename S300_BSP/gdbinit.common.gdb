set pagination off
set confirm off
set remotetimeout 20

# ELF is injected by Makefile via: -ex "file <path>"

# Connect to a gdbserver if available (OpenOCD/J-Link usually at :3333)
target extended-remote :3333

# Make sure we operate on the Cortex-M4 (avoid touching the M0 AON target)
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

# Optionally set VTOR to 0x20000000 (many servers do that on reset)
set {unsigned int}0xE000ED08 = 0x20000000

echo \n>>> Starting program...\n
continue
