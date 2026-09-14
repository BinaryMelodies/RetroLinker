
# One of these must be set to 1 according to the format of the binary generated

# Primarily uses the MS-DOS API (INT 0x20), this is also selected for Multitasking "European" MS-DOS 4.0
.ifndef	SYSTEM_MSDOS
.equ	SYSTEM_MSDOS, 0
.endif

# Primarily uses the CP/M-86 API (INT 0xE0)
.ifndef	SYSTEM_CPM86
.equ	SYSTEM_CPM86, 0
.endif

# One of these must be set to 1 according to the format of the binary generated

# MS-DOS flat .COM file
.ifndef	FORMAT_COM
.equ	FORMAT_COM, 0
.endif

# MS-DOS executable (.EXE) with 'MZ' signature
.ifndef	FORMAT_MZ_EXE
.equ	FORMAT_MZ_EXE, 0
.endif

# Multitasking "European" MS-DOS 4.0 executable (.EXE) with 'NE' signature
.ifndef	FORMAT_NE_EXE
.equ	FORMAT_NE_EXE, 0
.endif

# CP/M-86 executable, 8080 (a.k.a. tiny) model (single segment group)
.ifndef	FORMAT_CMD_TINY
.equ	FORMAT_CMD_TINY, 0
.endif

# CP/M-86 executable, small model (separate code and data segment groups)
.ifndef	FORMAT_CMD_SMALL
.equ	FORMAT_CMD_SMALL, 0
.endif

# CP/M-86 executable, compact model (separate code and multiple data segment groups)
.ifndef	FORMAT_CMD_COMPACT
.equ	FORMAT_CMD_COMPACT, 0
.endif

	.text
	.code16

	.global	entry

entry:

# TODO: implement RSXs

.if	FORMAT_COM
	# CP/M-80 commands have the same .com file extension but incompatible binary codes
	# To avoid accidentally executing the code, a stub code is executed that also works on CP/M-80
	# This is accomplished by selecting a specific set of opcodes that execute differently on an 8080 and 8086

	jmp	1f

	# This jump instruction compiles to the bytes 0xEB, 0x03 which correspond to the following sequence of harmless instructions on an 8080/Z80:
	# xchg
	# inx b

	# Next we jump to the CP/M-80 stub
	.byte	0xC3	# jmp
	.word	cpm80_stub
1:
	jmp	msdos_entry

	# The CP/M-80 stub will display an error message and quit the program
cpm80_stub:
	# lxi d, text_cpm80_message
	.byte	0x11
	.word	text_cpm80_message
	# mvi c, 9
	.byte	0x0E
	.byte	9
	# call 5
	.byte	0xCD
	.word	5
	# rst 0
	.byte	0xC7

text_cpm80_message:
	.ascii	"This program can only be executed under MS-DOS."
	.byte	13, 10, '$'

msdos_entry:

.endif

#### Initialization

# TODO: RSX set up

.if	FORMAT_MZ_EXE
setup_ds:
	# For MZ executables, DS points to the program segment prefix (PSP) instead of our data segment base
	# We will treat the stack and data segments as identical
	push	ss
	pop	ds

store_psp:
	# Store ES:0 as a return address to exit, with [exit_address + 2] doubling as storage for the PSP segment
	# This is the most convenient way to exit from MS-DOS 1.0
	xor	ax, ax
	mov	[exit_address], ax
	mov	[psp_segment], es
.endif

.if	SYSTEM_CPM86
	# Store the initial stack address for later study
	mov	[initial_sp], sp
	mov	[initial_ss], ss

setup_stack:
	# CP/M-86 uses a limited start-up stack segment, so we switch to our own
.if	!FORMAT_CMD_COMPACT
	mov	ax, ds
	mov	ss, ax
.else
	# For compact model executables, the stack segment can be obtained from either the base page (first 256 bytes of DS) or via relocations
	# Since early CP/M-86 does not support relocations, we will rely on the value in the base page
	# Retrieve stack segment value from zero page
	mov	ss, [0x15]
.endif
	# Note: moving to SS inhibits interrupts until next instruction
	mov	sp, offset stack_top
.endif

.if	FORMAT_NE_EXE
store_env:
	# AX contains the environment block handle at start up
	# 'NE' executables don't have a program segment prefix (PSP), so we will need this value for later
	mov	[env_segment], ax
