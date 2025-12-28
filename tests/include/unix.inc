
.macro	_StartUp
.endm

.macro	_Exit
	mov	$0, r0
	sys	1
.endm

.macro	_PutChar	char
	movb	\char, PutChar_char
.if	FORMAT_IMAGIC
	mov	$1, r0
	sys	0
	.word	PutChar_SysCall
.else
	mov	$1, r0
	sys	4
	.word	PutChar_char
	.word	1
.endif
.endm

.macro	_WaitForKey
.if	FORMAT_IMAGIC
	mov	$0, r0
	sys	0
	.word	WaitForKey_SysCall
.else
	mov	$1, r0
	sys	3
	.word	PutChar_char
	.word	1
.endif
.endm

.macro	_SysVars
	.data

.if	FORMAT_IMAGIC
PutChar_SysCall:
	sys	4
	.word	PutChar_char
	.word	1
WaitForKey_SysCall:
	sys	3
	.word	PutChar_char
	.word	1
.endif

PutChar_char:
	.byte	0
.endm

