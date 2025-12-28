
.include "../include/common.inc"

	.section	.text

	.global	PutLong
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

	.global	PutWord
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

	.global	PutByte
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

	.global	PutNibble
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

	.global	PutString
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

	.global	PutNewLine
PutNewLine:
	li	3, 10
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

