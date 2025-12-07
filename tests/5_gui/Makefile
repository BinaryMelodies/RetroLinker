
all:
	python3 ../../bake.py

.PHONY: all clean distclean verify

clean:
	python3 ../../bake.py clean

distclean: clean
	rm -rf *~

verify: all
	for file in ref/*; do diff $$file `basename $$file`; done
	for file in ref/.rsrc/*; do diff $$file .rsrc/`basename $$file`; done

