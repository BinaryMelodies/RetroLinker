
.include "../include/common.inc"

	.section	.text

	.global	PutLong
PutLong:
	move.l	d0, -(sp)
	swap	d0
	bsrs	PutWord
	move.l	(sp)+, d0
#	bras	PutWord

	.global	PutWord
PutWord:
	move.w	d0, -(sp)
	lsr.w	#8, d0
	bsrs	PutByte
	move.w	(sp)+, d0
#	bras	PutByte

	.global	PutByte
PutByte:
	move.w	d0, -(sp)
	lsr.w	#4, d0
	bsrs	PutNibble
	move.w	(sp)+, d0
#	bras	PutNibble

	.global	PutNibble
PutNibble:
	and.w	#0xF, d0
	cmp.b	#10, d0
	bccs	1f
	add.b	#'0', d0
	bras	2f
1:
	add.b	#'A' - 10, d0
2:
	bras	PutChar

	.global	PutString
PutString:
1:
	move.b	(a0)+, d0
	beqs	1f
	move.l	a0, -(sp)
	bsrs	PutChar
	move.l	(sp)+, a0
	bras	1b
1:
	rts

	.global	PutNewLine
PutNewLine:
.if OPTION_NEWLINE_PROC
	_PutNewLine
	rts
.else
.if !OPTION_ONLY_LF
	move.b	#13, d0
	bsr	PutChar
.endif
	move.b	#10, d0
#	bra	PutChar
.endif

	.global	PutChar
PutChar:
	_PutChar	d0
	rts

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	rts

	.global	Exit
Exit:
	_Exit

.if TARGET_MACOS
	.section	.a5world, "aw", @nobits
.else
	.section	.bss
.endif
	_SysVars

	.section	.bss
	.comm	common1, 5
	.comm	common2, 4

