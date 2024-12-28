
.include "../include/common.inc"

	.text

_start:
	_StartUp
	mov	$message, r5
	jsr	pc, PutString
	jsr	pc, PutNewLine
	jsr	pc, WaitForKey
	jsr	pc, Exit

PutString:
1:
	movb	(r5)+, r0
	beq	1f
	jsr	pc, PutChar
	br	1b
1:
	rts	pc

PutNewLine:
	mov	$13, r0
	jsr	pc, PutChar
	mov	$10, r0

PutChar:
	_PutChar
	rts	pc

WaitForKey:
	_WaitForKey
	rts	pc

Exit:
	_Exit

	.data

message:
	.ascii	"Greetings!"
	.ascii	" DX-DOS"
	.byte	0

	.bss

#	_SysVars

