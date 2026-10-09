
.macro	__ExitToShell
	.short	0xA9F4
.endm

	.section	.init, "ax", @progbits

	.global	init
init:
	__ExitToShell

