
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

	.data

error:
	.asciz	"Error"
	.align	4

message:
	.ascii	"Greetings!"
	.ascii	" DX-DOS"
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

