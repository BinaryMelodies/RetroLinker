
all: w/win16.exe l/win16.exe w/win16.dll l/win16.dll w/os2v1.exe l/os2v1.exe w/pmv1.exe l/pmv1.exe w/os2v1.dll l/os2v1.dll w/win16.386 l/win16.386 w/dos4gexe.exe l/dos4gexe.exe w/os2v2.exe l/os2v2.exe w/pmv2.exe l/pmv2.exe w/os2v2.dll l/os2v2.dll w/win32.exe w/gui32.exe w/win32.dll
# TODO: maybe l/win32.exe l/gui32.exe l/win32.dll

.PHONY: all clean distclean verify

clean:
	rm -rf w l *.o

distclean: clean
	rm -rf *~

verify: all
	for file in bak/*; do diff $$file l/`basename $$file`; done

w:
	if [ ! -d w ]; then mkdir w; fi

l:
	if [ ! -d l ]; then mkdir l; fi

w/win16.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_OBJ=1
	wlink option quiet system windows name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/win16.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_ELF=1
	../../link -Fwin `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=? -Mlarge
	mv -f `basename $@` $@

w/win16.dll: source.asm w
	nasm -fobj $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_OBJ=1
	wlink option quiet system windows dll name `basename $@` file `basename $@ .dll`.o option stub=stub
	mv -f `basename $@` $@

l/win16.dll: source.asm l ../../link
	nasm -felf $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_ELF=1
	../../link -Fwin_dll `basename $@ .dll`.o -o `basename $@` -d.entry=_start -$$=? -Mlarge
	mv -f `basename $@` $@

w/os2v1.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_OBJ=1
	wlink option quiet system os2 name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/os2v1.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_ELF=1
	../../link -Fos2v1 `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=? -Mlarge
	mv -f `basename $@` $@

w/pmv1.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_OBJ=1
	wlink option quiet system os2_pm name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/pmv1.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_ELF=1
	../../link -Fos2v1_pm `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=? -Mlarge
	mv -f `basename $@` $@

w/os2v1.dll: source.asm w
	nasm -fobj $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_OBJ=1
	wlink option quiet system os2_dll name `basename $@` file `basename $@ .dll`.o option stub=stub
	mv -f `basename $@` $@

l/os2v1.dll: source.asm l ../../link
	nasm -felf $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I86=1 -DTARGET_ELF=1
	../../link -Fos2v1_dll `basename $@ .dll`.o -o `basename $@` -d.entry=_start -$$=? -Mlarge
	mv -f `basename $@` $@

w/win16.386: source.asm w
	nasm -fobj $< -o `basename $@ .386`.o -DTARGET_WIN16=1 -DOUTPUT_VXD=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system win_vxd name `basename $@` file `basename $@ .386`.o option stub=stub export OneExport
	mv -f `basename $@` $@

l/win16.386: source.asm l ../../link
	nasm -felf $< -o `basename $@ .386`.o -DTARGET_WIN16=1 -DOUTPUT_VXD=1 -DCPU_I386=1 -DTARGET_ELF=1
	../../link -Fwin_vxd `basename $@ .386`.o -o `basename $@` -d.entry=_start -$$=?
	mv -f `basename $@` $@

w/dos4gexe.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system dos4g name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/dos4gexe.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_ELF=1
	../../link -Fdos4g `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=?
	mv -f `basename $@` $@

w/os2v2.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system os2v2 name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/os2v2.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_ELF=1
	../../link -Fos2v2 `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=?
	mv -f `basename $@` $@

w/pmv2.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system os2v2_pm name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

l/pmv2.exe: source.asm l ../../link
	nasm -felf $< -o `basename $@ .exe`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_ELF=1
	../../link -Fos2v2_pm `basename $@ .exe`.o -o `basename $@` -d.entry=_start -$$=?
	mv -f `basename $@` $@

w/os2v2.dll: source.asm w
	nasm -fobj $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system os2v2_dll name `basename $@` file `basename $@ .dll`.o option stub=stub
	mv -f `basename $@` $@

l/os2v2.dll: source.asm l ../../link
	nasm -felf $< -o `basename $@ .dll`.o -DTARGET_WIN16=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_ELF=1
	../../link -Fos2v2_dll `basename $@ .dll`.o -o `basename $@` -d.entry=_start -$$=?
	mv -f `basename $@` $@

w/win32.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN32=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system nt name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

w/gui32.exe: source.asm w
	nasm -fobj $< -o `basename $@ .exe`.o -DTARGET_WIN32=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system nt_win name `basename $@` file `basename $@ .exe`.o option stub=stub
	mv -f `basename $@` $@

w/win32.dll: source.asm w
	nasm -fobj $< -o `basename $@ .dll`.o -DTARGET_WIN32=1 -DOUTPUT_CLI=1 -DCPU_I386=1 -DTARGET_OBJ=1
	wlink option quiet system nt dll name `basename $@` file `basename $@ .dll`.o option stub=stub
	mv -f `basename $@` $@

