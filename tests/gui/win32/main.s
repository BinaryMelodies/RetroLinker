
	.section	.text
	.code32

	.global	_start
_start:
	# Main part

	pushd	offset 0
	pushd	offset dialog_title
	pushd	offset dialog_message
	pushd	offset 0
.set MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.extern	MessageBox
	call	MessageBox

	call	Exit

Exit:
	.extern	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	push	0
	call	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (32-bit)"
	.byte	0

