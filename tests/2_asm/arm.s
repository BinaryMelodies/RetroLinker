
.include "../include/common.inc"

	.section	.text

	.global	start
start:
	_StartUp
	ldr	r1, =stored_sp
	str	sp, [r1]
.if OPTION_EXPLICIT_STACK
	ldr	sp, =stack_top
.endif

	bl	1f
1:
	sub	r0, lr, #1b - start
	ldr	r1, =stored_pc
	str	r0, [r1]

	ldr	r1, =message
	bl	PutString
	bl	PutNewLine

	ldr	r1, =text_pc
	bl	PutString
	ldr	r1, =stored_pc
	ldr	r0, [r1]
	bl	PutLong
	bl	PutNewLine

	ldr	r1, =text_sp
	bl	PutString
	ldr	r1, =stored_sp
	ldr	r0, [r1]
	bl	PutLong
	bl	PutNewLine

	bl	WaitForKey
	bl	Exit

PutLong:
	push	{r0, lr}
	lsr	r0, #16
	bl	PutWord
	pop	{r0, lr}
#	b	PutWord

PutWord:
	push	{r0, lr}
	lsr	r0, #8
	bl	PutByte
	pop	{r0, lr}
#	b	PutByte

PutByte:
	push	{r0, lr}
	lsr	r0, #4
	bl	PutNibble
	pop	{r0, lr}
#	b	PutNibble

PutNibble:
	and	r0, r0, #0xF
	cmp	r0, #10
	addcc	r0, r0, #'0'
	addcs	r0, r0, #'A' - 10
	b	PutChar

PutString:
1:
	ldrb	r0, [r1], #1
	movs	r0, r0
	moveq	pc, lr
	push	{r1, lr}
	bl	PutChar
	pop	{r1, lr}
	b	1b

PutNewLine:
	mov	r0, #13
	push	{lr}
	bl	PutChar
	pop	{lr}
	mov	r0, #10
#	b	PutChar

PutChar:
	_PutChar	r0
	mov	pc, lr

WaitForKey:
	_WaitForKey
	mov	pc, lr

Exit:
	_Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if	TARGET_RISCOS
	.ascii	" RISC OS executable file"
.elseif TARGET_LINUX
	.ascii	" Linux (32-bit ARM)"
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

