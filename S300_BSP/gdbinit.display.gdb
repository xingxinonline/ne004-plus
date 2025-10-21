set pagination off
set confirm off
set remotetimeout 20

# Display Demo debug init (SRAM execution at 0x20000000)
# Usage:
#   arm-none-eabi-gdb -q -ex "file <path-to>/s300_display_demo" -ex "target extended-remote :3333" -x S300_BSP/gdbinit.display.gdb
# Or via Ninja target: ninja -C S300_BSP/build dbg_display

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

set {unsigned int}0x4000a018 = (*(unsigned int*)0x4000a018) & ~1
monitor sleep 100
restore ../bin/dsp_image/dsp_dtcm_boot.bin binary 0x44800000
restore ../bin/dsp_image/dsp_ptcm_boot.bin binary 0x44A00000
restore ../bin/dsp_image/dsp_sram0_boot.bin binary 0x44000000

# Optional: set HW break on HardFault to catch faults early
# monitor hwbp 0x00000003

# Go!
echo \n>>> Starting DISPLAY demo...\n
# continue
