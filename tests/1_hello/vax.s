
.include "../include/common.inc"

	.text

_start:
	.word	0
	_StartUp
	movl	$message, %r1
	jsb	PutString
	jsb	PutNewLine
	jsb	WaitForKey
	jsb	Exit

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

