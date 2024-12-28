
.macro	_StartUp
.endm

.macro	_Exit
	clr.w	-(sp)
	trap	#1
.endm

.macro	_PutChar char
	move.w	\char, -(sp)
	and.w	#0xFF, (sp)
	move.w	#0x02, -(sp)
	trap	#1
	addq.l	#4, sp
.endm

.macro	_WaitForKey
	move.w	#0x01, -(sp)
	trap	#1
	addq.l	#2, sp
.endm

.macro	_LoadA	ref, dst
	lea	\ref(pc), \dst
.endm

.macro	_LoadL	ref, dst
	move.l	\ref(pc), \dst
.endm

.macro	_StoreL	src, ref
.if OPTION_NO_RELOC
	lea	\ref(pc), a0
	move.l	\src, (a0)
.else
	move.l	\src, \ref
.endif
.endm

.macro	_SysVars
.endm

