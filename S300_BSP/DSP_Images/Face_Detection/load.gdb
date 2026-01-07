# DSP Boot Images Loader - Face Detection
# Paths are relative to CWD when launching GDB (which is build dir)

echo \n>>> Loading Face Detection DSP Images...\n
restore dsp_dtcm_boot.bin binary 0x44800000
restore dsp_ptcm_boot.bin binary 0x44A00000
restore dsp_sram0_boot.bin binary 0x44000000
