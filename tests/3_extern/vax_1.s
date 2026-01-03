
.include "../include/common.inc"

	.text

start:
	.word	0
	_StartUp

	movl	$message, %r1
	jsb	PutString
	jsb	PutNewLine

	movl	$text_extval, %r1
	jsb	PutString
	movl	$extval, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_extref, %r1
	jsb	PutString
	movl	$extref, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_extref2, %r1
	jsb	PutString
	movl	$extref2, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_common1, %r1
	jsb	PutString
	movl	$common1, %r0
	jsb	PutLong
	jsb	PutNewLine

	movl	$text_common2, %r1
	jsb	PutString
	movl	$common2, %r0
	jsb	PutLong
	jsb	PutNewLine

	jsb	WaitForKey
	jsb	Exit

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

