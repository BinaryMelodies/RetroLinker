
.include "../include/common.inc"

	.section	.text

start:
	_StartUp
	nop

	_LoadA	message, a0
	bsrw	PutString
	bsrw	PutNewLine

	_LoadA	text_extval, a0
	bsrw	PutString
	move.l	#extval, d0
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_extref, a0
	bsrw	PutString
	_LoadA	extref, a0
	move.l	a0, d0
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_extref2, a0
	bsrw	PutString
	_LoadA	extref2, a0
	move.l	a0, d0
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_common1, a0
	bsrw	PutString
	_LoadA	common1, a0
	move.l	a0, d0
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_common2, a0
	bsrw	PutString
	_LoadA	common2, a0
	move.l	a0, d0
	bsrw	PutLong
	bsrw	PutNewLine

	bsrw	WaitForKey
	bsrw	Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if TARGET_CPM68K
	.ascii	" CP/M-68K"
.elseif TARGET_GEMDOS
	.ascii	" GEMDOS"
.elseif TARGET_HUMAN68K
	.ascii	" Human68k"
.elseif TARGET_AMIGA
	.ascii	" AmigaOS"
.elseif TARGET_MACOS
	.ascii	" Mac OS"
.elseif TARGET_SQL
	.ascii	" Sinclair QL (QDOS)"
.endif

	.byte	0

text_extval:
	.asciz	"extval="
text_extref:
	.asciz	"extref="
text_extref2:
	.asciz	"extref2="
text_common1:
	.asciz	"common1="
text_common2:
	.asciz	"common2="

	.comm	common1, 4

#.ifdef TARGET_MSDOS
#.ifdef FORMAT_MZ
#	.section	.stack, "aw", @nobits
#	.fill	0x100
#.endif
#.endif

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

