
#if defined __ia16__
#define FP_SEG(value) ((unsigned short)((unsigned long)(value) >> 16))
#define FP_OFF(value) ((unsigned short)((unsigned long)(value) & 0xFFFF))
asm(
	".section\t.text\n\t"
#if FORMAT_COM
	"movw\t$AppStackTop, %sp\n\t"
#elif FORMAT_MZ || FORMAT_BW
	"movw\t%ss, %ax\n\t"
	"movw\t%ax, %ds\n\t"
#elif TARGET_CPM86
	"movw\t%ds, %ax\n\t"
	"movw\t%ax, %ss\n\t"
	"movw\t$AppStackTop, %sp\n\t"
#elif TARGET_WIN16
	// InitTask
	".extern\t$$IMPORT$KERNEL$005B, $$IMPSEG$KERNEL$005B\n\t"
	".byte\t0x9A\n\t"
	".word\t$$IMPORT$KERNEL$005B\n\t"
	".word\t$$IMPSEG$KERNEL$005B\n\t"
	"testw\t%ax, %ax\n\t"
	"jnz\t1f\n\t"
	"movw\t$0x4C01, %ax\n\t"
	"int\t$0x21\n"
	"1:\n\t"
	// hInstance, used by InitApp
	"pushw\t%di\n\t"
	"movw\t$0, %ax\n\t"
	"pushw\t%ax\n\t"
	// WaitEvent
	".extern\t$$IMPORT$KERNEL$001E, $$IMPSEG$KERNEL$001E\n\t"
	".byte\t0x9A\n\t"
	".word\t$$IMPORT$KERNEL$001E\n\t"
	".word\t$$IMPSEG$KERNEL$001E\n\t"
	// InitApp
	".extern\t$$IMPORT$USER$0005, $$IMPSEG$USER$0005\n\t"
	".byte\t0x9A\n\t"
	".word\t$$IMPORT$USER$0005\n\t"
	".word\t$$IMPSEG$USER$0005\n\t"
#endif
	"call\tAppMain\n\t"
	"call\tLibExit"
);
#elif defined __i386__
asm(
#if TARGET_WIN32
	"call\tLibInit\n\t"
#endif
	"call\tAppMain\n\t"
	"call\tLibExit"
);
#elif defined __amd64__
asm(
#if TARGET_WIN64
	"call\tLibInit\n\t"
#endif
	"call\tAppMain\n\t"
	"call\tLibExit"
);
#elif defined __m68k__
asm(
#if TARGET_GEMDOS
	"lea\tAppStackTop(%pc), %sp\n\t"
#elif FORMAT_X /*TARGET_HUMAN68K*/
	"lea\tAppStackTop(%pc), %sp\n\t"
#elif TARGET_MACOS
	"pea\t-4(%a5)\n\t"
	".word\t0xA86E\n\t"
	"bsrw\tLibInit\n\t"
#elif TARGET_AMIGA
	"move.l\t%sp, InitialStack.l\n\t"
	"bsrw\tLibInit\n\t"
#elif TARGET_SQL
	"move.l\t%sp, InitialStack.l\n\t"
	"move.l\t%fp, InitialFrame.l\n\t"
#endif
	"bsrw\tAppMain\n\t"
	"bsrw\tLibExit"
);
#elif defined __z8000__
asm(
	"calr\t_AppMain\n\t"
	"calr\t_LibExit"
);
#elif defined __arm__
asm(
	"ldr\tsp, =AppStackTop\n\t"
	"bl\tAppMain\n\t"
	"bl\tLibExit"
);
#elif defined __pdp11__
asm(
	"mov\t$0x200, sp\n\t"
	"jsr\tpc, _AppMain\n\t"
	"jsr\tpc, _LibExit"
);
#else
#error Unknown architecture
#endif

#if TARGET_WIN16
// must appear first in data segment
static char _instance_data[16] = { 1 };
#endif

#if TARGET_OS2V1
#define EXIT_THREAD 0
#define EXIT_PROCESS 1
void DosExit(unsigned short ActionCode, unsigned short ResultCode)
{
	asm volatile(
		"pushw\t%0\n\t"
		"pushw\t%1\n\t"
		".byte\t0x9A\n\t"
		".extern\t$$IMPORT$DOSCALLS$0005, $$IMPSEG$DOSCALLS$0005\n\t"
		".word\t$$IMPORT$DOSCALLS$0005\n\t"
		".word\t$$IMPSEG$DOSCALLS$0005"
			: : "g"(ActionCode), "g"(ResultCode));
}

