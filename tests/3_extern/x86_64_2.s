
.include "../include/common.inc"

	.section	.text
	.code64

	.global	PutQuad
PutQuad:
	push	rax
	shr	rax, 32
	call	PutLong
	pop	rax
#	jmp	PutLong

	.global	PutLong
PutLong:
	push	rax
	shr	eax, 16
	call	PutWord
	pop	rax
#	jmp	PutWord

	.global	PutWord
PutWord:
	push	rax
	mov	al, ah
	call	PutByte
	pop	rax
#	jmp	PutByte

	.global	PutByte
PutByte:
	push	rax
	shr	al, 4
	call	PutNibble
	pop	rax
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
	push	rsi
	call	PutChar
	pop	rsi
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

