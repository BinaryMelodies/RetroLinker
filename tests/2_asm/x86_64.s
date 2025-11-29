
.include "../include/common.inc"

	.section	.text
	.code64

.if OPTION_CUSTOM_ENTRY
	_StartUp
	lea	rsi, [rel + offset error]
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit
.endif

	.global	start
start:
	_StartUp
#.if TARGET_WIN16
#	pushw	offset 0
#	pushw	offset $$SEGOF$dialog_message
#	pushw	offset dialog_message
#	pushw	offset $$SEGOF$dialog_title
#	pushw	offset dialog_title
#	pushw	offset 0
#.set MessageBox, $$IMPORT$USER$0001
#.set $$SEGOF$MessageBox, $$IMPSEG$USER$0001
#	.extern	MessageBox, $$SEGOF$MessageBox
#	.byte	0x9A
#	.word	MessageBox
#	.word	$$SEGOF$MessageBox
#.endif

	lea	rsi, [rip + message]
	call	PutString
	call	PutNewLine

	lea	rsi, [rip + text_cs_rip]
	call	PutString
	mov	ax, cs
	call	PutWord
	mov	al, ':'
	call	PutChar
	lea	rax, [rip + start]
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_ss_rsp]
	call	PutString
	mov	ax, ss
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	rax, rsp
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_message]
	call	PutString
	lea	rax, [rip + message]
	call	PutQuad
	call	PutNewLine

	call	WaitForKey
	call	Exit

PutQuad:
	push	rax
	shr	rax, 32
	call	PutLong
	pop	rax
#	jmp	PutLong

PutLong:
	push	rax
	shr	eax, 16
	call	PutWord
	pop	rax
#	jmp	PutWord

PutWord:
	push	rax
	mov	al, ah
	call	PutByte
	pop	rax
#	jmp	PutByte

PutByte:
	push	rax
	shr	al, 4
	call	PutNibble
	pop	rax
#	jmp	PutNibble

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

PutNewLine:
	mov	al, 13
	call	PutChar
	mov	al, 10
#	jmp	PutChar

PutChar:
	_PutChar	al
	ret

WaitForKey:
	_WaitForKey
	ret

Exit:
	_Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if TARGET_WIN64
	.ascii	" Windows (64-bit)"
.endif

	.byte	0

text_cs_rip:
	.asciz	"CS:RIP="

text_ss_rsp:
	.asciz	"SS:RSP="

text_message:
	.asciz	"message="

	_SysVars

