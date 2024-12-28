
%ifndef	TARGET_ELF
%ifdef	CPU_I86
	section	code class=code
%endif
%ifdef	CPU_I386
	section	code use32 class=code
%endif
%else
	section	.text
%endif

%ifdef	CPU_I86
	bits	16
%endif
%ifdef	CPU_I386
	bits	32
%endif

%ifndef	OUTPUT_VXD
%ifndef	TARGET_ELF
	import	ImportedOrdinalFunction SomeDll.dll 0x1234
	import	ImportedNamedFunction ImportedDll.dll ImportedName
	export	ExportedOrdinalFunction ExportedOrdinal 0x0005
	export	ExportedNamedFunction ExportedName
%else
%define	ImportedNamedFunction ??IMPORT?ImportedDll.dll?_ImportedName
%define	ImportedOrdinalFunction ??IMPORT?SomeDll.dll?1234
%define	ImportedNamedFunction? ??IMPSEG?ImportedDll.dll?_ImportedName
%define	ImportedOrdinalFunction? ??IMPSEG?SomeDll.dll?1234
%define	ExportedNamedFunction ??EXPORT?ExportedName
%define	ExportedOrdinalFunction ??EXPORT?0005?_ExportedOrdinal
%endif

	extern	ImportedOrdinalFunction
%ifdef	TARGET_ELF
	extern	ImportedOrdinalFunction?
%endif

	extern	ImportedNamedFunction
%ifdef	TARGET_ELF
	extern	ImportedNamedFunction?
%endif

	global	ExportedOrdinalFunction
	global	ExportedNamedFunction
%endif

;;;; CODE

	dd	0x12345678
%ifndef	TARGET_ELF
..start:
%else
	global	_start
_start:
%endif

code_var:
%ifdef	CPU_I386
	dd	code_var
	dd	data_var
	dd	bss_var
%endif

	dw	code_var
	dw	data_var
	dw	bss_var

%ifndef TARGET_WIN32
%ifndef	TARGET_ELF
	dw	seg code_var
	dw	seg data_var
	dw	seg bss_var
	dw	seg extra_var
%else
	extern	??SEGOF?code_var
	dw	??SEGOF?code_var
	extern	??SEGOF?data_var
	dw	??SEGOF?data_var
	extern	??SEGOF?bss_var
	dw	??SEGOF?bss_var
	extern	??SEGOF?extra_var
	dw	??SEGOF?extra_var
%endif
%endif

%ifdef	OUTPUT_VXD
%ifdef	TARGET_ELF
%define	OneExport	??EXPORT?OneExport
%endif
	global	OneExport
%endif

OneExport:
	db	1
%ifndef	OUTPUT_VXD
ExportedNamedFunction:
%endif
	db	2
%ifndef	OUTPUT_VXD
ExportedOrdinalFunction:
%endif
	db	3

%ifndef	OUTPUT_VXD
%ifdef	CPU_I386
	dd	ImportedNamedFunction
%endif
	dw	ImportedNamedFunction
%ifdef	CPU_I86
%ifndef	TARGET_ELF
	dw	seg ImportedNamedFunction
%else
	dw	ImportedNamedFunction?
%endif
%endif
%ifdef	CPU_I386
	call	ImportedNamedFunction
%endif
%ifdef	CPU_I386
	dd	ImportedOrdinalFunction
%endif
	dw	ImportedOrdinalFunction
%ifdef	CPU_I86
%ifndef	TARGET_ELF
	dw	seg ImportedOrdinalFunction
%else
	dw	ImportedOrdinalFunction?
%endif
%endif
%ifdef	CPU_I386
	call	ImportedOrdinalFunction
%endif
%endif

%if 0
;;; TODO: does not work yet, even for NE
%ifndef	TARGET_ELF
	dw	code
	dw	data
	dw	bss
	dw	stack
%else
	extern	??SEGAT?.text
	dw	??SEGAT?.text
	extern	??SEGAT?.data
	dw	??SEGAT?.data
	extern	??SEGAT?.bss
	dw	??SEGAT?.bss
	extern	??SEGAT?.stack
	dw	??SEGAT?.stack
%endif
%endif

%ifndef	TARGET_ELF
%ifdef	CPU_I86
	section	data
%endif
%ifdef	CPU_I386
	section	data use32
%endif
%else
	section	.data
%endif

;;;; DATA

	dd	0xABCDEF00

data_var:
	dd	0x12345678
%ifdef	CPU_I386
	dd	code_var
	dd	bss_var
;%ifdef CPU_I86
	dd	extra_var
;%endif
%else
	dw	code_var
	dw	bss_var
;%ifdef CPU_I86
	dw	extra_var
;%endif
%endif

%ifndef	TARGET_ELF
%ifdef	CPU_I86
	section	bss
%endif
%ifdef	CPU_I386
	section	bss use32
%endif
%else
	section	.bss
%endif

	resb	4

bss_var:
	resb	4

;;;; BSS

%ifndef	TARGET_ELF
%ifdef	CPU_I86
	section	extra
%endif
%ifdef	CPU_I386
	section	extra use32
%endif
%else
	section	.extra write
%endif

;;;; EXTRA

	db	0
extra_var:
	db	"This is an extra segment"

%ifndef	TARGET_ELF
%ifdef	CPU_I86
	section	stack class=stack
%endif
%ifdef	CPU_I386
	section	stack use32 class=stack
%endif
	group dgroup data bss stack
%else
	section	.stack nobits alloc noexec write

;	resb	0x1000

;	global	stack_top
;stack_top:
%endif

;;;; STACK

