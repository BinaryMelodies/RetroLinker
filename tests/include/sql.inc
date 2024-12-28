
.macro	_StartUp
	move.l	sp, ExitStack.l
.endm

.macro	_Exit
	clr.l	d0
	move.l	ExitStack.l, sp
	rts
.endm

.macro	_PutChar char
	move.l	\char, d1
	and.l	#0xFF, d1
# channel ID
	move.l	#0x00010001, a0
# timeout
	moveq.l	#-1, d3
	moveq.l	#5, d0
	trap	#3
.endm

.macro	_WaitForKey
# channel ID
	move.l	#0x00010001, a0
# timeout
	moveq.l	#-1, d3
	moveq.l	#1, d0
	trap	#3
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
	.section	.bss
	.global	ExitStack
ExitStack:
	.long	0
.endm

