
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
.if	OPTION_EXPLICIT_STACK
	ldr	sp, =stack_top
.endif

	ldr	r1, =message
	bl	PutString
	bl	PutNewLine
	bl	WaitForKey
	bl	Exit

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

message:
	.ascii	"Greetings!"
.if	TARGET_RISCOS
	.ascii	" RISC OS executable file"
.elseif TARGET_LINUX
	.ascii	" Linux (32-bit ARM)"
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

