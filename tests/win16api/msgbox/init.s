
	.set	INITTASK, $$IMPORT$KERNEL$005B
	.set	WAITEVENT, $$IMPORT$KERNEL$001E
	.set	INITAPP, $$IMPORT$USER$0005

	.section .text
	.global	_start
	.code16
_start:

	# Start up code
	# InitTask()
	lcall	$INITTASK@OZSEG16, $INITTASK
	testw	%ax, %ax
	jnz	1f
	movw	$0x4C01, %ax
	int	$0x21
1:
	# hInstance
	pushw	%di
	# hPrevInstance
	pushw	%si
	# FP_SEG(lpszCmdLine)
	pushw	%es
	# FP_OFF(lpszCmdLine)
	pushw	%bx
	# nCmdShow
	pushw	%dx

	# WaitEvent(0)
	xorw	%ax, %ax
	pushw	%ax
	lcall	$WAITEVENT@OZSEG16, $WAITEVENT
	# InitApp(hInstance)
	# hInstance (preserved after WaitEvent call)
	pushw	%di
	lcall	$INITAPP@OZSEG16, $INITAPP

	# WinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
	lcall	$WinMain@OZSEG16, $WinMain

	# ExitProcess(return_value)
	movb	$0x4C, %ah
	int	$0x21

0:
	jmp	0b

	.section .beg.data, "aw", @progbits

	# Instance data, required for InitTask
	.fill	16

