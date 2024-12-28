
.include "../include/common.inc"

	.section	.text
	.code32

	.global	PutLong
PutLong:
	push	eax
	shr	eax, 16
	call	PutWord
	pop	eax
#	jmp	PutWord

	.global	PutWord
PutWord:
	push	eax
	mov	al, ah
	call	PutByte
	pop	eax
#	jmp	PutByte

	.global	PutByte
PutByte:
	push	eax
	shr	al, 4
	call	PutNibble
	pop	eax
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
	push	esi
	call	PutChar
	pop	esi
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

