
LINKER_HEADERS=$(addprefix src/linker/, buffer.h format.h linker.h location.h module.h position.h reader.h reference.h relocation.h resolution.h section.h segment.h symbol.h target.h writable.h writer.h)
LINKER_CXXFILES=$(LINKER_HEADERS:.h=.cc)
LINKER_OFILES=$(LINKER_CXXFILES:.cc=.o)

FORMAT_HEADERS=$(addprefix src/format/, 8bitexe.h aout.h as86obj.h binary.h bwexp.h coff.h cpm68k.h cpm86.h cpm8k.h dosexe.h elf.h geos.h gsos.h huexe.h hunk.h leexe.h macho.h macos.h minix.h mzexe.h neexe.h o65.h omf.h peexe.h pefexe.h pharlap.h pmode.h xenix.h xpexp.h)
FORMAT_CXXFILES=$(FORMAT_HEADERS:.h=.cc)
FORMAT_OFILES=$(FORMAT_CXXFILES:.cc=.o)

DUMPER_HEADERS=src/dumper/dumper.h
DUMPER_CXXFILES=$(DUMPER_HEADERS:.h=.cc)
DUMPER_OFILES=$(DUMPER_CXXFILES:.cc=.o)

SCRIPT_HEADERS=src/script/script.h
SCRIPT_CXXFILES=src/script/scan.cc src/script/parse.tab.cc
SCRIPT_OFILES=$(SCRIPT_CXXFILES:.cc=.o)

MAIN_HEADERS=src/common.h src/formats.h
MAIN_CXXFILES=$(MAIN_HEADERS:.h=.cc)
MAIN_OFILES=$(MAIN_CXXFILES:.cc=.o)

CXXFLAGS=-Wall -Wsuggest-override -Wold-style-cast -std=c++20
CXXFLAGS+= -O2
CXXFLAGS+= -g
LDFLAGS=-O2

all: link dump

.PHONY: all clean distclean tests tests_clean verify force docs unittests

link: src/link.o $(MAIN_HEADERS) $(MAIN_OFILES) $(LINKER_HEADERS) $(LINKER_OFILES) $(FORMAT_HEADERS) $(FORMAT_OFILES) $(DUMPER_HEADERS) $(DUMPER_OFILES) $(SCRIPT_HEADERS) $(SCRIPT_OFILES)
	g++ -o link src/link.o $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) $(CXXFLAGS) $(LDFLAGS)

dump: src/dump.o $(MAIN_HEADERS) $(MAIN_OFILES) $(LINKER_HEADERS) $(LINKER_OFILES) $(FORMAT_HEADERS) $(FORMAT_OFILES) $(DUMPER_HEADERS) $(DUMPER_OFILES) $(SCRIPT_HEADERS) $(SCRIPT_OFILES)
	g++ -o dump src/dump.o $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) $(CXXFLAGS) $(LDFLAGS)

force:
	rm -f link dump
	make all

clean: tests_clean
	rm -rf link dump $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) src/script/scan.cc src/script/parse.tab.cc src/script/parse.tab.hh

tests_clean:
	$(MAKE) -C tests/1_hello clean
	$(MAKE) -C tests/2_asm clean
	$(MAKE) -C tests/3_extern clean
	$(MAKE) -C tests/4_ctest clean
	$(MAKE) -C tests/watcom clean
	$(MAKE) -C tests/rsx/z80 clean
	$(MAKE) -C tests/rsx/i86 clean
	$(MAKE) -C unittest clean

distclean: clean
	rm -rf *~ src/*~ src/format/*~ src/linker/*~ src/dumper/*~ src/script/*~ __pycache__ results.xml
	rm -rf latex docs doxygen.log
	$(MAKE) -C tests/include distclean
	$(MAKE) -C tests/1_hello distclean
	$(MAKE) -C tests/2_asm distclean
	$(MAKE) -C tests/3_extern distclean
	$(MAKE) -C tests/4_ctest distclean
	$(MAKE) -C tests/watcom distclean
	$(MAKE) -C tests/rsx/z80 distclean
	$(MAKE) -C tests/rsx/i86 distclean
	$(MAKE) -C unittest distclean

tests:
	$(MAKE) -C tests/1_hello
	$(MAKE) -C tests/2_asm
	$(MAKE) -C tests/3_extern
	$(MAKE) -C tests/4_ctest
	$(MAKE) -C tests/watcom
	$(MAKE) -C tests/rsx/z80
	$(MAKE) -C tests/rsx/i86

verify:
	$(MAKE) -C tests/1_hello verify
	$(MAKE) -C tests/2_asm verify
	$(MAKE) -C tests/3_extern verify
	$(MAKE) -C tests/4_ctest verify
	$(MAKE) -C tests/watcom verify
	$(MAKE) -C tests/rsx/z80 verify
	$(MAKE) -C tests/rsx/i86 verify

unittests: link
	$(MAKE) -C unittest
	unittest/main

docs:
	doxygen Doxyfile
	$(MAKE) -C latex

src/script/scan.cc: src/script/scan.lex src/script/parse.tab.hh
	flex -o $@ $<

src/script/parse.tab.hh: src/script/parse.tab.cc

src/script/parse.tab.cc: src/script/parse.yy
	bison3 -d -o $@ $<

