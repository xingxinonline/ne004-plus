# DSP boot images

Place the DSP boot binary images in this folder. Expected filenames and addresses:

- dsp_dtcm_boot.bin  -> load address 0x44800000
- dsp_ptcm_boot.bin  -> load address 0x44A00000
- dsp_sram0_boot.bin -> load address 0x44000000

Optional additional images (if available in your SDK):

- dsp_psram_boot.bin
- dsp_sram1_boot.bin

Notes:

- These binaries were previously under S300_BSP/docs. They are now grouped here for clarity.
- The GDB init script gdbinit.display.dsp.gdb is configured to restore from this directory.
- If you still keep copies under docs/, you may remove them or keep as backup. Ensure the files here are the ones you want to load.
