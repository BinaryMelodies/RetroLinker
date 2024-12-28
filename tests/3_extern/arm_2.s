
.include "../include/common.inc"

	.section	.text

	.global	PutLong
PutLong:
	push	{r0, lr}
	lsr	r0, #16
	bl	PutWord
	pop	{r0, lr}
#	b	PutWord

	.global	PutWord
PutWord:
	push	{r0, lr}
	lsr	r0, #8
	bl	PutByte
	pop	{r0, lr}
#	b	PutByte

	.global	PutByte
PutByte:
	push	{r0, lr}
	lsr	r0, #4
	bl	PutNibble
	pop	{r0, lr}
#	b	PutNibble

	.global	PutNibble
PutNibble:
	and	r0, r0, #0xF
	cmp	r0, #10
	addcc	r0, r0, #'0'
	addcs	r0, r0, #'A' - 10
	b	PutChar

	.global	PutString
PutString:
1:
	ldrb	r0, [r1], #1
	movs	r0, r0
	moveq	pc, lr
	push	{r1, lr}
	bl	PutChar
	pop	{r1, lr}
	b	1b

	.global	PutNewLine
PutNewLine:
	mov	r0, #13
	push	{lr}
	bl	PutChar
	pop	{lr}
	mov	r0, #10
#	b	PutChar

	.global	PutChar
PutChar:
	_PutChar	r0
	mov	pc, lr

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	mov	pc, lr

	.global	Exit
Exit:
	_Exit

	.section	.bss
	_SysVars

	.comm	common1, 5
	.comm	common2, 4

