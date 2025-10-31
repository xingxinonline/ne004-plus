/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Linker script for Position Independent Code */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(flash_read_pic)

SECTIONS
{
	/* Start at 0 for position independence */
	. = 0x0;
	
	.text : {
		*(.text*)
		*(.rodata*)
	}
	
	.data : {
		*(.data*)
	}
	
	.bss : {
		*(.bss*)
	}
	
	/* Discard unnecessary sections */
	/DISCARD/ : {
		*(.ARM.exidx*)
		*(.ARM.attributes*)
		*(.comment)
	}
}
