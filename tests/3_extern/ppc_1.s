
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if OPTION_EXPLICIT_STACK
	# TODO
.endif

	lis	r4, message@ha
	la	r4, message@l(r4)
	bl	PutString
	bl	PutNewLine

	lis	r4, text_extval@ha
	la	r4, text_extval@l(r4)
	bl	PutString
	lis	r3, extval@ha
	la	r3, extval@l(r3)
	bl	PutLong
	bl	PutNewLine

	lis	r4, text_extref@ha
	la	r4, text_extref@l(r4)
	bl	PutString
	lis	r3, extref@ha
	la	r3, extref@l(r3)
	bl	PutLong
	bl	PutNewLine

	lis	r4, text_extref2@ha
	la	r4, text_extref2@l(r4)
	bl	PutString
	lis	r3, extref2@ha
	la	r3, extref2@l(r3)
	bl	PutLong
	bl	PutNewLine

	lis	r4, text_common1@ha
	la	r4, text_common1@l(r4)
	bl	PutString
	lis	r3, common1@ha
	la	r3, common1@l(r3)
	bl	PutLong
	bl	PutNewLine

	lis	r4, text_common2@ha
	la	r4, text_common2@l(r4)
	bl	PutString
	lis	r3, common2@ha
	la	r3, common2@l(r3)
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

