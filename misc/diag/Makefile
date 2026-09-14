
Z80_AS=z80-coff-as
I86_AS=ia16-elf-as -msyntax=intel -mnaked-reg
M68K_AS=m68k-elf-as -m68000 --bitwise-or --register-prefix-optional
LINK=../../link
Z8K_AS=z8k-coff-as

all: bin/x80/c.com bin/x80/m.prl bin/x80/x.com bin/x86/b.com bin/x86/m.exe bin/x86/n.exe bin/x86/t.cmd bin/x86/s.cmd bin/x86/c.cmd bin/m68k/c.68k bin/m68k/n.68k bin/m68k/g.prg bin/m68k/r.r bin/m68k/z.z bin/m68k/x.x bin/z8k/n.z8k bin/z8k/p.z8k bin/z8k/g.z8k

clean:
	rm -rf bin obj

distclean: clean
	rm -rf *~ src/*~

######## CP/M-80

#### .COM

obj/x80/com.o: src/x80.s
	mkdir -p `dirname $@`
	${Z80_AS} $< -o $@ --defsym SYSTEM_x80=1 --defsym FORMAT_COM=1

bin/x80/c.com: obj/x80/com.o
	mkdir -p `dirname $@`
	${LINK} -Fcom $< -o $@

obj/x80/prl.o: src/x80.s
	mkdir -p `dirname $@`
	${Z80_AS} $< -o $@ --defsym SYSTEM_x80=1 --defsym FORMAT_PRL=1

bin/x80/m.prl: obj/x80/prl.o
	mkdir -p `dirname $@`
	${LINK} -Fprl $< -o $@

obj/x80/rsx1.o: src/x80.s
	mkdir -p `dirname $@`
	${Z80_AS} $< -o $@ --defsym SYSTEM_x80=1 --defsym FORMAT_RSX=1 --defsym RSX_INDEX=1

obj/x80/rsx1.rsx: obj/x80/rsx1.o
	mkdir -p `dirname $@`
	${LINK} -Fprl $< -o $@

obj/x80/rsx2.o: src/x80.s
	mkdir -p `dirname $@`
	${Z80_AS} $< -o $@ --defsym SYSTEM_x80=1 --defsym FORMAT_RSX=1 --defsym RSX_INDEX=2

obj/x80/rsx2.rsx: obj/x80/rsx2.o
	mkdir -p `dirname $@`
	${LINK} -Fprl $< -o $@

obj/x80/cpm3.o: src/x80.s
	mkdir -p `dirname $@`
	${Z80_AS} $< -o $@ --defsym SYSTEM_x80=1 --defsym FORMAT_CPM3=1

bin/x80/x.com: obj/x80/cpm3.o obj/x80/rsx1.rsx obj/x80/rsx2.rsx
	mkdir -p `dirname $@`
	${LINK} -Fcpm3 $< -o $@ -Srsx=rsx1=obj/x80/rsx1.rsx,rsx2=obj/x80/rsx2.rsx

######## MS-DOS

#### .COM

obj/x86/com.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_MSDOS=1 --defsym FORMAT_COM=1

bin/x86/b.com: obj/x86/com.o
	mkdir -p `dirname $@`
	${LINK} -Fcom $< -o $@

#### MZ .EXE

obj/x86/mz.exe.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_MSDOS=1 --defsym FORMAT_MZ_EXE=1

bin/x86/m.exe: obj/x86/mz.exe.o
	mkdir -p `dirname $@`
	${LINK} -Fmz $< -o $@ -d.entry=entry -d.stack_top=stack_top -Sheader_align=512

#### NE .EXE

obj/x86/ne.exe.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_MSDOS=1 --defsym FORMAT_NE_EXE=1

bin/x86/n.exe: obj/x86/ne.exe.o
	mkdir -p `dirname $@`
	${LINK} -Fdos4 $< -o $@ -d.entry=entry -d.stack_top=stack_top

######## CP/M-86

#### Tiny/8080 model .CMD

obj/x86/tiny.cmd.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_CPM86=1 --defsym FORMAT_CMD_TINY=1

bin/x86/t.cmd: obj/x86/tiny.cmd.o
	mkdir -p `dirname $@`
	${LINK} -Fcmd_tiny $< -o $@

#### Small model .CMD

obj/x86/small.cmd.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_CPM86=1 --defsym FORMAT_CMD_SMALL=1

bin/x86/s.cmd: obj/x86/small.cmd.o
	mkdir -p `dirname $@`
	${LINK} -Fcmd_small $< -o $@

#### Compact model .CMD

obj/x86/compact.cmd.o: src/i86.s
	mkdir -p `dirname $@`
	${I86_AS} $< -o $@ --defsym SYSTEM_CPM86=1 --defsym FORMAT_CMD_COMPACT=1

bin/x86/c.cmd: obj/x86/compact.cmd.o
	mkdir -p `dirname $@`
	${LINK} -Fcmd_compact $< -o $@

######## CP/M-68K

#### Contiguous .68K

obj/m68k/601a.68k.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_CPM68K=1 --defsym FORMAT_601A=1

bin/m68k/c.68k: obj/m68k/601a.68k.o
	mkdir -p `dirname $@`
	${LINK} -Fcpm68k $< -o $@

#### Non-contiguous .68K

obj/m68k/601b.68k.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_CPM68K=1 --defsym FORMAT_601B=1

bin/m68k/n.68k: obj/m68k/601b.68k.o
	mkdir -p `dirname $@`
	${LINK} -Fcpm68k_noncont $< -o $@

######## GEMDOS/Atari TOS

#### Contiguous .PRG

obj/m68k/prg.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_GEMDOS=1 --defsym FORMAT_601A=1

bin/m68k/g.prg: obj/m68k/prg.o
	mkdir -p `dirname $@`
	${LINK} -Ftos $< -o $@

######## Human68k

#### .Z

obj/m68k/r.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_HUMAN68K=1 --defsym FORMAT_RFILE=1

bin/m68k/r.r: obj/m68k/r.o
	mkdir -p `dirname $@`
	${LINK} -Frfile $< -o $@

obj/m68k/x.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_HUMAN68K=1 --defsym FORMAT_HU_XFILE=1

bin/m68k/x.x: obj/m68k/x.o
	mkdir -p `dirname $@`
	${LINK} -Fxfile $< -o $@

obj/m68k/z.o: src/m68k.s
	mkdir -p `dirname $@`
	${M68K_AS} $< -o $@ --defsym SYSTEM_HUMAN68K=1 --defsym FORMAT_601A=1

bin/m68k/z.z: obj/m68k/z.o
	mkdir -p `dirname $@`
	${LINK} -Fzfile $< -o $@

######## CP/M-8000

#### Segmented .Z8K

obj/z8k/ee01.o: src/z8k.s
	mkdir -p `dirname $@`
	${Z8K_AS} $< -o $@ --defsym SYSTEM_CPM8K=1 --defsym FORMAT_EE01=1

bin/z8k/g.z8k: obj/z8k/ee01.o
	mkdir -p `dirname $@`
	${LINK} -Fee01 $< -o $@

#### Non-shared .Z8K

obj/z8k/ee03.o: src/z8k.s
	mkdir -p `dirname $@`
	${Z8K_AS} $< -o $@ --defsym SYSTEM_CPM8K=1 --defsym FORMAT_EE03=1

bin/z8k/n.z8k: obj/z8k/ee03.o
	mkdir -p `dirname $@`
	${LINK} -Fee03 $< -o $@

#### Split I&D .Z8K

obj/z8k/ee0b.o: src/z8k.s
	mkdir -p `dirname $@`
	${Z8K_AS} $< -o $@ --defsym SYSTEM_CPM8K=1 --defsym FORMAT_EE0B=1

bin/z8k/p.z8k: obj/z8k/ee0b.o
	mkdir -p `dirname $@`
	${LINK} -Fee0b $< -o $@

