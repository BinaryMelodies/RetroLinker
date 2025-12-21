
.include "../include/common.inc"

	.section	.text
	.code64

start:
	_StartUp
	lea	rsi, [rip + offset message]
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit

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

message:
	.ascii	"Greetings!"
.if TARGET_DX64
	.ascii	" MS-DOS (DX64"
.if FORMAT_ADAM33
	.ascii	" Adam format"
.elseif FORMAT_LV
	.ascii	" LV format"
.endif
	.ascii	" .exe 64-bit)"
.elseif TARGET_WIN64
	.ascii	" Windows (64-bit)"
.elseif TARGET_LINUX
	.ascii	" Linux (64-bit x86)"
.endif

	.byte	0

	.section	.bss

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

