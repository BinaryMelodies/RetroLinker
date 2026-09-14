
.macro	_StartUp
.endm

.macro	_Syscall
.if TARGET_MSXDOS
	# MSX-DOS also supports the CP/M call interface
	# however we can show off the MSX-DOS specific entry point
	# (identical behavior)
	call	0xF37D
.else
	call	5
.endif
.endm

.macro	_Exit
.if TARGET_MSXDOS
	# MSX-DOS specific exit, with status code stored in B
	# (similar to MS-DOS AH=0x4C)
	ld	b, 0
	ld	c, 0x62
	_Syscall
.else
	ld	c, 0
	_Syscall
.endif
.endm

.macro	_PutChar	char
	ld	e, \char
	ld	c, 0x02
	_Syscall
.endm

.macro	_WaitForKey
	# CP/M conin
	ld	c, 0x01
	_Syscall
.endm

.macro	_SysVars
.endm

