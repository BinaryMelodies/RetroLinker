
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if OPTION_EXPLICIT_STACK
	# TODO
.endif

	lis	4, message@ha
	la	4, message@l(4)
	bl	PutString
	bl	PutNewLine

	lis	4, text_extval@ha
	la	4, text_extval@l(4)
	bl	PutString
	lis	3, extval@ha
	la	3, extval@l(3)
	bl	PutLong
	bl	PutNewLine

	lis	4, text_extref@ha
	la	4, text_extref@l(4)
	bl	PutString
	lis	3, extref@ha
	la	3, extref@l(3)
	bl	PutLong
	bl	PutNewLine

	lis	4, text_extref2@ha
	la	4, text_extref2@l(4)
	bl	PutString
	lis	3, extref2@ha
	la	3, extref2@l(3)
	bl	PutLong
	bl	PutNewLine

	lis	4, text_common1@ha
	la	4, text_common1@l(4)
	bl	PutString
	lis	3, common1@ha
	la	3, common1@l(3)
	bl	PutLong
	bl	PutNewLine

	lis	4, text_common2@ha
	la	4, text_common2@l(4)
	bl	PutString
	lis	3, common2@ha
	la	3, common2@l(3)
	bl	PutLong
	bl	PutNewLine

	bl	WaitForKey
	bl	Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if TARGET_LINUX
	.ascii	" Linux (32-bit PowerPC)"
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
	.section	.bss
stack:
	.fill	0x100
stack_top:
.endif

