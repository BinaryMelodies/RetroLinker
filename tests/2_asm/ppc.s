
.include "../include/common.inc"

	.section	.text

	.global	start
start:
	_StartUp
.if OPTION_EXPLICIT_STACK
	# TODO
.endif

	bl	1f
1:
	mflr	3
	la	3, start - 1b(3)
	lis	4, stored_pc@ha
	stw	3, stored_pc@l(4)

	lis	4, message@ha
	la	4, message@l(4)
	bl	PutString
	bl	PutNewLine

	lis	4, text_pc@ha
	la	4, text_pc@l(4)
	bl	PutString
	lis	4, stored_pc@ha
	lwz	3, stored_pc@l(4)
	bl	PutLong
	bl	PutNewLine

	lis	4, text_sp@ha
	la	4, text_sp@l(4)
	bl	PutString
	mr	3, 1
	bl	PutLong
	bl	PutNewLine

	bl	WaitForKey
	bl	Exit

PutLong:
	mflr	0
	stwu	0, -4(1)
	stwu	3, -4(1)
	srawi	3, 3, 16
	bl	PutWord
	lwz	3, 0(1)
	lwz	0, 4(1)
	mtlr	0
	la	1, 8(1)
#	b	PutWord

PutWord:
	mflr	0
	stwu	0, -4(1)
	stwu	3, -4(1)
	srawi	3, 3, 8
	bl	PutByte
	lwz	3, 0(1)
	lwz	0, 4(1)
	mtlr	0
	la	1, 8(1)
#	b	PutByte

PutByte:
	mflr	0
	stwu	0, -4(1)
	stwu	3, -4(1)
	srawi	3, 3, 4
	bl	PutNibble
	lwz	3, 0(1)
	lwz	0, 4(1)
	mtlr	0
	la	1, 8(1)
#	b	PutNibble

PutNibble:
	andi.	3, 3, 0xF
	cmpi	cr0, 0, 3, 10
	bge	1f
	addi	3, 3, '0'
	b	2f
1:
	addi	3, 3, 'A' - 10
2:
	b	PutChar

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

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if TARGET_LINUX
	.ascii	" Linux (32-bit PowerPC)"
.endif

	.byte	0

text_pc:
	.asciz	"PC="

text_sp:
	.asciz	"SP="

	.section	.bss

stored_pc:
	.skip	4

stored_sp:
	.skip	4

	_SysVars

.if OPTION_EXPLICIT_STACK
stack:
	.fill	0x100
stack_top:
.endif

