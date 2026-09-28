
.macro	FLEXOS_EXIT
	mov	eax, 0
	mov	ecx, 25
	int	0xDD
.endm

.macro	FLEXOS_WRITE message
	mov	dword ptr [write_paramblock_buffer], offset \message

	mov	dword ptr [write_paramblock_bufsiz], offset \message\()_length

	lea	eax, [write_paramblock]
	mov	ecx, 8
	int	0xDD
.endm

	.section .text
	.code32

	.global	_start
_start:
	FLEXOS_WRITE text_greetings
	ret

	.section .data

write_paramblock:
	# sync
	.byte	0
	# option
	.byte	0
	# flags (flush, SEEK_CUR)
	.short	0x0101
	# swi
	.long	0
	# fnum
	.long	1
	# buffer
write_paramblock_buffer:
	.long	0
	# bufsiz
write_paramblock_bufsiz:
	.long	0
	# offset
	.long	0

text_greetings:
	.ascii	"Greetings from LIB.SLB!"
	.byte	13, 10
	.equ	text_greetings_length, $ - text_greetings

