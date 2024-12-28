
.macro	_StartUp
#	mov	$.text, sp # TODO: this is bugged, .text resolves to the absolute 0
	mov	$0x200, sp
# Alternatively, move pc to sp
#	mov	pc, sp
#0:
#	sub	$0b, sp
#	add	$.text, sp
.endm

.macro	_Exit
	iot
	.word	0
.endm

.macro	_PutChar	char
	iot
	.word	2
.endm

.macro	_WaitForKey
	iot
	.word	1
.endm

.macro	_SysVars
.endm

