
.macro	_StartUp
.endm

.macro	_Exit
	mov	ax, 0x4C00
	int	0x21
.endm

.macro	_PutChar char
	mov	PutChar_char, \char
	mov	dx, offset PutChar_char
	mov	ah, 0x09
	int	0x21
.endm

.macro	_WaitForKey
	mov	ah, 0x01
	int	0x21
.endm

.macro	_SysVars
	.section	.data

	.rept	16
	.byte	0
	.endr

PutChar_char:
	.byte	0, '$'
.endm

