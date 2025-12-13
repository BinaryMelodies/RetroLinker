
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
.elseif	CPU_ARM
	mov	r7, #1
	mov	r0, #0
	swi	0
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
.elseif	CPU_ARM
	stmdb	sp!, {r0, lr}
	mov	r7, #4
	mov	r0, #1
	mov	r1, sp
	mov	r2, #1
	swi	0
	ldmia	sp!, {r0, pc}
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
.elseif	CPU_ARM
	# TODO
.endif
.endm

.macro	_SysVars
.endm

