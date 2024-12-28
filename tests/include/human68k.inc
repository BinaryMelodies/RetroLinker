
.macro	_StartUp
.endm

.macro	_Exit
	.word	0xFF00
.endm

.macro	_PutChar char
	move.w	\char, -(sp)
	and.w	#0xFF, (sp)
	.word	0xFF02
	addq.l	#2, sp
.endm

.macro	_WaitForKey
	.word	0xFF01
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
.endm

