
.macro	_StartUp
.endm

.macro	_Exit
	mov	ax, 1
	xor	bx, bx
	int	0x80
.endm

.macro	_PutChar char
	push	ax
	mov	ax, 4
	mov	bx, 1
	mov	cx, sp
	mov	dx, 1
	int	0x80
	add	sp, 2
.endm

.macro	_WaitForKey
	push	ax
	mov	ax, 3
	mov	bx, 1
	mov	cx, sp
	mov	dx, 1
	int	0x80
	add	sp, 2
.endm

.macro	_SysVars
.endm

