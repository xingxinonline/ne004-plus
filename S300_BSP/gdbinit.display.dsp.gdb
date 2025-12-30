set pagination off
set confirm off
set remotetimeout 20

# Display Demo debug init with DSP boot images
# Usage:
#   arm-none-eabi-gdb -q -ex "file <path-to>/s300_display_demo" -ex "target extended-remote :3333" -x S300_BSP/gdbinit.display.dsp.gdb

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
set {unsigned int}0x4000a018 = (*(unsigned int*)0x4000a018) & ~1
monitor sleep 100

# Load DSP boot images (adjust paths if necessary)
# Images for Display_Demo are now placed under:
#   S300_BSP/Projects/Demo/Display_Demo/DSP_Images
# NOTE: GDB is launched from the Display_Demo/build directory in dbg_display_dsp,
# so we reference images relative to that working directory.

# Use shell to generate a temporary GDB script that only restores non-empty files
shell echo "# Generated restore commands" > restore_imgs.tmp
shell if [ -s "../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_dtcm_boot.bin" ]; then echo "restore ../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_dtcm_boot.bin binary 0x44800000" >> restore_imgs.tmp; fi
shell if [ -s "../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_ptcm_boot.bin" ]; then echo "restore ../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_ptcm_boot.bin binary 0x44A00000" >> restore_imgs.tmp; fi
shell if [ -s "../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_sram0_boot.bin" ]; then echo "restore ../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_sram0_boot.bin binary 0x44000000" >> restore_imgs.tmp; fi

source restore_imgs.tmp
shell rm restore_imgs.tmp




# Optional: set HW break on HardFault to catch faults early
# monitor hwbp 0x00000003

# Go!
echo \n>>> Starting DISPLAY demo with DSP boot...\n

# Set temporary breakpoint at app init (after PSRAM init) to load PSRAM image
tbreak display_demo_app_init
commands
  echo \n>>> PSRAM initialized. Checking for PSRAM image...\n
  shell echo "# Generated restore commands for PSRAM" > restore_psram.tmp
  shell if [ -s "../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_psram_boot.bin" ]; then echo "restore ../../../../Projects/Demo/Display_Demo/DSP_Images/dsp_psram_boot.bin binary 0x80000000" >> restore_psram.tmp; fi
  source restore_psram.tmp
  shell rm restore_psram.tmp
  continue
end

set {unsigned int}0x4000a018 = 1

# keep paused for inspection; uncomment to run automatically
continue
