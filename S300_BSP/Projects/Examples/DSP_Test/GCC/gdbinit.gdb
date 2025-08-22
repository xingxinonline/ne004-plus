target remote localhost:3333
set confirm off
set pagination off

monitor halt

file build/dsp_test.elf
load

# SRAM vector table
set {int}0xE000ED08 = 0x20000000
set $sp=0x2005FFF0
set $pc=Reset_Handler

break main
continue
