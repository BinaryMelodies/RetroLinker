
.ifndef	SYSTEM_CPM68K
.equ	SYSTEM_CPM68K, 0
.endif
.ifndef	SYSTEM_GEMDOS
.equ	SYSTEM_GEMDOS, 0
.endif
.ifndef	SYSTEM_HUMAN68K
.equ	SYSTEM_HUMAN68K, 0
.endif

.ifndef	FORMAT_601A
.equ	FORMAT_601A, 0
.endif
.ifndef	FORMAT_601B
.equ	FORMAT_601B, 0
.endif
.ifndef	FORMAT_RFILE
.equ	FORMAT_RFILE, 0
.endif
.ifndef	FORMAT_HU_XFILE
.equ	FORMAT_HU_XFILE, 0
.endif

.macro	LOAD	address, destination
.if	!FORMAT_601B
	lea	\address(pc), \destination
.else
	lea	(\address).w, \destination
.endif
.endm

.macro	SUPER	stack
.if	SYSTEM_CPM68K
	move.w	#0x3E, d0
	trap	#2
.elseif	SYSTEM_GEMDOS
	move.l	\stack, -(sp)
	move.w	#0x20, -(sp)
	trap	#1
	addq.l	#6, sp
.elseif	SYSTEM_HUMAN68K
	move.l	\stack, -(sp)
	.word	0xFF02
	addq.l	#4, sp
.endif
.endm

.macro	ENTERSUPER
.if	SYSTEM_CPM68K
.elseif	SYSTEM_GEMDOS
	SUPER	#0
	LOAD	user_sp, a0
	move.l	d0, (a0)
.elseif	SYSTEM_HUMAN68K
	SUPER	#0
	LOAD	user_sp, a0
	move.l	d0, (a0)
.endif
.endm

.macro	EXITSUPER
.if	SYSTEM_CPM68K
.elseif	SYSTEM_GEMDOS
	SUPER	user_sp(pc)
.elseif	SYSTEM_HUMAN68K
	SUPER	user_sp(pc)
.endif
.endm

	.section	.text
	.global	start

start:
.if	FORMAT_601A || FORMAT_601B
	# Skip first word for easier disassembly
	nop
.endif

.if	!SYSTEM_HUMAN68K
	LOAD	base_page_address, a0
	move.l	4(sp), (a0)
.else
	LOAD	memory_control_address, a5
	move.l	a0, (a5)
	LOAD	end_address, a5
	move.l	a1, (a5)
	LOAD	command_line_address, a5
	move.l	a2, (a5)
	LOAD	environment_address, a5
	move.l	a3, (a5)
	LOAD	start_address, a5
	move.l	a4, (a5)
.endif

	# Store the temporary stack on entry for later study
.if	!FORMAT_601B
	lea	initial_sp(pc), a0
	move.l	sp, (a0)
	lea	stack_top(pc), sp
.else
	move.l	sp, (initial_sp).w
.endif
	LOAD	stack_top, sp

# TODO: resize memory

	# Friendly greeting and initial registers
	LOAD	text_greeting, a5
	bsr	write_string_newline

	LOAD	text_explain_initial, a5
	bsr	write_string_newline

	LOAD	text_pc, a5
	bsr	write_string
	lea	start(pc), a0
	move.l	a0, d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_sp, a5
	bsr	write_string
	move.l	initial_sp(pc), d0
	bsr	write_long
	bsr	write_newline

.if	SYSTEM_HUMAN68K
# TODO: A0-A4
.endif

	LOAD	text_f_line, a5
	bsr	write_string
	ENTERSUPER
	move.l	4 * 0xB, d7
	EXITSUPER
	move.l	d7, d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_trap_1, a5
	bsr	write_string
	ENTERSUPER
	move.l	4 * 0x21, d7
	EXITSUPER
	move.l	d7, d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_trap_2, a5
	bsr	write_string
	ENTERSUPER
	move.l	4 * 0x22, d7
	EXITSUPER
	move.l	d7, d0
	bsr	write_long
	bsr	write_newline

.if	!SYSTEM_HUMAN68K
	LOAD	text_base_page, a5
	bsr	write_string
	move.l	base_page_address(pc), d0
	bsr	write_long
	bsr	write_newline
.else
	LOAD	text_memory_control, a5
	bsr	write_string
	move.l	memory_control_address(pc), d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_end, a5
	bsr	write_string
	move.l	end_address(pc), d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_command_line, a5
	bsr	write_string
	move.l	command_line_address(pc), d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_environment, a5
	bsr	write_string
	move.l	environment_address(pc), d0
	bsr	write_long
	bsr	write_newline

	LOAD	text_start, a5
	bsr	write_string
	move.l	start_address(pc), d0
	bsr	write_long
	bsr	write_newline
.endif

	# Display PSP/base page

	move.l	base_page_address(pc), -(sp)
# TODO: testing, this can be removed
#.if	SYSTEM_GEMDOS
#	move.l	base_page_address(pc), a0
#	move.l	0x2C(a0), -(sp)
#.elseif	SYSTEM_HUMAN68K
#	move.l	base_page_address(pc), a0
#	move.l	0x10(a0), -(sp)
#.endif

	clr.l	d0
	move.l	d0, a0
1:
	move.l	(sp), a1
	move.b	(a1,a0), d0
	addq.l	#1, a0
	move.l	a0, -(sp)
	bsr	write_byte
	move.l	(sp)+, a0
	move.l	a0, d0
	and.b	#0xF, d0
	beq	2f
	move.l	a0, -(sp)
	move.b	#' ', d0
	bsr	write_char
	move.l	(sp)+, a0
	bra	1b
2:
	move.l	a0, d0
	tst.b	d0
	beq	4f

	move.l	a0, -(sp)
	andb	#0x70, d0
	bne	3f
	bsr	waitkey
