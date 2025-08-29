/**
 * @file rbl_linker.ld
 * @brief RBL链接脚本
 */

/* 内存布局定义 */
MEMORY
{
    /* SRAM区域: ROMBOOT将RBL加载到这里 */
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

/* 程序入口点 */
ENTRY(Reset_Handler)

/* 段定义 */
SECTIONS
{
    /* 中断向量表 - 必须位于开始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >SRAM
    
    /* 程序代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        
        KEEP (*(.init))
        KEEP (*(.fini))
        
        . = ALIGN(4);
        _etext = .;
    } >SRAM
    
    /* 只读数据段 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >SRAM
    
    /* ARM异常处理相关 */
    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >SRAM
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >SRAM
    
    /* 构造函数表 */
    .preinit_array     :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >SRAM
    
    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >SRAM
    
    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >SRAM
    
    /* 已初始化数据段 */
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
    . = ALIGN(4);
    .bss :
    {
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >SRAM
    
    /* 用户堆栈区域 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >SRAM
    
    /* 栈顶地址 */
    _estack = ORIGIN(SRAM) + LENGTH(SRAM);
}

/* 堆栈大小定义 */
_Min_Heap_Size = 0x1000;  /* 4KB 堆 */
_Min_Stack_Size = 0x2000; /* 8KB 栈 */

/* 段大小符号 */
_sidata = LOADADDR(.data);
_stext = ADDR(.text);
_etext = ADDR(.text) + SIZEOF(.text);

/* 检查内存溢出 */
ASSERT((_estack - _Min_Stack_Size) > _ebss, "Stack overflow detected!")
