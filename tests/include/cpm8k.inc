
.if FORMAT_Z8K_SEGMENTED
	.z8001
.else
	.z8002
.endif

.macro	_StartUp
.endm

.macro	_Exit
	clr	r5
	sc	#2
.endm

.if FORMAT_Z8K_SEGMENTED
.macro	LDA	rw, rl, name
#	ldl	\rl, #$$SEGADR$\name
	ldl	\rl, #\name
.endm
.else
.macro	LDA	rw, rl, name
	ld	\rw, #\name
.endm
.endif

.macro	_PutChar char
	ld	r7, \char
	ld	r5, #0x02
	sc	#2
.endm

.macro	_WaitForKey
	ld	r5, #0x01
	sc	#2
.endm

.macro	_SysVars
.endm

