
.include "../include/common.inc"

	.text

	.global	start
start:
	.word	0
	jsb	1f
1:
	movl	(%sp)+, %r0
	subl2	$3, %r0
	movl	%r0, stored_pc

	_StartUp

	movl	$message, %r1
	jsb	PutString
	jsb	PutNewLine

	movl	$text_pc, %r1
	jsb	PutString
	movl	stored_pc, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_sp, %r1
	jsb	PutString
	movl	%sp, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_message, %r1
	jsb	PutString
	movl	$message, %r0
	jsb	PutLong
	jsb	PutNewLine

	jsb	WaitForKey
	jsb	Exit

PutLong:
	pushl	%r0
	ashl	$-16, %r0, %r0
	jsb	PutWord
	movl	(%sp)+, %r0
	#brb	PutWord

PutWord:
	pushl	%r0
	ashl	$-8, %r0, %r0
	jsb	PutByte
	movl	(%sp)+, %r0
	#brb	PutByte

PutByte:
	pushl	%r0
	ashl	$-4, %r0, %r0
	jsb	PutNibble
	movl	(%sp)+, %r0
	#brb	PutNibble

PutNibble:
	bicb2	$~0xF, %r0
	cmpb	$10, %r0
	blequ	1f
	addb2	$'0', %r0
	brb	2f
1:
	addb2	$'A' - 10, %r0
2:
	brb	PutChar

PutString:
	movb	(%r1)+, %r0
	tstb	%r0
	beql	1f
	pushl	%r1
	jsb	PutChar
	movl	(%sp)+, %r1
	brb	PutString
1:
	rsb

PutNewLine:
	movb	$10, %r0

PutChar:
	_PutChar	%r0
	rsb

WaitForKey:
	_WaitForKey
	rsb

Exit:
	_Exit

	_SysVars

	.data

error:
	.asciz	"Error"
	.align	4

message:
	.ascii	"Greetings!"
.if TARGET_UNIX || TARGET_BSD
.if TARGET_UNIX
.if FORMAT_COFF
	.ascii	" AT&T UNIX System V"
.else
	.ascii	" AT&T UNIX System III"
.endif
.elseif TARGET_BSD
	.ascii	" 4.3BSD"
.endif
.if FORMAT_OMAGIC
	.ascii	" impure executable"
.elseif FORMAT_NMAGIC
	.ascii	" pure executable"
.elseif FORMAT_ZMAGIC
	.ascii	" demand paged executable"
.elseif FORMAT_COFF
	.ascii	" COFF executable"
.endif
.endif

	.byte	0

text_pc:
	.asciz	"PC="

text_sp:
	.asciz	"SP="

text_message:
	.asciz	"message="

	.comm	stored_pc, 4