unsigned short DosWrite(unsigned short FileHandle, void __far * BufferArea, unsigned short BufferLength, unsigned short __far * BytesWritten)
{
	register unsigned short result;
	asm(
		"pushw\t%1\n\t"
		"pushw\t%2\n\t"
		"pushw\t%3\n\t"
		"pushw\t%4\n\t"
		"pushw\t%5\n\t"
		"pushw\t%6\n\t"
		".byte\t0x9A\n\t"
		".extern\t$$IMPORT$DOSCALLS$008A, $$IMPSEG$DOSCALLS$008A\n\t"
		".word\t$$IMPORT$DOSCALLS$008A\n\t"
		".word\t$$IMPSEG$DOSCALLS$008A"
			: "=a"(result) : "g"(FileHandle), "g"(FP_SEG(BufferArea)), "g"(FP_OFF(BufferArea)), "g"(BufferLength), "g"(FP_SEG(BytesWritten)), "g"(FP_OFF(BytesWritten)));
	return result;
}
#elif TARGET_OS2V2
#define EXIT_THREAD 0
#define EXIT_PROCESS 1
void DosExit(unsigned short ActionCode, unsigned short ResultCode)
{
	asm volatile(
		"push\t%0\n\t"
		"push\t%1\n\t"
		".extern\t$$IMPORT$DOSCALLS$00EA\n\t"
		"call\t($$IMPORT$DOSCALLS$00EA)"
			: : "g"(ResultCode), "g"(ActionCode));
}

unsigned short DosWrite(unsigned long FileHandle, void * BufferArea, unsigned long BufferLength, unsigned long * BytesWritten)
{
	register unsigned short result;
	asm(
		"push\t%1\n\t"
		"push\t%2\n\t"
		"push\t%3\n\t"
		"push\t%4\n\t"
		".extern\t$$IMPORT$DOSCALLS$011A\n\t"
		"call\t($$IMPORT$DOSCALLS$011A)\n\t"
		"addl\t$16, %%esp"
			: "=a"(result) : "g"(BytesWritten), "g"(BufferLength), "g"(BufferArea), "g"(FileHandle));
	return result;
}
#elif TARGET_WIN32 || TARGET_WIN64
void ExitProcess(unsigned int uExitCode)
{
#if TARGET_WIN32
	asm volatile(
		"push\t%0\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$ExitProcess$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$ExitProcess$0000)"
			: : "g"(uExitCode));
#elif TARGET_WIN64
	asm volatile(
		"andq\t$~0xF, %%rsp\n\t"
		"subq\t$0x20, %%rsp\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$ExitProcess$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$ExitProcess$0000)(%%rip)"
			: : "c"(uExitCode));
#endif
}

void * GetStdHandle(unsigned int nStdHandle)
{
	register void * result;
#if TARGET_WIN32
	asm(
		"push\t%1\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$GetStdHandle$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$GetStdHandle$0000)"
			: "=a"(result) : "g"(nStdHandle));
#elif TARGET_WIN64
	asm(
		"pushq\t%%rbp\n\t"
		"movq\t%%rsp, %%rbp\n\t"
		"andq\t$~0xF, %%rsp\n\t"
		"subq\t$0x20, %%rsp\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$GetStdHandle$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$GetStdHandle$0000)(%%rip)\n\t"
		"movq\t%%rbp, %%rsp\n\t"
		"pop\t%%rbp"
			: "=a"(result) : "c"(nStdHandle));
#endif
	return result;
}

