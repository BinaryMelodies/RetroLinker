
# For VAX, see unix.inc

.macro	_StartUp
.endm

.macro	_Exit
	mov	$0, -(sp)
	clr	-(sp)
	sys	1
.endm

.macro	_PutChar	char
	movb	\char, PutChar_char
	mov	$1, -(sp)
	mov	$PutChar_char, -(sp)
	mov	$1, -(sp)
	clr	-(sp)
	sys	4
	add	$8, sp
.endm

.macro	_WaitForKey
	mov	$1, -(sp)
	mov	$PutChar_char, -(sp)
	mov	$1, -(sp)
	clr	-(sp)
	sys	3
	add	$8, sp
.endm

.macro	_SysVars
	.data

PutChar_char:
	.byte	0
.endm

