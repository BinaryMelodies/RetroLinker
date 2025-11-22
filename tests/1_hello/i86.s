
.include "../include/common.inc"

	.section	.text
	.code16

start:
	_StartUp
	mov	si, offset message
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit

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
.if TARGET_DOS16M
	.ascii	" MS-DOS (DOS/16M)"
.elseif TARGET_MSDOS
	.ascii	" MS-DOS"
.if FORMAT_COM
	.ascii	" flat binary .com file"
.elseif FORMAT_MZ
	.ascii	" \"MZ\" .exe file"
.elseif FORMAT_NE
	.ascii	" \"NE\" .exe file"
.endif
.elseif TARGET_CPM86
	.ascii	" CP/M-86"
.if FORMAT_CMD_TINY
	.ascii	" 8080 model .cmd file"
.elseif FORMAT_CMD_SMALL
	.ascii	" small model .cmd file"
.endif
.elseif TARGET_ELKS
	.ascii	" ELKS"
.if FORMAT_AOUT_COM
	.ascii	" combined file"
.elseif FORMAT_AOUT_SEP
	.ascii	" split file"
.endif
.elseif TARGET_WIN16
	.ascii	" Windows (16-bit)"
.elseif TARGET_OS2V1
	.ascii	" OS/2 1.0 (16-bit)"
.endif

	.byte	0

#.if FORMAT_BW
## we need at least 1 relocation
#	.word	$$SEG$.data
#.endif

	.section	.bss

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	0x100
.endif

