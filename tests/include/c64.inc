
.macro	_StartUp
	tsx
	stx	ExitStack
.endm

.macro	_Exit
	ldx	ExitStack
	txs
	rts
.endm

.macro	_PutChar
	jsr	0xFFD2	# CHROUT
.endm

.macro	_PutNewLine
	lda	#13
	jsr	PutChar
.endm

.macro	_WaitForKey
0:
	jsr	0xFFE4	# GETIN
	cmp	#0
	beq	0b
.endm

.macro	_SysVars
ExitStack:
	.byte	0
.endm

