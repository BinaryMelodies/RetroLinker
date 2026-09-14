
# One of these must be set to 1 according to the target operating system (not used)

# Targets any 8-bit version of CP/M for the 8080 and compatibles
.ifndef	SYSTEM_CPM80
.equ	SYSTEM_CPM80, 0
.endif

# Targets any version of MSX-DOS
.ifndef	SYSTEM_MSXDOS
.equ	SYSTEM_MSXDOS, 0
.endif

# One of these must be set to 1 according to the format of the binary generated

# CP/M flat .COM file
.ifndef	FORMAT_COM
.equ	FORMAT_COM, 0
.endif

# MP/M .PRL file
.ifndef	FORMAT_PRL
.equ	FORMAT_PRL, 0
.endif

# CP/M Plus .COM file
.ifndef	FORMAT_CPM3
.equ	FORMAT_CPM3, 0
.endif

# CP/M Plus .RSX file, to be attached to the .COM file
.ifndef	FORMAT_RSX
.equ	FORMAT_RSX, 0
.endif

	.equ	SYSTEM_TYPE_CPM80, 1
	.equ	SYSTEM_TYPE_MPM80, 2
	.equ	SYSTEM_TYPE_MSXDOS, 3

	.text

.if	FORMAT_RSX
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
	.ascii	"RSX"
	.byte	'0' + RSX_INDEX
.rept 8 - (. - name)
	.byte	' '
.endr
	.byte	0, 0, 0
.endif

	.equ	PROGRAM_BASE, entry - 256

.if	FORMAT_RSX
	.equ	BDOS_ENTRY, next
.else
	# Assembler breaks without an additional line
	.equ	BDOS_ENTRY, PROGRAM_BASE + 5
.endif

	# Note that while this file is written in the Zilog Z80 syntax, it does not contain instructions that do not also work on an Intel 8080

	.global	entry
entry:
.if	FORMAT_COM
	# MS-DOS commands have the same .com file extension but incompatible binary codes
	# To avoid accidentally executing the code, a stub code is executed that also works on MS-DOS
	# This is accomplished by selecting a specific set of opcodes that execute differently on an 8080 and 8086

	# In 8086, this is a near jump to msdos_stub
	# In 8080/Z80, this corresponds to "ex de, hl" and "dec b", two harmless but not particularly useful instructions
	.byte	0xEB, 0x05

	# We can undo the effects of these two instructions, even though it's not necessary
	ex	de, hl
	inc	b
	# Alternatively, we could have created a 3-byte jump consisting of 0xEB, 0x03, which translates to "ex de, hl" and "dec bc"

	# Next we jump to the CP/M-80 entry
	jp	cpm80_entry

	# The MS-DOS stub will display an error message and quit the program
msdos_stub:
	# mov dx, text_msdos_message
	.byte	0xBA
	.word	text_msdos_message
	# mov ah, 9
	.byte	0xB4
	.byte	9
	# int 0x21
	.byte	0xCD, 0x21
	# int 0x20
	.byte	0xCD, 0x20

text_msdos_message:
	.ascii	"This program can only be executed under CP/M-80."
	.byte	13, 10, '$'

cpm80_entry:
.endif

.if	FORMAT_RSX
	# Only run on special BDOS call
	ld	a, c
	# Overload call 0xFF
	cp	0xFF
	# Otherwise, resume to next RSX in chain
	jp	nz, next

	# Switch stacks
	ld	hl, 0
	add	hl, sp
	ld	(caller_stack), hl
	ld	sp, local_stack
.else
	# Store initial SP
	ld	hl, 0
	add	hl, sp
	ld	(initial_sp), hl
	# Set SP
	ld	sp, stack_top
.endif

#### Actual code

	# Friendly greeting and initial registers

	ld	hl, text_greeting
	call	write_string_newline

	ld	hl, text_explain_initial
	call	write_string_newline

	ld	hl, text_pc
	call	write_string
	# To calculate the PC, we issue a CALL instruction, which pushes the next PC value to the stack
	call	1f
1:
	pop	hl
	# Then we adjust the distance from the actual start
.if	FORMAT_RSX
	ld	de, entry_point - 1b
.else
	ld	de, entry - 1b
.endif
	add	hl, de
	call	write_word
	call	write_newline

.if	FORMAT_RSX
	# Write entry
	ld	hl, text_start
	call	write_string

	ld	hl, entry
	call	write_word
	call	write_newline

	# Write next in chain
	ld	hl, text_next
	call	write_string

	ld	hl, (next + 1)
	call	write_word
	call	write_newline

	# Write previous in chain
	ld	hl, text_previous
	call	write_string

	ld	hl, (prev)
	call	write_word
	call	write_newline

