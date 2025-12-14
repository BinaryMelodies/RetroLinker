
all: main.exe

clean:
	rm -rf *.o *.exe

distclean: clean
	rm -rf *~

main.exe: main.o
	../../../link -Fos2_pm $< -o $@ -d.entry=_start

%.o: %.s
	i686-elf-as -msyntax=intel -mnaked-reg $< -o $@