.endif

.if	SYSTEM_MSDOS
check_version:
	# http://www.ctyme.com/intr/rb-2711.htm
	# Check for DOS version: invoked via AH = 0x30
	# DOS 1.* leaves AX unchanged, so we clear AL as well, which would normally contain the major version
	# This will result in the invalid version number "0.24" that we can use to establish that this is a pre-2.* DOS
	mov	ax, 0x3000
	int	0x21

	# Is AL = 0?
	test	al, al
	jnz	1f

	# Then clear the version value to 0
	xor	ax, ax
1:
	mov	[dos_version], ax
.endif

.if	FORMAT_COM
	# Store the initial stack address for later study
	mov	[initial_sp], sp

	# Restrict memory used to only what is needed
	mov	sp, offset stack_top

release_memory:
	# When DOS loads a COM file, it preallocates all the available memory
	# If we want to allocate memory for COM files, first we must release all the memory preallocated to us by DOS
	# Reallocation calls require DOS 2+
	cmpb	[dos_version], 2
	jb	1f

	# http://www.ctyme.com/intr/rb-2936.htm
	# Resize the memory used
	mov	ah, 0x4A
	# Set ES:BX to point to the first byte after the last paragraph (i.e. 16-byte unit) used by our program
	push	cs
	pop	es
	# Align stack top to paragraph (i.e. 16-byte) boundary
	mov	bx, offset stack_top + 0xF
	mov	cl, 4
	shr	bx, cl
	int	0x21

#	jnc	1f
# TODO: handle resizing error
1:
.endif

#### Actual code

	# Friendly greeting and initial registers

	mov	si, offset text_greeting
	call	write_string_newline

	mov	si, offset text_explain_initial
	call	write_string_newline

	mov	si, offset text_cs_ip
	call	write_string
	mov	ax, cs
	call	write_word
	mov	al, ':'
	call	write_char
	# To calculate the IP, we issue a CALL instruction, which pushes the next IP value to the stack
	call	1f
1:
	pop	ax
	# Then we adjust the distance from the actual start
	add	ax, offset entry - 1b
	call	write_word
	call	write_newline

# TODO: RSX

	mov	si, offset text_ss_sp
	call	write_string
.if	SYSTEM_CPM86
	# We have modified the initial SS
	mov	ax, [initial_ss]
.else
	mov	ax, ss
.endif
	call	write_word
	mov	al, ':'
	call	write_char
.if	SYSTEM_CPM86 || FORMAT_COM
	# We have modified the initial SP
	mov	ax, [initial_sp]
.else
	mov	ax, sp
.endif
	call	write_word
	call	write_newline

	mov	si, offset text_ds
	call	write_string
.if	FORMAT_MZ_EXE
	# We have modified the initial DS
	mov	ax, [initial_ds]
.else
	mov	ax, ds
.endif
	call	write_word
	call	write_newline

	mov	si, offset text_es
	call	write_string
	mov	ax, es
	call	write_word
	call	write_newline
	xor	ax, ax
	mov	es, ax

	# Display entry points for interrupt calls

	xor	ax, ax
	mov	es, ax

	# INT 0x20 is the second shortest way to exit a .COM program on MS-DOS (the shortest is a RET instruction)
	mov	si, offset text_int20
	call	write_string
	es mov	ax, [4 * 0x20 + 2]
	call	write_word
	mov	al, ':'
	call	write_char
	es mov	ax, [4 * 0x20]
	call	write_word
	call	write_newline

	# INT 0x21 is the main entry point to the MS-DOS API
	mov	si, offset text_int21
	call	write_string
	es mov	ax, [4 * 0x21 + 2]
	call	write_word
	mov	al, ':'
	call	write_char
	es mov	ax, [4 * 0x21]
	call	write_word
	call	write_newline

	# INT 0xE0 is the entry point to the CP/M-86 API
	mov	si, offset text_intE0
	call	write_string
	es mov	ax, [4*0xE0+2]
	call	write_word
	mov	al, ':'
	call	write_char
	es mov	ax, [4*0xE0]
	call	write_word
	call	write_newline