.else
	ld	hl, text_sp
	call	write_string
	# We have modified the initial SP, so we use the stored value instead
	ld	hl, (initial_sp)
	call	write_word
	call	write_newline

	# Test BIOS entry point
	ld	a, (PROGRAM_BASE)
	cp	0xC3
	jp	z, write_bios_entry

	# BIOS entry is not a call
	ld	hl, text_not_bios_jump
	call	write_string_newline

write_bios_entry:
	ld	hl, text_bios_entry
	call	write_string
	ld	hl, (PROGRAM_BASE + 0x0001)
	call	write_word
	call	write_newline

	# Test BDOS entry
	ld	a, (BDOS_ENTRY)
	cp	0xC3
	jp	z, write_bdos_entry

	ld	hl, text_not_bdos_jump
	call	write_string_newline

write_bdos_entry:
	ld	hl, text_bdos_entry
	call	write_string
	ld	hl, (BDOS_ENTRY + 0x0001)
	call	write_word
	call	write_newline

	# Test BIOS jump table
	ld	hl, (PROGRAM_BASE + 0x0001)
	ld	a, (hl)

	cp	0xC3
	jp	z, write_jump_table

	# BIOS entry is not a jump table
	ld	hl, text_not_jump_table
	call	write_string_newline

write_jump_table:
	inc	hl
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	de

	ld	hl, text_bios_jump_table
	call	write_string

	pop	hl
	call	write_word
	call	write_newline
.endif

	call	waitkey

.if	!FORMAT_RSX
	# Determine CP/M version
	# This API call exists since CP/M 2.0
	ld	c, 0x0C
	call	BDOS_ENTRY

	# CP/M 1.* returns BA=0000
	or	b
	jp	nz, is_cpm2

	# CP/M 1.4 also returns HL=0000
	ld	a, h
	or	l
	jp	z, is_cpm14

	# CP/M 1.3 and later have: a JP 0x??03 instruction at address 0
	ld	a, (PROGRAM_BASE + 1)
	cp	0x03
	jp	z, is_cpm13

is_cpm10:
	# CP/M 1975, arbitrarily assigned version 1.0
	ld	hl, 0x0010
	jp	is_cpm2
is_cpm13:
	ld	hl, 0x0013
	jp	is_cpm2
is_cpm14:
	ld	hl, 0x0014
	jp	is_cpm2
is_cpm2:
	ld	(cpm_version), hl

	# Display version information
	ld	hl, text_call5_0C
	call	write_string
	ld	c, 0x0C
	call	BDOS_ENTRY
	push	hl
	# CP/M 1 returns this in BA instead of HL
	ld	l, a
	ld	h, b
	call	write_word
	# But we also display the value in HL
	ld	hl, text_and_hl
	call	write_string
	pop	hl
	call	write_word
	call	write_newline

.if	FORMAT_COM
	# Check if CP/M version is 2.2, all MSX-DOS versions report 2.2
	ld	hl, (cpm_version)
	ld	a, h
	cp	0x00
	jp	nz, is_cpm80
	ld	a, l
	cp	0x22
	jp	nz, is_cpm80

	# To check whether this is a true CP/M or MSX-DOS, we will see if user numbers are supported
	# CP/M understands up to 16 user numbers (basically disjoint file storage regions on the disk)
	# MSX-DOS has no concept of user numbers
	ld	e, 0xFF
	ld	c, 0x20
	call	BDOS_ENTRY
	or	a
	# If the user number is non-zero, this is a CP/M system
	jp	nz, is_cpm80

	ld	e, 1
	ld	c, 0x20
	call	BDOS_ENTRY
	ld	e, 0xFF
	ld	c, 0x20
	call	BDOS_ENTRY
	or	a
	# If the user number did not change, this is not a CP/M system
	jp	z, is_msxdos

	# Reset the user number to its original state
	ld	e, 0
	ld	c, 0x20
	call	BDOS_ENTRY
	jp	is_cpm80

is_msxdos:
	ld	a, SYSTEM_TYPE_MSXDOS
	ld	(system_type), a

	# Retrieve MSX-DOS and MSXDOS2.SYS versions
	# In the unlikely case CP/M got here, make sure the next system call does not do something weird
	ld	de, zeroes
	# Random value to check if B has changed, C is Get MSX-DOS version number
	ld	bc, 0xBA6F
	# Random value to check if A has changed
	ld	a, 0xAB
	call	BDOS_ENTRY
	# A: error, B: major version number
	or	a
	jp	nz, is_cpm80
	# system
	push	de
	# kernel
	push	bc
	pop	hl
	push	hl
	ld	(msxdos_version), hl
	ld	hl, text_call5_6F
	call	write_string
	pop	hl
	call	write_word
	ld	hl, text_and_de
	call	write_string
	pop	hl
	call	write_word
	call	write_newline

	ld	hl, text_msxdos
	call	write_string_newline
	jp	1f
