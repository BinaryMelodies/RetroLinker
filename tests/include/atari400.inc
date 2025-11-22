
.macro	_StartUp
.endm

.macro	_PutChar
	ldy	OutputPointer
	cmp	#10
	beq	FlushBuffer

	sta	OutputBuffer,y
	iny
	tya
	cmp	#40
	beq	FlushBuffer
	sty	OutputPointer
.endm

.macro	_Exit
	# TODO
.endm

.macro	_PutNewLine	char
	ldy	OutputPointer

FlushBuffer:
	# Y must contain buffer length

	# CIO 0
	ldx	#0
	# command
	lda	#9
	sta	0x0342
	lda	#<OutputBuffer
	sta	0x0344
	lda	#>OutputBuffer
	sta	0x0345
	# length
	tya
	sta	0x0348
	lda	#0
	sta	0x0349
	# CIO vector
	jsr	0xE456

	ldy	#0
	sty	OutputPointer
.endm

.macro	_WaitForKey
	# CIO 0
	ldx	#0
	# command
	lda	#5
	sta	0x0342
	lda	#<InputBuffer
	sta	0x0344
	lda	#>InputBuffer
	sta	0x0345
	# length
	lda	#1
	sta	0x0348
	lda	#0
	sta	0x0349
	# CIO vector
	jsr	0xE456
.endm

.macro	_SysVars
	.data
InputBuffer:
	.byte	0

OutputBuffer:
	.fill	40, 1, 0
OutputPointer:
	.byte	0
.endm

