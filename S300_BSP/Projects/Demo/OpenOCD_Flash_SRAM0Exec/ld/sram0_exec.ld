/* OpenOCD Flash SRAM0 Exec demo linker script
 * Host code/data run from SRAM1, loader/resident objects occupy SRAM0.
 */
ENTRY(Reset_Handler)

MEMORY
{
  SRAM0 (rwx) : ORIGIN = 0x10000000, LENGTH = 8K
  SRAM1 (rwx) : ORIGIN = 0x20000000, LENGTH = 384K - 256
}

_estack = ORIGIN(SRAM1) + LENGTH(SRAM1);

SECTIONS
{
  /* Loader code + const data live entirely inside SRAM0 */
  .loader_text :
  {
    __loader_text_start__ = .;
    *(.loader.isr_vector)
    *(.loader.text*)
    *(.loader.rodata*)
    __loader_text_end__ = .;
  } > SRAM0

  .loader_data :
  {
    __loader_data_start__ = .;
    *(.loader.data*)
    __loader_data_end__ = .;
  } > SRAM0

  .loader_bss (NOLOAD) :
  {
    __loader_bss_start__ = .;
    *(.loader.bss*)
    *(COMMON)
    __loader_bss_end__ = .;
  } > SRAM0

  .loader_stack (NOLOAD) :
  {
    __loader_stack_start__ = .;
    *(.loader.stack*)
    __loader_stack_end__ = .;
  } > SRAM0

  .loader_shared (NOLOAD) :
  {
    __loader_shared_start__ = .;
    *(.loader.shared*)
    __loader_shared_end__ = .;
  } > SRAM0

  .text :
  {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
  } > SRAM1

  .data :
  {
    *(.data*)
  } > SRAM1

  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss*)
    *(COMMON)
    _ebss = .;
    __bss_end__ = _ebss;
  } > SRAM1
}
