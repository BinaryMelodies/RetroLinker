
all: main.com

clean:
	rm -rf *.o *.rsx *.com

distclean: clean
	rm -rf *~

verify: main.com
	diff main.com ref/main.com

rsx1.o: rsx1.s common.h
	z80-coff-as $< -o $@

rsx1.rsx: rsx1.o
	../../../link -Fprl $< -o $@

rsx2.o: rsx2.s common.h
	z80-coff-as $< -o $@

rsx2.rsx: rsx2.o
	../../../link -Fprl $< -o $@

main.o: main.s common.h
	z80-coff-as $< -o $@

main.com: main.o rsx1.rsx rsx2.rsx
	../../../link -Fcpm3 $< -o $@ -Srsx=rsx1.rsx,rsx2.rsx

.PHONY: all clean distclean verify
