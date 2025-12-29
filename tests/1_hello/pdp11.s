
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
.if TARGET_DXDOS
	mov	$13, r0
	jsr	pc, PutChar
.endif
	mov	$10, r0

PutChar:
	_PutChar	r0
	rts	pc

WaitForKey:
	_WaitForKey
	rts	pc

Exit:
	_Exit

	_SysVars

	.data

message:
	.ascii	"Greetings!"
.if TARGET_DXDOS
	.ascii	" DX-DOS"
.elseif TARGET_UNIX || TARGET_BSD
.if TARGET_UNIX
	.ascii	" AT&T UNIX"
.if FORMAT_V1
	.ascii	" Version 1"
.elseif FORMAT_V2
	.ascii	" Version 2"
.else
	.ascii	" Version 7"
.endif
.elseif TARGET_BSD
	.ascii	" 2.11BSD"
.endif
.if FORMAT_RAW
	.ascii	" raw binary"
.elseif FORMAT_OMAGIC || FORMAT_V1 || FORMAT_V2
	.ascii	" impure executable"
.elseif FORMAT_NMAGIC
	.ascii	" pure executable"
.elseif FORMAT_IMAGIC
	.ascii	" executable with separate spaces"
.endif
.endif
	.byte	0

	.bss