3:
	bsr	write_newline
	move.l	(sp)+, a0
	bra	1b
4:
	addq.l	#4, sp
	bsr	write_newline
	bsr	waitkey

	# Display command line
	move.l	base_page_address(pc), a0

.if	SYSTEM_HUMAN68K
	move.l	0x20(a0), a0
.else
	add.l	#0x80, a0
.endif

	clr.l	d1
	move.b	(a0)+, d1

	beq	2f
1:
	move.b	(a0)+, d0
	move.l	a0, -(sp)
	move.l	d1, -(sp)
	bsr	write_char
	move.l	(sp)+, d1
	move.l	(sp)+, a0
	dbra	d1, 1b
	bsr	write_newline
	bsr	waitkey
2:

.if	!SYSTEM_CPM68K
	# Display environment variables
	move.l	base_page_address(pc), a0
.if	SYSTEM_GEMDOS
	move.l	0x2C(a0), a5
.elseif	SYSTEM_HUMAN68K
	move.l	0x10(a0), a5
	move.l	(a5)+, a4
	adda.l	a5, a4
.endif

1:
	bsr	write_string
.if	SYSTEM_GEMDOS
	bsr	write_string
.endif
	bsr	write_newline

#.if	SYSTEM_HUMAN68K
#	cmpa.l	a5, a4
# TODO: jump
#.endif
	tst.b	(a5)
	bne	1b

.endif

	bsr	waitkey
	bsr	exit

write_string:
	clr.l	d0
	move.b	(a5)+, d0
	beq	1f
	bsr	write_char
	bra	write_string
1:
	rts

write_string_newline:
	bsr	write_string
#	bra	write_newline

write_newline:
	move.b	#13, d0
	bsr	write_char
	move.b	#10, d0
	bra	write_char

write_long:
	move.l	d0, -(sp)
	swap	d0
	bsr	write_word
	move.l	(sp)+, d0

write_word:
	move.w	d0, -(sp)
	asr	#8, d0
	bsr	write_byte
	move.w	(sp)+, d0

write_byte:
	move.w	d0, -(sp)
	asr	#4, d0
	bsr	write_nibble
	move.w	(sp)+, d0

write_nibble:
	and.b	#0xF, d0
	cmp.b	#10, d0
	bhs	1f
	add.b	#'0', d0
	bra	2f
1:
	add.b	#'A'-10, d0
2:

write_char:
.if	SYSTEM_CPM68K
	move.w	d0, d1
	move.w	#0x06, d0
	trap	#2
.endif
.if	SYSTEM_GEMDOS
	move.w	d0, -(sp)
	move.w	#0x06, -(sp)
	trap	#1
	addq.l	#4, sp
.endif
.if	SYSTEM_HUMAN68K
	move.w	d0, -(sp)
	.word	0xFF06
	addq.l	#2, sp
.endif
	rts

waitkey:
.if	SYSTEM_CPM68K
	move.w	#0x01, d0
	trap	#2
.endif
.if	SYSTEM_GEMDOS
	move.w	#0x01, -(sp)
	trap	#1
	addq.l	#2, sp
.endif
.if	SYSTEM_HUMAN68K
	.word	0xFF01
.endif
	rts

exit:
.if	SYSTEM_CPM68K
	clr.l	d0
	trap	#2
.endif
.if	SYSTEM_GEMDOS
	clr.w	-(sp)
	trap	#1
.endif
.if	SYSTEM_HUMAN68K
	.word	0xFF00
.endif

	.align	4

	.section	.rodata

text_greeting:
	.ascii	"Greetings!"
	.byte	13, 10
.if	FORMAT_601A
.if	SYSTEM_CPM68K
	.ascii	" * CP/M-68K contiguous .68K file *"
.elseif	SYSTEM_GEMDOS
	.ascii	" * GEMDOS/Atari TOS .PRG file *"
.elseif	SYSTEM_HUMAN68K
	.ascii	" * Human68k .Z file *"
.endif
.elseif	FORMAT_601B
	.ascii	" * CP/M-68K non-contiguous .68K file *"
.elseif	FORMAT_RFILE
	.ascii	" * Human68k .R file *"
.elseif	FORMAT_HU_XFILE
	.ascii	" * Human68k HU .X file *"
.endif
	.byte	0

text_explain_initial:
	.ascii	"Initial register values:"
	.byte	0

text_pc:
	.ascii	"PC: "
	.byte	0

text_sp:
	.ascii	"SP: "
	.byte	0

text_f_line:
	.ascii	"F-line entry: "
	.byte	0

text_trap_1:
	.ascii	"Trap #0x1 entry: "
	.byte	0

text_trap_2:
	.ascii	"Trap #0x2 entry: "
	.byte	0

.if	!SYSTEM_HUMAN68K
text_base_page:
	.ascii	"Base page: "
	.byte	0
.else
text_memory_control:
	.ascii	"Memory control pointer: "
	.byte	0
text_end:
	.ascii	"Program end: "
	.byte	0
text_command_line:
	.ascii	"Command line pointer: "
	.byte	0
text_environment:
	.ascii	"Environment pointer: "
	.byte	0
text_start:
	.ascii	"Program start: "
	.byte	0
.endif

	.align	4

	.section	.bss

initial_sp:
	.skip	4

user_sp:
	.skip	4

.if	!SYSTEM_HUMAN68K
base_page_address:
	.skip	4
.else
base_page_address:
memory_control_address:
	.skip	4
end_address:
	.skip	4
command_line_address:
	.skip	4
environment_address:
	.skip	4
start_address:
	.skip	4
.endif

	.align	4

	.section .stack, "aw", @nobits

	.skip 0x100
	.global	stack_top
stack_top:

