
.macro	_StartUp
.endm

.macro	_Exit
.if	CPU_I86
	mov	ax, 1
	xor	bx, bx
	int	0x80
.elseif	CPU_I386
	mov	eax, 1
	xor	ebx, ebx
	int	0x80
.elseif	CPU_X86_64
	mov	rax, 60
	mov	rdi, 0
	syscall
.elseif	CPU_M68K
	move.l	#1, d0
	clr.l	d1
	trap	#0
.elseif	CPU_ARM
	mov	r7, #1
	mov	r0, #0
	swi	0
.elseif	CPU_AARCH64
	# TODO: untested
	mov	w8, #93
	mov	w0, #0
	svc	#0
.elseif	CPU_PPC
	li	r3, 0
	li	r0, 1
	sc
.endif
.endm

.macro	_PutChar char
.if	CPU_I86
	push	ax
	mov	ax, 4
	mov	bx, 1
	mov	cx, sp
	mov	dx, 1
	int	0x80
	add	sp, 2
.elseif	CPU_I386
	push	eax
	mov	eax, 4
	mov	ebx, 1
	mov	ecx, esp
	mov	edx, 1
	int	0x80
	add	esp, 4
.elseif	CPU_X86_64
	push	rsi
	push	rax
	mov	eax, 1
	mov	edi, 1
	mov	rsi, rsp
	mov	edx, 1
	syscall
	add	rsp, 8
	pop	rsi
.elseif	CPU_M68K
	move.w	d0, -(sp)
	move.l	#4, d0
	move.l	#1, d1
	lea.l	1(sp), a0
	move.l	a0, d2
	move.l	#1, d3
	trap	#0
	addq.l	#2, sp
	rts
.elseif	CPU_ARM
	stmdb	sp!, {r0}
	mov	r7, #4
	mov	r0, #1
	mov	r1, sp
	mov	r2, #1
	swi	0
	ldmia	sp!, {r0}
.elseif	CPU_AARCH64
	# TODO: untested
	stp	x0, x1, [sp, #-16]!
	mov	w8, #64
	mov	w0, #1
	ldr	x1, sp
	mov	w2, #1
	svc	#0
	ldp	x0, x1, [sp], #16
.elseif	CPU_PPC
	addi	r1, r1, -4
	stb	r3, 0(r1)
	li	r5, 1
	mr	r4, r1
	li	r3, 1
	li	r0, 4
	sc
	addi	r1, r1, 4
.endif
.endm

.macro	_WaitForKey
.if	CPU_I86
	push	ax
	mov	ax, 3
	mov	bx, 1
	mov	cx, sp
	mov	dx, 1
	int	0x80
	add	sp, 2
.elseif	CPU_I386
	push	eax
	mov	eax, 3
	mov	ebx, 1
	mov	ecx, esp
	mov	edx, 1
	int	0x80
	add	esp, 4
.elseif	CPU_X86_64
	push	rsi
	push	rax
	mov	eax, 0
	mov	edi, 1
	mov	rsi, rsp
	mov	edx, 1
	syscall
	add	rsp, 8
	pop	rsi
.elseif	CPU_M68K
	move.w	d0, -(sp)
	move.l	#3, d0
	move.l	#1, d1
	lea.l	1(sp), a0
	move.l	a0, d2
	move.l	#1, d3
	trap	#0
	addq.l	#2, sp
	rts
.elseif	CPU_ARM
	stmdb	sp!, {r0}
	mov	r7, #3
	mov	r0, #1
	mov	r1, sp
	mov	r2, #1
	swi	0
	ldmia	sp!, {r0}
.elseif	CPU_AARCH64
	# TODO: untested
	stp	x0, x1, [sp, #-16]!
	mov	w8, #63
	mov	w0, #1
	ldr	x1, sp
	mov	w2, #1
	svc	#0
	ldp	x0, x1, [sp], #16
.elseif	CPU_PPC
	stwu	r0, -4(r1)
	li	r5, 1
	mr	r4, r1
	li	r3, 1
	li	r0, 3
	sc
	addi	r1, r1, 4
.endif
.endm

.if	CPU_M68K
.macro	_LoadA	ref, dst
	lea	\ref(pc), \dst
.endm

.macro	_LoadL	ref, dst
	move.l	\ref(pc), \dst
.endm

.macro	_StoreL	src, ref
	move.l	\src, \ref
.endm
.endif

.macro	_SysVars
.endm

