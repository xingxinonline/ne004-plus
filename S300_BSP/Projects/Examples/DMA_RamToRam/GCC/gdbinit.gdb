# GDB init for SRAM-run (sram.ld): load to RAM, set VTOR/SP/PC from vector table, then run
target extended-remote localhost:3333
monitor reset halt
monitor arm semihosting enable

# Load the freshly built ELF
file build/dma_ram_to_ram.elf
load

# Vector table base in SRAM1 (sram.ld ORIGIN = 0x20000000)
set {int}0xE000ED08 = 0x20000000
# Load SP and PC from vector table
set $sp = *(unsigned int*)0x20000000
set $pc = *(unsigned int*)0x20000004

# Break at main and run
break main
continue
