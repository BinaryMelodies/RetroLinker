
	.section .text
	.code32

	.global	_start
_start:
	.set	__imp__GetStdHandle, $$IMPORT$KERNEL32.dll$GetStdHandle$0000
	push	-11
	call	[__imp__GetStdHandle]
	# eax = hStdOut

	push	0	# temporary
	lea	ebx, [esp]
	.set	__imp__WriteFile, $$IMPORT$KERNEL32.dll$WriteFile$0000
	push	0
	push	ebx
	push	offset	length_msg_start
	push	offset	msg_start
	push	eax
	call	[__imp__WriteFile]
	add	esp, 4

	.set	__imp__DisplayMessage, $$IMPORT$LIB.dll$DisplayMessage$0000
	call	[__imp__DisplayMessage]

	push	-11
	call	[__imp__GetStdHandle]
	# eax = hStdOut

	push	0	# temporary
	lea	ebx, [esp]
	push	0
	push	ebx
	push	offset	length_msg_quit
	push	offset	msg_quit
	push	eax
	call	[__imp__WriteFile]
	add	esp, 4

	.set	__imp__ExitProcess, $$IMPORT$KERNEL32.dll$ExitProcess$0000
	push	0
	call	[__imp__ExitProcess]

	.section .data

msg_start:
	.ascii	"Starting 32-bit"
	.byte	10
	.equ	length_msg_start, . - msg_start

msg_quit:
	.ascii	"Quitting"
	.byte	10
	.equ	length_msg_quit, . - msg_quit

