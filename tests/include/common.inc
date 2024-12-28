
.ifdef CPU_Z80
# .set is replaced by defl

TARGET_CPM86 defl 0
TARGET_MSDOS defl 0
TARGET_ELKS defl 0
TARGET_WIN16 defl 0
TARGET_OS2 defl 0
TARGET_CPM68K defl 0
TARGET_GEMDOS defl 0
TARGET_HUMAN68K defl 0
TARGET_AMIGA defl 0
TARGET_MACOS defl 0
TARGET_RISCOS defl 0
TARGET_SQL defl 0
TARGET_CPM8K defl 0
TARGET_DXDOS defl 0

.ifndef TARGET_CPM80
TARGET_CPM80 defl 0
.endif

.ifndef TARGET_MSXDOS
TARGET_MSXDOS defl 0
.endif

.ifndef FORMAT_COM
FORMAT_COM defl 0
.endif

.ifndef FORMAT_PRL
FORMAT_PRL defl 0
.endif

.else

.set CPU_Z80, 0
.set TARGET_CPM80, 0
.set TARGET_MSXDOS, 0
#.set FORMAT_COM, 0 # used by other targets
.set FORMAT_PRL, 0

# not CPU_Z80

.ifndef CPU_86
.set CPU_86, 0
.endif

.ifndef CPU_386
.set CPU_386, 0
.endif

.ifndef CPU_68K
.set CPU_68K, 0
.endif

.ifndef CPU_Z8K
.set CPU_Z8K, 0
.endif

.ifndef CPU_PDP11
.set CPU_PDP11, 0
.endif

.ifndef CPU_ARM
.set CPU_ARM, 0
.endif

#### 8086/80286 targets

.ifndef TARGET_MSDOS1
.set TARGET_MSDOS1, 0
.else
.set TARGET_MSDOS, 1
.endif

.ifndef TARGET_MSDOS
.set TARGET_MSDOS, 0
.endif

.ifndef TARGET_CPM86
.set TARGET_CPM86, 0
.endif

.ifndef TARGET_ELKS
.set TARGET_ELKS, 0
.endif

.ifndef TARGET_WIN16
.set TARGET_WIN16, 0
.else
.set OPTION_NO_INTERSEG, 1
.set OPTION_NO_DIFF_INTERSEG, 1
.endif

.ifndef TARGET_OS2V1
.set TARGET_OS2V1, 0
.else
.set TARGET_OS2, 1
.set OPTION_NO_INTERSEG, 1
.set OPTION_NO_DIFF_INTERSEG, 1
.endif

.ifndef TARGET_DOS16M
.set TARGET_DOS16M, 0
.else
.set TARGET_MSDOS, 1
.set OPTION_CUSTOM_ENTRY, 1
.endif

#### 80386 targets

.ifndef TARGET_DJGPP
.set TARGET_DJGPP, 0
.else
.set TARGET_MSDOS, 1
.endif

.ifndef TARGET_DOS4G
.set TARGET_DOS4G, 0
.else
.set TARGET_MSDOS, 1
.endif

.ifndef TARGET_PHARLAP
.set TARGET_PHARLAP, 0
.else
.set TARGET_MSDOS, 1
.endif

.ifndef TARGET_OS2V2
.set TARGET_OS2V2, 0
.else
.set TARGET_OS2, 1
.endif

.ifndef TARGET_PDOS386
.set TARGET_PDOS386, 0
.else
.set TARGET_MSDOS, 1 # Note: uses a 32-bit API
.endif

#### 68000 targets

.ifndef TARGET_CPM68K
.set TARGET_CPM68K, 0
.endif

.ifndef TARGET_GEMDOS
.set TARGET_GEMDOS, 0
.endif

.ifndef TARGET_HUMAN68K
.set TARGET_HUMAN68K, 0
.endif

.ifndef TARGET_AMIGA
.set TARGET_AMIGA, 0
.else
.set OPTION_NO_INTERSEG, 1
.set OPTION_ONLY_LF, 1
.endif

.ifndef TARGET_MACOS
.set TARGET_MACOS, 0
.else
.set OPTION_NEWLINE_PROC, 1
.set OPTION_CUSTOM_ENTRY, 1
.set OPTION_NO_RELOC, 1
.endif

.ifndef TARGET_SQL
.set TARGET_SQL, 0
.else
.set OPTION_NO_INTERSEG, 1
.set OPTION_NO_RELOC, 1
.set OPTION_ONLY_LF, 1
.endif

#### Z8000 targets

.ifndef TARGET_CPM8K
.set TARGET_CPM8K, 0
.endif

#### PDP-11 targets

.ifndef TARGET_DXDOS
.set TARGET_DXDOS, 0
.endif

#### ARM targets

.ifndef TARGET_RISCOS
.set TARGET_RISCOS, 0
.endif

#### Other targets

# either 8086/80386
.ifndef TARGET_OS2
.set TARGET_OS2, 0
.endif

#### MS-DOS formats

.ifndef FORMAT_COM
.set FORMAT_COM, 0
.else
.set OPTION_NO_RELOC, 1
.endif

.ifndef FORMAT_MZ
.set FORMAT_MZ, 0
.else
.set OPTION_CUSTOM_ENTRY, 1
.endif

.ifndef FORMAT_NE
.set FORMAT_NE, 0
.endif

.ifndef FORMAT_BW
.set FORMAT_BW, 0
.endif

.ifndef FORMAT_MP
.set FORMAT_MP, 0
.endif

.ifndef FORMAT_MQ
.set FORMAT_MQ, 0
.endif

.ifndef FORMAT_P3
.set FORMAT_P3, 0
.endif

#### CP/M-86/FlexOS formats

