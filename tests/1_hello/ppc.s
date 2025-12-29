
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if	OPTION_EXPLICIT_STACK
	# TODO
.endif

	lis	r4, message@ha
	la	r4, message@l(r4)
	bl	PutString
	bl	PutNewLine
	bl	WaitForKey
	bl	Exit

PutString:
	mflr	r0
	stwu	r0, -4(r1)
1:
	lbz	r3, 0(r4)
	cmpi	cr0, 0, r3, 0
	beq	2f
	la	r4, 1(r4)
	stwu	r4, -4(r1)
	bl	PutChar
	lwz	r4, 0(r1)
	la	r1, 4(r1)
	b	1b
2:
	lwz	r0, 0(r1)
	la	r1, 4(r1)
	mtlr	r0
	blr

PutNewLine:
	li	r3, 10
#	b	PutChar

PutChar:
	_PutChar	r3
	blr

WaitForKey:
	_WaitForKey
	blr

Exit:
	_Exit

	.section	.data

message:
	.ascii	"Greetings!"
.if TARGET_LINUX
	.ascii	" Linux (32-bit PowerPC)"
.endif

	.byte	0

	.align	4

	.section	.bss

	_SysVars

.if OPTION_EXPLICIT_STACK
stack:
	.fill	0x100
stack_top:
.endif

