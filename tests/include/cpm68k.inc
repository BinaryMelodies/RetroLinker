
.macro	_StartUp
	nop
.endm

.macro	_Exit
	clr.w	d0
	trap	#2
.endm

.macro	_PutChar char
	move.w	\char, d1
	move.w	#0x02, d0
	trap	#2
.endm

.macro	_WaitForKey
	move.w	#0x01, d0
	trap	#2
.endm

.macro	_LoadA	ref, dst
.if FORMAT_68K_NONCONT
	move.w	#\ref, \dst
.else
	lea	\ref(pc), \dst
.endif
.endm

.macro	_LoadL	ref, dst
.if FORMAT_68K_NONCONT
	move.l	\ref, \dst
.else
	move.l	\ref(pc), \dst
.endif
.endm

.macro	_StoreL	src, ref
.if FORMAT_68K_NONCONT
	move.l	\src, \ref
.else
	lea	\ref(pc), a0
	move.l	\src, (a0)
.endif
.endm

.macro	_SysVars
.endm

