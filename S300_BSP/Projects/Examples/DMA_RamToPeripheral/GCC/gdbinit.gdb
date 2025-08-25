# GDB init for SRAM-run (sram.ld)
target extended-remote localhost:3333
monitor reset halt
monitor arm semihosting enable

file build/dma_m2p_uart1_tx.elf
load

set {int}0xE000ED08 = 0x20000000
set $sp = *(unsigned int*)0x20000000
set $pc = *(unsigned int*)0x20000004

break main
continue
