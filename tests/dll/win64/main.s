
	.section .text
	.code64

	.global	_start
_start:
	.set	__imp__GetStdHandle, $$IMPORT$KERNEL32.dll$GetStdHandle$0000
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 32
	mov	ecx, -11
	call	[rip + __imp__GetStdHandle]
	# rax = hStdOut
	mov	rsp, rbp
	pop	rbp

	push	0	# temporary
	lea	r9, [rsp]
	.set	__imp__WriteFile, $$IMPORT$KERNEL32.dll$WriteFile$0000
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 8
	push	0
	sub	rsp, 32
	mov	r8d, offset length_msg_start
	lea	rdx, [rip + msg_start]
	mov	rcx, rax
	call	[rip + __imp__WriteFile]
	mov	rsp, rbp
	pop	rbp
	add	rsp, 4

	.set	__imp__DisplayMessage, $$IMPORT$LIB.dll$DisplayMessage$0000
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 32
	call	[rip + __imp__DisplayMessage]
	mov	rsp, rbp
	pop	rbp

	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 32
	mov	ecx, -11
	call	[rip + __imp__GetStdHandle]
	# rax = hStdOut
	mov	rsp, rbp
	pop	rbp

	push	0	# temporary
	lea	r9, [rsp]
	push	rbp
	mov	rbp, rsp
	and	rsp, ~0xF
	sub	rsp, 8
	push	0
	sub	rsp, 32
	mov	r8d, offset length_msg_quit
	lea	rdx, [rip + msg_quit]
	mov	rcx, rax
	call	[rip + __imp__WriteFile]
	mov	rsp, rbp
	pop	rbp
	add	rsp, 4

	.set	__imp__ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000
	and	rsp, ~0xF
	sub	rsp, 32
	mov	rcx, 0
	call	[rip + __imp__ExitProcess]

	.section .data

msg_start:
	.ascii	"Starting 64-bit"
	.byte	10
	.equ	length_msg_start, . - msg_start

msg_quit:
	.ascii	"Quitting"
	.byte	10
	.equ	length_msg_quit, . - msg_quit

