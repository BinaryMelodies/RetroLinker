
.include "../include/common.inc"

	.section	.text

start:
	_StartUp

	ld	hl, message
	call	PutString
	call	PutNewLine

	ld	hl, text_extval
	call	PutString
	ld	hl, extval
	call	PutWord
	call	PutNewLine

	ld	hl, text_extref
	call	PutString
	ld	hl, extref
	call	PutWord
	call	PutNewLine

	ld	hl, text_extref2
	call	PutString
	ld	hl, extref2
	call	PutWord
	call	PutNewLine

	ld	hl, text_common1
	call	PutString
	ld	hl, common1
	call	PutWord
	call	PutNewLine

	ld	hl, text_common2
	call	PutString
	ld	hl, common2
	call	PutWord
	call	PutNewLine

	call	WaitForKey
	call	Exit

	.section	.data

error:
	.asciz	"Error"
	.align	4

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

