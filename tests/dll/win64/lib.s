
	.section .text
	.code64

	.set	MessageBoxA, $$IMPORT$USER32.dll$MessageBoxA$0000

	.global	_start
_start:
	ret

	#.set	DisplayMessage, $$EXPORT$DisplayMessage$0001
	.global	$$EXPORT$DisplayMessage$0001
$$EXPORT$DisplayMessage$0001:
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF

	mov	r9d, 0
	lea	r8, [rip + text_title]
	lea	rdx, [rip + text_message]
	mov	ecx, 0
	call	MessageBoxA

	mov	rsp, rbp
	pop	rbp
	ret

	.section .data

text_title:
	.ascii	"64-bit"
	.byte	0

text_message:
	.ascii	"MessageBox called from DLL"
	.byte	0

