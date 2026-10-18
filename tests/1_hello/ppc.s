
.include "../include/common.inc"

	.section	.text

.if	TARGET_MACOS
	.section	.data
	.global	start
start:
	.long	.start, TOC, 0
	.section	.text
.start:
.else
start:
.endif
	_StartUp
.if	OPTION_EXPLICIT_STACK
	# TODO
.endif

.if	!TARGET_MACOS
	lis	r4, message@ha
	la	r4, message@l(r4)
.else
	.section	.toc
message_TOC:
	.long	message
	.section	.text
	lwz	r4, message_TOC - .toc(r2)
.endif
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
.if	OPTION_NEWLINE_PROC
	mflr	r0
	stwu	r0, -4(r1)
	_PutNewLine
	lwz	r0, 0(r1)
	la	r1, 4(r1)
	mtlr	r0
	blr
.else
	li	r3, 10
#	b	PutChar
.endif

PutChar:
.if	TARGET_MACOS
	mflr	r0
	stwu	r0, -4(r1)
.endif
	_PutChar	r3
.if	TARGET_MACOS
	lwz	r0, 0(r1)
	la	r1, 4(r1)
	mtlr	r0
.endif
	blr

WaitForKey:
.if	TARGET_MACOS
	mflr	r0
	stwu	r0, -4(r1)
.endif
	_WaitForKey
.if	TARGET_MACOS
	lwz	r0, 0(r1)
	la	r1, 4(r1)
	mtlr	r0
.endif
	blr

Exit:
	_Exit

	.section	.data

message:
	.ascii	"Greetings!"
.if TARGET_LINUX
	.ascii	" Linux (32-bit PowerPC)"
.elseif	TARGET_MACOS
.if	!VERSION_CARBON
	.ascii	" Mac OS (PowerPC)"
.else
	.ascii	" Mac OS (Carbon, PowerPC)"
.endif	# VERSION_CARBON
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

