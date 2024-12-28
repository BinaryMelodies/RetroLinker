
.include "../include/common.inc"

	.section	.text

#.if OPTION_CUSTOM_ENTRY
#	_StartUp
#	ld	hl, offset error
#	call	PutString
#	call	PutNewLine
#	call	WaitForKey
#	call	Exit
#.endif

	.global	start
start:
	call	1f
1:
	pop	hl
	ld	bc, start - 1b
	add	hl, bc
	ld	(stored_pc), hl
	_StartUp

	ld	hl, message
	call	PutString
	call	PutNewLine

	ld	hl, text_pc
	call	PutString
	ld	hl, (stored_pc)
	call	PutWord
	call	PutNewLine

	ld	hl, text_sp
	call	PutString
	ld	hl, 0
	add	hl, sp
	call	PutWord
	call	PutNewLine

	ld	hl, text_message
	call	PutString
	ld	hl, message
	call	PutWord
	call	PutNewLine

	call	WaitForKey
	call	Exit

PutWord:
	push	hl
	ld	a, h
	call	PutByte
	pop	hl
	ld	a, l
#	jp	PutByte

PutByte:
	push	af
	rra
	rra
	rra
	rra
	call	PutNibble
	pop	af
#	jmp	PutNibble

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

PutNewLine:
	ld	a, 13
	call	PutChar
	ld	a, 10
#	jp	PutChar

PutChar:
	_PutChar	a
	ret

WaitForKey:
	_WaitForKey
	ret

Exit:
	_Exit
	.section	.data

error:
	.asciz	"Error"
	.align	4

message:
	.ascii	"Greetings!"
.if TARGET_CPM80
	.ascii	" CP/M-80"
.if FORMAT_COM
	.ascii	" flat binary .com file"
.elseif FORMAT_PRL
	.ascii	" page relocatable .prl file"
.endif
.elseif TARGET_MSXDOS
	.ascii	" MSX-DOS"
.endif

	.byte	0

text_pc:
	.asciz	"PC="

text_sp:
	.asciz	"SP="

text_message:
	.asciz	"message="

	.section	.bss

stored_pc:
	.skip	2

	_SysVars