unsigned int WriteFile(void * hFile, void * lpBuffer, unsigned int nNumberOfBytesToWrite, void * lpNumberOfBytesWritten, void * lpOverlapped)
{
	register unsigned int result;
#if TARGET_WIN32
	asm(
		"push\t%5\n\t"
		"push\t%4\n\t"
		"push\t%3\n\t"
		"push\t%2\n\t"
		"push\t%1\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$WriteFile$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$WriteFile$0000)"
			: "=a"(result) : "g"(hFile), "g"(lpBuffer), "g"(nNumberOfBytesToWrite), "g"(lpNumberOfBytesWritten), "g"(lpOverlapped));
#elif TARGET_WIN64
	register unsigned int _nNumberOfBytesToWrite asm("r8") = nNumberOfBytesToWrite;
	register void * _lpNumberOfBytesWritten asm("r9") = lpNumberOfBytesWritten;
	asm(
		"pushq\t%%rbp\n\t"
		"movq\t%%rsp, %%rbp\n\t"
		"andq\t$~0xF, %%rsp\n\t"
		"subq\t$8, %%rsp\n\t"
		"push\t%5\n\t"
		"subq\t$0x20, %%rsp\n\t"
		".extern\t$$IMPORT$KERNEL32.dll$WriteFile$0000\n\t"
		"call\t*($$IMPORT$KERNEL32.dll$WriteFile$0000)(%%rip)\n\t"
		"movq\t%%rbp, %%rsp\n\t"
		"pop\t%%rbp"
			: "=a"(result) : "c"(hFile), "d"(lpBuffer), "r"(_nNumberOfBytesToWrite), "r"(_lpNumberOfBytesWritten), "r"(lpOverlapped));
#endif
	return result;
}
#elif TARGET_AMIGA
typedef unsigned long size_t;
static void * DosHandle;
static void * ConsoleHandle;
long Write(void * file, void * buffer, long length)
{
	/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01D1.html */
	register size_t d0 __asm__("d0");
	register size_t d1 __asm__("d1") = (size_t)file;
	register size_t d2 __asm__("d2") = (size_t)buffer;
	register size_t d3 __asm__("d3") = length;
	register size_t a6 = (size_t)DosHandle;
	asm("exg\t%1, %%a6\n\t\
jsr\t-48(%%a6)\n\t\
exg\t%1, %%a6" : "=r"(d0) : "r"(a6), "r"(d1), "r"(d2), "r"(d3));
	return d0;
}
#elif TARGET_MACOS
#include "../include/macos.h"
static inline struct a5world * a5(void)
{
	register struct a5world * a5 __asm__("a5");
	asm("" : "=r"(a5));
	return &a5[-1];
}

void InitGraf(void * globalPtr)
{
	asm("move.l\t%0, -(%%sp)\n\t\
.word\t0xA86E" : : "g"(globalPtr));
}

void DrawChar(short c)
{
	asm("move.w\t%0, -(%%sp)\n\t\
.word\t0xA883" : : "g"(c));
}

void TextMode(short mode)
{
	asm("move.w\t%0, -(%%sp)\n\t\
.word\t0xA889" : : "g"(mode));
}

void MoveTo(short x, short y)
{
	asm("move.w\t%0, -(%%sp)\n\t\
move.w\t%1, -(%%sp)\n\t\
.word\t0xA893" : : "g"(x), "g"(y));
}

short GetNextEvent(short mask, void * event)
{
	short result;
	asm("subq.l\t#2, %%sp\n\t\
move.w\t%1, -(%%sp)\n\t\
move.l\t%2, -(%%sp)\n\t\
.word\t0xA970\n\t\
move.w\t(%%sp)+, %0" : "=g"(result) : "g"(mask), "g"(event));
	return result;
}

void ExitToShell(void)
{
	asm(".word\t0xA9F4");
}

void InitFonts(void)
{
	asm(".word\t0xA8FE");
}

void InitWindows(void)
{
	asm(".word\t0xA912");
}

void InitMenus(void)
{
	asm(".word\t0xA930");
}

struct WindowRecord * NewCWindow(void * wStorage, const struct Rect * boundsRect, const unsigned char * title, bool visible, short theProc, struct WindowRecord * behind, bool goAwayFlag, long refCon)
{
	struct WindowRecord * result;
	asm("subq.l\t#4, %%sp\n\t\
move.l\t%1, -(%%sp)\n\t\
move.l\t%2, -(%%sp)\n\t\
move.l\t%3, -(%%sp)\n\t\
move.b\t%4, -(%%sp)\n\t\
move.w\t%5, -(%%sp)\n\t\
move.l\t%6, -(%%sp)\n\t\
move.b\t%7, -(%%sp)\n\t\
move.l\t%8, -(%%sp)\n\t\
.word\t0xAA45\n\t\
move.l\t(%%sp)+, %0\n\t\
" : "=g"(result) : "g"(wStorage), "g"(boundsRect), "g"(title), "g"(visible), "g"(theProc), "g"(behind), "g"(goAwayFlag), "g"(refCon));
	return result;
}

