
	.section	.text
	.code64

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
.set MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.extern	MessageBox
	call	MessageBox
	mov	rsp, rbp
	popq	rbp

	call	Exit

Exit:
	.extern	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, 0
	call	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (64-bit)"
	.byte	0

