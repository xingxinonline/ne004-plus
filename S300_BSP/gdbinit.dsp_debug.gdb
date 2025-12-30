set pagination off
set confirm off
set remotetimeout 20

# DSP Debug Demo init
# Usage:
#   arm-none-eabi-gdb -q -ex "file <path-to-elf>" -ex "target extended-remote :3333" -x S300_BSP/gdbinit.dsp_debug.gdb

monitor targets ne005.m4

monitor halt
monitor soft_reset_halt
monitor wait_halt 2000
load

set $sp = *(unsigned int*)0x20000000
set $pc = *(unsigned int*)0x20000004
set {unsigned int}0xE000ED08 = 0x20000000

# Enable required bitfields for DSP
set {unsigned int}0x4000a018 = (*(unsigned int*)0x4000a018) & ~1
monitor sleep 100

# Go!
echo \n>>> Starting DSP Debug app...\n

# Break after PSRAM init to load images
# init_mailbox is called right after init_psram in main.c
tbreak init_mailbox
commands
  echo \n>>> PSRAM initialized. Loading DSP images...\n
  
  shell echo "Current GDB Directory:"
  shell pwd
  shell echo "Checking for images in ../../../../Projects/Demo/DSP_Debug/DSP_Images/ :"
  shell ls -l ../../../../Projects/Demo/DSP_Debug/DSP_Images/

  # Generate restore commands for any present images
  shell echo "# Generated restore commands" > restore_dsp.tmp
  
  # DTCM (0x44800000)
  shell if [ -s "../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_dtcm_boot.bin" ]; then echo "restore ../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_dtcm_boot.bin binary 0x44800000" >> restore_dsp.tmp; else echo "echo [WARN] Skipping dsp_dtcm_boot.bin (not found or empty)\\n" >> restore_dsp.tmp; fi
  
  # PTCM (0x44A00000)
  shell if [ -s "../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_ptcm_boot.bin" ]; then echo "restore ../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_ptcm_boot.bin binary 0x44A00000" >> restore_dsp.tmp; else echo "echo [WARN] Skipping dsp_ptcm_boot.bin (not found or empty)\\n" >> restore_dsp.tmp; fi
  
  # SRAM0 (0x44000000)
  shell if [ -s "../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_sram0_boot.bin" ]; then echo "restore ../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_sram0_boot.bin binary 0x44000000" >> restore_dsp.tmp; else echo "echo [WARN] Skipping dsp_sram0_boot.bin (not found or empty)\\n" >> restore_dsp.tmp; fi
  
  # SRAM1 (0x44040000)
  shell if [ -s "../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_sram1_boot.bin" ]; then echo "restore ../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_sram1_boot.bin binary 0x44040000" >> restore_dsp.tmp; else echo "echo [WARN] Skipping dsp_sram1_boot.bin (not found or empty)\\n" >> restore_dsp.tmp; fi
  
  # PSRAM (0x80000000)
  shell if [ -s "../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_psram_boot.bin" ]; then echo "restore ../../../../Projects/Demo/DSP_Debug/DSP_Images/dsp_psram_boot.bin binary 0x80000000" >> restore_dsp.tmp; else echo "echo [WARN] Skipping dsp_psram_boot.bin (not found or empty)\\n" >> restore_dsp.tmp; fi

  source restore_dsp.tmp
  shell rm restore_dsp.tmp
  
  echo \n>>> DSP Images loaded. Continuing...\n
  
  # Release DSP from reset (set bit 0 of 0x4000a018 to 1)
  set {unsigned int}0x4000a018 = (*(unsigned int*)0x4000a018) | 1
  
  continue
end

continue
