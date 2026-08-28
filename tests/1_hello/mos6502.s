
	.text

#ifdef	TARGET_APPLE2
	tmp_address=$40
#else
	tmp_address=$20
#endif

	.as
	.xs

start:
#ifdef	TARGET_C64
	tsx
	stx	ExitStack
#endif
#ifdef	TARGET_APPLE2
#ifdef	FORMAT_BIN
	tsx
	stx	ExitStack
#endif
#endif

	lda	#>Message
	sta	<tmp_address+1
	lda	#<Message
	sta	<tmp_address

	ldy	#0
Loop:
	lda	(tmp_address),y
	cmp	#0
	beq	PrintDone

	sty	<tmp_address+2
	jsr	PutChar
	ldy	<tmp_address+2
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
#ifdef	TARGET_APPLE2
	.aasc	" APPLE ]["
#endif
	.byte	0

#ifdef	TARGET_ATARI400
#include "../include/atari400.asm"
#endif

#ifdef	TARGET_C64
#include "../include/c64.asm"
#endif

#ifdef	TARGET_APPLE2
#include "../include/apple2.asm"
#endif

