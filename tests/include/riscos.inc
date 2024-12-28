
.macro	_StartUp
.endm

.macro	_Exit
	mov	r0, #0
	swi	0x11
.endm

.macro	_PutChar char
	mov	r0, \char
	swi	0x00
.endm

.macro	_WaitForKey
	swi	0x04
.endm

.macro	_SysVars
.endm

