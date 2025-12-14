
all:
	make -C win16
	make -C win32
	make -C win64

clean:
	make -C win16 clean
	make -C win32 clean
	make -C win64 clean

distclean: clean
	rm -rf *~
	make -C win16 distclean
	make -C win32 distclean
	make -C win64 distclean

verify: all
	for file in ref/*/*; do diff $$file $${file#ref/}; done

.PHONY: all clean distclean verify

