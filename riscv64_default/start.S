	.section .init
	.globl _start
	.type _start,@function
_start:
	.cfi_startproc
	.cfi_undefined ra

	csrw mideleg, 0
    csrw medeleg, 0
	# dis interrupt
	csrw mie,zero
	csrw mip,zero

	# misa
	csrr t0,misa
	
	la t0,vector_table
	ori t0,t0,1 # Asynchronous interrupts set pc to BASE+4×cause
	csrw mtvec,t0

    # la t0, isr_stack_top
    # csrw mscratch,t0

    csrw mcause,zero
    csrw mepc,zero
    csrw mtval,zero

    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10,0
    li x11,0
    li x12,0
    li x13,0
    li x14,0
    li x15,0
    li x16,0
    li x17,0
    li x18,0
    li x19,0
    li x20,0
    li x21,0
    li x22,0
    li x23,0
    li x24,0
    li x25,0
    li x26,0
    li x27,0
    li x28,0
    li x29,0
    li x30,0
    li x31,0

    /* set to initial state of FPU and disable interrupt */
    li t0, 0x00006000U
    csrs mstatus, t0

    fssr    x0
    fmv.w.x f0, x0
    fmv.w.x f1, x0
    fmv.w.x f2, x0
    fmv.w.x f3, x0
    fmv.w.x f4, x0
    fmv.w.x f5, x0
    fmv.w.x f6, x0
    fmv.w.x f7, x0
    fmv.w.x f8, x0
    fmv.w.x f9, x0
    fmv.w.x f10,x0
    fmv.w.x f11,x0
    fmv.w.x f12,x0
    fmv.w.x f13,x0
    fmv.w.x f14,x0
    fmv.w.x f15,x0
    fmv.w.x f16,x0
    fmv.w.x f17,x0
    fmv.w.x f18,x0
    fmv.w.x f19,x0
    fmv.w.x f20,x0
    fmv.w.x f21,x0
    fmv.w.x f22,x0
    fmv.w.x f23,x0
    fmv.w.x f24,x0
    fmv.w.x f25,x0
    fmv.w.x f26,x0
    fmv.w.x f27,x0
    fmv.w.x f28,x0
    fmv.w.x f29,x0
    fmv.w.x f30,x0
    fmv.w.x f31,x0

	# init sp gp
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

    # hartid == 0?
	li a1,0 # hard0
	csrr a0,mhartid
	bne a0,a1,secondary //多核的时候只允许hart0运行，其他挂起。

    la sp,_stack_top

	// ld data section
	la a0, _data_lma
	la a1, _data
	la a2, _edata
	bgeu a1, a2, 2f
1:
	ld t0, (a0)
	sd t0, (a1)
	addi a0, a0, 8
	addi a1, a1, 8
	bltu a1, a2, 1b
2:

	// Clear bss section
	la a0, _bss
	la a1, _ebss
	bgeu a0, a1, 2f
1:
	sd zero, (a0)
	addi a0, a0, 8
	bltu a0, a1, 1b

2:
	// argc, argv, envp is 0
	li  a0, 0
	li  a1, 0
	li  a2, 0
    li  t0, 0
	li  t1, 0
	jal main
1:
	wfi
	j 1b

secondary:
	// TODO: Multicore is not supported
	wfi
	j secondary
	.cfi_endproc
