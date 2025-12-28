
.include "../include/common.inc"

	.text

	.global	PutWord
PutWord:
	mov	r0, -(sp)
	ash	$-8, r0
	jsr	pc, PutByte
	mov	(sp)+, r0
#	jmp	PutByte

	.global	PutByte
PutByte:
	mov	r0, -(sp)
	ash	$-4, r0
	jsr	pc, PutNibble
	mov	(sp)+, r0
#	jmp	PutNibble

	.global	PutNibble
PutNibble:
	bic	$0xFFF0, r0
	cmp	r0, $10
	bge	1f
	add	$'0', r0
	br	2f
1:
	add	$'A' - 10, r0
2:
	jmp	PutChar

	.global	PutString
PutString:
1:
	movb	(r5)+, r0
	beq	1f
	jsr	pc, PutChar
	br	1b
1:
	rts	pc

	.global	PutNewLine
PutNewLine:
.if TARGET_DXDOS
	mov	$13, r0
	jsr	pc, PutChar
.endif
	mov	$10, r0

	.global	PutChar
PutChar:
	_PutChar	r0
	rts	pc

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	rts	pc

	.global	Exit
Exit:
	_Exit

	.comm	common1, 5
	.comm	common2, 4

	.bss

	_SysVars

