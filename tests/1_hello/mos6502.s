
	.text

	.as
	.xs

start:
#ifdef	TARGET_C64
	tsx
	stx	ExitStack
#endif

	lda	#>Message
	sta	<$21
	lda	#<Message
	sta	<$20

	ldy	#0
Loop:
	lda	($20),y
	cmp	#0
	beq	PrintDone

	sty	<$22
	jsr	PutChar
	ldy	<$22
	iny
	jmp	Loop

PrintDone:
	jsr	PutNewLine
	jsr	WaitForKey
	jsr	WaitForKey

End:
	jsr	Exit

	.data

Message:
	.aasc	"GREETINGS!"
#ifdef	TARGET_ATARI400
	.aasc	" ATARI 400/800"
#endif
#ifdef	TARGET_C64
	.aasc	" COMMODORE 64"
#endif
	.byte	0

#ifdef	TARGET_ATARI400
#include "../include/atari400.asm"
#endif

#ifdef	TARGET_C64
#include "../include/c64.asm"
#endif

