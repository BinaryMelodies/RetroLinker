
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if OPTION_EXPLICIT_STACK
	ldr	sp, =stack_top
.endif

	ldr	r1, =message
	bl	PutString
	bl	PutNewLine

	ldr	r1, =text_extval
	bl	PutString
	ldr	r0, =extval
	bl	PutLong
	bl	PutNewLine

	ldr	r1, =text_extref
	bl	PutString
	ldr	r0, =extref
	bl	PutLong
	bl	PutNewLine

	ldr	r1, =text_extref2
	bl	PutString
	ldr	r0, =extref2
	bl	PutLong
	bl	PutNewLine

	ldr	r1, =text_common1
	bl	PutString
	ldr	r0, =common1
	bl	PutLong
	bl	PutNewLine

	ldr	r1, =text_common2
	bl	PutString
	ldr	r0, =common2
	bl	PutLong
	bl	PutNewLine

	bl	WaitForKey
	bl	Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if	TARGET_RISCOS
	.ascii	" RISC OS"
.elseif TARGET_LINUX
	.ascii	" Linux (32-bit ARM)"
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

