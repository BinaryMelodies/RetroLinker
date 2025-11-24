
.include "../include/common.inc"

	.section	.text
	.code16

start:
	_StartUp

	mov	si, offset message
	call	PutString
	call	PutNewLine

	mov	si, offset text_extval
	call	PutString
	mov	ax, offset extval
	call	PutWord
	call	PutNewLine

	mov	si, offset text_extref
	call	PutString
	mov	ax, offset $$SEGOF$extref
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	ax, offset extref
	call	PutWord
	call	PutNewLine

	mov	si, offset text_extref2
	call	PutString
	mov	ax, offset $$SEGOF$extref2
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	ax, offset extref2
	call	PutWord
	call	PutNewLine

	mov	si, offset text_common1
	call	PutString
	mov	ax, offset common1
	call	PutWord
	call	PutNewLine

	mov	si, offset text_common2
	call	PutString
	mov	ax, offset common2
	call	PutWord
	call	PutNewLine

	call	WaitForKey
	call	Exit

	.section	.data

error:
	.asciz	"Error"
	.align	16

message:
	.ascii	"Greetings!"
.if TARGET_DOS16M
	.ascii	" MS-DOS (DOS/16M)"
.elseif TARGET_MSDOS
	.ascii	" MS-DOS"
.elseif TARGET_CPM86
	.ascii	" CP/M-86"
.elseif TARGET_ELKS
	.ascii	" ELKS"
.elseif TARGET_WIN16
	.ascii	" Windows (16-bit)"
.elseif TARGET_OS2V1
	.ascii	" OS/2 1.0 (16-bit)"
.elseif TARGET_FLEXOS286
	.ascii	" FlexOS 286 (16-bit)"
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

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

