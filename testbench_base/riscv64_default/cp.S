	.attribute unaligned_access, 0
	.attribute stack_align, 16	
	.section .text
#	.align	1
	.global cp
	.type cp,@function
# void cp(int *dst,int*src,int len)
cp:
	addi	sp,sp,-48
	sd	ra,40(sp)
	sd	t0,32(sp)
	sd	t1,24(sp)
	sd	t2,16(sp)
	sd	t3,8(sp)
	sd	t4,0(sp)

	addi t1,a0,0# t1 <- dst
	addi t2,a1,0# t2 <- src

	addw t0,t1,a2# t0 <= dst + len
1:	 	
	lwu t3,0(t2)
	sw t3,0(t1)
	addi t1,t1,4
	addi t2,t2,4
	bltu t1,t0,1b

	ld	t4,0(sp)
	ld	t3,8(sp)
	ld	t2,16(sp)
	ld	t1,24(sp)
	ld	t0,32(sp)
	ld	ra,40(sp)
	addi	sp,sp,48
	ret

	.global cp1
	.type cp1,@function
# void cp1(int *dst,int*src,int len)
cp1:
	addi	sp,sp,-48
	sd	ra,40(sp)
	sd	t0,32(sp)
	sd	t1,24(sp)
	sd	t2,16(sp)
	sd	t3,8(sp)
	sd	t4,0(sp)

	addi t1,a0,0# t1 <- dst
	addi t2,a1,0# t2 <- src

	addw t0,t1,a2# t0 <= dst + len
1:	
	slliw t4,t0,2
	addiw t4,t4,2
	remw t4,t4,a2
	add t4,t2,t4

	lwu t3,0(t4)
	sw t3,0(t1)
	addi t1,t1,4
	addi t2,t2,4
	bltu t1,t0,1b

	ld	t4,0(sp)
	ld	t3,8(sp)
	ld	t2,16(sp)
	ld	t1,24(sp)
	ld	t0,32(sp)
	ld	ra,40(sp)
	addi	sp,sp,48
	ret
