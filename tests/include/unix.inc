
# On PDP-11, the parameters are typically stored after the sys instruction
# Later versions support a redirecting call using sys 0, but the inline method works on all systems
# As an exception, IMAGIC binaries cannot use the old style calls, so we need to use indirect system calls

# On VAX however, arguments are passed on the stack

.macro	_StartUp
.endm

.macro	_Exit
	# sys_exit
.if	CPU_PDP11
	mov	$0, r0
	sys	1
.elseif	CPU_VAX
	pushl $0
	# imitate a callg $1, _exit
	pushl $1
	movl %sp, %ap
	chmk $1
.endif
.endm

.macro	_PutChar	char
	# sys_write
.if	CPU_PDP11
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
.elseif	CPU_VAX
	movb	\char, PutChar_char
	pushl	$1
	pushal	PutChar_char
	pushl	$1
	# imitate a callg $3, write
	pushl	$3
	movl	%sp, %ap
	chmk	$4
	addl2	$16, %sp
.endif
.endm

.macro	_WaitForKey
	# sys_read
.if	CPU_PDP11
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
.elseif	CPU_VAX
	pushl	$1
	pushal	PutChar_char
	pushl	$1
	# imitate a callg $3, write
	pushl	$3
	movl	%sp, %ap
	chmk	$3
	addl2	$16, %sp
.endif
.endm

.macro	_SysVars
	.data

.if	FORMAT_IMAGIC
	# indirect system call parameter lists for PDP-11
PutChar_SysCall:
	sys	4
	.word	PutChar_char
	.word	1
WaitForKey_SysCall:
	sys	3
	.word	PutChar_char
	.word	1
.endif

	# buffer for sys_write
PutChar_char:
	.byte	0
.endm

