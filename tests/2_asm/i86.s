
.include "../include/common.inc"

	.section	.text
	.code16

.if OPTION_CUSTOM_ENTRY
	_StartUp
	mov	si, offset error
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit
.endif

	.global	start
start:
	call	1f
1:
	pop	ax
	sub	ax, 1b - start
.if FORMAT_MZ | (!MODEL_TINY & (FORMAT_COM | FORMAT_CMD_TINY))
# MZ: initial DS points to PSP
# .com and tiny .cmd: initial DS is the same as CS, which is not what we want for a tiny model
	cs mov	$$WRTSEG$stored_ip$.text, ax
.else
	mov	stored_ip, ax
.endif
.if FORMAT_MZ
# MZ DS points elsewhere
	cs mov	$$WRTSEG$stored_ds$.text, ds
.endif
	_StartUp
.if TARGET_WIN16
	pushw	offset 0
	pushw	offset $$SEGOF$dialog_message
	pushw	offset dialog_message
	pushw	offset $$SEGOF$dialog_title
	pushw	offset dialog_title
	pushw	offset 0
.set MessageBox, $$IMPORT$USER$0001
.set $$SEGOF$MessageBox, $$IMPSEG$USER$0001
	.extern	MessageBox, $$SEGOF$MessageBox
	.byte	0x9A
	.word	MessageBox
	.word	$$SEGOF$MessageBox
.endif

	mov	si, offset message
	call	PutString
	call	PutNewLine

	mov	si, offset text_cs_ip
	call	PutString
	mov	ax, cs
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	ax, stored_ip
	call	PutWord
	call	PutNewLine

	mov	si, offset text_ss_sp
	call	PutString
	mov	ax, ss
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	ax, sp
	call	PutWord
	call	PutNewLine

.if FORMAT_MZ
	mov	si, offset text_ds
	call	PutString
	mov	ax, stored_ds
	call	PutWord
	call	PutNewLine
.endif

	mov	si, offset text_seg_data
	call	PutString
	mov	ax, offset $$SEG$.data
	call	PutWord
	mov	si, offset text_and_ds
	call	PutString
	mov	ax, ds
	call	PutWord
	call	PutNewLine

.if MODEL_COMPACT
	mov	si, offset text_seg_stack
	call	PutString
	mov	ax, offset $$SEG$.stack
	call	PutWord
	mov	si, offset text_and_ss
	call	PutString
	mov	ax, ss
	call	PutWord
	call	PutNewLine

	mov	si, offset text_seg_extra
	call	PutString
	mov	ax, offset $$SEG$.extra
	call	PutWord
	call	PutNewLine

.if !(OPTION_NO_RELOC & OPTION_NO_DIFF_INTERSEG)
	push	ds
.if OPTION_NO_RELOC
	mov	ax, cs
	add	ax, offset $$SEGDIF$.extra$.text
.else
	mov	ax, offset $$SEG$.extra
.endif
	mov	ds, ax
	mov	si, offset extra_message
	call	PutString
	call	PutNewLine
	pop	ds
.elseif FORMAT_CMD_COMPACT
	push	ds
	mov	ds, [0x0F]
	mov	si, offset extra_message
	call	PutString
	call	PutNewLine
	pop	ds
.endif
.endif

.if !OPTION_NO_DIFF_INTERSEG
	mov	si, offset text_diff_data_text
	call	PutString
	mov	ax, offset $$SEGDIF$.data$.text
	call	PutWord
	call	PutNewLine
.endif

	mov	si, offset text_message
	call	PutString
	mov	ax, offset $$SEGOF$message
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	ax, offset message
	call	PutWord
	call	PutNewLine

	mov	si, offset text_seg_at_message
	call	PutString
	mov	ax, offset $$SEGAT$message
	call	PutWord
	call	PutNewLine

.if !OPTION_NO_DIFF_INTERSEG
	mov	si, offset text_message_wrt_text
	call	PutString
	mov	ax, offset $$WRTSEG$message$.text
	call	PutWord
	call	PutNewLine
.endif

	call	WaitForKey
	call	Exit

PutWord:
	push	ax
	mov	al, ah
	call	PutByte
	pop	ax
#	jmp	PutByte

PutByte:
	push	ax
	mov	cl, 4
	shr	al, cl
	call	PutNibble
	pop	ax
#	jmp	PutNibble

PutNibble:
	and	al, 0xF
	cmp	al, 10
	jnc	1f
	add	al, '0'
	jmp	2f
1:
	add	al, 'A' - 10
2:
	jmp	PutChar

PutString:
1:
	lodsb
	test	al, al
	jz	1f
	push	si
	call	PutChar
	pop	si
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

error:
	.asciz	"Error"
	.align	16

message:
	.ascii	"Greetings!"
.if TARGET_DOS16M
	.ascii	" MS-DOS (DOS/16M)"
.elseif TARGET_MSDOS
	.ascii	" MS-DOS"
.if FORMAT_COM
	.ascii	" flat binary .com file"
.elseif FORMAT_MZ
	.ascii	" \"MZ\" .exe file"
.endif
.elseif TARGET_CPM86
	.ascii	" CP/M-86"
.if FORMAT_CMD_TINY
	.ascii	" 8080 model .cmd file"
.elseif FORMAT_CMD_SMALL
	.ascii	" small model .cmd file"
.endif
.elseif TARGET_ELKS
	.ascii	" ELKS"
.if FORMAT_AOUT_COM
	.ascii	" combined file"
.elseif FORMAT_AOUT_SEP
	.ascii	" split file"
.endif
.elseif TARGET_WIN16
	.ascii	" Windows (16-bit)"
.elseif TARGET_OS2V1
	.ascii	" OS/2 1.0 (16-bit)"
.endif

.if MODEL_TINY
	.ascii	" (tiny model)"
.elseif MODEL_SMALL
	.ascii	" (small model)"
.elseif MODEL_COMPACT
	.ascii	" (compact model)"
.endif

	.byte	0

text_cs_ip:
	.asciz	"CS:IP="

text_ss_sp:
	.asciz	"SS:SP="

text_seg_data:
	.asciz	"seg .data="

text_and_ds:
	.ascii	", "
text_ds:
	.asciz	"DS="

text_seg_stack:
	.asciz	"seg .stack="

text_and_ss:
	.asciz	", SS="

text_seg_extra:
	.asciz	"seg .extra="

text_seg_at_message:
	.asciz	"seg at message="

.if !OPTION_NO_DIFF_INTERSEG
text_message_wrt_text:
	.asciz	"message wrt .text="
.endif

text_message:
	.asciz	"message="

text_diff_data_text:
	.asciz	".data-.text="

.if MODEL_COMPACT
	.section	.extra, "aw", @progbits
extra_message:
	.asciz	"Message from the extra segment"
.endif

.if TARGET_WIN16
dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.asciz	"Graphical Greetings!"
.endif

	.section	.bss

stored_ip:
	.skip	2

.if FORMAT_MZ
stored_ds:
	.skip	2
.endif

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	0x100
.endif

