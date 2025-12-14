
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

	.section	.text
	.code16

	.set	WinInitialize, $$IMPORT$PMWIN$00F6
	.set	$$SEGOF$WinInitialize, $$IMPSEG$PMWIN$00F6

	.set	WinCreateMsgQueue, $$IMPORT$PMWIN$003A
	.set	$$SEGOF$WinCreateMsgQueue, $$IMPSEG$PMWIN$003A

	.set	WinMessageBox, $$IMPORT$PMWIN$008B
	.set	$$SEGOF$WinMessageBox, $$IMPSEG$PMWIN$008B

	.set	WinDestroyMsgQueue, $$IMPORT$PMWIN$003B
	.set	$$SEGOF$WinDestroyMsgQueue, $$IMPSEG$PMWIN$003B

	.set	WinTerminate, $$IMPORT$PMWIN$00F7
	.set	$$SEGOF$WinTerminate, $$IMPSEG$PMWIN$00F7

	.set	DosExit, $$IMPORT$DOSCALLS$0005
	.set	$$SEGOF$DosExit, $$IMPSEG$DOSCALLS$0005

	.global	_start
_start:
	push	0
	CALLFAR	$$SEGOF$WinInitialize, WinInitialize
	mov	[hab], ax
	mov	[hab + 2], dx

	push	0
	push	0
	push	0
	CALLFAR	$$SEGOF$WinCreateMsgQueue, WinCreateMsgQueue

	push	0
	push	1
	push	0
	push	1
	push	offset $$SEGOF$dialog_message
	pushw	offset dialog_message
	pushw	offset $$SEGOF$dialog_title
	pushw	offset dialog_title
	push	0
	push	0
	CALLFAR	$$SEGOF$WinMessageBox, WinMessageBox

	push	[hmq + 2]
	push	[hmq]
	CALLFAR	$$SEGOF$WinDestroyMsgQueue, WinDestroyMsgQueue

	push	[hab + 2]
	push	[hab]
	CALLFAR	$$SEGOF$WinTerminate, WinTerminate

	push	1
	push	0
	CALLFAR	$$SEGOF$DosExit, DosExit

	.section	.data

dialog_title:
	.asciz	"Sample Application"

dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" OS/2 (16-bit)"
	.byte	0

	.section	.bss

hab:
	.skip	4
hmq:
	.skip	4

