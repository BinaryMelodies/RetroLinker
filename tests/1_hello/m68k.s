
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
	_LoadA	message, a0
	bsrw	PutString
	bsrw	PutNewLine
	bsrw	WaitForKey
	bsrw	Exit

PutString:
1:
	move.b	(a0)+, d0
	beqs	1f
	move.l	a0, -(sp)
	bsr	PutChar
	move.l	(sp)+, a0
	bra	1b
1:
	rts

PutNewLine:
.if OPTION_NEWLINE_PROC
	_PutNewLine
	rts
.else
.if !OPTION_ONLY_LF
	move.b	#13, d0
	bsr	PutChar
.endif
	move.b	#10, d0
#	bra	PutChar
.endif

PutChar:
	_PutChar	d0
	rts

WaitForKey:
	_WaitForKey
	rts

Exit:
	_Exit

	.section	.data

message:
	.ascii	"Greetings!"
.if TARGET_CPM68K
	.ascii	" CP/M-68K"
.if FORMAT_68K_CONT
	.ascii	" contiguous .68k file"
.elseif FORMAT_68K_NONCONT
	.ascii	" non-contiguous .68k file"
.endif
.elseif TARGET_GEMDOS
	.ascii	" GEMDOS"
.elseif TARGET_HUMAN68K
	.ascii	" Human68k"
.if FORMAT_R
	.ascii	" relocatable flat binary .r file"
.elseif FORMAT_Z
	.ascii	" 68K .z file"
.elseif FORMAT_X
	.ascii	" \"HU\" .x file"
.endif
.elseif TARGET_AMIGA
	.ascii	" AmigaOS"
.elseif TARGET_MACOS
	.ascii	" Mac OS"
.elseif TARGET_SQL
	.ascii	" Sinclair QL (QDOS)"
.endif

	.byte	0

	.section	.bss

	_SysVars

.if	TARGET_MACOS
# Making it 32-bit aware
	.section	$$RSRC$_SIZE$FFFF, "a", @progbits
# reserved, acceptSuspendResumeEvents, reserved, canBackground, doesActivateOnFGSwitch, backgroundAndForeground, dontGetFrontClicks, ignoreChildDiedEvents
	.byte	0x58
# is32BitCompatible, notHighLevelEventAware, onlyLocalHLEvents, notStationaryAware, dontUseTextEditServices, notDisplayManagerAware, reserved, reserved
	.byte	0x80
# preferred memory
	.long	100 * 1024
# minimum memory
	.long	100 * 1024
.endif

