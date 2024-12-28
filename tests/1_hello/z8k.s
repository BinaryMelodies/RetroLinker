
.include	"../include/common.inc"

	.section	.text

start:
	_StartUp

	LDA	r3, rr2, message
	calr	PutString
	calr	PutNewLine
	calr	WaitForKey
	calr	Exit

PutString:
1:
.if FORMAT_Z8K_SEGMENTED
	ldb	rl0, @rr2
	addl	rr2, #1
.else
	ldb	rl0, @r3
	add	r3, #1
.endif
	testb	rl0
	jr	z, 1f
.if FORMAT_Z8K_SEGMENTED
	pushl	@sp, rr2
.else
	push	@sp, r3
.endif
	calr	PutChar
.if FORMAT_Z8K_SEGMENTED
	popl	rr2, @sp
.else
	pop	r3, @sp
.endif
	jr	1b
1:
	ret

PutNewLine:
	ld	r0, #13
	calr	PutChar
	ld	r0, #10
	jr	PutChar

PutChar:
	_PutChar	r0
	ret

WaitForKey:
	_WaitForKey
	ret

Exit:
	_Exit

	.section	.rodata

message:
	.ascii	"Greetings!"
.if TARGET_CPM8K
	.ascii	" CP/M-8000"
.if FORMAT_Z8K_SEGMENTED
	.ascii	" segmented .z8k file"
.elseif FORMAT_Z8K_NONSHARED
	.ascii	" non-shared .z8k file"
.elseif FORMAT_Z8K_SPLIT
	.ascii	" split .z8k file"
.endif
.endif
	.byte	0

