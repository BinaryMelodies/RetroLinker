
.if	TARGET_WIN32
	.extern	hStdOut
.endif

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
	.extern	$$IMPORT$KERNEL32.dll$GetStdHandle$0000

	push	-11
	call	[$$IMPORT$KERNEL32.dll$GetStdHandle$0000]
	mov	[hStdOut], eax
.endif
.if	TARGET_WIN64
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, -11
	call	[rip + $$IMPORT$KERNEL32.dll$GetStdHandle$0000]
	mov	[rip + hStdOut], rax
	# shadow space
	add	rsp, 0x20

	mov	rsp, rbp
	pop	rbp
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
	call	[$$IMPORT$KERNEL32.dll$ExitProcess$0000]
.endif
.if	TARGET_WIN64
	.extern	$$IMPORT$KERNEL32.dll$ExitProcess$0000

	and	rsp, ~0xF

	# shadow space
	sub	rsp, 0x20
	mov	ecx, 0
	call	[rip + $$IMPORT$KERNEL32.dll$ExitProcess$0000]
.endif
.endm

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
	call	[$$IMPORT$KERNEL32.dll$WriteFile$0000]
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
	call	[rip + $$IMPORT$KERNEL32.dll$WriteFile$0000]

	mov	rsp, rbp
	pop	rbp
.endif
.endm

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
	.section	.beg.data, "aw", @progbits

.if	TARGET_WIN16
	# Instance data
	.rept	16
	.byte	0
	.endr
.endif

	.section	.data

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
.endm

