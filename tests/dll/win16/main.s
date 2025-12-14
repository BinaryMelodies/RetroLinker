
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

	.section .text
	.code16

	.set	InitTask, $$IMPORT$KERNEL$005B
	.set	$$SEGOF$InitTask, $$IMPSEG$KERNEL$005B

	.set	WaitEvent, $$IMPORT$KERNEL$001E
	.set	$$SEGOF$WaitEvent, $$IMPSEG$KERNEL$001E

	.set	InitApp, $$IMPORT$USER$0005
	.set	$$SEGOF$InitApp, $$IMPSEG$USER$0005

	.set	MessageBox, $$IMPORT$USER$0001
	.set	$$SEGOF$MessageBox, $$IMPSEG$USER$0001

	.set	DisplayMessage, $$IMPORT$LIB$_DISPLAYMESSAGE
	.set	$$SEGOF$DisplayMessage, $$IMPSEG$LIB$_DISPLAYMESSAGE

	.global	_start
_start:
	CALLFAR	$$SEGOF$InitTask, InitTask

	test	ax, ax
	jnz	.1

	mov	ax, 0x4C01
	int	0x21
.1:

	# hInstance, used by InitApp
	push	di

	mov	ax, 0
	push	ax
	CALLFAR	$$SEGOF$WaitEvent, WaitEvent

	CALLFAR	$$SEGOF$InitApp, InitApp

	mov	ax, 0
	push	ax
	mov	ax, offset $$SEGOF$text_start
	push	ax
	mov	ax, offset text_start
	push	ax
	mov	ax, offset $$SEGOF$text_title
	push	ax
	mov	ax, offset text_title
	push	ax
	mov	ax, 0
	push	ax
	CALLFAR	$$SEGOF$MessageBox, MessageBox

	CALLFAR	$$SEGOF$DisplayMessage, DisplayMessage

	mov	ax, 0
	push	ax
	mov	ax, offset $$SEGOF$text_quit
	push	ax
	mov	ax, offset text_quit
	push	ax
	mov	ax, offset $$SEGOF$text_title
	push	ax
	mov	ax, offset text_title
	push	ax
	mov	ax, 0
	push	ax
	CALLFAR	$$SEGOF$MessageBox, MessageBox

	mov	ax, 0x4C00
	int	0x21

	.section .data

	.skip	16

text_title:
	.ascii	"16-bit"
	.byte	0

text_start:
	.ascii	"Starting 16-bit"
	.byte	0

text_quit:
	.ascii	"Quitting"
	.byte	0

