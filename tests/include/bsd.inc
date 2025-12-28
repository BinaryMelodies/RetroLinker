
.macro	_StartUp
.endm

.macro	_Exit
	mov $0, -(sp)
	clr -(sp)
	sys 1
.endm

.macro	_PutChar	char
putchar:
	movb \char, buffer
	mov $1, -(sp)
	mov $buffer, -(sp)
	mov $1, -(sp)
	clr -(sp)
	sys 4
	add $8, sp
.endm

.macro	_WaitForKey
	# TODO
.endm

.macro	_SysVars
	.data

buffer:
	.byte	0
.endm