.ifndef FORMAT_CMD_TINY
.set FORMAT_CMD_TINY, 0
.endif

.ifndef FORMAT_CMD_SMALL
.set FORMAT_CMD_SMALL, 0
.else
.set OPTION_NO_DIFF_INTERSEG, 1
.endif

.ifndef FORMAT_CMD_COMPACT
.set FORMAT_CMD_COMPACT, 0
.else
.set OPTION_NO_DIFF_INTERSEG, 1
.endif

#### MINIX/ELKS formats

.ifndef FORMAT_AOUT_SEP
.set FORMAT_AOUT_SEP, 0
.else
.set FORMAT_AOUT, 1
.endif

.ifndef FORMAT_AOUT_COM
.set FORMAT_AOUT_COM, 0
.else
.set FORMAT_AOUT, 1
.endif

#### DJGPP formats

.ifndef FORMAT_AOUT
.set FORMAT_AOUT, 0
.endif

.ifndef FORMAT_COFF
.set FORMAT_COFF, 0
.endif

#### CP/M-68K formats

.ifndef FORMAT_68K_CONT
.set FORMAT_68K_CONT, 0
.endif

.ifndef FORMAT_68K_NONCONT
.set FORMAT_68K_NONCONT, 0
.else
.set OPTION_NO_INTERSEG, 1
.endif

#### TOS/GEMDOS formats

.ifndef FORMAT_PRG
.set FORMAT_PRG, 0
.endif

#### Human68k formats

.ifndef FORMAT_R
.set FORMAT_R, 0
.else
.set OPTION_NO_RELOC, 1
.endif

.ifndef FORMAT_Z
.set FORMAT_Z, 0
.endif

.ifndef FORMAT_X
.set FORMAT_X, 0
.else
.set OPTION_CUSTOM_ENTRY, 1
.endif

#### CP/M-8000 formats

.ifndef FORMAT_Z8K_NONSHARED
.set FORMAT_Z8K_NONSHARED, 0
.endif

.ifndef FORMAT_Z8K_SPLIT
.set FORMAT_Z8K_SPLIT, 0
.endif

.ifndef FORMAT_Z8K_SEGMENTED
.set FORMAT_Z8K_SEGMENTED, 0
.endif

#### Memory models

.ifndef MODEL_TINY
.set MODEL_TINY, 0
.endif

.ifndef MODEL_SMALL
.set MODEL_SMALL, 0
.endif

.ifndef MODEL_COMPACT
.set MODEL_COMPACT, 0
.endif

.ifndef MODEL_LARGE
.set MODEL_LARGE, 0
.endif

.ifndef MODEL_TINY
.set MODEL_TINY, 0
.endif

#### Options

.if TARGET_MSDOS & (FORMAT_MZ | FORMAT_NE | FORMAT_BW | FORMAT_MP | FORMAT_MQ | FORMAT_P3)
.set OPTION_EXPLICIT_STACK, 1
.endif

.if FORMAT_P3
.set OPTION_SEPARATE_STACK, 1
.endif

.if MODEL_COMPACT
.set OPTION_EXPLICIT_STACK, 1
.set OPTION_SEPARATE_STACK, 1
.endif

.if TARGET_MSDOS & FORMAT_NE
.set OPTION_NO_DIFF_INTERSEG, 1
.endif

# TODO: probably incorrect
#.if TARGET_DJGPP & FORMAT_AOUT
#.set OPTION_EXPLICIT_STACK, 1
#.endif

.if TARGET_RISCOS
.set OPTION_EXPLICIT_STACK, 1
.endif

#### Default options

.ifndef OPTION_CUSTOM_ENTRY
.set OPTION_CUSTOM_ENTRY, 0
.endif

.ifndef OPTION_NO_DIFF_INTERSEG
.set OPTION_NO_DIFF_INTERSEG, 0
.endif

.ifndef OPTION_EXPLICIT_STACK
.set OPTION_EXPLICIT_STACK, 0
.endif

.ifndef OPTION_SEPARATE_STACK
.set OPTION_SEPARATE_STACK, 0
.endif

.ifndef OPTION_NO_RELOC
.set OPTION_NO_RELOC, 0
.endif

.ifndef OPTION_NO_INTERSEG
.set OPTION_NO_INTERSEG, 0
.endif

.ifndef OPTION_NEWLINE_PROC
.set OPTION_NEWLINE_PROC, 0
.endif

.ifndef OPTION_ONLY_LF
.set OPTION_ONLY_LF, 0
.endif

.endif
# CPU_Z80

#### General includes

.if TARGET_CPM80 | TARGET_MSXDOS
.include "../include/cpm80.inc"
.endif

.if TARGET_CPM86
.include "../include/cpm86.inc"
.endif

.if TARGET_MSDOS
.include "../include/msdos.inc"
.endif

.if TARGET_ELKS
.include "../include/elks.inc"
.endif

.if TARGET_WIN16
.include "../include/win16.inc"
.endif

.if TARGET_OS2
.include "../include/os2.inc"
.endif

.if TARGET_CPM68K
.include "../include/cpm68k.inc"
.endif

.if TARGET_GEMDOS
.include "../include/gemdos.inc"
.endif

.if TARGET_HUMAN68K
.include "../include/human68k.inc"
.endif

.if TARGET_AMIGA
.include "../include/amiga.inc"
.endif

.if TARGET_MACOS
.include "../include/macos.inc"
.endif

.if TARGET_SQL
.include "../include/sql.inc"
.endif

.if TARGET_CPM8K
.include "../include/cpm8k.inc"
.endif

.if TARGET_RISCOS
.include "../include/riscos.inc"
.endif

.if TARGET_DXDOS
.include "../include/dxdos.inc"
.endif

