
	.section	.text
	.code16

# The layout of the first 16 bytes is predetermined
entry_point:
	jmp.d32	start
terminate_flag:
	.byte	0xFF
next_rsx:
	.word	0, 0
rsx_name:
	.ascii	"RSX2"
.rept 8 - (. - rsx_name)
	.byte	' '
.endr
base_page:
	.word	0
	.skip	14

start:
# only run on special INT 0xE0 call with CL=0xFF
	cmpb	$0xFF, %cl
	je	1f
# resume to next RSX in chain
	ljmp	*%cs:next_rsx

1:
# load RSX data segment
	pushw	%ds
	movw	%cs:(base_page), %ds
	popw	(old_ds)

	movw	%sp, (old_sp)
	movw	%ss, (old_ss)
# switch stacks
	cli
	movw	%ds, %ax
	movw	%ax, %ss
	movw	$stack_top, %sp
# store registers
	pushw	%es
	pushw	%di
	pushw	%si
	pushw	%bp
	pushw	%bx
	pushw	%dx
	pushw	%cx
	pushw	%ax

# write message
	movw	$text_greetings, %si
	call	write_string_newline

# write entry
	movw	$text_entry, %si
	call	write_string

	movw	%ds, %ax
	call	write_word
	call	write_colon

	movw	$entry_point, %ax
	call	write_word
	call	write_newline

# write next in chain
	movw	$text_next, %ax
	call	write_string

	movw	%cs:(next_rsx + 2), %ax
	call	write_word
	call	write_colon

	movw	%cs:(next_rsx), %ax
	call	write_word
	call	write_newline

# restore registers
	popw	%ax
	popw	%cx
	popw	%dx
	popw	%bx
	popw	%bp
	popw	%si
	popw	%di
	popw	%es

# restore stack and data segment
	cli
	movw	(old_sp), %sp
	movw	(old_ss), %ss
	movw	(old_ds), %ds

# chain onto next call
	ljmp	*%cs:next_rsx

# set return value and return
#	xorw	%ax, %ax
#	lret

.include "common.h"

	.section	.data

text_greetings:
	.ascii	"Greetings from RSX 2!"
	.byte	0

text_entry:
	.ascii	"Entry: "
	.byte	0

text_next:
	.ascii	"Next in chain: "
	.byte	0

	.section	.bss

old_sp:
	.word	0
old_ss:
	.word	0
old_ds:
	.word	0

	.skip	512
stack_top:

