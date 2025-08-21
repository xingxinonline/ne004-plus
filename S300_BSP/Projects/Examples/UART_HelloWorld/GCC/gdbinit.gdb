target remote localhost:3333
set confirm off
set pagination off

# Optional TUI layouts; uncomment if using gdb-tui
# layout src
# layout regs

monitor halt

# Load the freshly built ELF
file build/uart_helloworld.elf
load

# VTOR -> 0x00000000 (if remapping is desired). Our SystemInit sets VTOR=0x20000000.
# If you need to override, uncomment next line
set {int}0xE000ED08 = 0x20000000

# Initialize SP/PC to SRAM addresses if needed
# These are optional because ELF load with vector may set them; adjust as needed
set $sp=0x2005FFF0
set $pc=Reset_Handler

# Break at main and run
break main
commands
silent
printf "\n>>> Hit main, dump RCC/APB1 and UART3 regs (for UART3 bring-up)\n"
printf "RCC APB1 CLK_EN @0x4000A00C: "
x/w 0x4000A00C
printf "RCC APB1 RST_CTL @0x4000A024: "
x/w 0x4000A024
printf "UART3 LSR @0x40013014: "
x/w 0x40013014
printf "UART3 USR @0x4001307C: "
x/w 0x4001307C
printf "IO_MUX_CFG1 @0x40009004: "
x/w 0x40009004
printf "IO_MAT_CFG1 @0x40008004: "
x/w 0x40008004
end
continue
