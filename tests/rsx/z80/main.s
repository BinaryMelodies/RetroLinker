
	.section	.text

	.equ	BDOS, 5

start:
	ld	hl, text_greetings
	call	write_string_newline

# invoke RSX
	ld	c, 0xFF
	call	5

	ld	hl, text_regards
	call	write_string_newline

	ret

.include "common.h"

text_greetings:
	.ascii	"Greetings from COM file!"
	.byte	0

text_regards:
	.ascii	"Best regards!"
	.byte	0

