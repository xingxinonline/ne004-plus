/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   S300 Flash Loader - Position Independent Code                         *
 *   Based on OpenOCD STMQSPI loader architecture                          *
 *   Using Cadence QSPI Controller for W25Qxx flash                        *
 ***************************************************************************/

	.text
	.syntax unified
	.cpu cortex-m4
	.thumb
	.thumb_func

/* Cadence QSPI Register Offsets */
	.equ CQSPI_REG_CONFIG,          0x00
	.equ CQSPI_REG_CMDCTRL,         0x90
	.equ CQSPI_REG_CMDADDRESS,      0x94
	.equ CQSPI_REG_CMDREADDATALOWER,  0xA0
	.equ CQSPI_REG_CMDWRITEDATALOWER, 0xA8

	.equ CQSPI_CMDCTRL_EXECUTE,     (1 << 0)
	.equ CQSPI_CMDCTRL_INPROGRESS,  (1 << 1)
	.equ CQSPI_CMDCTRL_WR_EN,       (1 << 15)
	.equ CQSPI_CMDCTRL_ADDR_EN,     (1 << 19)
	.equ CQSPI_CMDCTRL_RD_EN,       (1 << 23)
	
	.equ CQSPI_CFG_IDLE,            (1 << 31)

/* W25Qxx Flash Commands */
	.equ W25Q_CMD_READ,    0x03
	.equ W25Q_CMD_WREN,    0x06
	.equ W25Q_CMD_PP,      0x02
	.equ W25Q_CMD_SE_4K,   0x20
	.equ W25Q_CMD_BE_32K,  0x52
	.equ W25Q_CMD_BE_64K,  0xD8
	.equ W25Q_CMD_RDSR1,   0x05

/* Status Register Bits */
	.equ SR_WIP,           (1 << 0)  /* Write In Progress */


/* Macro: Wait for QSPI controller idle */
	.macro qspi_wait_idle qspi_base, timeout_reg, tmp_reg
	ldr		\timeout_reg, =100000