struct WindowRecord * NewWindow(void * wStorage, const struct Rect * boundsRect, const unsigned char * title, bool visible, short theProc, struct WindowRecord * behind, bool goAwayFlag, long refCon)
{
	struct WindowRecord * result;
	asm("subq.l\t#4, %%sp\n\t\
move.l\t%1, -(%%sp)\n\t\
move.l\t%2, -(%%sp)\n\t\
move.l\t%3, -(%%sp)\n\t\
move.b\t%4, -(%%sp)\n\t\
move.w\t%5, -(%%sp)\n\t\
move.l\t%6, -(%%sp)\n\t\
move.b\t%7, -(%%sp)\n\t\
move.l\t%8, -(%%sp)\n\t\
.word\t0xA913\n\t\
move.l\t(%%sp)+, %0\n\t\
" : "=g"(result) : "g"(wStorage), "g"(boundsRect), "g"(title), "g"(visible), "g"(theProc), "g"(behind), "g"(goAwayFlag), "g"(refCon));
	return result;
}

void SetPort(struct GrafPort * port)
{
	asm("move.l\t%0, -(%%sp)\n\t\
.word\t0xA873" : : "g"(port));
}

void TextFont(short font)
{
	asm("move.w\t%0, -(%%sp)\n\t\
.word\t0xA887" : : "g"(font));
}

void TextFace(short face)
{
	asm("move.w\t%0, -(%%sp)\n\t\
.word\t0xA888" : : "g"(face));
}

void TextSize(short size)
{
	asm("move.w\t%0, -(%%sp)\n\t\
.word\t0xA88A" : : "g"(size));
}

void FillRect(struct Rect * rect, struct Pattern * pattern)
{
	asm("move.l\t%0, -(%%sp)\n\t\
move.l\t%1, -(%%sp)\n\t\
.word\t0xA8A5" : : "g"(rect), "g"(pattern));
}

void CTabChanged(struct ColorTable ** ctab)
{
	asm("move.l\t%0, -(%%sp)\n\t\
.word\t0x203C\n\t\
.word\t0x0004\n\t\
.word\t0x0007\n\t\
.word\t0xAB1D" : : "g"(ctab));
}
#endif /* TARGET_MACOS */

void LibExit(void)
{
#if TARGET_MSDOS || TARGET_WIN16 || TARGET_DJGPP || TARGET_DOS4G || TARGET_PDOS386 || TARGET_DOS16M || TARGET_PHARLAP
	asm(
		"movw\t$0x4C00, %ax\n\t"
		"int\t$0x21"
	);
#elif TARGET_CPM86
	asm(
		"xorb\t%cl, %cl\n\t"
		"int\t$0xE0"
	);
#elif TARGET_ELKS
	asm(
		"xorw\t%bx, %bx\n\t"
		"movw\t$0x01, %ax\n\t"
		"int\t$0x80"
	);
#elif TARGET_OS2V1 || TARGET_OS2V2
	DosExit(EXIT_PROCESS, 0);
#elif TARGET_WIN32 || TARGET_WIN64
	ExitProcess(0);
#elif TARGET_CPM68K
	asm(
		"clr.w\t%d0\n\t"
		"trap\t#2"
	);
#elif TARGET_GEMDOS
	asm(
		"clr.w\t-(%sp)\n\t"
		"trap\t#1"
	);
#elif TARGET_HUMAN68K
	asm(
		".word\t0xFF00"
	);
#elif TARGET_AMIGA
	/* remove frame and return address */
	asm(
		"move.l\tInitialStack.l, %sp\n\t"
		"rts"
	);
#elif TARGET_SQL
	/* remove frame and return address */
	asm(
		"clr.l\t%d0\n\t"
		"move.l\tInitialStack.l, %sp\n\t"
		"move.l\tInitialFrame.l, %fp\n\t"
		"rts"
	);
#elif TARGET_MACOS
	ExitToShell();
#elif TARGET_CPM8K
	asm(
		"clr\tr5\n\t"
		"sc\t#2"
	);
#elif TARGET_RISCOS
	asm(
		"mov\tr0, #0\n\t"
		"swi\t0x11"
	);
#elif TARGET_DXDOS
	asm(
		"iot\n\t"
		".word\t0"
	);
#else
#warning Unimplemented: LibExit
#endif
}

#if TARGET_WIN32 || TARGET_WIN64
static void * hStdOut = 0;
#endif

