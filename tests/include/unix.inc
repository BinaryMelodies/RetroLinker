
.macro	_StartUp
.endm

.macro	_Exit
	mov $0, r0
	sys 1
.endm

.macro	_PutChar	char
putchar:
	movb \char, buffer
.if	FORMAT_IMAGIC
	mov $1, r0
	sys 0
	.word write_data
.else
	mov $1, r0
	sys 4
	.word buffer
	.word 1
.endif
.endm

.macro	_WaitForKey
	# TODO
.endm

.macro	_SysVars
	.data

.if	FORMAT_IMAGIC
	.word	0
write_data:
	sys 4
	.word buffer
	.word 1
.endif

buffer:
	.byte	0
.endm

