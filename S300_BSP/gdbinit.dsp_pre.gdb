set pagination off
set confirm off
set remotetimeout 20

# Display Demo debug init - PRE DSP LOAD (Part 1)
# Usage:
#   gdb -x gdbinit.dsp_pre.gdb -x <load_images.gdb> -x gdbinit.dsp_post.gdb

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

# Enable required bitfields and give DSP time to bootload
# Clear the bit to hold DSP in reset/wait state
set {unsigned int}0x4000a018 = (*(unsigned int*)0x4000a018) & ~1
monitor sleep 100

echo \n>>> M4 Loaded. DSP reset hold. Ready to load DSP images...\n
