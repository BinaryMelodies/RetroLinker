
.include "../include/common.inc"

	.section	.text
	.code64

start:
	_StartUp

	lea	rsi, [rip + message]
	call	PutString
	call	PutNewLine

	lea	rsi, [rip + text_extval]
	call	PutString
	lea	rax, [rip + extval]
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_extref]
	call	PutString
	lea	rax, [rip + extref]
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_extref2]
	call	PutString
	lea	rax, [rip + extref2]
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_common1]
	call	PutString
	lea	rax, [rip + common1]
	call	PutQuad
	call	PutNewLine

	lea	rsi, [rip + text_common2]
	call	PutString
	lea	rax, [rip + common2]
	call	PutQuad
	call	PutNewLine

	call	WaitForKey
	call	Exit

	.section	.data

error:
	.asciz	"Error"

message:
	.ascii	"Greetings!"
.if TARGET_WIN64
	.ascii	" Windows (64-bit)"
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

