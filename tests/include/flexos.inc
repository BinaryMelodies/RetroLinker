
.macro	_StartUp
.endm

.macro	_Exit
.if	TARGET_FLEXOS286
	mov	ax, 0
	# F_EXIT
	mov	cx, 25
	int	0xDC
.endif
.if	TARGET_FLEXOS386
	mov	eax, 0
	# F_EXIT
	mov	ecx, 25
	int	0xDD
.endif
.if	TARGET_FLEXOS68K
	move.l	#0, d1
	# F_EXIT
	move.l	25, d0
	trap	#0xE
.endif
.endm

.macro	_PutChar char
.if	TARGET_FLEXOS286
	mov	[PutChar_char], al

	# sync
	mov	byte ptr [FlexOS_paramblock], 0
	# option
	mov	byte ptr [FlexOS_paramblock + 1], 0
	# flags (flush, SEEK_CUR)
	mov	word ptr [FlexOS_paramblock + 2], 0x0101
	# swi
	mov	word ptr [FlexOS_paramblock + 4], 0
	mov	word ptr [FlexOS_paramblock + 6], 0
	# fnum
	mov	word ptr [FlexOS_paramblock + 8], 1
	mov	word ptr [FlexOS_paramblock + 10], 0
	# buffer
	mov	word ptr [FlexOS_paramblock + 12], offset PutChar_char
	mov	word ptr [FlexOS_paramblock + 14], ds
	# bufsiz
	mov	word ptr [FlexOS_paramblock + 16], 1
	mov	word ptr [FlexOS_paramblock + 18], 0
	# offset
	mov	word ptr [FlexOS_paramblock + 20], 0
	mov	word ptr [FlexOS_paramblock + 22], 0

	lea	ax, [FlexOS_paramblock]
	mov	bx, ds
	# F_WRITE
	mov	cx, 8
	int	0xDC

	ret
.endif
.if	TARGET_FLEXOS386
	mov	[PutChar_char], al

	# sync
	mov	byte ptr [FlexOS_paramblock], 0
	# option
	mov	byte ptr [FlexOS_paramblock + 1], 0
	# flags (flush, SEEK_CUR)
	mov	word ptr [FlexOS_paramblock + 2], 0x0101
	# swi
	mov	dword ptr [FlexOS_paramblock + 4], 0
	# fnum
	mov	dword ptr [FlexOS_paramblock + 8], 1
	# buffer
	mov	dword ptr [FlexOS_paramblock + 12], offset PutChar_char
	# bufsiz
	mov	dword ptr [FlexOS_paramblock + 16], 1
	# offset
	mov	dword ptr [FlexOS_paramblock + 20], 0

	lea	eax, [FlexOS_paramblock]
	# F_WRITE
	mov	ecx, 8
	int	0xDD

	ret
.endif
.if	TARGET_FLEXOS68K
	move.b	d0, (PutChar_char)

	# sync
	move.b	#0, (FlexOS_paramblock)
	# option
	move.b	#0, (FlexOS_paramblock + 1)
	# flags (flush, SEEK_CUR)
	move.w	#0x0101, (FlexOS_paramblock + 2)
	# swi
	move.l	#0, (FlexOS_paramblock + 4)
	# fnum
	move.l	#1, (FlexOS_paramblock + 8)
	# buffer
	move.l	#PutChar_char, (FlexOS_paramblock + 12)
	# bufsiz
	move.l	#1, (FlexOS_paramblock + 16)
	# offset
	move.l	#0, (FlexOS_paramblock + 20)

	move.l	a0, -(sp)
	lea.l	FlexOS_paramblock, a0
	move.l	a0, d1
	move.l	(sp)+, a0
	# F_WRITE
	move.l	#8, d0
	trap	#14

	rts
.endif
.endm

.macro	_WaitForKey
	# TODO
.endm

.macro	_SysVars
	.section	.bss

FlexOS_paramblock:
	.skip	24

PutChar_char:
	.skip	1
.endm

.if	TARGET_FLEXOS68K
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
.endif

