target remote localhost:3333
set confirm off
set pagination off

monitor halt

# Load the freshly built ELF
file build/flash_test.elf
load

# Ensure VTOR points to SRAM vector table (SystemInit may already set it)
set {int}0xE000ED08 = 0x20000000

# Optional: init SP/PC
set $sp=0x2005FFF0
set $pc=Reset_Handler

break main
continue
