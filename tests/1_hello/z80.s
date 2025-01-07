
#.include	"../include/common.inc"
.include	"../include/z80.inc"

	.section	.text

start:
	_StartUp
	ld	hl, message
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit

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

	.section	.bss

	_SysVars

