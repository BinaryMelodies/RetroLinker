
all: main.exe

clean:
	rm -rf *.o *.exe

distclean: clean
	rm -rf *~

.PHONY: all clean distclean

main.exe: init.o main.o libc.o
	Link -Fwin16 $^ -o $@ -d.entry=_start

%.o: %.c windows.h
	ia16-elf-gcc -c -mcmodel=small -mprotected-mode $< -o $@ #-mno-callee-assume-ds-ss

%.o: %.s
	ia16-elf-as $< -o $@

