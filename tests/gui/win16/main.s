
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

	.section	.text
	.code16

	.set	InitTask, $$IMPORT$KERNEL$005B
	.set	$$SEGOF$InitTask, $$IMPSEG$KERNEL$005B

	.set	WaitEvent, $$IMPORT$KERNEL$001E
	.set	$$SEGOF$WaitEvent, $$IMPSEG$KERNEL$001E

	.set	InitApp, $$IMPORT$USER$0005
	.set	$$SEGOF$InitApp, $$IMPSEG$USER$0005

	.set	MessageBox, $$IMPORT$USER$0001
	.set	$$SEGOF$MessageBox, $$IMPSEG$USER$0001

	.global	_start
_start:
	CALLFAR	$$SEGOF$InitTask, InitTask

	test	ax, ax
	jnz	1f

	mov	ax, 0x4C01
	int	0x21
1:

	# hInstance, used by InitApp
	push	di

	mov	ax, 0
	push	ax
	CALLFAR	$$SEGOF$WaitEvent, WaitEvent

	CALLFAR	$$SEGOF$InitApp, InitApp

	# Main part

	pushw	offset 0
	pushw	offset $$SEGOF$dialog_message
	pushw	offset dialog_message
	pushw	offset $$SEGOF$dialog_title
	pushw	offset dialog_title
	pushw	offset 0
	CALLFAR	$$SEGOF$MessageBox, MessageBox

	call	Exit

Exit:
	mov	ax, 0x4C00
	int	0x21

	.section	.beg.data, "aw", @progbits

	# Instance data
	.rept	16
	.byte	0
	.endr

	.section	.data

dialog_title:
	.asciz	"Sample Application"

dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (16-bit)"
	.byte	0