.if FORMAT_NE_EXE
	# Multitasking "European" MS-DOS 4.0 supports dynamic linking libraries
	# We will use the DOSCALLS entry point ALLOCSEG to demonstrate the dynamic nature
	mov	si, offset text_doscalls_allocseg
	call	write_string
	mov	ax, offset $$IMPSEG$DOSCALLS$_ALLOCSEG
	call	write_word
	mov	al, ':'
	call	write_char
	mov	ax, offset $$IMPORT$DOSCALLS$_ALLOCSEG
	call	write_word
	call	write_newline
.endif

	call	waitkey

	# Display various reported system version numbers

.if SYSTEM_MSDOS
	# Some Digital Research systems permit CP/M programs to call 0x21, but others will terminate the program
	# Better be safe and not compile this into CP/M programs
print_version:
	xor	ax, ax
	mov	es, ax

	# http://www.ctyme.com/intr/rb-2711.htm
	# Get DOS version
	# Supported since DOS 2.*, this is the main way to determine the version of a DOS system, or the level of compatibility with MS-DOS for non-Microsoft DOSes
	mov	si, offset text_int21_30
	call	write_string

	mov	ax, 0x3000
	int	0x21
	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-2730.htm
	# DOS 5 - Get true version
	# Since DOS 5.0, the reported DOS version number can be altered, and this call will return the actual version number
	mov	si, offset text_int21_3306
	call	write_string

	mov	ax, 0x3306
	int	0x21
	mov	ax, bx
	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-2919.htm
	# Concurrent DOS 3.2 - Installation check
	# For concurrent/multiuser variants of Digital Research DOS, this returns the BDOS (kernel) version
	# This is different from the supported MS-DOS API compatibility version, since these two systems evolved separately
	mov	si, offset text_int21_4451
	call	write_string

	mov	ax, 0x4451
	int	0x21

	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-2920.htm
	# DR DOS 3.41 - Determine DOS type/version
	# For single user variants of Digital Research DOS, this returns the BDOS (kernel) version
	# This is different from the supported MS-DOS API compatibility version, since these two systems evolved separately
	mov	si, offset text_int21_4452
	call	write_string

	mov	ax, 0x4452
	int	0x21

	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-2926.htm, http://www.ctyme.com/intr/rb-8514.htm
	# DR Multiuser DOS 5.0 - API
	# Get BDOS version
	# The CP/M-86 API is accessible via INT 0x44, AH=0x59, with CL containing the function number
	# This returns the BDOS (kernel) version
	mov	si, offset text_int21_4459_0C
	call	write_string

	mov	ax, 0x4459
	mov	cl, 0x0C
	int	0x21

	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-2926.htm, http://www.ctyme.com/intr/rb-8569.htm
	# DR Multiuser DOS 5.0 - API
	# Get OS version
	# The CP/M-86 API is accessible via INT 0x44, AH=0x59, with CL containing the function number
	# This returns the OS version, the number used for publishing, which may be different from the kernel version number
	mov	si, offset text_int21_4459_A3
	call	write_string

	mov	ax, 0x4459
	mov	cl, 0xA3
	int	0x21

	call	write_word
	call	write_newline

	# Skip next calls if int E0 points to zero
	es mov	ax, [4*0xE0+2]
	es or	ax, [4*0xE0]
	jz	no_cpm

	# Many early DOSes have a non-zero int E0 interrupt, skip that as well (this only impacts Concurrent CP/M-86 3.1)
	cmpb	[dos_version], 2
	jb	no_cpm
.endif

	# http://www.ctyme.com/intr/rb-8514.htm
	# Get BDOS version
	mov	si, offset text_intE0_0C
	call	write_string

	mov	cl, 0x0C
	int	0xE0

	call	write_word
	call	write_newline

	# http://www.ctyme.com/intr/rb-8569.htm
	# Get OS version
	mov	si, offset text_intE0_A3
	call	write_string

	mov	cl, 0xA3
	int	0xE0

	call	write_word
	call	write_newline

no_cpm:

.if	SYSTEM_MSDOS
	# Display DOS version (either real version for MS/IBM or emulated version for DR)
	mov	si, offset text_dos_version
	call	write_string
	mov	al, [dos_version]
	call	write_digits
	mov	al, '.'
	call	write_char
	mov	al, [dos_version + 1]
	call	write_both_digits
	call	write_newline
.endif

	# Display CP/M versions
