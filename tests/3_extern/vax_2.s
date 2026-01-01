
.include "../include/common.inc"

	.text

	.global	PutLong
PutLong:
	pushl	%r0
	ashl	$-16, %r0, %r0
	jsb	PutWord
	movl	(%sp)+, %r0
	#brb	PutWord

	.global	PutWord
PutWord:
	pushl	%r0
	ashl	$-8, %r0, %r0
	jsb	PutByte
	movl	(%sp)+, %r0
	#brb	PutByte

	.global	PutByte
PutByte:
	pushl	%r0
	ashl	$-4, %r0, %r0
	jsb	PutNibble
	movl	(%sp)+, %r0
	#brb	PutNibble

	.global	PutNibble
PutNibble:
	bicb2	$~0xF, %r0
	cmpb	$10, %r0
	blequ	1f
	addb2	$'0', %r0
	brb	2f
1:
	addb2	$'A' - 10, %r0
2:
	brb	PutChar

	.global	PutString
PutString:
	movb	(%r1)+, %r0
	tstb	%r0
	beql	1f
	pushl	%r1
	jsb	PutChar
	movl	(%sp)+, %r1
	brb	PutString
1:
	rsb

	.global	PutNewLine
PutNewLine:
	movb	$10, %r0

	.global	PutChar
PutChar:
	_PutChar	%r0
	rsb

	.global	WaitForKey
WaitForKey:
	_WaitForKey
	rsb

	.global	Exit
Exit:
	_Exit

	.comm	common1, 5
	.comm	common2, 4

	_SysVars

