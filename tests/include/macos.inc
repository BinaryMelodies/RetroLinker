
.macro	__InitGraf
	.short	0xA86E
.endm
.macro	__InitFonts
	.short	0xA8FE
.endm
.macro	__InitWindows
	.short	0xA912
.endm
.macro	__InitCursor
	.short	0xA850
.endm
.macro	__DrawChar
	.short	0xA883
.endm
.macro	__TextMode
	.short	0xA889
.endm
.macro	__MoveTo
	.short	0xA893
.endm
.macro	__GetNextEvent
	.short	0xA970
.endm
.macro	__ExitToShell
	.short	0xA9F4
.endm

.set	fontHeight, 15

.macro	_StartUp
	pea	-4(a5)
	__InitGraf

	__InitFonts
	__InitWindows
	__InitCursor

# TextMode(srcCopy)
	clr.w	-(sp)
	__TextMode

	clr.w	-(sp)
	move.w	#32, -(sp)
	__MoveTo
	lea	current_row(a5), a0
	move.w	#32, (a0)
.endm

.macro	_Exit
	__ExitToShell
.endm

.macro	_PutChar char
	move.w	\char, -(sp)
	__DrawChar
.endm

.macro	_PutNewLine
	clr.w	-(sp)
	lea	current_row(a5), a0
	add.w	#fontHeight, (a0)
	move.w	(a0), -(sp)
	__MoveTo
.endm

.macro	_WaitForKey
1:
	subq.l	#2, sp
# everyEvent
	move.w	#-1, -(sp)
# event structure
	pea	event(a5)
	__GetNextEvent

	move.b	(sp), d0
	addq.l	#2, sp
	tst.b	d0
	beq	1b

	move.w	event(a5), d0
	cmp.w	#3, d0
	bne	1b
.endm

.macro	_LoadA	ref, dst
	lea	\ref(pc), \dst
.endm

.macro	_LoadL	ref, dst
	move.l	\ref(pc), \dst
.endm

.macro	_StoreL	src, ref
	lea	\ref(pc), a0
	move.l	\src, (a0)
.endm

.macro	_SysVars
	.section	.a5world, "aw", @nobits
	.global	current_row
current_row:
	.skip	2

event:
	.skip	16

# QD globals
	.skip	248
.endm

