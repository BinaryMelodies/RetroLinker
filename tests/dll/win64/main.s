
	.section .text
	.code64

	.set	MessageBoxA, $$IMPORT$USER32.dll$MessageBoxA$0000
	.set	ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000

	.set	DisplayMessage, $$IMPORT$LIB.dll$DisplayMessage$0000

	.global	_start
_start:
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	mov	r9d, 0
	lea	r8, [rip + text_title]
	lea	rdx, [rip + text_start]
	mov	ecx, 0
	call	MessageBoxA
	mov	rsp, rbp
	pop	rbp

	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 32
	call	DisplayMessage
	mov	rsp, rbp
	pop	rbp

	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	mov	r9d, 0
	lea	r8, [rip + text_title]
	lea	rdx, [rip + text_quit]
	mov	ecx, 0
	call	MessageBoxA
	mov	rsp, rbp
	pop	rbp

	and	rsp, ~0xF
	sub	rsp, 32
	mov	rcx, 0
	call	ExitProcess

	.section .data

text_title:
	.ascii	"64-bit"
	.byte	0

text_start:
	.ascii	"Starting 64-bit"
	.byte	0

text_quit:
	.ascii	"Quitting"
	.byte	0

