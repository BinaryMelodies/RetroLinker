
.include "../include/common.inc"

	.section	.text
	.code32

	.global	start
start:
	_StartUp

.if TARGET_WIN32
	pushd	offset 0
	pushd	offset dialog_title
	pushd	offset dialog_message
	pushd	offset 0
.set MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.extern	MessageBox
	call	MessageBox
.endif

	call	Exit

Exit:
	_Exit

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
.if TARGET_WIN32
	.ascii	" Windows (32-bit)"
.endif
	.byte	0

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

