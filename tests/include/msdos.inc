
.macro	_StartUp
.if TARGET_DX64
	.code32
	# DX64 is a 64-bit DOS extender but the program starts out in 32-bit mode
.if FORMAT_ADAM33
	# Adam files enable relocations so we can issue a far call to a 64-bit code segment
	call	0x28, 1f
1:
.elseif FORMAT_LV
	# LV files do not contain relocations, so we need to dynamically set up the target address
1:
	call	2f
2:
	pop	edi
	add	edi, 4f - 2b # edi contains the 4 address
	mov	[edi + 3f - 4f], edi # edi relative access to 3
	jmp	fword ptr [edi + 3f - 4f]
3:
	.long	0
	.word	0x28
4:
.endif
	.code64
.endif
.if FORMAT_COM
.if MODEL_TINY
.elseif MODEL_SMALL
	# In small memory model, DS must point after CS
	# So we add the paragraph difference to CS
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
	mov	ss, ax
	mov	cl, 4
	shr	ax, cl
	sub	sp, ax
.elseif MODEL_COMPACT
	# In compact memory model, DS and SS must point after CS
	# So we add the paragraph difference to CS
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
	# MS-DOS 1.0 has no simple way to exit, since AH=0x4C is not yet implemented
	# Instead we will issue a RETF to PSP:0, since the first byte of the PSP is an int 0x20 instruction
	# An int 0x20 expects CS to be set to the PSP, which RETF will do
	# So set up the stack to contain a far return address to PSP:0
	push	es
	xor	ax, ax
	push	ax
.endif
.if MODEL_TINY
	# DS points to the PSP at startup, so we set it to point to SS
	mov	ax, ss
	mov	ds, ax
.elseif MODEL_SMALL
	# DS points to the PSP at startup, so we set it to point to SS
	mov	ax, ss
	mov	ds, ax
.elseif MODEL_COMPACT
	# DS points to the PSP at startup, so we set it to point to CS+paragraph difference
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
.endif
.if TARGET_MSDOS1
	# Store the current stack to be able to exit at any point
	mov	ExitStack, sp
	mov	ExitStack + 2, ss
.endif
.elseif FORMAT_BW
	# In DOS/16M binaries, we expect DS=SS, so this is the easiest way to set up the data segment
	# Since segment differences do not make sense in protected mode
	push	ss
	pop	ds
.endif
.endm

.macro	_Exit
.if TARGET_MSDOS1
	# Restore the stack to the initial state and issue the RETF to return to PSP:0 and execute int 0x20
	lss	sp, ExitStack
	retf
.else
	# Standard way to exit in MS-DOS 2.0 and later
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
	# MS-DOS conin
	mov	ah, 0x01
	int	0x21
.endm

.macro	_SysVars
.if TARGET_MSDOS1
	.section	.bss
	# Initial stack pointer, required for exit
	.global	ExitStack
ExitStack:
	.word	0
	.word	0
.endif
.endm

