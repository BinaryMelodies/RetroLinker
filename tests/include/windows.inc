
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
	# align stack pointer
	sub	rsp, 8
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
	# TODO
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
	# TODO
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

.if	TARGET_WIN32
	.global	hStdOut
hStdOut:
	.long	0
NumberOfBytesWritten:
	.long	0
.endif

PutChar_char:
	.byte	0
.if	TARGET_WIN16
	.byte	'$'
.endif
.endm