.endif

	# Display system type (CP/M, MP/M, MSX-DOS)
is_cpm80:
.if	FORMAT_COM || FORMAT_PRL
	ld	a, (cpm_version + 1)
	and	1
	jp	nz, is_mpm80
	ld	a, SYSTEM_TYPE_CPM80
	ld	(system_type), a
.endif
	ld	hl, text_cpm80
.if	FORMAT_COM || FORMAT_PRL
	jp	is_xpm80
is_mpm80:
	ld	a, SYSTEM_TYPE_MPM80
	ld	(system_type), a

	# Retrieve MP/M version
	ld	c, 0xA3
	call	BDOS_ENTRY
	push	hl
	ld	hl, text_call5_A3
	call	write_string
	pop	hl
	call	write_word
	call	write_newline

	ld	hl, text_mpm80
is_xpm80:
.endif
	call	write_string_newline
1:

	# Display BDOS version
	ld	hl, text_bdos_version
	call	write_string
	ld	a, (cpm_version)
	rlca
	rlca
	rlca
	rlca
	call	write_nibble
	ld	a, '.'
	call	write_char
	ld	a, (cpm_version)
	call	write_nibble
	call	write_newline

	call	waitkey

	# Display zero page
	ld	hl, text_zero_page
	call	write_string_newline

	ld	hl, 0
2:
	ld	a, (hl)
	push	hl
	call	write_byte
	pop	hl
	inc	l
	ld	a, l
	and	0xF
	jp	z, 3f
	push	hl
	ld	a, ' '
	call	write_char
	pop	hl
	jp	2b
3:
	ld	a, l
	or	a
	jp	z, 5f

	ld	a, l
	and	0x70
	push	hl
	jp	nz, 4f
	call	write_newline
	call	waitkey
4:
	call	write_newline
	pop	hl
	jp	2b
5:
	call	write_newline
	call	waitkey

.if	FORMAT_COM
	# Display environment variables (MSX-DOS 2 only)
	ld	a, (system_type)
	cp	SYSTEM_TYPE_MSXDOS
	jp	nz, no_environment

	# MSX-DOS 1 has no environment variables
	ld	a, (msxdos_version + 1)
	or	a
	jp	z, no_environment

	ld	hl, text_environment
	call	write_string_newline

	# Fetch all environment variable names, indexed by DE
	ld	de, 1
1:
	ld	hl, env_buffer
	ld	b, 255
	ld	c, 0x6D
	push	de
	call	BDOS_ENTRY

	ld	a, (hl)
	or	a
	jp	z, end_environment

	call	write_string

	ld	a, '='
	call	write_char

	# Retrieve value for environment variable
	ld	hl, env_buffer
	ld	de, env_buffer2
	ld	b, 255
	ld	c, 0x6B
	call	BDOS_ENTRY

	ld	hl, env_buffer2
	call	write_string

	call	write_newline
	pop	de
	inc	de
	jp	1b

end_environment:
	call	waitkey

no_environment:
.endif

	# Pre-1.3 versions do not seem to have a command line
	ld	a, (cpm_version)
	cp	0x13
	jp	c, no_command_line

	# Display command line
	# The command line is stored at offset 0x0080 (length, data)
	ld	hl, text_command_line
	call	write_string

	ld	hl, 0x80
	ld	c, (hl)
	inc	hl
	ld	a, c
	or	a
	jp	z, 2f
1:
	ld	a, (hl)
	push	hl
	push	bc
	call	write_char
	pop	bc
	pop	hl
	inc	hl
	dec	c
	jp	nz, 1b

2:
	ld	a, '"'
	call	write_char
	call	write_newline
	call	waitkey
.endif

no_command_line:

.if	FORMAT_CPM3
	# Invoke RSX
	ld	c, 0xFF
	call	BDOS_ENTRY
.elseif	FORMAT_RSX
	# Restore stack
	ld	hl, (caller_stack)
	ld	sp, hl

	# Chain onto next call
	ld	c, 0xFF
	jp	next

	# Return value, return
	#	xor	a
	#	ret
.else
	call	exit
.endif

#### Support calls

exit:
	ld	c, 0
	call	BDOS_ENTRY
	ret

write_string:
	ld	a, (hl)
	and	a
	ret	z
	push	hl
	call	write_char
	pop	hl
	inc	hl
	jp	write_string

write_string_newline:
	call	write_string
#	jp	write_newline

write_newline:
	ld	a, 13
	call	write_char
	ld	a, 10
	jp	write_char

write_word:
	push	hl
	ld	a, h
	call	write_byte
	pop	hl
	ld	a, l
