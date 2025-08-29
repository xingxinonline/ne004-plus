/*
 * SBL链接脚本 - S300二级引导程序专用
 * 运行在Flash XIP，最大化APP可用SRAM
 */

/* 入口点 */
ENTRY(Reset_Handler)

/* 定义系统堆栈大小 */
_Min_Heap_Size = 0x200;  /* 512 bytes */
_Min_Stack_Size = 0x1000; /* 4KB - SBL需要更大栈空间 */

/* S300内存映射定义 */
MEMORY
{
    /* QSPI Flash: SBL运行区域 (64KB专用) */
    /* 分区布局: RBL(0x0-0x8000) + 分区表(0x8000) + SBL(0x10000-0x20000) */
    /* XIP地址映射: Flash物理地址0x10000 → XIP地址0x08010000 */
    FLASH (rx)  : ORIGIN = 0x08010000, LENGTH = 64K
    
    /* SRAM: 384KB (0x20000000-0x20060000) - SBL仅使用栈空间，其余完全保留给APP */
    /* SBL栈在SRAM顶部，APP使用0x20000000开始的空间 */
    SRAM (xrw)  : ORIGIN = 0x2005B000, LENGTH = 20K
}

/* 定义栈顶 - 使用SRAM顶部空间 */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/* 段定义 */
SECTIONS
{
    /* 中断向量表 - 放在Flash开始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* 代码段 - 在Flash中XIP执行 */
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
    } >FLASH

    /* 只读数据段 - 在Flash中 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ARM异常处理表 */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } >FLASH

    /* ARM异常处理索引 */
    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } >FLASH

    /* C++构造函数表 */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } >FLASH

    /* 已初始化数据段的加载地址在Flash，运行地址在SRAM */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)

        . = ALIGN(4);
        _edata = .;
    } >SRAM AT>FLASH

    /* 未初始化数据段 - 仅在SRAM中分配栈空间 */
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

    /* SBL栈空间 - 仅分配必要的栈 */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >SRAM

    /* SBL信息段 - 在Flash中 */
    .sbl_info :
    {
        . = ALIGN(4);
        _sbl_info_start = .;
        
        /* SBL版本信息 */
        KEEP(*(.sbl_version))
        
        /* SBL配置信息 */
        KEEP(*(.sbl_config))
        
        . = ALIGN(4);
        _sbl_info_end = .;
    } >FLASH

    /* 调试信息段 */
    .ARM.attributes 0 : { *(.ARM.attributes) }
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_ranges   0 : { *(.debug_ranges) }
    .debug_line     0 : { *(.debug_line) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .comment        0 : { *(.comment) }

    /* 丢弃段 */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}

/* 定义全局符号 */
PROVIDE(_sbss = __bss_start__);
PROVIDE(_ebss = __bss_end__);
PROVIDE(_sdata_lma = LOADADDR(.data));

/* 内存使用检查 */
ASSERT(_estack > _ebss, "ERROR: SRAM stack overflow")
ASSERT((_estack - _ebss) >= (_Min_Heap_Size + _Min_Stack_Size), "ERROR: Insufficient SRAM for SBL stack")

/* SBL大小限制检查 */
_sbl_flash_size = _sbl_info_end - ORIGIN(FLASH);
ASSERT(_sbl_flash_size <= 64K, "ERROR: SBL size exceeds 64KB Flash limit")

/* SRAM使用统计 */
_sbl_sram_used = _estack - _ebss;
_app_sram_available = 0x20060000 - 0x20000000 - _sbl_sram_used;

/* APP完整SRAM区域 */
_app_sram_start = 0x20000000;
_app_sram_end = _ebss; /* SBL栈之前的所有SRAM都可用 */
_app_sram_size = _app_sram_end - _app_sram_start;

/* SBL信息导出 */
PROVIDE(sbl_version_addr = _sbl_info_start);
PROVIDE(sbl_flash_size = _sbl_flash_size);
PROVIDE(sbl_sram_used = _sbl_sram_used);
PROVIDE(app_sram_start = _app_sram_start);
PROVIDE(app_sram_size = _app_sram_size);
PROVIDE(app_sram_available = _app_sram_available);
