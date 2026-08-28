
	.text

PutChar:
	clc
	adc	#$80
	jmp	$FDF0

PutNewLine:
	jmp	$FC62

WaitForKey:
	lda	$C000
	bpl	WaitForKey
	sta	$C010
	rts

Exit:
#ifdef FORMAT_BIN
	ldx	ExitStack
	txs
	rts

ExitStack:
	.byte	0
#endif

#ifdef FORMAT_SYS
	jsr	$BF00
	.byte	$65
	.word	quit_parameters
quit_parameters:
	.byte	$04
	.byte	$00
	.word	$0000
	.byte	$00
	.word	$0000
#endif

