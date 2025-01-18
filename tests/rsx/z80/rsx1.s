
	.section	.text

	.equ	BDOS, next

start:
# LOADER fills in the serial number
	.byte	0, 0, 0, 0, 0, 0
# actual entry point, from CALL 5
entry_point:
	jp	entry
# jump to next RSX in chain
next:
	jp	0
# address of previous RSX in chain
prev:
	.word	0
# set if RSX must be removed after system warm start
remove:
	.byte	0xFF
# set if RSX must only be loaded on non-banked systems
nonbank:
	.byte	0
# the name of this RSX
name:
	.ascii	"RSX1"
.rept 8 - (. - name)
	.byte	' '
.endr
	.byte	0, 0, 0

entry:

# only run on special BDOS call 0xFF
	ld	a, c
# overload call 0xFF
	cp	0xFF
# resume to next RSX in chain
	jp	nz, next

# switch stacks
	ld	hl, 0
	add	hl, sp
	ld	(caller_stack), hl
	ld	sp, local_stack

# write message
	ld	hl, text_greetings
	call	write_string_newline

# write entry
	ld	hl, text_entry
	call	write_string

	ld	hl, entry_point
	call	write_word
	call	write_newline

# write next in chain
	ld	hl, text_next
	call	write_string

	ld	hl, (next + 1)
	call	write_word
	call	write_newline

# write previous in chain
	ld	hl, text_previous
	call	write_string

	ld	hl, (prev)
	call	write_word
	call	write_newline

# restore stack
	ld	hl, (caller_stack)
	ld	sp, hl

# chain onto next call
	ld	c, 0xFF
	jp	next

# return value, return
#	xor	a
#	ret

.include "common.h"

text_greetings:
	.ascii	"Greetings from RSX 1!"
	.byte	0

text_entry:
	.ascii	"Entry: "
	.byte	0

text_next:
	.ascii	"Next in chain: "
	.byte	0

text_previous:
	.ascii	"Previous in chain: "
	.byte	0

caller_stack:
	.word	0

.rept	32
	.word	0
.endr
local_stack:

