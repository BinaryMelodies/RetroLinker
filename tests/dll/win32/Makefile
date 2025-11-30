
all: main.exe lib.dll

clean:
	rm -rf *.o *.dll *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin32 $< -o $@ -d.entry=_start

lib.dll: lib.o
	../../../link -Fwin32 -Soutput=dll $< -o $@ -d.entry=_start

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

