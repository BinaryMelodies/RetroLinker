
.macro	_StartUp
.endm

.macro	_Exit
.if TARGET_MSXDOS
	ld	b, 0
	ld	c, 0x62
	call	0xF37D
.else
	ld	c, 0
	call	5
.endif
.endm

.macro	_PutChar	char
	ld	e, \char
	ld	c, 0x02
.if TARGET_MSXDOS
	call	0xF37D
.else
	call	5
.endif
.endm

.macro	_WaitForKey
	ld	c, 0x01
.if TARGET_MSXDOS
	call	0xF37D
.else
	call	5
.endif
.endm

.macro	_SysVars
.endm

