
all: main.cmd

clean:
	rm -rf *.o *.rsx *.cmd

distclean: clean
	rm -rf *~

verify: main.cmd
	diff main.cmd ref/main.cmd

rsx1.o: rsx1.s common.h
	i686-elf-as $< -o $@

rsx1.rsx: rsx1.o
	../../../link -Frsx $< -o $@

rsx2.o: rsx2.s common.h
	i686-elf-as $< -o $@

rsx2.rsx: rsx2.o
	../../../link -Frsx $< -o $@

main.o: main.s common.h
	i686-elf-as $< -o $@

main.cmd: main.o rsx1.rsx rsx2.rsx
	../../../link -Fcmd $< -o $@ -Srsx=rsx1.rsx,rsx2.rsx

.PHONY: all clean distclean verify
