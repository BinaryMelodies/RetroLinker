
.set	SystemBase, 4
.set	OpenLibrary, -552
.set	Open, -30
.set	Write, -48

.macro	_StartUp
	move.l	sp, ExitStack.l

	lea	text_dos_library, a1
	move.l	#0, d0
	move.l	SystemBase, a6
	jsr	OpenLibrary(a6)
	move.l	d0, (DosHandle).l

	move.l	d0, a6
	lea	text_console, a0
	move.l	a0, d1
	move.l	#1005, d2
	jsr	Open(a6)
	move.l	d0, (ConsoleHandle).l
.endm

.macro	_Exit
	move.l	ExitStack.l, sp
	rts
.endm

.macro	_PutChar char
	move.b	\char, (CharBuffer).l
	move.l	(ConsoleHandle).l, d1
	lea	(CharBuffer).l, a0
	move.l	a0, d2
	move.l	#1, d3
	move.l	(DosHandle).l, a0
	jsr	Write(a0)
.endm

.macro	_WaitForKey
# TODO
.endm

.macro	_LoadA	ref, dst
	move.l	#\ref, \dst
.endm

.macro	_LoadL	ref, dst
	move.l	\ref, \dst
.endm

.macro	_StoreL	src, ref
	move.l	\src, \ref
.endm

.macro	_SysVars
	.section	.data
	.global	text_dos_library
text_dos_library:
	.asciz	"dos.library"
	.global	text_console
text_console:
	.asciz	"CONSOLE:"
	.align	4

	.section	.bss
	.global	ExitStack
ExitStack:
	.long	0
	.global	DosHandle
DosHandle:
	.long	0
	.global	ConsoleHandle
ConsoleHandle:
	.long	0
CharBuffer:
	.byte	0
	.align	4
.endm

