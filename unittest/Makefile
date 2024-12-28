
LINKER_HEADERS=$(addprefix ../linker/, format.h linker.h location.h module.h position.h reader.h relocation.h resolution.h section.h segment.h symbol.h target.h writable.h writer.h)
LINKER_CXXFILES=$(LINKER_HEADERS:.h=.cc)
LINKER_OFILES=$(LINKER_CXXFILES:.cc=.o)

FORMAT_HEADERS=$(addprefix ../format/, 8bitexe.h aout.h as86obj.h binary.h bwexp.h coff.h cpm68k.h cpm86.h cpm8k.h dosexe.h elf.h geos.h gsos.h huexe.h hunk.h leexe.h macho.h macos.h minix.h mzexe.h neexe.h o65.h omf.h peexe.h pefexe.h pharlap.h pmode.h xenix.h xpexp.h)
FORMAT_CXXFILES=$(FORMAT_HEADERS:.h=.cc)
FORMAT_OFILES=$(FORMAT_CXXFILES:.cc=.o)

DUMPER_HEADERS=../dumper/dumper.h
DUMPER_CXXFILES=$(DUMPER_HEADERS:.h=.cc)
DUMPER_OFILES=$(DUMPER_CXXFILES:.cc=.o)

SCRIPT_HEADERS=../script/script.h
SCRIPT_CXXFILES=../script/scan.cc ../script/parse.tab.cc
SCRIPT_OFILES=$(SCRIPT_CXXFILES:.cc=.o)

MAIN_HEADERS=../common.h
MAIN_CXXFILES=../common.cc
MAIN_OFILES=$(MAIN_CXXFILES:.cc=.o)

CXXFLAGS=-Wall -g

.PHONY: all clean distclean

all:
	g++ -o main main.cc $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) -lcppunit $(CXXFLAGS)

clean:
	rm -rf main results.xml

distclean: clean
	rm -rf *~ linker/*~ format/*~

