
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

.if TARGET_WIN64
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 0x20
	mov	ecx, 0
	lea	rdx, [rip + dialog_message]
	lea	r8, [rip + dialog_title]
	mov	r9d, 0
.set __imp__MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.extern	__imp__MessageBox
	call	[rip + __imp__MessageBox]
	mov	rsp, rbp
	pop	rbp
.endif

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

.if TARGET_WIN64
dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.asciz	"Graphical Greetings!"
.endif

	_SysVars