#	jp	write_byte

write_byte:
	push	af
	rrca
	rrca
	rrca
	rrca
	call	write_nibble
	pop	af
#	jp	write_nibble

write_nibble:
	and	0xF
	cp	10
	jp	nc, 1f
	add	a, '0'
	jp	2f
1:
	add	a, 'A'-10
2:
#	jp	write_char

write_char:
	ld	e, a
	ld	c, 2
	call	BDOS_ENTRY
	ret

waitkey:
	ld	hl, text_waitkey
	call	write_string_newline
	ld	c, 1
	call	BDOS_ENTRY
	ret

	.section	.rodata

text_greeting:
.if	!FORMAT_RSX
	.ascii	"Greetings!"
	.byte	13, 10
.endif

.if	FORMAT_COM
	.ascii	" * CP/M-80 flat .COM file *"
.elseif	FORMAT_PRL
	.ascii	" * MP/M-80 .PRL file *"
.elseif	FORMAT_CPM3
	.ascii	" * CP/M Plus .COM file *"
.elseif	FORMAT_RSX
	.ascii	" * CP/M Plus .RSX file #"
	.byte	'0' + RSX_INDEX
	.ascii	" *"
.endif
	.byte	0

text_waitkey:
	.ascii	" ~ Press a key ~"
	.byte	0

text_explain_initial:
	.ascii	"Initial register values:"
	.byte	0

text_pc:
	.ascii	"Initial PC:         "
	.byte	0

.if	FORMAT_RSX
text_start:
	.ascii	"Start:              "
	.byte	0

text_next:
	.ascii	"Next in chain:      "
	.byte	0

text_previous:
	.ascii	"Previous in chain:  "
	.byte	0
.else
text_sp:
	.ascii	"Initial SP:         "
	.byte	0

text_command_line:
	.ascii	"Command line: "
	.byte	'"', 0

text_bios_entry:
	.ascii	"BIOS entry address: "
	.byte	0

text_bdos_entry:
	.ascii	"BDOS entry address: "
	.byte	0

text_bios_jump_table:
	.ascii	"BIOS jump table:    "
	.byte	0
.endif

text_not_bdos_jump:
.if	FORMAT_RSX
	.ascii	"Not jump at 0x0006"
.else
	.ascii	"Not jump at 0x0005"
.endif
	.byte	0

.if	!FORMAT_RSX
text_not_bios_jump:
	.ascii	"Error: Not jump at 0x0000"
	.byte	0

text_not_jump_table:
	.ascii	"Error: Not jump in BIOS jump table"
	.byte	0

text_bdos_version:
	.ascii	"BDOS version "
	.byte	0

text_call5_0C:
	.ascii	"BDOS version (call 5, C=0C):"
	.byte	13, 10
	.ascii	"    BA = "
	.byte	0

.if	FORMAT_COM
text_call5_6F:
	.ascii	"MSX-DOS version (call 5, C=6F):"
	.byte	13, 10
	.ascii	"    BC = "
	.byte	0
.endif

.if	FORMAT_COM || FORMAT_PRL
text_call5_A3:
	.ascii	"MP/M version (call 5, C=A3):"
	.byte	13, 10
	.ascii	"    HL = "
	.byte	0
.endif

text_and_hl:
	.ascii	", HL="
	.byte	0

.if	FORMAT_COM
text_and_de:
	.ascii	", DE="
	.byte	0
.endif

text_cpm80:
	.ascii	"This is CP/M-80"
	.byte	0

.if	FORMAT_COM || FORMAT_PRL
text_mpm80:
	.ascii	"This is MP/M-80"
	.byte	0
.endif

.if	FORMAT_COM
text_msxdos:
	.ascii	"This is MSX-DOS"
	.byte	0
.endif

text_zero_page:
	.ascii	"Zero page:"
	.byte	0
.endif

.if	FORMAT_COM
text_environment:
	.ascii	"Environment:"
	.byte	0
.endif

	.align	4

	.section	.bss

initial_sp:
	.skip	2

cpm_version:
	.skip	2

.if	FORMAT_COM
msxdos_version:
	.word	0
.endif

.if	FORMAT_COM || FORMAT_PRL
system_type:
	.skip	1
.endif

.if	FORMAT_COM
env_buffer:
	.skip	255
env_buffer2:
	.skip	255
.endif

.if	FORMAT_COM
	# to make sure CP/M 3 won't write anything
zeroes:
	.word	0, 0
.endif

.if	FORMAT_RSX
caller_stack:
	.word	0

.rept	32
	.word	0
.endr
local_stack:
.else
	.section .stack, "bw"

	.skip 0x100
	.global	stack_top
stack_top:
.endif

