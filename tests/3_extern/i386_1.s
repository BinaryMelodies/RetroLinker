
.include "../include/common.inc"

	.section	.text
	.code32

start:
	_StartUp

	mov	esi, offset message
	call	PutString
	call	PutNewLine

	mov	esi, offset text_extval
	call	PutString
	mov	eax, offset extval
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_extref
	call	PutString
	mov	eax, offset extref
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_extref2
	call	PutString
	mov	eax, offset extref2
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_common1
	call	PutString
	mov	eax, offset common1
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_common2
	call	PutString
	mov	eax, offset common2
	call	PutLong
	call	PutNewLine

	call	WaitForKey
	call	Exit

	.section	.data

error:
	.asciz	"Error"

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
	.ascii	" MS-DOS (DOS4G)"
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
.if TARGET_OS2V2
	.ascii	" OS/2 2.0 (32-bit)"
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

