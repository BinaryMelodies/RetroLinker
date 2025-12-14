
.include "../include/common.inc"

	.section	.text
	.code32

.if OPTION_CUSTOM_ENTRY
	_StartUp
	mov	esi, offset error
	call	PutString
	call	PutNewLine
	call	WaitForKey
	call	Exit
.endif

	.global	start
start:
	call	1f
1:
	pop	eax
	sub	eax, 1b - start
	mov	stored_eip, eax

	_StartUp

.if TARGET_WIN32
	push	offset 0
	push	offset dialog_title
	push	offset dialog_message
	push	offset 0
.set __imp__MessageBox, $$IMPORT$USER32.dll$MessageBoxA$0000
	.extern	__imp__MessageBox
	call	[__imp__MessageBox]
.endif

	mov	esi, offset message
	call	PutString
	call	PutNewLine

	mov	esi, offset text_cs_eip
	call	PutString
	mov	ax, cs
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	eax, stored_eip
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_ss_esp
	call	PutString
	mov	ax, ss
	call	PutWord
	mov	al, ':'
	call	PutChar
	mov	eax, esp
	call	PutLong
	call	PutNewLine

	mov	esi, offset text_message
	call	PutString
	mov	eax, offset message
	call	PutLong
	call	PutNewLine

	call	WaitForKey
	call	Exit

PutLong:
	push	eax
	shr	eax, 16
	call	PutWord
	pop	eax
#	jmp	PutWord

PutWord:
	push	eax
	mov	al, ah
	call	PutByte
	pop	eax
#	jmp	PutByte

PutByte:
	push	eax
	shr	al, 4
	call	PutNibble
	pop	eax
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
.elseif TARGET_DOS4G
	.ascii	" MS-DOS (DOS4G)"
.elseif TARGET_PHARLAP
	.ascii	" MS-DOS (386|DOS-Extender)"
.if FORMAT_MP
	.ascii	" MP .exp file"
.elseif FORMAT_MQ
	.ascii	" MQ .rex file"
.elseif FORMAT_P3
	.ascii	" P3 flat .exp file"
.endif
.elseif TARGET_WIN32
	.ascii	" Windows (32-bit)"
.elseif TARGET_OS2V2
	.ascii	" OS/2 2.0 (32-bit)"
.elseif TARGET_FLEXOS386
	.ascii	" FlexOS 386 (32-bit)"
.elseif TARGET_LINUX
	.ascii	" Linux (32-bit x86)"
.endif

	.byte	0

text_cs_eip:
	.asciz	"CS:EIP="

text_ss_esp:
	.asciz	"SS:ESP="

text_message:
	.asciz	"message="

.if TARGET_WIN32
dialog_title:
	.asciz	"Sample Application"
dialog_message:
	.asciz	"Graphical Greetings!"
.endif

	.section	.bss

stored_eip:
	.skip	4

	_SysVars

.if OPTION_EXPLICIT_STACK
	.section	.stack, "aw", @nobits
	.fill	OPTION_STACK_SIZE
.endif

