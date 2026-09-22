
	.text

StartUp:
	brk
	.byt	$C8
	.word	OpenBlock
	lda	ConsoleRef
	sta	WriteRef
	sta	ReadRef
	rts

OpenBlock:
	.byt	4
	.word	Console
ConsoleRef:
	.byte	0
	.word	OpenOptionList
	.byte	1

OpenOptionList:
	.word	3

Console:
	.byte	8
	.aasc	".CONSOLE"

PutNewLine:
	lda	#$0D
	jsr	PutChar
	lda	#$0A

PutChar:
	sta	WriteBuffer

	brk
	.byt	$CB
	.word	WriteBlock
	rts

WriteBlock:
	.byt	3
WriteRef:
	.byt	0
	.word	WriteBuffer
	.word	1

WriteBuffer:
	.byt	0

WaitForKey:
	brk
	.byt	$CA
	.word	ReadBlock

	lda	ReadCount
	beq	WaitForKey
	rts

ReadBlock:
	.byt	4
ReadRef:
	.byt	0
	.word	ReadBuffer
; requested byte count
	.word	1
ReadCount:
	.word	0

ReadBuffer:
	.byt	0

Exit:
	brk
	.byt	$CC
	.word	CloseAllBlock

ExitBlock:
	brk
	.byt	$65
; Exit block must contain a single 0 byte
	.word	ExitBlock

CloseAllBlock:
	.byte	1
; 0=close all
	.byte	0

