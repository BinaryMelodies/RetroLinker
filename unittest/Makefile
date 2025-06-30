
LINKER_HEADERS=$(addprefix ../src/linker/, buffer.h format.h image.h location.h module.h module_collector.h options.h position.h reader.h reference.h relocation.h resolution.h section.h segment.h segment_manager.h symbol_definition.h symbol_name.h target.h writer.h)
LINKER_CXXFILES=$(LINKER_HEADERS:.h=.cc)
LINKER_OFILES=$(LINKER_CXXFILES:.cc=.o)

FORMAT_HEADERS=$(addprefix ../src/format/, 8bitexe.h aif.h aout.h arch.h as86obj.h binary.h bflt.h bwexp.h coff.h cpm68k.h cpm86.h cpm8k.h dosexe.h elf.h epoc.h geos.h gsos.h huexe.h hunk.h java.h leexe.h macho.h macos.h minix.h mzexe.h neexe.h o65.h omf.h pcos.h peexe.h pefexe.h pharlap.h pmode.h w3exe.h xenix.h xpexp.h)
FORMAT_CXXFILES=$(FORMAT_HEADERS:.h=.cc)
FORMAT_OFILES=$(FORMAT_CXXFILES:.cc=.o)

DUMPER_HEADERS=../src/dumper/dumper.h
DUMPER_CXXFILES=$(DUMPER_HEADERS:.h=.cc)
DUMPER_OFILES=$(DUMPER_CXXFILES:.cc=.o)

SCRIPT_HEADERS=../src/script/script.h
SCRIPT_CXXFILES=../src/script/scan.cc ../src/script/parse.tab.cc
SCRIPT_OFILES=$(SCRIPT_CXXFILES:.cc=.o)

MAIN_HEADERS=../src/common.h
MAIN_CXXFILES=$(MAIN_HEADERS:.h=.cc)
MAIN_OFILES=$(MAIN_CXXFILES:.cc=.o)

CXXFLAGS=-Wall -Wsuggest-override -Woverloaded-virtual -Wold-style-cast -std=c++20
CXXFLAGS+= -g

.PHONY: all clean distclean

all: main

main: main.cc $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) linker/*.cc format/*.cc
	$(CXX) -o main main.cc $(MAIN_OFILES) $(LINKER_OFILES) $(FORMAT_OFILES) $(DUMPER_OFILES) $(SCRIPT_OFILES) -lcppunit $(CXXFLAGS)

clean:
	rm -rf main results.xml

distclean: clean
	rm -rf *~ linker/*~ format/*~

