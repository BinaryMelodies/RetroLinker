
.include "../include/common.inc"

	.text

start:
	_StartUp

	mov	$message, r5
	jsr	pc, PutString
	jsr	pc, PutNewLine

	mov	$text_extval, r5
	jsr	pc, PutString
	mov	$extval, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_extref, r5
	jsr	pc, PutString
	mov	$extref, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_extref2, r5
	jsr	pc, PutString
	mov	$extref2, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_common1, r5
	jsr	pc, PutString
	mov	$common1, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	mov	$text_common2, r5
	jsr	pc, PutString
	mov	$common2, r0
	jsr	pc, PutWord
	jsr	pc, PutNewLine

	jsr	pc, WaitForKey
	jsr	pc, Exit

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
.if FORMAT_OMAGIC || FORMAT_V1 || FORMAT_V2
	.ascii	" impure executable"
.elseif FORMAT_NMAGIC
	.ascii	" pure executable"
.elseif FORMAT_IMAGIC
	.ascii	" executable with separate spaces"
.endif
.endif
	.byte	0

text_extval:
	.asciz	"extval="
text_extref:
	.asciz	"extref="
text_extref2:
	.asciz	"extref2="
text_common1:
	.asciz	"common1="
text_common2:
	.asciz	"common2="

	.comm	common1, 4

