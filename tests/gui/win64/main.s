
	.section	.text
	.code64

	.set	MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.set	ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000

	.global	_start
_start:
	# Main part

	pushq	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	# shadow space
	sub	rsp, 0x20
	mov	ecx, 0
	lea	rdx, [rip + dialog_message]
	lea	r8, [rip + dialog_title]
	mov	r9d, 0
	call	MessageBox
	mov	rsp, rbp
	popq	rbp

	call	Exit

Exit:
	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, 0
	call	ExitProcess

	.section	.data

dialog_title:
	.asciz	"Sample Application"

dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (64-bit)"
	.byte	0

