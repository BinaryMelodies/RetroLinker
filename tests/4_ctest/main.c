
void LibExit(void);
void LibPutChar(char c);
void LibWaitForKey(void);
void LibPutString(const char * s);
void LibPutNewLine(void);
void LibPutHex(unsigned value);

int v1 = 0x1234;
const int v2 = 0x2345;
static int v3 = 0x3456;
//#if !TARGET_MACOS
int v4;
static int v5;
//#endif

int AppMain(void)
{
	LibPutString("Hello! Welcome to "
#if TARGET_DOS16M
		"MS-DOS, DOS/16M extender"
#elif TARGET_MSDOS
		"MS-DOS"
#elif TARGET_DJGPP
		"MS-DOS, DJGPP DOS extender"
#elif TARGET_DOS4G
		"MS-DOS, DOS/4G extender"
#elif TARGET_PHARLAP
		"MS-DOS, 386|DOS-Extender extender"
#elif TARGET_PDOS386
		"PDOS/386 (or PD-Windows)"
#elif TARGET_CPM86
		"CP/M-86"
#elif TARGET_ELKS
		"ELKS (Linux-8086)"
#elif TARGET_WIN16
		"Windows (16-bit)"
#elif TARGET_WIN32
		"Windows (32-bit)"
#elif TARGET_OS2V1
		"OS/2 (16-bit)"
#elif TARGET_OS2V2
		"OS/2 (32-bit)"
#elif TARGET_CPM68K
		"CP/M-68000"
#elif TARGET_GEMDOS
		"Atari TOS/GEMDOS"
#elif TARGET_HUMAN68K
		"Human68k"
#elif TARGET_AMIGA
		"AmigaOS"
#elif TARGET_MACOS
		"Mac OS"
#elif TARGET_SQL
		"QDOS for Sinclair QL"
#elif TARGET_CPM8K
		"CP/M-8000"
#elif TARGET_RISCOS
		"RISC OS"
#elif TARGET_DXDOS
		"DX-DOS"
#else
#warning Unknown operating system
		"an unknown operating system"
#endif
	"!"); LibPutNewLine();
	LibPutHex(0x12AB); LibPutNewLine();
	LibPutHex(v1); LibPutNewLine();
	LibPutHex(v2); LibPutNewLine();
	LibPutHex(v3); LibPutNewLine();
//#if !TARGET_MACOS
	v4 = 0x4567;
	LibPutHex(v4); LibPutNewLine();
	v5 = 0x5678;
	LibPutHex(v5); LibPutNewLine();
//#endif
//	LibPutString("???");
	LibWaitForKey();
	return 0;
}

