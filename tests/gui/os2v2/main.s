
	.section	.text
	.code32

	.set	WinInitialize, $$IMPORT$PMWIN$02FB
	.set	WinCreateMsgQueue, $$IMPORT$PMWIN$02CC
	.set	WinMessageBox, $$IMPORT$PMWIN$0315
	.set	WinDestroyMsgQueue, $$IMPORT$PMWIN$02D6
	.set	WinTerminate, $$IMPORT$PMWIN$0378

	.set	DosExit, $$IMPORT$DOSCALLS$00EA

	.global	_start
_start:
	push	0
	call	WinInitialize
	mov	[hab], eax
	add	esp, 4

	push	0
	push	0
	call	WinCreateMsgQueue
	mov	[hmq], eax
	add	esp, 8

	push	0
	push	0
	push	offset dialog_title
	push	offset dialog_message
	push	1
	push	1
	call	WinMessageBox
	add	esp, 24

	push	[hmq]
	call	WinDestroyMsgQueue
	add	esp, 4

	push	[hab]
	call	WinTerminate
	add	esp, 4

	push	0
	push	1
	call	DosExit

	.section	.data

dialog_title:
	.asciz	"Sample Application"

dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" OS/2 (32-bit)"
	.byte	0

	.section	.bss

hab:
	.skip	4
hmq:
	.skip	4

