
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
	call	[lib_code]
	FLEXOS_WRITE text_quit
	FLEXOS_EXIT

	.section .init, "ax", @progbits

	.extern $$IMPORT$lib.slb.VER.1.0
	.long	$$IMPORT$lib.slb.VER.1.0
lib_code:
	.long	0
lib_data:
	.long	0

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
	.ascii	"Greetings from MAIN.386!"
	.byte	13, 10
	.equ	text_greetings_length, . - text_greetings

text_quit:
	.ascii	"Quitting"
	.byte	13, 10
	.equ	text_quit_length, . - text_quit

	.section	.stack, "aw", @nobits
	.skip	0x1000

