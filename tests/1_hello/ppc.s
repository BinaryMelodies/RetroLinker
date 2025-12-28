
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if	OPTION_EXPLICIT_STACK
	# TODO
.endif

	lis	4, message@ha
	la	4, message@l(4)
	bl	PutString
	bl	PutNewLine
	bl	WaitForKey
	bl	Exit

PutString:
	mflr	0
	stwu	0, -4(1)
1:
	lbz	3, 0(4)
	cmpi	cr0, 0, 3, 0
	beq	2f
	la	4, 1(4)
	stwu	4, -4(1)
	bl	PutChar
	lwz	4, 0(1)
	la	1, 4(1)
	b	1b
2:
	lwz	0, 0(1)
	la	1, 4(1)
	mtlr	0
	blr

PutNewLine:
	li	3, 10
#	b	PutChar

PutChar:
	_PutChar	3
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