.if	SYSTEM_MSDOS
	xor	ax, ax
	mov	es, ax

	# Check if INT 0xE0 interface exists
	es mov	ax, [4*0xE0+2]
	es or	ax, [4*0xE0]
	jnz	.has_intE0

	# Check if DR-DOS version check exists
	mov	ax, 0x4452
	int	0x21
	jc	.has_no_intE0

	# Do not attempt to call INT 0xE0
	movb	[has_cpm_osver_call], 0
	# Use this value to display BDOS version
	jmp	.display_bdos_version
.endif

.has_intE0:
	mov	cl, 0xA3
	int	0xE0

	# Check if this call is implemented
	cmp	ax, 0xFFFF
	jne	.display_bdos_version

	# Fall back to other system call, do not invoke this again
	movb	[has_cpm_osver_call], 0
	mov	cl, 0x0C
	int	0xE0

.display_bdos_version:
	push	ax
	mov	si, offset text_cpm_version
	call	write_string

	# Display reported version number
	pop	ax
	push	ax
	mov	cl, 4
	shr	ax, cl
	call	write_nibble

	mov	al, '.'
	call	write_char

	pop	ax
	push	ax
	call	write_nibble

	# Distinguish between single user, multitasking and multiuser versions
	pop	ax
	push	ax
	test	ah, 0x04
	jz	.non_multiuser
	mov	si, offset text_multiuser_cpm
	call	write_string
	jmp	.after_sysname

.non_multiuser:
	test	ah, 0x01
	jz	.non_mpm
	mov	si, offset text_mpm
	call	write_string
	jmp	.after_sysname

.non_mpm:
	mov	si, offset text_cpm
	call	write_string
	jmp	.after_sysname

.after_sysname:
	# Check if CP/Net is available
	pop	ax
	push	ax
	test	ah, 0x02
	jnz	.non_cpnet
	mov	si, offset text_cpnet
.non_cpnet:

	# Compare with BDOS version
	mov	al, [has_cpm_osver_call]
	test	al, al
	jz	.only_bdos_version

	mov	cl, 0x0C
	int	0xE0
	pop	bx
	cmp	ax, bx
	je	.same_bdos_version

	# Display BDOS version separately
	push	ax
	mov	si, offset text_bdos_version
	call	write_string
	pop	ax
	push	ax

	mov	cl, 4
	shr	ax, cl
	call	write_nibble

	mov	al, '.'
	call	write_char

	pop	ax
	call	write_nibble

.only_bdos_version:
.same_bdos_version:
	call	write_newline
.has_no_intE0:

	call	waitkey

.if	!FORMAT_NE_EXE
	# Display the program segment prefix (PSP) for MS-DOS, the base page for CP/M-86
	# Note that 'NE' executables don't have a PSP
.if	FORMAT_MZ_EXE
	mov	ax, [psp_segment]
.endif

	xor	si, si
1:
	lodsb
	push	si
	call	write_byte
	pop	si
	test	si, 0xF
	jz	2f
	push	si
	mov	al, ' '
	call	write_char
	pop	si
	jmp	1b
2:
	test	si, 0xFF
	jz	4f

	test	si, 0x70
	push	si
	call	write_newline
	jnz	3f
	call	waitkey
3:
	pop	si
	jmp	1b
4:

	call	write_newline
	call	waitkey

.if	FORMAT_MZ_EXE
	# Restore DS
	push	ss
	pop	ds
.endif
.endif

	# Environment and command line

.if	SYSTEM_MSDOS
	# The following code will only be compiled for MS-DOS, not CP/M-86

print_environment:
	# We now print the environment variables
.if	!FORMAT_NE_EXE
	# But only version 2 and later have an environment block segment
	cmpb	[dos_version], 2
	jb	after_print_environment
.endif

	mov	si, offset text_envseg
	call	write_string

.if	FORMAT_NE_EXE
	# NE executables receive the environment segment handle at start up via the AX register
	mov	ax, [env_segment]
.else
	# Otherwise, we fetch the environment block segment from the PSP
.if	FORMAT_MZ_EXE
	mov	es, [psp_segment]
	es mov	ax, [0x2C]
.else
	mov	ax, [0x2C]
.endif	# FORMAT_MZ_EXE
.endif

	call	write_word
	call	write_newline

	mov	si, offset text_environment
	call	write_string_newline

