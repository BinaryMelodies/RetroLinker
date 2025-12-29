
.include "../include/common.inc"

	.text

	.global	start
start:
	jsr	pc, 1f
1:
	mov	(sp)+, r0
	sub	$1b - start, r0
	mov	r0, stored_pc

	mov	sp, stored_sp

	_StartUp

	mov	$message, r5
	jsr	pc, PutString
	jsr	pc, PutNewLine

	mov	$text_pc, r5
	jsr	pc, PutString
	mov	stored_pc, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_sp, r5
	jsr	pc, PutString
	mov	sp, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_message, r5
	jsr	pc, PutString
	mov	$message, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	jsr	pc, WaitForKey
	jsr	pc, Exit

PutWord:
	mov	r0, -(sp)
	ash	$-8, r0
	jsr	pc, PutByte
	mov	(sp)+, r0
#	jmp	PutByte

PutByte:
	mov	r0, -(sp)
	ash	$-4, r0
	jsr	pc, PutNibble
	mov	(sp)+, r0
#	jmp	PutNibble

PutNibble:
	bic	$0xFFF0, r0
	cmp	r0, $10
	bge	1f
	add	$'0', r0
	br	2f
1:
	add	$'A' - 10, r0
2:
	jmp	PutChar

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

error:
	.asciz	"Error"
	.align	4

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

text_pc:
	.asciz	"PC="

text_sp:
	.asciz	"SP="

text_message:
	.asciz	"message="

	.bss

stored_pc:
	.skip	2

stored_sp:
	.skip	2

