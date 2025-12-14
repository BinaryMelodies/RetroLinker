
.include "../include/common.inc"

	.section	.text
	.code64

	.global	start
start:
	_StartUp

.if TARGET_WIN64
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
.endif

	call	Exit

Exit:
	_Exit

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
.if TARGET_WIN64
	.ascii	" Windows (64-bit)"
.endif
	.byte	0

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

