OUTPUT_ARCH("riscv")
ENTRY( _start )

MEMORY{
	debug_ctrl (rwx) : ORIGIN = 0x0, LENGTH = 0x1000
	ROM (RXAI) : ORIGIN = 0x00010000, LENGTH = 0x80000
	clint_ctrl (rw) : ORIGIN = 0x02000000, LENGTH = 0x10000
	plic_ctrl (rw) : ORIGIN = 0x0C000000, LENGTH = 0x40000
	MMIO (RWX) : ORIGIN = 0x60000000, LENGTH = 0x80000
	RAM (RWX) : ORIGIN = 0x80000000, LENGTH = 256K
	PSRAM(RWX) : ORIGIN = 0x80080000, LENGTH = 32M
}

RAM_BASE = ORIGIN(RAM);
RAM_SIZE = LENGTH(RAM);

SECTIONS
{
    . = RAM_BASE;

	__stack_size = DEFINED(__stack_size) ? __stack_size : 4K;
    
    .stack :
	{
		. = ALIGN(16);
		. += __stack_size;
		PROVIDE(_stack_top = .);
		PROVIDE(__freertos_irq_stack_top = .);/* ADDED THIS LINE. */
	} > RAM

    .init :
	{
		_text = .;
		KEEP (*(SORT_NONE(.init)))
	} > RAM

	.text :
	{
		*(.text.unlikely .text.unlikely.*)
		*(.text.startup .text.startup.*)
		*(.text .text.*)
		*(.gnu.linkonce.t.*)
	} > RAM

    .fini :
	{
		KEEP (*(SORT_NONE(.fini)))
		_etext = .;
	} > RAM

    .rodata.align :
	{
		. = ALIGN(16);
		_rodata = .;
	} > RAM

    .rodata.start :
	{
		_rodata_lma = LOADADDR(.rodata.start);
	} > RAM

	.rodata :
	{
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)

		. = ALIGN(16);
		_erodata = .;
	} > RAM

	.data.align :
	{
		. = ALIGN(16);
		_data = .;
	} > RAM

    .data.start :
	{
		_data_lma = LOADADDR(.data.start);
	} > RAM

	.data : 
	{
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(16);
		PROVIDE( __global_pointer$ = . + 0x800 );
		*(.sdata .sdata.*)
		*(.sdata2 .sdata2.*)
		*(.gnu.linkonce.s.*)
		. = ALIGN(16);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
        *(._user_heap ._user_heap*)
		. = ALIGN(16);
		_edata = .;
	} > RAM

	.bss.align :
	{
		. = ALIGN(16);
		_bss = .;
	} > RAM

	.bss.start :
	{
		_bss_lma = LOADADDR(.bss.start);
	} > RAM

	.bss :
	{
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(16);
		_ebss = .;
        
	} > RAM

    ._user_heap :
    {
        . = ALIGN(16);
        KEEP(*(SORT_NONE(._user_heap))) ;
    } > RAM 
    
    /* End of uninitalized data segement */
    _end = .;
    PROVIDE(end = .);
    PROVIDE( __heap_start = . );
    PROVIDE( __heap_end = 0X80040000 );
	
	.udata : {
		*(.udata)
	} > PSRAM
}
