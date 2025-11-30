
all: main.exe lib.dll

clean:
	rm -rf *.o *.dll *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin64 $< -o $@ -d.entry=_start

lib.dll: lib.o
	../../../link -Fwin64 -Soutput=dll $< -o $@ -d.entry=_start

%.o: %.s
	x86_64-elf-as -msyntax=intel -mnaked-reg $< -o $@

