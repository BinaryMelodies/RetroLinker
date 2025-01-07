
;#define _StartUp	tsx	:	stx	ExitStack

	.text

PutChar:
	jmp	$FFD2	; CHROUT

PutNewLine:
	lda	#13
	jmp	PutChar

WaitForKey:
	jsr	$FFE4	; GETIN
	cmp	#0
	beq	WaitForKey
	rts

Exit:
	ldx	ExitStack
	txs
	rts

	.data

ExitStack:
	.byte	0

