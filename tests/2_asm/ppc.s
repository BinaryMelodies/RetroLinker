
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
	mflr	r3
	la	r3, start - 1b(r3)
	lis	r4, stored_pc@ha
	stw	r3, stored_pc@l(r4)

	lis	r4, message@ha
	la	r4, message@l(r4)
	bl	PutString
	bl	PutNewLine

	lis	r4, text_pc@ha
	la	r4, text_pc@l(r4)
	bl	PutString
	lis	r4, stored_pc@ha
	lwz	r3, stored_pc@l(r4)
	bl	PutLong
	bl	PutNewLine

	lis	r4, text_sp@ha
	la	r4, text_sp@l(r4)
	bl	PutString
	mr	r3, r1
	bl	PutLong
	bl	PutNewLine

	bl	WaitForKey
	bl	Exit

PutLong:
	mflr	r0
	stwu	r0, -4(r1)
	stwu	r3, -4(r1)
	srawi	r3, r3, 16
	bl	PutWord
	lwz	r3, 0(r1)
	lwz	r0, 4(r1)
	mtlr	r0
	la	r1, 8(r1)
#	b	PutWord

PutWord:
	mflr	r0
	stwu	r0, -4(r1)
	stwu	r3, -4(r1)
	srawi	r3, r3, 8
	bl	PutByte
	lwz	r3, 0(r1)
	lwz	r0, 4(r1)
	mtlr	r0
	la	r1, 8(r1)
#	b	PutByte

PutByte:
	mflr	r0
	stwu	r0, -4(r1)
	stwu	r3, -4(r1)
	srawi	r3, r3, 4
	bl	PutNibble
	lwz	r3, 0(r1)
	lwz	r0, 4(r1)
	mtlr	r0
	la	r1, 8(r1)
#	b	PutNibble

PutNibble:
	andi.	r3, r3, 0xF
	cmpi	cr0, 0, r3, 10
	bge	1f
	addi	r3, r3, '0'
	b	2f
1:
	addi	r3, r3, 'A' - 10
2:
	b	PutChar

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

