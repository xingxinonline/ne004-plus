/**
 * @file app.ld
 * @brief App链接脚本 - 使用完整364KB SRAM
 */

/* 内存定义 */
MEMORY
{
    /* 从SBL优化后得到的完整SRAM空间 */
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 364K
    
    /* Flash XIP区域（只读） */
    FLASH (rx) : ORIGIN = 0x08030000, LENGTH = 1M
}

/* 栈和堆大小 */
_stack_size = 8K;
_heap_size = 32K;

/* 栈顶（SRAM末尾） */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/* 入口点 */
ENTRY(Reset_Handler)

/* 段定义 */
SECTIONS
{
    /* 中断向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >SRAM
    
    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        
        /* Reset处理程序放在开头 */
        *(.text.Reset_Handler)
        
        /* 其他代码 */
        *(.text)
        *(.text*)
        
        /* 只读数据 */
        *(.rodata)
        *(.rodata*)
        
        /* C++支持 */
        KEEP (*(.init))
        KEEP (*(.fini))
        
        /* 构造函数和析构函数表 */
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        
        . = ALIGN(4);
    } >SRAM
    
    /* ARM异常处理 */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >SRAM
    
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >SRAM
    
    /* 初始化数据段 */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        
        *(.data)
        *(.data*)
        
        . = ALIGN(4);
        _edata = .;
    } >SRAM
    
    /* 数据段初始化源（与数据段相同，因为我们在SRAM中运行） */
    _sidata = LOADADDR(.data);
    
    /* 未初始化数据段 */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        
        *(.bss)
        *(.bss*)
        *(COMMON)
        
        . = ALIGN(4);
        _ebss = .;
    } >SRAM
    
    /* 堆区域 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        _heap_start = .;
        . = . + _heap_size;
        _heap_end = .;
        . = . + _stack_size;
        . = ALIGN(8);
    } >SRAM
    
    /* 删除调试信息 */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
    
    /* DWARF调试段 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end ) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges   0 : { *(.debug_ranges) }
    .debug_macro    0 : { *(.debug_macro) }
    
    .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) KEEP (*(.gnu.attributes)) }
    .comment       0 : { *(.comment) }
    .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
}

/* 内存使用检查 */
ASSERT(_heap_end <= _estack - _stack_size, "Heap and stack overlap!")
ASSERT(SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.bss) <= LENGTH(SRAM) - _heap_size - _stack_size, "Program too large for SRAM!")

/* 符号定义 */
_sram_start = ORIGIN(SRAM);
_sram_size = LENGTH(SRAM);
_sram_end = ORIGIN(SRAM) + LENGTH(SRAM);

/* 提供给代码使用的符号 */
PROVIDE(_heap_size_actual = _heap_end - _heap_start);
PROVIDE(_stack_size_actual = _stack_size);
PROVIDE(_sram_free = LENGTH(SRAM) - (SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.bss) + _heap_size + _stack_size));
