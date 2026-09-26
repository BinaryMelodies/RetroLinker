
all: main.286 lib.srl

clean:
	rm -rf *.o *.286 *.srl

distclean: clean
	rm -rf *~

main.286: main.o
	../../../link -Fflexos $< -o $@ -d.entry=_start

lib.srl: lib.o
	../../../link -Fflexos -Ssrtl=LIB.1.0 $< -o $@ -d.entry=_start

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

