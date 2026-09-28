
all:
	make -C win16
	make -C win32
	make -C win64
	make -C flex286
	make -C flex386

clean:
	rm -rf *~
	make -C win16 clean
	make -C win32 clean
	make -C win64 clean
	make -C flex286 clean
	make -C flex386 clean

distclean: clean
	make -C win16 distclean
	make -C win32 distclean
	make -C win64 distclean
	make -C flex286 distclean
	make -C flex386 distclean

verify:
	for file in ref/*/*; do diff $$file $${file#ref/}; done

.PHONY: all clean distclean verify