.if	FORMAT_NE_EXE
	# In Multitasking MS-DOS, to access the environment segment, first we need to call the LOCKSEG function
	# This takes a segment handle in DX and returns a segment value in ES
	mov	dx, [env_segment]
	# https://betawiki.net/wiki/Multitasking_MS-DOS_4
	# Far call to LOCKSEG
	.byte	0x9A
	.word	$$IMPORT$DOSCALLS$_LOCKSEG
	.word	$$IMPSEG$DOSCALLS$_LOCKSEG
.else
	# Otherwise, fetch the environment block segment from the program segment prefix (PSP)
.if	FORMAT_MZ_EXE
	mov	es, [psp_segment]
	es mov	es, [0x2C]
.else
	mov	es, [0x2C]
.endif	# FORMAT_MZ_EXE
.endif

	# Print environment values of the form "KEY=VALUE\0"
	xor	di, di
1:
	es mov	al, [di]
	inc	di
	test	al, al
	jz	3f
2:
	call	write_char
	jmp	1b
3:
	call	write_newline
	es mov	al, [di]
	inc	di
	test	al, al
	jnz	2b

.if	!FORMAT_NE_EXE
	# Only version 3 and later store argv0 in the environment block segment
	cmpb	[dos_version], 3
	jb	after_print_environment
.endif

	# Skip two null bytes
	inc	di
	inc	di

print_program_name:
	mov	si, offset text_program_name
	call	write_string

1:
	es mov	al, [di]
	test	al, al
	jz	2f
	inc	di
	call	write_char
	jmp	1b
2:
	mov	al, '"'
	call	write_char
	call	write_newline

after_print_environment:
.endif
	# The following code is compiled for both MS-DOS and CP/M-86

	# The command line is stored starting at offset 0x80 (length, data) in the PSP (for MS-DOS) or in the zero page (CP/M-86)
	# Multitasking MS-DOS 4 stores it in the environment block
	mov	si, offset text_command_line
	call	write_string

.if	!FORMAT_NE_EXE
.if	FORMAT_MZ_EXE
	mov	ds, [psp_segment]
.endif
	# The first byte is an 8-bit value representing the length of the command line
	# The first actual character in the command line is at offset 0x81
	mov	si, 0x80
	lodsb
	test	al, al
	jz	2f
	mov	cl, al
	mov	ch, 0x00
1:
	lodsb
	push	cx
	call	write_char
	pop	cx
	loop	1b
2:
.if	FORMAT_MZ_EXE
	# restore DS
	push	ss
	pop	ds
.endif
.else # FORMAT_NE_EXE
	# The actual command line follows the command name, separated by a null character
	inc	di
4:
	es mov	al, [di]
	test	al, al
	jz	5f
	inc	di
	call	write_char
	jmp	4b
5:
.endif

.if	FORMAT_NE_EXE
	# Release the environment segment, we will not need it any longer
	mov	dx, [env_segment]
	# https://betawiki.net/wiki/Multitasking_MS-DOS_4
	# Far call to UNLOCKSEG
	.byte	0x9A
	.word	$$IMPORT$DOSCALLS$_UNLOCKSEG
	.word	$$IMPSEG$DOSCALLS$_UNLOCKSEG
.endif

	mov	al, '"'
	call	write_char
	call	write_newline

	call	waitkey

	# Display message directly on screen

	mov	si, offset text_detecting_video
	call	write_string

	# Check if this is an IBM PC by observing the memory addresses 0xFFFF7 and 0xFFFFA
	mov	ax, 0xFFFF
	mov	es, ax

	es cmpb	[0x7], '/'
	jne	not_ibm_pc
	es cmpb	[0xA], '/'
	jne	not_ibm_pc

is_ibm_pc:
	# Check if it uses a color monitor (CGA or compatible) or monochrome monitor (MDA/Hercules)
	# by observing the memory address 0x00449
	xor	ax, ax
	mov	es, ax
	es cmpb	[0x449], 0x07
	jne	is_cga
is_mda:
	mov	si, offset text_mda
	call	write_string_newline

	mov	ax, 0xB000
	jmp	is_mda_or_cga
is_cga:
	mov	si, offset text_cga
	call	write_string_newline

	mov	ax, 0xB800
is_mda_or_cga:
	mov	es, ax
	mov	si, offset text_direct_greetings
	# line 5 to avoid issues with scrolling
	mov	di, 10 * 160 + 8
	mov	ah, 0x1E

