
all: main.386 lib.slb

clean:
	rm -rf *.o *.386 *.slb

distclean: clean
	rm -rf *~

main.386: main.o
	../../../link -Fflexos386 $< -o $@ -d.entry=_start

lib.slb: lib.o
	../../../link -Fflexos386 -Sslib=lib.slb\$$1.0 $< -o $@ -d.entry=_start

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

