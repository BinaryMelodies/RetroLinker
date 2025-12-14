
	.section .text
	.code32

	.set	MessageBoxA, $$IMPORT$USER32.dll$MessageBoxA$0000

	.global	_start
_start:
	ret	12

	#.set	DisplayMessage, $$EXPORT$DisplayMessage$0001
	.global	$$EXPORT$DisplayMessage$0001
$$EXPORT$DisplayMessage$0001:
	push	0
	push	offset	text_title
	push	offset	text_message
	push	0
	call	MessageBoxA
	ret

	.section .data

text_title:
	.ascii	"32-bit"
	.byte	0

text_message:
	.ascii	"MessageBox called from DLL"
	.byte	0

