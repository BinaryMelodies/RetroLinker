
.include	"../include/common.inc"

	.section	.text

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

	.section	.bss

	_SysVars

	.comm	common1, 5
	.comm	common2, 4

