
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

	.section .text
	.code16

	.global	_start
_start:
	push	ds
	pop	ax
	nop

	inc	bp
	push	bp
	mov	bp, sp
	push	ds
	mov	ds, ax

	jcxz	_no_heap
	push	ds
	xor	ax, ax
	push	ax
	push	cx
	.set	LocalInit, $$IMPORT$KERNEL$0004
	.set	$$SEGOF$LocalInit, $$IMPSEG$KERNEL$0004
	CALLFAR	$$SEGOF$LocalInit, LocalInit
_no_heap:

	pop	ds
	mov	sp, bp
	pop	bp
	dec	bp
	retf

	.set	DisplayMessage, $$EXPORT$DISPLAYMESSAGE$0001
	.global	$$EXPORT$DISPLAYMESSAGE$0001
$$EXPORT$DISPLAYMESSAGE$0001:
	mov	ax, offset $$SEG$.data
	nop

	push	ds
	mov	ds, ax

	mov	ax, 0
	push	ax
	mov	ax, offset $$SEGOF$text_message
	push	ax
	mov	ax, offset text_message
	push	ax
	mov	ax, offset $$SEGOF$text_title
	push	ax
	mov	ax, offset text_title
	push	ax
	mov	ax, 0
	push	ax
	.set	MessageBox, $$IMPORT$USER$0001
	.set	$$SEGOF$MessageBox, $$IMPSEG$USER$0001
	CALLFAR	$$SEGOF$MessageBox, MessageBox

	pop	ds
	retf

	.section .data

	.skip	16

text_title:
	.ascii	"16-bit"
	.byte	0
text_message:
	.ascii	"MessageBox called from DLL"
	.byte	0

