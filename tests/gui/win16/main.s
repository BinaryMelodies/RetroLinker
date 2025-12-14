
	.section	.text
	.code16

	.global	_start
_start:
	# InitTask
	.extern	$$IMPORT$KERNEL$005B, $$IMPSEG$KERNEL$005B
	.byte	0x9A
	.word	$$IMPORT$KERNEL$005B
	.word	$$IMPSEG$KERNEL$005B

	test	ax, ax
	jnz	1f

	mov	ax, 0x4C01
	int	0x21
1:

	# hInstance, used by InitApp
	push	di

	mov	ax, 0
	push	ax
	# WaitEvent
	.extern	$$IMPORT$KERNEL$001E, $$IMPSEG$KERNEL$001E
	.byte	0x9A
	.word	$$IMPORT$KERNEL$001E
	.word	$$IMPSEG$KERNEL$001E

	# InitApp
	.extern	$$IMPORT$USER$0005, $$IMPSEG$USER$0005
	.byte	0x9A
	.word	$$IMPORT$USER$0005
	.word	$$IMPSEG$USER$0005

	# Main part

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

	call	Exit

Exit:
	mov	ax, 0x4C00
	int	0x21

	.section	.data

dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.ascii	"Graphical Greetings!"
	.ascii	" Windows (16-bit)"
	.byte	0

	.section	.beg.data, "aw", @progbits

	# Instance data
	.rept	16
	.byte	0
	.endr

