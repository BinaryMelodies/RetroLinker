
.include "../include/common.inc"

	.section	.text

	.global	PutLong
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

	.global	PutWord
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

	.global	PutByte
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

	.global	PutNibble
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

	.global	PutString
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

	.global	PutNewLine
PutNewLine:
	li	r3, 10
#	b	PutChar

	.global	PutChar
PutChar:
	_PutChar	r0
	blr

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	blr

	.global	Exit
Exit:
	_Exit

	.section	.bss
	_SysVars

	.comm	common1, 5
	.comm	common2, 4

