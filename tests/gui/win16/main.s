
.include "../include/common.inc"

	.section	.text
	.code16

	.global	start
start:
	_StartUp

.if TARGET_WIN16
	pushw	offset 0
	pushw	offset $$SEGOF$dialog_message
	pushw	offset dialog_message
	pushw	offset $$SEGOF$dialog_title
	pushw	offset dialog_title
	pushw	offset 0
.set MessageBox, $$IMPORT$USER$0001
.set $$SEGOF$MessageBox, $$IMPSEG$USER$0001
	.extern	MessageBox, $$SEGOF$MessageBox
	.byte	0x9A
	.word	MessageBox
	.word	$$SEGOF$MessageBox
.endif

	call	Exit

Exit:
	_Exit

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
.if TARGET_WIN16
	.ascii	" Windows (16-bit)"
.endif
	.byte	0

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

