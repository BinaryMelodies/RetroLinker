
	.text

;#define _StartUp

PutChar:
	ldy	OutputPointer
	cmp	#10
	beq	FlushBuffer

	sta	OutputBuffer,y
	iny
	tya
	cmp	#40
	beq	FlushBuffer
	sty	OutputPointer

	rts

PutNewLine:
	ldy	OutputPointer

FlushBuffer:
	; Y must contain buffer length

	; CIO 0
	ldx	#0
	; command
	lda	#9
	sta	$0342
	lda	#<OutputBuffer
	sta	$0344
	lda	#>OutputBuffer
	sta	$0345
	; length
	tya
	sta	$0348
	lda	#0
	sta	$0349
	; CIO vector
	jsr	$E456

	ldy	#0
	sty	OutputPointer
	rts

WaitForKey:
	; CIO 0
	ldx	#0
	; command
	lda	#5
	sta	$0342
	lda	#<InputBuffer
	sta	$0344
	lda	#>InputBuffer
	sta	$0345
	; length
	lda	#1
	sta	$0348
	lda	#0
	sta	$0349
	; CIO vector
	jmp	$E456

Exit:
	jmp	Exit

	.data

InputBuffer:
	.byte	0

OutputBuffer:
	.dsb	40, 0
OutputPointer:
	.byte	0