void LibPutChar(char c)
{
#if TARGET_MSDOS || TARGET_DJGPP || TARGET_DOS4G || TARGET_PDOS386 || TARGET_DOS16M || TARGET_PHARLAP
# if defined __ia16__
	asm(
		"movb\t$0x02, %%ah\n\t"
		"int\t$0x21" : : "Rdl"(c));
# elif defined __i386__
	asm(
		"movb\t$0x02, %%ah\n\t"
		"int\t$0x21" : : "d"(c));
# endif
#elif TARGET_CPM86
	asm(
		"movb\t$0x02, %%cl\n\t"
		"int\t$0xE0" : : "Rdl"(c));
#elif TARGET_ELKS
	asm(
		"movw\t$0x04, %%ax\n\t"
		"movw\t$1, %%bx\n\t"
		"movw\t$1, %%dx\n\t"
		"int\t$0x80" : : "c"(&c));
#elif TARGET_WIN16
	char buff[2];
	buff[0] = c;
	buff[1] = '$';
	asm(
		"movb\t$0x09, %%ah\n\t"
		"int\t$0x21" : : "d"(&buff));
#elif TARGET_WIN32 || TARGET_WIN64
	unsigned int result;
	WriteFile(hStdOut, &c, 1, &result, 0);
#elif TARGET_OS2V1 || TARGET_OS2V2
	char buff[1];
	buff[0] = c;
# if TARGET_OS2V1
	unsigned short out;
# elif TARGET_OS2V2
	unsigned long out;
# endif
	DosWrite(1, buff, 1, &out);
#elif TARGET_CPM68K
	register short d1 asm("%d1") = (unsigned char)c;
	asm(
		"move.w\t#0x02, %%d0\n\t"
		"trap\t#2" : : "r"(d1));
#elif TARGET_GEMDOS
	asm(
		"move.w\t%0, -(%%sp)\n\t"
		"move.w\t#0x02, -(%%sp)\n\t"
		"trap\t#1\n\t"
		"addq.l\t#4, %%sp" : : "g"((short)(unsigned char)c));
#elif TARGET_HUMAN68K
	asm(
		"move.w\t%0, -(%%sp)\n\t"
		".word\t0xFF02\n\t"
		"addq.l\t#2, %%sp" : : "g"((short)(unsigned char)c));
#elif TARGET_AMIGA
	Write(ConsoleHandle, &c, 1);
#elif TARGET_SQL
	register char d1 asm("%d1") = c;
	asm(
		"and.l\t#0xFF, %%d1\n\t"
		"move.l\t#0x00010001, %%a0\n\t"
		"moveq.l\t#-1, %%d3\n\t"
		"moveq.l\t#5, %%d0\n\t"
		"trap\t#3" : : "r"(d1));
#elif TARGET_MACOS
	DrawChar(c);
#elif TARGET_CPM8K
	register short r7 asm("r1") = (unsigned char)c;
	asm(
		"ld\tr5, #0x02\n\t"
		"sc\t#2" : : "r"(r7));
#elif TARGET_RISCOS
	register int r0 asm("r0") = c;
	asm("swi\t0x00" : : "r"(r0));
#elif TARGET_DXDOS
	asm(
		"mov\t%0, r0\n\t"
		"iot\n\t"
		".word\t2" : : "g"(c));
#else
#warning Unimplemented: LibPutChar
#endif
}

void LibWaitForKey(void)
{
#if TARGET_MSDOS || TARGET_DJGPP || TARGET_DOS4G || TARGET_PDOS386 || TARGET_DOS16M || TARGET_PHARLAP
	asm(
		"mov\t$0x01, %ah\n\t"
		"int\t$0x21");
#elif TARGET_CPM86
	asm(
		"mov\t$0x01, %cl\n\t"
		"int\t$0xE0");
#elif TARGET_ELKS
	int tmp;
	asm(
		"movw\t$0x03, %%ax\n\t"
		"movw\t$1, %%bx\n\t"
		"movw\t$1, %%dx\n\t"
		"int\t$0x80" : : "c"(&tmp));
#elif TARGET_WIN16
	/* TODO */
	/*asm(
		"mov\t$0x01, %ah\n\t"
		"int\t$0x21");*/
#elif TARGET_WIN32 || TARGET_WIN64
	/* TODO */
#elif TARGET_OS2V1 || TARGET_OS2V2
	/* TODO */
#elif TARGET_CPM68K
	asm(
		"move.w\t#0x01, %d0\n\t"
		"trap\t#2");
#elif TARGET_GEMDOS
	asm(
		"move.w\t#0x01, -(%sp)\n\t"
		"trap\t#1\n\t"
		"addq.l\t#2, %sp");
#elif TARGET_HUMAN68K
	asm(
		".word\t0xFF01\n\t");
#elif TARGET_AMIGA
	/* TODO */
#elif TARGET_SQL
//	register unsigned char d1 asm("%d1");
	asm(
		"move.l\t#0x00010001, %a0\n\t"
		"moveq.l\t#-1, %d3\n\t"
		"moveq.l\t#1, %d0\n\t"
		"trap\t#3"); // : "=r"(d1));
#elif TARGET_MACOS
	union event event;
	do
	{
		while(GetNextEvent(everyEvent, &event) == 0)
			;
	} while(event.w[0] != 3);
#elif TARGET_CPM8K
	asm(
		"ld\tr5, #0x01\n\t"
		"sc\t#2");
#elif TARGET_RISCOS
	asm("swi\t0x04");
#elif TARGET_DXDOS
	asm(
		"iot\n\t"
		".word\t1");
#else
#warning Unimplemented: LibWaitForKey
#endif
}

