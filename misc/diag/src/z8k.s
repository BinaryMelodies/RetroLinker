
.ifndef	SYSTEM_CPM8K
.equ	SYSTEM_CPM8K, 0
.endif

.ifndef	FORMAT_EE01
.equ	FORMAT_EE01, 0
.endif
.ifndef	FORMAT_EE03
.equ	FORMAT_EE03, 0
.endif
.ifndef	FORMAT_EE0B
.equ	FORMAT_EE0B, 0
.endif

.macro	INITIALIZE
# TODO
#	# store base page
#	movel	4(%sp), (addr_base_page)
.endm

.macro	PUT_CHAR	char
	clr	r7
	ldb	rl7, \char
	ld	r5, #0x0002
	sc	#0x02
.endm

.macro	PUT_STRING	address
.if	FORMAT_EE01
	# TODO: not working
	ld	r7, #\address
	ld	r6, r14
.else
	lda	r7, \address
	clr	r6
.endif
	ld	r5, #0x0009
	sc	#0x02
.endm

.macro	WAIT_KEY
	ld	r5, #0x0001
	sc	#0x02
.endm

.macro	EXIT
	clr	r5
	sc	#0x02
.endm

.macro	SUPER	stack
	# TODO
.endm

.equ	END_OF_STRING, '$'

	.text
	.z8002

	INITIALIZE
	PUT_STRING	text_greetings
	lda	r3, text_description
	call	putstring

	#ldl	rr0, #0x1A2B3C4D
	#call	putlong
	# TODO: extend

	WAIT_KEY
	EXIT

putstring:
	ldb	rl0, @r3
	add	r3, #1
	testb	rl0
	jr	z, 1f
	push	@r15, r3
	call	putchar
	pop	r3, @r15
	jr	putstring
1:
	ret

putnewline:
	ldb	rl0, 13
	call	putchar
	ldb	rl0, 10
	jp	putchar

putlong:
	push	@r15, r1
	call	putword
	pop	r0, @r15

putword:
	push	@r15, r0
	ldb	rl0, rh0
	call	putbyte
	pop	r0, @r15

putbyte:
	push	@r15, r0
	rr	r0, #2
	rr	r0, #2
	call	putnibble
	pop	r0, @r15

putnibble:
	andb	rl0, #0xF
	cpb	rl0, #10
	jp	nc, 1f
	addb	rl0, #'0'
	jp	2f
1:
	addb	rl0, #'A'-10
2:

putchar:
	PUT_CHAR	rl0
	ret

	.section	.rodata

text_error:
	.ascii	"Error: this message should not be displayed"
	.byte	13, 10, END_OF_STRING

text_greetings:
	.ascii	"Greetings!"
	.byte	13, 10, END_OF_STRING

text_description:
.if	SYSTEM_CPM8K
	.ascii	"Digital Research CP/M-8000"
.if	FORMAT_EE01
	.ascii	" segmented"
.elseif	FORMAT_EE03
	.ascii	" non-shared"
.elseif	FORMAT_EE0B
	.ascii	" split"
.endif
	.ascii	" .Z8K"
.endif
	.byte	13, 10, 0

	.align	4

	.section	.bss

	.align	4

	.section .stack, "bw"

	.skip 0x100
.stack_top:

