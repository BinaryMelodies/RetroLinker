
.macro	CALLFAR	seg, off
	.byte	0x9A
	.word	\off
	.word	\seg
.endm

.macro	FLEXOS_EXIT
	mov	ax, 0
	mov	cx, 25
	int	0xDC
.endm

.macro	FLEXOS_WRITE message
	mov	word ptr [write_paramblock_buffer], offset \message
	mov	word ptr [write_paramblock_buffer + 2], ds

	mov	word ptr [write_paramblock_bufsiz], offset \message\()_length
	mov	word ptr [write_paramblock_bufsiz + 2], offset \message\()_length >> 16

	lea	ax, [write_paramblock]
	mov	bx, ds
	mov	cx, 8
	int	0xDC
.endm

	.section .text
	.code16

	.global	_start
_start:
	push	ds
	mov	ax, offset $$SEG$.data
	mov	ds, ax
	FLEXOS_WRITE text_greetings
	pop	ds
	retf

	.section .data

write_paramblock:
	# sync
	.byte	0
	# option
	.byte	0
	# flags (flush, SEEK_CUR)
	.word	0x0101
	# swi
	.word	0, 0
	# fnum
	.word	1, 0
	# buffer
write_paramblock_buffer:
	.word	0, 0
	# bufsiz
write_paramblock_bufsiz:
	.word	0, 0
	# offset
	.word	0, 0

text_greetings:
	.ascii	"Greetings from LIB.SRL!"
	.byte	13, 10
	.equ	text_greetings_length, $ - text_greetings