1:
	lodsb
	test	al, al
	jz	2f
	stosw
	jmp	1b
2:
	jmp	done

not_ibm_pc:
	# Check if this is a NEC PC-98 by looking at the word at address 0xFFFF3
	es cmpw	[0x3], 0xFD80
	jne	is_unknown

is_nec_pc98:
	mov	si, offset text_pc98
	call	write_string_newline

	mov	ax, 0xA000
	mov	es, ax
	mov	si, offset text_direct_greetings
	# line 5 to avoid issues with scrolling
	mov	di, 5 * 160
	xor	ax, ax

1:
	lodsb
	test	al, al
	jz	2f
	stosw
	jmp	1b
2:
	mov	cx, si
	sub	cx, offset text_direct_greetings + 1
	mov	di, 0x2000 + 5 * 160
	mov	al, 0xC5
	rep	stosw

	jmp	done

is_unknown:
	mov	si, offset text_video_unknown
	call	write_string_newline

done:

	call	waitkey

	call	exit

#### Support calls

exit:
.if	FORMAT_COM
	int	0x20
.elseif	FORMAT_MZ_EXE
	# DOS 2+ supports the int 0x21/AH=0x4C call, earlier versions would return
	mov	ax, 0x4C00
	int	0x21
	# The int 0x20 call does not work, becaues CS has to point to the PSP
	# The easiest way is to jump to PSP:0, which is stored at the exit_address variable
	call	far [exit_address]
.elseif	FORMAT_NE_EXE
	# NE executables only support the DOS 2+ method of exit
	mov	ax, 0x4C00
	int	0x21
.elseif	SYSTEM_CPM86
	xor	cl, cl
	int	0xE0
.endif

# Prints the contents of AL as a decimal value
write_both_digits:
	# Reset CF and AF
	subb	ah, ah
	# Convert to decimal
	aam	10
	push	ax
	mov	al, ah
	call	write_nibble
	pop	ax
	jmp	write_nibble

# Prints the contents of AL as a decimal value
write_digits:
	# Reset CF and AF
	sub	ah, ah
	# Convert to decimal
	aam	10

	test	ah, ah
	jz	write_nibble

	push	ax
	mov	al, ah
	call	write_nibble
	pop	ax
	jmp	write_nibble

write_string:
	lodsb
	test	al, al
	jz	1f
	call	write_char
	jmp	write_string
1:
	ret

write_string_newline:
	call	write_string
#	jmp	write_newline

write_newline:
	mov	al, 13
	call	write_char
	mov	al, 10
	jmp	write_char

write_word:
	push	ax
	mov	al, ah
	call	write_byte
	pop	ax

write_byte:
	push	ax
	mov	cl, 4
	shr	al, cl
	call	write_nibble
	pop	ax

write_nibble:
	and	al, 0xF
	cmp	al, 10
	jae	1f
	add	al, '0'
	jmp	2f
1:
	add	al, 'A'-10
2:

write_char:
.if	SYSTEM_MSDOS
	mov	dl, al
	mov	ah, 0x06
	int	0x21
.elseif	SYSTEM_CPM86
	mov	dl, al
	mov	cl, 0x06
	int	0xE0
.endif
	ret

waitkey:
	mov	si, offset text_waitkey
	call	write_string_newline
.if	SYSTEM_MSDOS
	mov	ah, 0x01
	int	0x21
.elseif	SYSTEM_CPM86
	mov	cl, 0x01
	int	0xE0
.endif
	ret

	.data
text_greeting:
	.ascii	"Greetings!"
	.byte	13, 10

.if	FORMAT_COM
	.ascii	" * MS-DOS flat .COM file *"
.elseif	FORMAT_MZ_EXE
	.ascii	" * MS-DOS 'MZ' .EXE file *"
	#, no relocations, 512 byte header)"
.elseif	FORMAT_CMD_TINY
	.ascii	" * CP/M-86 8080 model .CMD file *"
	#, no relocations)"
.elseif	FORMAT_CMD_SMALL
	.ascii	" * CP/M-86 small model .CMD file *"
	#, no relocations)"
.elseif	FORMAT_CMD_COMPACT
	.ascii	" * CP/M-86 compact model .CMD file *"
	#, no relocations)"
