
all: main.exe lib.dll

clean:
	rm -rf *.o *.dll *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin64 $< -o $@ -d.entry=_start -Starget=winnt -Ssubsystem=windows -Simport_thunks

lib.dll: lib.o
	../../../link -Fwin64 -Soutput=dll $< -o $@ -d.entry=_start -Starget=winnt -Ssubsystem=windows -Simport_thunks

%.o: %.s
	x86_64-elf-as -msyntax=intel -mnaked-reg $< -o $@

