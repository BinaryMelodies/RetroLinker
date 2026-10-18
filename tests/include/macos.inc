
.if	CPU_PPC
	.section	.toc, "aw", @progbits
	.global	TOC
TOC:
.macro	__SYSCALL	__syscall, __wordcount
	# Get transition vector address
	lwz	r12, \__syscall\()_TOC - .toc(r2)
	# Store current global address
	stwu	r2, -4(r1)
	# Load function address
	lwz	r0, 0(r12)
	# Load global address for function
	lwz	r2, 4(r12)
	# Create linkage area/parameter area to avoid overwriting stack
	la	r1, -24 - 4 * \__wordcount(r1)
	# Jump and link to function
	mtlr	r0
	blrl
	# Delay slot
	nop
	# Restore stack pointer
	la	r1, 24 + 4 * \__wordcount(r1)
	# Restore current global address
	lwz	r2, 0(r1)
	la	r1, 4(r1)
.endm
.macro	__DEFSYSCALL	__syscall
	.section	.toc
\__syscall\()_TOC:
.if	!VERSION_CARBON
	.long	$$IMPORT$InterfaceLib$_\__syscall
.else
	.long	$$IMPORT$CarbonLib$_\__syscall
.endif
	.section	.text
.endm
.if	!VERSION_CARBON
__DEFSYSCALL	InitGraf
__DEFSYSCALL	InitFonts
__DEFSYSCALL	InitWindows
__DEFSYSCALL	InitCursor
.endif
__DEFSYSCALL	DrawChar
__DEFSYSCALL	TextMode
__DEFSYSCALL	MoveTo
.if	!VERSION_CARBON
__DEFSYSCALL	GetNextEvent
.else
__DEFSYSCALL	NewRgn
__DEFSYSCALL	WaitNextEvent
.endif
__DEFSYSCALL	ExitToShell
.endif

	# Toolbox calls
.if	!VERSION_CARBON
.macro	__InitGraf
.if	CPU_M68K
	.short	0xA86E
.elseif	CPU_PPC
	__SYSCALL	InitGraf, 1
.endif
.endm
.macro	__InitFonts
.if	CPU_M68K
	.short	0xA8FE
.elseif	CPU_PPC
	__SYSCALL	InitFonts, 0
.endif
.endm
.macro	__InitWindows
.if	CPU_M68K
	.short	0xA912
.elseif	CPU_PPC
	__SYSCALL	InitWindows, 0
.endif
.endm
.macro	__InitCursor
.if	CPU_M68K
	.short	0xA850
.elseif	CPU_PPC
	__SYSCALL	InitCursor, 0
.endif
.endm
.endif
.macro	__DrawChar
.if	CPU_M68K
	.short	0xA883
.elseif	CPU_PPC
	__SYSCALL	DrawChar, 1
.endif
.endm
.macro	__TextMode
.if	CPU_M68K
	.short	0xA889
.elseif	CPU_PPC
	__SYSCALL	TextMode, 1
.endif
.endm
.macro	__MoveTo
.if	CPU_M68K
	.short	0xA893
.elseif	CPU_PPC
	__SYSCALL	MoveTo, 2
.endif
.endm
.if	!VERSION_CARBON
.macro	__GetNextEvent
.if	CPU_M68K
	.short	0xA970
.elseif	CPU_PPC
	__SYSCALL	GetNextEvent, 2
.endif
.endm
.endif
.if	VERSION_CARBON
.macro	__NewRgn
	__SYSCALL	NewRgn, 0
.endm
.macro	__WaitNextEvent
	__SYSCALL	WaitNextEvent, 4
.endm
.endif
.macro	__ExitToShell
.if	CPU_M68K
	.short	0xA9F4
.elseif	CPU_PPC
	__SYSCALL	ExitToShell, 0
.endif
.endm

.set	fontHeight, 15

.macro	_StartUp
	# Initialize basic Toolbox managers
.if	!VERSION_CARBON
.if	CPU_M68K
	pea	-4(a5)
.elseif	CPU_PPC
	.section	.toc
