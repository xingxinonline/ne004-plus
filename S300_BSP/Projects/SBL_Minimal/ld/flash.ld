/*
 * S300 SBL Minimal Linker Script - Flash Execution
 * SBL运行在Flash中（0x00100000开始）
 */

/* Memory layout for S300 */
MEMORY
{
    /* SBL运行在QSPI Flash XIP模式，从64KB偏移开始，大小64KB */
    FLASH (rx) : ORIGIN = 0x08010000, LENGTH = 64K
    
    /* SRAM1 用于数据和栈 */
    SRAM1 (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Entry point */
ENTRY(Reset_Handler)

/* Stack size */
_stack_size = 0x2000; /* 8KB stack */

SECTIONS
{
    /* Vector table at start of Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ARM unwinding tables */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH
    
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Initialized data (will be copied from Flash to SRAM) */
    .data : 
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM1 AT> FLASH
    
    _sidata = LOADADDR(.data);

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM1

    /* Stack */
    .stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;
    } > SRAM1

    /* Heap (if needed) */
    .heap :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + 0x1000; /* 4KB heap */
        . = ALIGN(4);
        _eheap = .;
    } > SRAM1

    /* Check for overflow */
    _sram_end = ORIGIN(SRAM1) + LENGTH(SRAM1);
    ASSERT(_eheap <= _sram_end, "SRAM overflow!")
}
