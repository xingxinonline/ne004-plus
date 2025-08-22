/* Bootloader in FLASH at 0x08000000, size 64KB */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 384K
}

ENTRY(Reset_Handler)

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text : { *(.text*) *(.rodata*) *(.glue_7*) *(.glue_7t*) } > FLASH
  .ARM.exidx : { *(.ARM.exidx*) } > FLASH

  .data : {
    _sdata = .; *(.data*) _edata = .;
  } > RAM AT > FLASH

  .bss (NOLOAD) : {
    _sbss = .; *(.bss*) *(COMMON) _ebss = .;
  } > RAM

  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