qd_thePort:
	.long	qd + 202
	.section	.text
	lwz	r3, qd_thePort - .toc(r2)
.endif
	__InitGraf

	__InitFonts
	__InitWindows
	__InitCursor
.endif

.if	VERSION_CARBON
	# Create a new region handle, to be used with WaitNextEvent
	__NewRgn
	lwz	r9, cursor_rgn_TOC - .toc(r2)
	stw	r3, 0(r9)
.endif

	# Initialize text writing routines
# TextMode(srcCopy)
.if	CPU_M68K
	clr.w	-(sp)
.elseif	CPU_PPC
	li	r3, 0
.endif
	__TextMode

.if	CPU_M68K
	clr.w	-(sp)
	move.w	#32, -(sp)
.elseif	CPU_PPC
	li	r3, 0
	li	r4, 32
.endif
	__MoveTo

.if	CPU_M68K
	lea	current_row(a5), a0
	move.w	#32, (a0)
.elseif	CPU_PPC
	lwz	r9, current_row_TOC - .toc(r2)
	li	r4, 32
	sth	r4, 0(r9)
.endif
.endm

.macro	_Exit
	__ExitToShell
.endm

.macro	_PutChar char
	# Brute force output text to screen
.if	CPU_M68K
	move.w	\char, -(sp)
.elseif	CPU_PPC
	mr	r3, \char
	li	r4, 255
	and	r3, r3, r4
.endif
	__DrawChar
.endm

.macro	_PutNewLine
	# Advance cursor pointer to next line
.if	CPU_M68K
	clr.w	-(sp)
	lea	current_row(a5), a0
	add.w	#fontHeight, (a0)
	move.w	(a0), -(sp)
.elseif	CPU_PPC
	li	r3, 0
	lwz	r9, current_row_TOC - .toc(r2)
	lhz	r4, 0(r9)
	addi	r4, r4, fontHeight
	sth	r4, 0(r9)
.endif
	__MoveTo
.endm

.macro	_WaitForKey
	# Wait for events and filter for keyboard input
1:
.if	CPU_M68K
# space for return value
	subq.l	#2, sp
# everyEvent
	move.w	#-1, -(sp)
# event structure
	pea	event(a5)
.elseif	CPU_PPC
# everyEvent
	li	r3, -1
# event structure
	.section	.toc
event_TOC:
	.long	event
	.section	.text
	lwz	r4, event_TOC - .toc(r2)
.endif

.if	!VERSION_CARBON
	__GetNextEvent
.else
	li	r5, 0
	lwz	r6, cursor_rgn_TOC - .toc(r2)
	__WaitNextEvent
.endif

.if	CPU_M68K
	move.b	(sp), d0
	addq.l	#2, sp
	tst.b	d0
	beq	1b

	move.w	event(a5), d0
	cmp.w	#3, d0
	bne	1b
.elseif	CPU_PPC
	extsb	r3, r3
	cmpwi	cr0, r3, 0
	beq	cr0, 1b

	lwz	r4, event_TOC - .toc(r2)
	lhz	r9, 0(r4)
	extsh	r9, r9
	cmpwi	cr0, r9, 3
	bne	cr0, 1b
.endif
.endm

.if	CPU_M68K
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

.macro	_SysVars
.if	CPU_M68K
	.section	.a5world, "aw", @nobits
.elseif	CPU_PPC
.if	VERSION_CARBON
	.section	.toc
cursor_rgn_TOC:
	.long	cursor_rgn
	.section	.bss
cursor_rgn:
	.skip	4
.endif
	.section	.toc
current_row_TOC:
	.long	current_row
	.section	.bss
.endif
	# store the pixel offset for the current row
	.global	current_row
.if	CPU_PPC
	.align	4 # TODO: is this necessary?
.endif
current_row:
	.skip	2

	# store event structure for GetNextEvent
.if	CPU_PPC
	.align	4 # TODO: is this necessary?
.endif
event:
	.skip	16

	# QD globals, must come at end
	# TODO: use 206 instead?
.if	CPU_PPC
	.align	4 # TODO: is this necessary?
.endif
qd:
	.skip	248
.endm

