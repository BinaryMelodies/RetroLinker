
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

	.section .text
	.code16

	.global	_start
_start:

	.set	InitTask, $$IMPORT$KERNEL$005B
	.set	$$SEGOF$InitTask, $$IMPSEG$KERNEL$005B
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
	.set	WaitEvent, $$IMPORT$KERNEL$001E
	.set	$$SEGOF$WaitEvent, $$IMPSEG$KERNEL$001E
	CALLFAR	$$SEGOF$WaitEvent, WaitEvent

	.set	InitApp, $$IMPORT$USER$0005
	.set	$$SEGOF$InitApp, $$IMPSEG$USER$0005
	CALLFAR	$$SEGOF$InitApp, InitApp

	mov	dx, offset msg_start
	mov	ah, 0x09
	int	0x21

	.set	DisplayMessage, $$IMPORT$LIB$_DISPLAYMESSAGE
	.set	$$SEGOF$DisplayMessage, $$IMPSEG$LIB$_DISPLAYMESSAGE
	CALLFAR	$$SEGOF$DisplayMessage, DisplayMessage

	mov	dx, offset msg_quit
	mov	ah, 0x09
	int	0x21

	mov	ax, 0x4C00
	int	0x21

	.section .data

	.skip	16

msg_start:
	.ascii	"Starting 16-bit"
	.byte	13, 10
	.byte	'$'

msg_quit:
	.ascii	"Quitting"
	.byte	13, 10
	.byte	'$'

