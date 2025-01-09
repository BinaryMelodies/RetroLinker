
.include	"../include/common.inc"

	.text

	.longa	off
	.longi	off

start:
	_StartUp

	lda	#>Message
	sta	<0x21
	lda	#<Message
	sta	<0x20

	ldy	#0
Loop:
	lda	(0x20),y
$$FIX$C9$1:
	# a bug in w65-wdc-none deletes the next byte
	cmp	#0
	beq	PrintDone

	sty	<0x22
	jsr	PutChar
	ldy	<0x22
	iny
	jmp	Loop

PrintDone:
	jsr	PutNewLine
	jsr	WaitForKey
	jsr	WaitForKey

End:
	jsr	Exit
	jmp	End

PutChar:
	_PutChar
	rts

PutNewLine:
	_PutNewLine
	rts

WaitForKey:
	_WaitForKey
	rts

Exit:
	_Exit
	rts

	.data

Message:
	.ascii	"GREETINGS FROM BINUTILS!"
.if	TARGET_ATARI400
	.ascii	" ATARI 400/800"
.elseif	TARGET_C64
	.ascii	" COMMODORE 64"
.endif
	.byte	0

	_SysVars

