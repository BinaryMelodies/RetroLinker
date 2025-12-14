
	.section	.text
	.code32

	.set	MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.set	ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000

	.global	_start
_start:
	# Main part

	pushd	offset 0
	pushd	offset dialog_title
	pushd	offset dialog_message
	pushd	offset 0
	call	MessageBox

	call	Exit

Exit:
	call	ExitProcess

	.section	.data

dialog_title:
	.asciz	"Sample Application"

dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (32-bit)"
	.byte	0

