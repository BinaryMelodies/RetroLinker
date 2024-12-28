
.include	"../include/common.inc"

	.section	.text

	.global	start
start:
	_StartUp

	LDA	r3, rr2, message
	calr	PutString
	calr	PutNewLine

	LDA	r3, rr2, text_extval
	calr	PutString
.if FORMAT_Z8K_SEGMENTED
	ldl	rr0, #extval
	calr	PutLong
.else
	ld	r0, #extval
	calr	PutWord
.endif
	calr	PutNewLine

	LDA	r3, rr2, text_extref
	calr	PutString
.if FORMAT_Z8K_SEGMENTED
	ldl	rr0, #extref
	calr	PutLong
.else
	ld	r0, #extref
	calr	PutWord
.endif
	calr	PutNewLine

	LDA	r3, rr2, text_common1
	calr	PutString
.if FORMAT_Z8K_SEGMENTED
	ldl	rr0, #common1
	calr	PutLong
.else
	ld	r0, #common1
	calr	PutWord
.endif
	calr	PutNewLine

	LDA	r3, rr2, text_common2
	calr	PutString
.if FORMAT_Z8K_SEGMENTED
	ldl	rr0, #common2
	calr	PutLong
.else
	ld	r0, #common2
	calr	PutWord
.endif
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

.if FORMAT_Z8K_SEGMENTED
PutLong:
	push	@sp, r1
	calr	PutWord
	pop	r0, @sp
.endif

PutWord:
	push	@sp, r0
	ldb	rl0, rh0
	calr	PutByte
	pop	r0, @sp

PutByte:
	push	@sp, r0
	rr	r0, #2
	rr	r0, #2
	calr	PutNibble
	pop	r0, @sp

PutNibble:
	andb	rl0, #0xF
	cpb	rl0, #10
	jr	nc, 1f
	addb	rl0, #'0'
	jr	2f
1:
	addb	rl0, #'A'-10
2:

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

text_extval:
	.asciz	"extval="
text_extref:
	.asciz	"extref="
text_extref2:
	.asciz	"extref2="
text_common1:
	.asciz	"common1="
text_common2:
	.asciz	"common2="

	.comm	common1, 4

