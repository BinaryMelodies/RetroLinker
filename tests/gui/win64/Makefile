
all: main.exe

clean:
	rm -rf *.o *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin64 $< -o $@ -d.entry=_start -Starget=winnt -Ssubsystem=windows -Simport_thunks

%.o: %.s
	x86_64-elf-as -msyntax=intel -mnaked-reg $< -o $@

