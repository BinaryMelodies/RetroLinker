
# 2.11BSD call sequences use the stack to pass arguments instead of the PDP-11 UNIX sequence
# For VAX, see unix.inc

.macro	_StartUp
.endm

.macro	_Exit
	# sys_exit
	mov	$0, -(sp)
	clr	-(sp)
	sys	1
.endm

.macro	_PutChar	char
	# sys_write
	movb	\char, PutChar_char
	mov	$1, -(sp)
	mov	$PutChar_char, -(sp)
	mov	$1, -(sp)
	clr	-(sp)
	sys	4
	add	$8, sp
.endm

.macro	_WaitForKey
	# sys_read
	mov	$1, -(sp)
	mov	$PutChar_char, -(sp)
	mov	$1, -(sp)
	clr	-(sp)
	sys	3
	add	$8, sp
.endm

.macro	_SysVars
	.data

	# buffer for sys_write
PutChar_char:
	.byte	0
.endm

