
.include "../include/common.inc"

	.section	.text
	.code16

	.global	PutWord
PutWord:
	push	ax
	mov	al, ah
	call	PutByte
	pop	ax
#	jmp	PutByte

	.global	PutByte
PutByte:
	push	ax
	mov	cl, 4
	shr	al, cl
	call	PutNibble
	pop	ax
#	jmp	PutNibble

	.global	PutNibble
PutNibble:
	and	al, 0xF
	cmp	al, 10
	jnc	1f
	add	al, '0'
	jmp	2f
1:
	add	al, 'A' - 10
2:
	jmp	PutChar

	.global	PutString
PutString:
1:
	lodsb
	test	al, al
	jz	1f
	push	si
	call	PutChar
	pop	si
	jmp	1b
1:
	ret

	.global	PutNewLine
PutNewLine:
	mov	al, 13
	call	PutChar
	mov	al, 10
#	jmp	PutChar

	.global	PutChar
PutChar:
	_PutChar	al
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