1:
	ldr		\tmp_reg, [\qspi_base, #CQSPI_REG_CONFIG]
	tst		\tmp_reg, #CQSPI_CFG_IDLE
	bne		2f
	subs	\timeout_reg, #1
	bne		1b
	movs	r0, #1					/* timeout error */
	b		error_exit
2:
	.endm


/* Macro: Execute STIG command and wait */
	.macro qspi_exec_cmd qspi_base, timeout_reg, tmp_reg
	/* Set EXECUTE bit to start command */
	ldr		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	orr		\tmp_reg, #CQSPI_CMDCTRL_EXECUTE
	str		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	
	/* Data Synchronization Barrier - ensure write completes */
	dsb
	
	/* Wait for command completion */
	ldr		\timeout_reg, =100000
3:
	ldr		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	tst		\tmp_reg, #CQSPI_CMDCTRL_INPROGRESS
	beq		4f
	subs	\timeout_reg, #1
	bne		3b
	movs	r0, #2					/* timeout error */
	b		error_exit
4:
	/* Clear execute bit */
	movs	\tmp_reg, #0
	str		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	
	/* Data Synchronization Barrier - ensure write completes */
	dsb
	
	/* Wait for controller to become idle - CRITICAL! */
	qspi_wait_idle \qspi_base, \timeout_reg, \tmp_reg
	.endm


/* Macro: Send Write Enable command */
	.macro send_wren qspi_base, tmp_reg
	/* Setup WREN command (no address, no data) */
	movs	\tmp_reg, #W25Q_CMD_WREN
	lsls	\tmp_reg, #24			/* opcode at bits [31:24] */
	str		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	
	qspi_exec_cmd \qspi_base, r11, r12
	qspi_wait_idle \qspi_base, r11, r12
	.endm


/* Macro: Wait for WIP bit to clear */
	.macro wait_wip qspi_base, tmp_reg
	ldr		r11, =1000000			/* long timeout for erase */
5:
	/* Read Status Register */
	movs	\tmp_reg, #W25Q_CMD_RDSR1
	lsls	\tmp_reg, #24			/* opcode at [31:24] */
	orr		\tmp_reg, #(1 << 20)	/* read 1 byte */
	orr		\tmp_reg, #CQSPI_CMDCTRL_RD_EN
	str		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDCTRL]
	
	qspi_exec_cmd \qspi_base, r10, r12
	
	/* Check WIP bit */
	ldr		\tmp_reg, [\qspi_base, #CQSPI_REG_CMDREADDATALOWER]
	tst		\tmp_reg, #SR_WIP
	beq		6f						/* WIP cleared, done */
	
	subs	r11, #1
	bne		5b
	movs	r0, #3					/* WIP timeout */
	b		error_exit
6:
	.endm


/* 
 * Flash Read Function (Position Independent)
 * Uses STIG mode for simplicity (can be upgraded to indirect/DMA)
 */
	.global flash_read_pic
	.type flash_read_pic, %function
flash_read_pic:
	push	{r4-r7, r10-r12, lr}
	
	mov		r4, r0					/* flash address */
	mov		r5, r1					/* dest buffer */
	mov		r6, r2					/* count */
	mov		r7, r3					/* QSPI base */
	
read_loop:
	cmp		r6, #0
	beq		read_done
	
	/* Read up to 8 bytes at a time (STIG limit) */
	movs	r10, #8
	cmp		r6, r10
	blo		1f
	movs	r10, #8
	b		2f
1:
	mov		r10, r6
2:
	
	/* Setup READ command: opcode=0x03, address=r4, read r10 bytes */
	str		r4, [r7, #CQSPI_REG_CMDADDRESS]
	
	movs	r12, #W25Q_CMD_READ
	lsls	r12, #24				/* opcode at [31:24] */
	orr		r12, #(3 << 16)			/* 3-byte address */
	orr		r12, #CQSPI_CMDCTRL_ADDR_EN
	subs	r10, #1					/* CMDCTRL uses N-1 encoding */
	lsls	r10, #20				/* shift to bits [22:20] */
	orrs	r12, r10				/* combine */
	orr		r12, #CQSPI_CMDCTRL_RD_EN
	str		r12, [r7, #CQSPI_REG_CMDCTRL]
	
	qspi_exec_cmd r7, r11, r12
	
	/* Read data from register */
	ldr		r12, [r7, #CQSPI_REG_CMDREADDATALOWER]
	adds	r10, #1					/* restore actual count */
	
	/* Save actual bytes read for loop counter update */
	mov		r8, r10
	
	/* Copy bytes to buffer */
read_copy:
	strb	r12, [r5], #1
	lsrs	r12, #8
	subs	r10, #1
	bne		read_copy
	
	/* Update counters with actual bytes read */
	subs	r6, r6, r8
	add		r4, r4, r8
	b		read_loop
	
read_done:
	movs	r0, #0					/* success */
	pop		{r4-r7, r10-r12, pc}

	.size flash_read_pic, .-flash_read_pic


/* 
 * Flash Write Function (Position Independent)
 * Implements page program with 256-byte page boundary handling
 */
	.global flash_write_pic
	.type flash_write_pic, %function
flash_write_pic:
	push	{r4-r8, r10-r12, lr}
	
	mov		r4, r0					/* flash address */
	mov		r5, r1					/* source buffer */
	mov		r6, r2					/* count */
	mov		r7, r3					/* QSPI base */
	
write_page_loop:
	cmp		r6, #0
	beq		write_done
	
	/* Calculate bytes to page boundary (256-byte pages) */
	mov		r8, r4
	and		r8, r8, #0xFF			/* offset within page */
	rsb		r8, r8, #256			/* bytes to page end */
	cmp		r6, r8
	blo		3f
	mov		r8, r6
3:
	
	/* Send Write Enable */
	send_wren r7, r12
	
	/* Setup Page Program command */
	str		r4, [r7, #CQSPI_REG_CMDADDRESS]
	
	/* Load up to 8 bytes into write data register (STIG limit) */
	cmp		r8, #8
	bls		4f
	movs	r10, #8
	b		5f
4:
	mov		r10, r8
5:
	
	/* Pack bytes into register (little-endian) */
	movs	r12, #0
	mov		r11, r10
pack_loop:
	ldrb	r2, [r5], #1
	orr		r12, r2
	subs	r11, #1
	beq		pack_done
	lsl		r12, #8
	b		pack_loop
	
pack_done:
	str		r12, [r7, #CQSPI_REG_CMDWRITEDATALOWER]
	
	/* Save actual bytes to write before N-1 encoding */
	mov		r11, r10
	
	/* Execute Page Program */
	movs	r12, #W25Q_CMD_PP
	lsls	r12, #24
	orr		r12, #(3 << 16)			/* 3-byte address */
	orr		r12, #CQSPI_CMDCTRL_ADDR_EN
	subs	r10, #1					/* N-1 encoding */
	lsls	r10, #12				/* shift to bits [14:12] */
	orrs	r12, r10				/* combine */
	orr		r12, #CQSPI_CMDCTRL_WR_EN
	str		r12, [r7, #CQSPI_REG_CMDCTRL]
	
	qspi_exec_cmd r7, r10, r12
	
	/* Wait for program to complete */
	wait_wip r7, r12
	
	/* Update counters with actual bytes written */
	subs	r6, r6, r11
	add		r4, r4, r11
	b		write_page_loop
	
write_done:
	movs	r0, #0
	pop		{r4-r8, r10-r12, pc}

	.size flash_write_pic, .-flash_write_pic


/* 
 * Flash Erase Function (Position Independent)
 * Supports 4K/32K/64K sector erase
 */
	.global flash_erase_pic
	.type flash_erase_pic, %function
flash_erase_pic:
	push	{r4-r7, r10-r12, lr}
	
	mov		r4, r0					/* flash address */
	mov		r5, r1					/* erase type */
	mov		r7, r2					/* QSPI base */
	
	/* Select erase command based on type */
	adr		r6, erase_cmd_table
	ldrb	r6, [r6, r5]			/* load command opcode */
	
	/* Send Write Enable */
	send_wren r7, r12
	
	/* Setup erase command */
	str		r4, [r7, #CQSPI_REG_CMDADDRESS]
	
	lsl		r6, #24					/* opcode at [31:24] */
	orr		r6, #(3 << 16)			/* 3-byte address */
	orr		r6, #CQSPI_CMDCTRL_ADDR_EN
	str		r6, [r7, #CQSPI_REG_CMDCTRL]
	
	qspi_exec_cmd r7, r11, r12
	
	/* Wait for erase to complete (can take seconds) */
	wait_wip r7, r12
	
	movs	r0, #0
	pop		{r4-r7, r10-r12, pc}

error_exit:
	/* r0 already contains error code */
	pop		{r4-r7, r10-r12, pc}

	.align 2
erase_cmd_table:
	.byte	W25Q_CMD_SE_4K
	.byte	W25Q_CMD_BE_32K
	.byte	W25Q_CMD_BE_64K
	.byte	0

	.size flash_erase_pic, .-flash_erase_pic


/* 
 * CRC32 Calculation (Position Independent)
 * Based on OpenOCD stmqspi_crc32.S implementation
 * Uses hardware-optimized algorithm with polynomial 0x04C11DB7
 * 
 * Params:
 * r0 - data pointer
 * r1 - length in bytes
 * 
 * Returns:
 * r0 - CRC32 result (inverted final value)
 */
	.global crc32_calc_pic
	.type crc32_calc_pic, %function
crc32_calc_pic:
	push	{r4, r5, r6, lr}
	
	subs	r1, r1, #1				/* decrement for loop */
	movs	r2, #0					/* CRC = 0x00000000 */
	mvns	r2, r2					/* CRC = 0xFFFFFFFF */
	adr		r3, crc32_poly
	ldr		r3, [r3]				/* Load polynomial */
	
crc32_loop:
	cmp		r1, #0
	bmi		crc32_done
	
	ldrb	r4, [r0], #1			/* Load next byte */
	lsls	r4, r4, #24				/* Shift into MSB */
	eors	r2, r2, r4				/* XOR into CRC */
	
	/* Unrolled 8-bit loop for performance */
	.rept	8
	asrs	r4, r2, #31				/* Copy bit 31 to all bits */
	ands	r4, r4, r3				/* mask with poly */
	lsls	r2, r2, #1				/* shift CRC left */
	eors	r2, r2, r4				/* XOR with masked poly */
	.endr
	
	subs	r1, r1, #1
	b		crc32_loop
	
crc32_done:
	mvns	r0, r2					/* Final inversion */
	pop		{r4, r5, r6, pc}

	.size crc32_calc_pic, .-crc32_calc_pic


/* 
 * Get Function Table (Position Independent)
 * Returns pointer to function table structure
 * 
 * Returns:
 * r0 - pointer to flash_ops_table (runtime address)
 */
	.global get_flash_ops_table
	.type get_flash_ops_table, %function
get_flash_ops_table:
	adr		r0, flash_ops_table		/* Get runtime address using PC-relative */
	bx		lr

	.size get_flash_ops_table, .-get_flash_ops_table


	.align 2
crc32_poly:
	.word	0x04C11DB7				/* CRC32 polynomial (same as OpenOCD) */

	.align 2
flash_ops_table:
	.word	flash_read_pic
	.word	flash_write_pic
	.word	flash_erase_pic
	.word	crc32_calc_pic
	.word	0						/* Reserved for future use */


/* Alignment padding for clean binary end */
	.align 4
	.word	0xDEADBEEF				/* End marker */
