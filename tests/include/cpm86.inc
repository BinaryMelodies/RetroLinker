
.macro	_StartUp
.if FORMAT_CMD_TINY
.if MODEL_SMALL
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
.elseif MODEL_COMPACT
	mov	ax, cs
	add	ax, offset $$SEGDIF$.data$.text
	mov	ds, ax
# TODO: set up stack
.endif
.elseif FORMAT_CMD_SMALL
.if MODEL_COMPACT
# TODO: set up stack
.endif
.endif
.endm

.macro	_Exit
	mov	cl, 0
	int	0xE0
.endm

.macro	_PutChar char
	mov	dl, \char
	mov	cl, 0x02
	int	0xE0
.endm

.macro	_WaitForKey
	mov	cl, 0x01
	int	0xE0
.endm

.macro	_SysVars
.endm

