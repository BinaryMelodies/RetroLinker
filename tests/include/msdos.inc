
.macro	_StartUp
.if FORMAT_COM
.if MODEL_TINY
.elseif MODEL_SMALL
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
	mov	ss, ax
	mov	cl, 4
	shr	ax, cl
	sub	sp, ax
.elseif MODEL_COMPACT
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
	mov	ax, cs
	add	ax, offset $$SEGDIF$.stack$.text
	mov	ss, ax
	mov	cl, 4
	shr	ax, cl
	sub	sp, ax
.endif
.elseif FORMAT_MZ
.if TARGET_MSDOS1
	# retf to PSP:0 with an int 0x20 (CS must also be set)
	push	es
	xor	ax, ax
	push	ax
.endif
.if MODEL_TINY
	mov	ax, ss
	mov	ds, ax
.elseif MODEL_SMALL
	mov	ax, ss
	mov	ds, ax
.elseif MODEL_COMPACT
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
.endif
.if TARGET_MSDOS1
	mov	ExitStack, sp
	mov	ExitStack + 2, ss
.endif
.elseif FORMAT_BW
	push	ss
	pop	ds
.endif
.endm

.macro	_Exit
.if TARGET_MSDOS1
	lss	sp, ExitStack
	retf
.else
	mov	ax, 0x4C00
	int	0x21
.endif
.endm

.macro	_PutChar char
	mov	dl, \char
	mov	ah, 0x02
	int	0x21
.endm

.macro	_WaitForKey
	mov	ah, 0x01
	int	0x21
.endm

.macro	_SysVars
.if TARGET_MSDOS1
	.section	.bss
	.global	ExitStack
ExitStack:
	.word	0
	.word	0
.endif
.endm

