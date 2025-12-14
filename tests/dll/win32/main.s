
	.section .text
	.code32

	.set	MessageBoxA, $$IMPORT$USER32.dll$MessageBoxA$0000
	.set	ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000

	.set	DisplayMessage, $$IMPORT$LIB.dll$DisplayMessage$0000

	.global	_start
_start:
	push	0
	push	offset	text_title
	push	offset	text_start
	push	0
	call	MessageBoxA

	call	DisplayMessage

	push	0
	push	offset	text_title
	push	offset	text_quit
	push	0
	call	MessageBoxA

	push	0
	call	ExitProcess

	.section .data

text_title:
	.ascii	"32-bit"
	.byte	0

text_start:
	.ascii	"Starting 32-bit"
	.byte	0

text_quit:
	.ascii	"Quitting"
	.byte	0