void LibPutString(const char * s)
{
	unsigned i;
	for(i = 0; s[i]; i++)
	{
		LibPutChar(s[i]);
	}
}

void LibPutHex(unsigned value)
{
	char buffer[sizeof(value) * 2 + 1];
	char * ptr = buffer + sizeof(buffer);
	*--ptr = '\0';
	do
	{
		int digit = value & 0xF;
		if(digit < 10)
			*--ptr = digit + '0';
		else
			*--ptr = digit + 'A' - 10;
		value >>= 4;
	} while(value != 0);
	LibPutString(ptr);
}

#if TARGET_MACOS
#define font_height 15
#endif

void LibPutNewLine(void)
{
#if TARGET_MACOS
	MoveTo(0, a5()->current_row += font_height);
#elif TARGET_AMIGA | TARGET_SQL
	LibPutString("\n");
#else
	LibPutString("\r\n");
#endif
}

#if TARGET_SQL
static void * InitialStack;
static void * InitialFrame;
#endif

#if TARGET_WIN32 || TARGET_WIN64
void LibInit(void)
{
	hStdOut = GetStdHandle(-11);
}
#elif TARGET_AMIGA
static void * InitialStack;

/* https://anadoxin.org/blog/amigaos-stdlib-vector-tables.html/ */

void * OpenLibrary(const char * name, int version)
{
	/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0222.html */
	register size_t d0 __asm__("d0") = version;
	register size_t a1 __asm__("a1") = (size_t)name;
	register size_t a6 = *(size_t *)4;
	asm("exg\t%1, %%a6\n\t\
jsr\t-552(%%a6)\n\t\
exg\t%1, %%a6" : "=r"(d0) : "r"(a6), "r"(d0), "r"(a1));
	return (void *)d0;
}

void * Open(const char * filename, int accessmode)
{
	/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node02D6.html */
	register size_t d0 __asm__("d0");
	register size_t d1 __asm__("d1") = (size_t)filename;
	register size_t d2 __asm__("d2") = accessmode;
	register size_t a6 = (size_t)DosHandle;
	asm("exg\t%1, %%a6\n\t\
jsr\t-30(%%a6)\n\t\
exg\t%1, %%a6" : "=r"(d0) : "r"(a6), "r"(d1), "r"(d2));
	return (void *)d0;
}

void LibInit(void)
{
	DosHandle = OpenLibrary("dos.library", 0);
	ConsoleHandle = Open("CONSOLE:", 1005);
}
#elif TARGET_MACOS
void LibInit(void)
{
	InitFonts();
	InitWindows();
//	InitMenus();
	TextMode(srcCopy);
	a5()->current_row = 32;
	MoveTo(0, 32);
}
#endif

#if TARGET_MSDOS || TARGET_CPM86 || TARGET_GEMDOS || TARGET_HUMAN68K || TARGET_DOS16M || TARGET_PHARLAP
char * AppStack[512] __attribute__((section(".stack,\"aw\",@nobits\n#")));
char * AppStackTop __attribute__((section(".stack,\"aw\",@nobits\n#")));
#endif

#if TARGET_RISCOS
int * AppStack[512/sizeof(int)];
int * AppStackTop;
#endif

//#if TARGET_DOS16M
///* TODO: at least one relocation is required */
//asm(".section .data\n\
//	.word	$$SEG$.data");
//#endif

#if	TARGET_MACOS
asm("	.section	$$RSRC$_SIZE$FFFF, \"a\", @progbits\n\
	.byte	0x58\n\
	.byte	0x80\n\
	.long	100 * 1024\n\
	.long	100 * 1024");
#endif

