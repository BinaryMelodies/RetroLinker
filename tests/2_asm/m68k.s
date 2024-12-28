
.include "../include/common.inc"

	.section	.text

.if OPTION_CUSTOM_ENTRY
	_StartUp
	_LoadA	error, a0
	bsrw	PutString
	bsrw	PutNewLine
	bsrw	WaitForKey
	bsrw	Exit
.endif

	.global	start
start:
.if FORMAT_68K_CONT | TARGET_GEMDOS | FORMAT_R
	nop
.endif
	_StartUp
.if TARGET_MACOS
	.word	0xA912
.endif
	bsrw	1f
1:
	move.l	(sp)+, d0
	sub.l	#1b - start, d0
.if TARGET_MACOS
	move.l	d0, stored_pc(a5)
.else
	_StoreL	d0, stored_pc
.endif

	_LoadA	message, a0
	bsrw	PutString
	bsrw	PutNewLine

.if !OPTION_NO_RELOC
	move.l	#text_absolute, a0
	bsrw	PutString
	bsrw	PutNewLine
.endif

.if !OPTION_NO_INTERSEG
	lea	text_relative(pc), a0
	bsrw	PutString
	bsrw	PutNewLine
.endif

	_LoadA	text_pc, a0
	bsrw	PutString
.if TARGET_MACOS
	move.l	stored_pc(a5), d0
.else
	_LoadL	stored_pc, d0
.endif
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_sp, a0
	bsrw	PutString
	move.l	sp, d0
	bsrw	PutLong
	bsrw	PutNewLine

.if TARGET_MACOS
	_LoadA	text_a5, a0
	bsrw	PutString
	move.l	a5, d0
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_bss_var, a0
	bsrw	PutString
.if MODEL_TINY
	lea.l	bss_var(pc), a0
	move.l	a0, d0
.else
	lea.l	bss_var(a5), a0
	move.l	a0, d0
.endif
	bsrw	PutLong
	bsrw	PutNewLine

	_LoadA	text_a5world_var, a0
	bsrw	PutString
	lea.l	a5world_var(a5), a0
	move.l	a0, d0
	bsrw	PutLong
	bsrw	PutNewLine
.endif

	bsrw	WaitForKey
	bsrw	Exit

PutLong:
	move.l	d0, -(sp)
	swap	d0
	bsrw	PutWord
	move.l	(sp)+, d0
#	bras	PutWord

PutWord:
	move.w	d0, -(sp)
	lsr.w	#8, d0
	bsrw	PutByte
	move.w	(sp)+, d0
#	bras	PutByte

PutByte:
	move.w	d0, -(sp)
	lsr.w	#4, d0
	bsrw	PutNibble
	move.w	(sp)+, d0
#	bras	PutNibble

PutNibble:
	and.w	#0xF, d0
	cmp.b	#10, d0
	bccs	1f
	add.b	#'0', d0
	bras	2f
1:
	add.b	#'A' - 10, d0
2:
	bras	PutChar

PutString:
1:
	move.b	(a0)+, d0
	beqs	1f
	move.l	a0, -(sp)
	bsrw	PutChar
	move.l	(sp)+, a0
	bras	1b
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

error:
	.asciz	"Error"

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

.ifndef OPTION_SUPPRESS_RELOC
.set OPTION_SUPPRESS_RELOC, OPTION_NO_RELOC
.endif

.if OPTION_SUPPRESS_RELOC
	.ascii	" (non-relocatable)"
.else
	.ascii	" (relocatable)"
.endif

	.byte	0

text_pc:
	.asciz	"PC="

text_sp:
	.asciz	"SP="

.if TARGET_MACOS
text_a5:
	.asciz	"A5="
.endif

.if !OPTION_NO_RELOC
text_absolute:
	.asciz	"Text accessed via absolute addressing"
.endif

.if !OPTION_NO_INTERSEG
text_relative:
	.asciz	"Text accessed via relative addressing"
.endif

.if	TARGET_MACOS
text_bss_var:
	.asciz	"bss_var: "
text_a5world_var:
	.asciz	"a5world_var: "
.endif

	.section	.bss

stored_pc:
	.skip	4

.if	TARGET_MACOS
bss_var:
	.skip	4
.endif

.if	TARGET_MACOS
	.section	.a5world, "aw", @nobits
a5world_var:
	.skip	4
.endif
	_SysVars

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

