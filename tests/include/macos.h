
typedef _Bool bool;
typedef unsigned char uint8_t;

struct Point
{
	short v, h;
};
struct Rect
{
	short top, left, bottom, right;
};
struct Region
{
	short rgnSize;
	struct Rect rgnBBox;
	char rgnData[1];
};
struct BitMap
{
	char * baseAddr;
	short rowBytes;
	struct Rect bounds;
};
struct PixMap
{
	void * baseAddr;
	short rowBytes;
	struct Rect bounds;
	short pmVersion, packType;
	long packSize;
	long hRes, vRes;
	short pixelType, pixelSize, cmpCount, cmpSize;
	long planeBytes;
	struct ColorTable ** pmTable;
	long pmReserved;
} __attribute__((packed));
struct Pattern
{
	unsigned char pat[8];
};
struct GrafPort
{
	short device;
	struct BitMap portBits;
	struct Rect portRect;
	struct Region ** visRgn, ** clipRgn;
	struct Pattern bkPat, fillPat;
	struct Point pnLoc, pnSize;
	short pnMode;
	struct Pattern pnPat;
	short pnVis, txFont;
	char txFace;
	char filler;
	short txMode, txSize;
	long spExtra;
	long fgColor, bgColor;
	short colrBit, patStretch;
	void ** picSave, ** rgnSave, ** polySave;
	void * grafProcs;
} __attribute__((packed));
struct RGBColor
{
	unsigned short red, green, blue;
};
struct ColorSpec
{
	short value;
	struct RGBColor rgb;
};
struct ColorTable
{
	long ctSeed;
	short ctFlags, ctSize;
	struct ColorSpec ctTable[1];
} __attribute__((packed));
struct CGrafPort
{
	short device;
	struct PixMap ** portPixMap;
	short portVersion;
	void ** grafVars;
	short chExtra, pnLocHFrac;
	struct Rect portRect;
	struct Region ** visRgn, ** clipRgn;
	struct PixPattern ** bkPixPat;
	struct RGBColor rgbFgColor, rgbBkColor;
	struct Point pnLoc, pnSize;
	short pnMode;
	struct PixPattern ** pnPixPat, ** fillPixPat;
	short pnVis, txFont;
	char txFace;
	char filler;
	short txMode, txSize;
	long spExtra, fgColor, bgColor;
	short colrBit, patStretch;
	void ** picSave, ** rgnSave, ** polySave;
	struct CQDProcs * grafProcs;
} __attribute__((packed));

struct CWindowRecord
{
	struct CGrafPort port;
	short windowKind;
	bool visible, hilited, goAwayFlag, spareFlag;
	struct Region ** strucRgn, ** contRgn, ** updateRgn;
	void ** windowDefProc, ** dataHandle;
	const char ** titleHandle;
	short titleWidth;
	struct Control ** controlList;
	struct CWindowRecord * nextWindow;
	struct Pic ** windowPic;
	long refCon;
} __attribute__((packed));

struct WindowRecord
{
	struct GrafPort port;
	short windowKind;
	bool visible, hilited, goAwayFlag, spareFlag;
	struct Region ** strucRgn, ** contRgn, ** updateRgn;
	void ** windowDefProc, ** dataHandle;
	const char ** titleHandle;
	short titleWidth;
	struct Control ** controlList;
	struct WindowRecord * nextWindow;
	struct Pic ** windowPic;
	long refCon;
} __attribute__((packed));

/* TODO */
union event
{
	char b[16];
	short w[8];
	long l[4];
};

typedef uint8_t Cursor[68];

static struct a5world
{
	char __other[108]; /* make it have at least 0xF8 bytes */
//	struct WindowRecord * window;
	short current_row;

	long randSeed;
	struct BitMap screenBits;
	Cursor arrow;
	struct Pattern dkGray, ltGray, gray, black, white;
	struct GrafPort * thePort;
} __a5world;

static inline struct a5world * a5(void);

void InitGraf(void * globalPtr);
void InitFonts(void);
void InitWindows(void);
void InitMenus(void);
void ExitToShell(void);
#define srcCopy 0
void TextMode(short mode);
void MoveTo(short x, short y);
#define everyEvent (-1)
short GetNextEvent(short mask, void * event);
#define documentProc 0
struct WindowRecord * NewCWindow(void * wStorage, const struct Rect * boundsRect, const unsigned char * title, bool visible, short theProc, struct WindowRecord * behind, bool goAwayFlag, long refCon);
struct WindowRecord * NewWindow(void * wStorage, const struct Rect * boundsRect, const unsigned char * title, bool visible, short theProc, struct WindowRecord * behind, bool goAwayFlag, long refCon);
void SetPort(struct GrafPort * port);
void TextFont(short font);
void TextFace(short face);
void TextSize(short size);

