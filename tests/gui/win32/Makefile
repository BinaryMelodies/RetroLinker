
all: main.exe

clean:
	rm -rf *.o *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin32 $< -o $@ -d.entry=_start -Starget=win32s -Ssubsystem=windows -Simport_thunks

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

