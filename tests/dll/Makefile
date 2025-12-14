
all:
	make -C win16
	make -C win32
	make -C win64

clean:
	rm -rf *~
	make -C win16 clean
	make -C win32 clean
	make -C win64 clean

distclean: clean
	make -C win16 distclean
	make -C win32 distclean
	make -C win64 distclean

verify:
	#make -C win16 verify
	#make -C win32 verify
	#make -C win64 verify

