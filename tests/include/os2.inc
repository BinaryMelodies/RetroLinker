
.macro	_StartUp
.endm

.macro	_Exit
.if	TARGET_OS2V1
	pushw	1 # EXIT_PROCESS
	pushw	0 # status
	# DosExit
	.byte	0x9A
	.extern	$$IMPORT$DOSCALLS$0005, $$IMPSEG$DOSCALLS$0005
	.word	$$IMPORT$DOSCALLS$0005
	.word	$$IMPSEG$DOSCALLS$0005
.endif
.if	TARGET_OS2V2
	push	0 # status
	push	1 # EXIT_PROCESS
	# DosExit
	.extern	$$IMPORT$DOSCALLS$00EA
	call	$$IMPORT$DOSCALLS$00EA
.endif
.endm

.macro	_PutChar char
.if	TARGET_OS2V1
	mov	PutChar_char, \char

	push	ax # temporary
	push	ax
	mov	ax, sp

	pushw	1 # stdout?
	push	ds
	push	offset PutChar_char
	push	1
	push	ss
	push	ax
	# DosWrite
	.byte	0x9A
	.extern	$$IMPORT$DOSCALLS$008A, $$IMPSEG$DOSCALLS$008A
	.word	$$IMPORT$DOSCALLS$008A
	.word	$$IMPSEG$DOSCALLS$008A

	pop	ax
	pop	ax
.endif
.if	TARGET_OS2V2
	mov	PutChar_char, \char

	push	eax # temporary
	mov	eax, esp

	push	eax
	push	1
	push	offset PutChar_char
	push	1 # stdout?
	# DosWrite
	.extern	$$IMPORT$DOSCALLS$011A
	call	$$IMPORT$DOSCALLS$011A

	add	esp, 16

	pop	eax
.endif
.endm

.macro	_WaitForKey
	# TODO
.endm

.macro	_SysVars
	.section	.bss
PutChar_char:
	.byte	0
.endm

