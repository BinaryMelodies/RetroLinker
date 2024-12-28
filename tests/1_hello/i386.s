
.include "../include/common.inc"

	.section	.text
	.code32

start:
	_StartUp
	mov	esi, offset message
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit

PutString:
1:
	lodsb
	test	al, al
	jz	1f
	push	esi
	call	PutChar
	pop	esi
	jmp	1b
1:
	ret

PutNewLine:
	mov	al, 13
	call	PutChar
	mov	al, 10
#	jmp	PutChar

PutChar:
	_PutChar	al
	ret

WaitForKey:
	_WaitForKey
	ret

Exit:
	_Exit

	.section	.data

message:
	.ascii	"Greetings!"
.if TARGET_DJGPP
	.ascii	" MS-DOS (DJGPP/CWSDMI)"
.if FORMAT_AOUT
	.ascii	" a.out .exe file"
.elseif FORMAT_COFF
	.ascii	" COFF .exe file"
.endif
.endif
.if TARGET_DOS4G
	.ascii	" MS-DOS (DOS/4G)"
.endif
.if TARGET_PHARLAP
	.ascii	" MS-DOS (386|DOS-Extender)"
.if FORMAT_MP
	.ascii	" MP .exp file"
.elseif FORMAT_MQ
	.ascii	" MQ .rex file"
.elseif FORMAT_P3
	.ascii	" P3 flat .exp file"
.endif
.endif
.if TARGET_PDOS386
	.ascii	" PDOS/386 a.out .exe file"
.endif
.if TARGET_OS2V2
	.ascii	" OS/2 2.0 (32-bit)"
.endif

	.byte	0

	.section	.bss

	_SysVars

.if OPTION_EXPLICIT_STACK
.if OPTION_SEPARATE_STACK
	.section	.stack, "aw", @nobits
.endif
	.fill	0x100
.endif

