
.ifndef OPTION_GUI_ONLY
.set OPTION_GUI_ONLY, 0
.endif

.ifndef OPTION_IMPORT_THUNKS
.set OPTION_IMPORT_THUNKS, 0
.endif

#.if	TARGET_WIN32
#	.extern	hStdOut
#.endif

.macro	_StartUp
.if	TARGET_WIN16
	# InitTask
	.extern	$$IMPORT$KERNEL$005B, $$IMPSEG$KERNEL$005B
	.byte	0x9A
	.word	$$IMPORT$KERNEL$005B
	.word	$$IMPSEG$KERNEL$005B

	test	ax, ax
	jnz	1f

	mov	ax, 0x4C01
	int	0x21
1:

	# hInstance, used by InitApp
	push	di

	mov	ax, 0
	push	ax
	# WaitEvent
	.extern	$$IMPORT$KERNEL$001E, $$IMPSEG$KERNEL$001E
	.byte	0x9A
	.word	$$IMPORT$KERNEL$001E
	.word	$$IMPSEG$KERNEL$001E

	# InitApp
	.extern	$$IMPORT$USER$0005, $$IMPSEG$USER$0005
	.byte	0x9A
	.word	$$IMPORT$USER$0005
	.word	$$IMPSEG$USER$0005
.endif
.if	TARGET_WIN32
.if	!OPTION_GUI_ONLY
	.extern	$$IMPORT$KERNEL32.dll$GetStdHandle$0000

	push	-11
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$GetStdHandle$0000
.else
	call	[$$IMPORT$KERNEL32.dll$GetStdHandle$0000]
.endif
	mov	[hStdOut], eax
.endif
.endif
.if	TARGET_WIN64
.if	!OPTION_GUI_ONLY
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, -11
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$GetStdHandle$0000
.else
	call	[rip + $$IMPORT$KERNEL32.dll$GetStdHandle$0000]
.endif
	mov	[rip + hStdOut], rax

	mov	rsp, rbp
	pop	rbp
.endif
.endif
.endm

.macro	_Exit
.if	TARGET_WIN16
	mov	ax, 0x4C00
	int	0x21
.endif
.if	TARGET_WIN32
	.extern	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	push	0
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$ExitProcess$0000
.else
	call	[$$IMPORT$KERNEL32.dll$ExitProcess$0000]
.endif
.endif
.if	TARGET_WIN64
	.extern	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, 0
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$ExitProcess$0000
.else
	call	[rip + $$IMPORT$KERNEL32.dll$ExitProcess$0000]
.endif
.endif
.endm

.if	!OPTION_GUI_ONLY
.macro	_PutChar char
.if	TARGET_WIN16
	mov	PutChar_char, \char
	mov	dx, offset PutChar_char
	mov	ah, 0x09
	int	0x21
.endif
.if	TARGET_WIN32
	.extern	$$IMPORT$KERNEL32.dll$WriteFile$0000

	movb	PutChar_char, \char
	push	0
	push	offset NumberOfBytesWritten
	push	1
	push	offset PutChar_char
	push	[hStdOut]
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$WriteFile$0000
.else
	call	[$$IMPORT$KERNEL32.dll$WriteFile$0000]
.endif
.endif
.if	TARGET_WIN64
	.extern	$$IMPORT$KERNEL32.dll$WriteFile$0000

	movb	[rip + PutChar_char], \char

	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 8
	push	0

	# shadow space
	sub	rsp, 0x20
	lea	r9, [rip + NumberOfBytesWritten]
	mov	r8d, 1
	lea	rdx, [rip + PutChar_char]
	mov	rcx, [rip + hStdOut]
.if	OPTION_IMPORT_THUNKS
	call	$$IMPORT$KERNEL32.dll$WriteFile$0000
.else
	call	[rip + $$IMPORT$KERNEL32.dll$WriteFile$0000]
.endif

	mov	rsp, rbp
	pop	rbp
.endif
.endm
.endif

.macro	_WaitForKey
.if	TARGET_WIN16
	# TODO
	#mov	ah, 0x01
	#int	0x21
.endif
.if	TARGET_WIN32
	# TODO
.endif
.if	TARGET_WIN64
	# TODO
.endif
.endm

.macro	_SysVars
.if	TARGET_WIN16
	.section	.beg.data, "aw", @progbits

	# Instance data
	.rept	16
	.byte	0
	.endr
.endif

	.section	.data

.if	!OPTION_GUI_ONLY
.if	TARGET_WIN32 || TARGET_WIN64
	.global	hStdOut
hStdOut:
.if	TARGET_WIN32
	.long	0
.elseif	TARGET_WIN64
	.quad	0
.endif
NumberOfBytesWritten:
	.long	0
.endif

PutChar_char:
	.byte	0
.if	TARGET_WIN16
	.byte	'$'
.endif
.endif
.endm

