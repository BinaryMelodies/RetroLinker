
	.section	.text
	.code16

start:
	movw	%ds, %ax
	cli
	movw	%ax, %ss
	movw	$stack_top, %sp
	sti

	movw	$text_greetings, %si
	call	write_string_newline

	movb	$0xff, %cl
	int	$0xe0

	movw	$text_regards, %dx
	call	write_string_newline

	movb	$0, %cl
	int	$0xe0

.include "common.h"

	.section	.data

text_greetings:
	.ascii	"Greetings from COM file!"
	.byte	0

text_regards:
	.ascii	"Best regards!"
	.byte	0

	.section	.bss

	.skip	64
stack_top:

