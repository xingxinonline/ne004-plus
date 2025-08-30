/* SBL链接脚本 - 二级引导程序 */

MEMORY
{
    /* S300 SRAM内存布局 
       Total: 512KB SRAM (0x20000000 - 0x2007FFFF)
       SBL: 64KB (0x20000000 - 0x2000FFFF)  
       Stack: 16KB (0x20010000 - 0x20013FFF)
       App: 剩余空间由应用使用 */
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
    STACK (rw) : ORIGIN = 0x20010000, LENGTH = 16K
}

_estack = ORIGIN(STACK) + LENGTH(STACK);

ENTRY(Reset_Handler)

SECTIONS
{
    /* 向量表必须在最前面 */
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
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        
        /* 保持SBL版本信息 */
        KEEP(*(.version))
        
        /* 保持构造函数和析构函数 */
        KEEP(*(.init))
        KEEP(*(.fini))
        
        /* C++异常处理 */
        *(.eh_frame)
        
        . = ALIGN(4);
        _etext = .;
    } >SRAM

    /* ARM异常处理表 */
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

    /* 未初始化数据段 */
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
    } >SRAM

    /* 堆栈区域 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >SRAM

    /* 调试信息 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

    /* 删除不需要的段 */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}

/* 最小堆栈大小 */
_Min_Heap_Size = 0x1000;  /* 4KB堆 */
_Min_Stack_Size = 0x2000; /* 8KB栈 */

/* 内存区域大小验证 */
ASSERT((_edata - ORIGIN(SRAM)) <= LENGTH(SRAM), "SBL代码超出SRAM限制")
ASSERT((ORIGIN(STACK) + LENGTH(STACK) - _ebss) >= _Min_Stack_Size, "栈空间不足")
