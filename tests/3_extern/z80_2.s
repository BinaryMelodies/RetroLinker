
.include "../include/common.inc"

	.section	.text

	.global	PutWord
PutWord:
	push	hl
	ld	a, h
	call	PutByte
	pop	hl
	ld	a, l
#	jp	PutByte

	.global	PutByte
PutByte:
	push	af
	rra
	rra
	rra
	rra
	call	PutNibble
	pop	af
#	jmp	PutNibble

	.global	PutNibble
PutNibble:
	and	0xF
	cp	10
	jp	nc, 1f
	add	a, '0'
	jp	2f
1:
	add	a, 'A' - 10
2:
	jp	PutChar

	.global	PutString
PutString:
1:
	ld	a, (hl)
	cp	0
	jp	z, 1f
	push	hl
	call	PutChar
	pop	hl
	inc	hl
	jp	1b
1:
	ret

	.global	PutNewLine
PutNewLine:
	ld	a, 13
	call	PutChar
	ld	a, 10
#	jp	PutChar

	.global	PutChar
PutChar:
	_PutChar	a
	ret

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	ret

	.global	Exit
Exit:
	_Exit

	.comm	common1, 5
	.comm	common2, 4

	.section	.bss

	_SysVars