.elseif	FORMAT_NE_EXE
	.ascii	" * Multitasking MS-DOS 4.0 'NE' .EXE file *"
.endif
	.byte	0

text_waitkey:
	.ascii	" ~ Press a key ~"
	.byte	0

text_explain_initial:
	.ascii	"Initial register values:"
	.byte	0

text_cs_ip:
	.ascii	"Initial CS:IP:     "
	.byte	0

text_ss_sp:
	.ascii	"Initial SS:SP:     "
	.byte	0

text_ds:
	.ascii	"Initial DS:        "
	.byte	0

text_es:
	.ascii	"Initial ES:        "
	.byte	0

text_command_line:
	.ascii	"Command line: "
	.byte	'"', 0

.if	SYSTEM_MSDOS
text_environment:
	.ascii	"Environment:"
	.byte	0

.if	!FORMAT_NE_EXE
text_envseg:
	.ascii	"Environment block segment: "
	.byte	0
.else
text_envseg:
	.ascii	"Environment handle: "
	.byte	0
.endif

text_program_name:
	.ascii	"Program name: "
	.byte	'"', 0
.endif

text_int20:
	.ascii	"INT 0x20 entry:    "
	.byte	0

text_int21:
	.ascii	"INT 0x21 entry:    "
	.byte	0

text_intE0:
	.ascii	"INT 0xE0 entry:    "
	.byte	0

.if	FORMAT_NE_EXE
text_doscalls_allocseg:
	.ascii	"DOSCALLS.ALLOCSEG: "
	.byte	0
.endif

.if	SYSTEM_MSDOS
text_int21_30:
	.ascii	"INT 21, AH=30 - DOS get version: "
	.byte	0

text_int21_3306:
	.ascii	"INT 21, AX=3306 - Get true version: "
	.byte	0

text_int21_4451:
	.ascii	"INT 21, AX=4451 - Concurrent DOS installation check: "
	.byte	0

text_int21_4452:
	.ascii	"INT 21, AX=4452 - DR DOS get DOS version: "
	.byte	0

text_int21_4459_0C:
	.ascii	"INT 21, AX=4459, CL=0C - Get BDOS version: "
	.byte	0

text_int21_4459_A3:
	.ascii	"INT 21, AX=4459, CL=A3 - Multiuser DOS get OS version: "
	.byte	0
.endif

text_intE0_0C:
	.ascii	"INT E0, CL=0C - Get BDOS version: "
	.byte	0

text_intE0_A3:
	.ascii	"INT E0, CL=A3 - Get OS version: "
	.byte	0

.if	SYSTEM_MSDOS
text_dos_version:
	.ascii	"PC DOS version: "
	.byte	0
.endif

text_cpm_version:
	.ascii	"CP/M version: "
	.byte	0

text_multiuser_cpm:
	.ascii	", concurrent/multiuser"
	.byte	0

text_mpm:
	.ascii	", multitasking"
	.byte	0

text_cpm:
	.ascii	", single user"
	.byte	0

text_cpnet:
	.ascii	"with CP/Net "
	.byte	0

text_bdos_version:
	.ascii	", BDOS version: "
	.byte	0

text_detecting_video:
	.ascii	"Detected video type: "
	.byte	0

text_mda:
	.ascii	"IBM PC monochrome (MDA/Hercules)"
	.byte	0

text_cga:
	.ascii	"IBM PC color (CGA or compatible)"
	.byte	0

text_pc98:
	.ascii	"NEC PC-98"
	.byte	0

text_video_unknown:
	.ascii	"unknown"
	.byte	0

text_direct_greetings:
	.ascii	"Direct video access"
	.byte	0

has_cpm_osver_call:
	.byte	1

	.bss

.if	FORMAT_MZ_EXE
exit_address:
	.skip	2
psp_segment:
initial_ds:
	.skip	2
.endif

.if	FORMAT_NE_EXE
env_segment:
	.skip	2
.endif

.if	SYSTEM_MSDOS
dos_version:
	.skip	2
.endif

.if	SYSTEM_CPM86 || FORMAT_COM
initial_sp:
	.skip	2
.endif

.if	SYSTEM_CPM86
initial_ss:
	.skip	2
.endif

	.section .stack, "aw", @nobits

	.skip	0x100
	.align	4
	.global	stack_top
stack_top:

