
all: main.exe

clean:
	rm -rf *.o *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fwin16 $< -o $@ -d.entry=_start

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

