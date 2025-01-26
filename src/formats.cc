
#include "formats.h"
#include "format/8bitexe.h" /* 8-bit binary formats */ /* TODO: not yet finished or tested */
#include "format/aif.h" /* AIF format */ /* TODO: not implemented */
#include "format/aout.h" /* a.out format */
#include "format/arch.h" /* UNIX archive format */
#include "format/as86obj.h" /* TODO: not implemented */
#include "format/binary.h" /* .com, .r (Human68k) */
#include "format/bwexp.h" /* .exp (DOS/16M) */
#include "format/coff.h" /* COFF format (DJGPP, Concurrend DOS 68K) */
#include "format/cpm68k.h" /* .68k (CP/M-68K), .prg (GEMDOS), .z (Human68k) */
#include "format/cpm86.h" /* .cmd (CP/M-86) */
#include "format/cpm8k.h" /* .z8k (CP/M-8000) */
#include "format/dosexe.h" /* TODO: not yet finished or tested */
#include "format/elf.h"
#include "format/geos.h" /* TODO: not implemented */
#include "format/gsos.h" /* TODO: not yet finished or tested */
#include "format/huexe.h" /* .x (Human68k "HU") */
#include "format/hunk.h" /* AmigaOS */
#include "format/leexe.h" /* .exe (OS/2 "LE" and "LX") */
#include "format/macho.h" /* TODO: not implemented */
#include "format/macos.h" /* Mac OS Classic for 68000 */
#include "format/minix.h"
#include "format/mzexe.h" /* .exe (MS-DOS "MZ") */
#include "format/neexe.h" /* .exe (Win16 "NE") */
#include "format/o65.h" /* TODO: not implemented */
#include "format/omf.h" /* TODO: not implemented */
#include "format/pcos.h" /* M20 PCOS files */ /* TODO: not yet finished or tested */
#include "format/peexe.h" /* TODO: not implemented */
#include "format/pefexe.h" /* TODO: not implemented */
#include "format/pharlap.h" /* .exp, .rex (Phar Lap) */
#include "format/pmode.h" /* TODO: not implemented */
#include "format/w3exe.h" /* TODO: not implemented */
#include "format/xenix.h" /* TODO: not implemented */
#include "format/xpexp.h" /* TODO: not implemented */

using namespace Linker;

using namespace Amiga;
using namespace AOut;
using namespace Archive;
using namespace AS86Obj;
using namespace Binary;
using namespace COFF;
using namespace DOS16M;
using namespace DigitalResearch;
using namespace ELF;
using namespace MachO;
using namespace Microsoft;
using namespace MINIX;
using namespace PharLap;
using namespace X68000;

typedef Apple::OMFFormat GSOS_OMFFormat;
typedef OMF::OMFFormat Intel_OMFFormat;

output_format_type formats[] =
{
	/* binary */
	{ "bin",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BinaryFormat>(0, ""); },
		"Flat binary (no system)" },
	{ "com",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BinaryFormat>(0x0100, ".com"); },
		"Flat binary file (.com) for CP/M-80, MS-DOS, OUP/M or DOS/65 (6502) [not supported], DX-DOS (Elektronika BK)" },
	{ "rfile",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BinaryFormat>(/*position independent,*/ ".r"); },
		"Flat relocatable binary file (.r) for Human68k (X68000)" },
	{ "ff8",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BinaryFormat>(0x00008000, ",ff8"); },
		"Flat binary file for RISC OS" },
	{ "riscos" },
	{ "acorn" },
	{ "sql",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BinaryFormat>(0x00038000); },
		"Flat binary file for the Sinclair QL" },
	{ "ql" },
	/* PRL */
	{ "prl",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<PRLFormat>(0x0100, ".prl"); },
		"Page relocatable (.prl) for MP/M-80 and CP/M-80 Plus (Version 3)" },
	/* CP/M 3 */
	{ "cpm3",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM3Format>(); },
		"CP/M-80 Plus format (Version 3)" },
	/* MZ */
	{ "exe",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<MZFormat>(); },
		"MS-DOS \"MZ\" executable (.exe)" },
	{ "mz" },
	{ "dos" },
	{ "msdos" },
	{ "pc" },
	{ "ibmpc" },
	/* CMD */
	{ "cmd_tiny",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM86Format>(CPM86Format::FORMAT_8080); },
		"CP/M-86 8080 model executable (.cmd), single segment" },
	{ "cpm86_tiny" },
	{ "cpm_tiny" },
	{ "cmd_8080" },
	{ "cpm86_8080" },
	{ "cmd_small",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM86Format>(CPM86Format::FORMAT_SMALL); },
		"CP/M-86 small model executable (.cmd), separate code/data segments" },
	{ "cmd" },
	{ "cpm86_small" },
	{ "cpm86" },
	{ "cpm_small" },
	{ "cmd_compact",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM86Format>(CPM86Format::FORMAT_COMPACT); },
		"CP/M-86 compact model executable (.cmd), multiple segments" },
	{ "cpm86_compact" },
	{ "cpm_compact" },
	{ "rsx",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM86Format>(CPM86Format::FORMAT_SMALL, CPM86Format::APPL_RSX); }, /* TODO: other models? */
		"CP/M-86 small model resident system extension (.rsx), separate code/data segments" },
	{ "flexos",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM86Format>(CPM86Format::FORMAT_FLEXOS, CPM86Format::APPL_286); } /* TODO: multiple models */,
		"FlexOS executable (.286), same format as CP/M-86 (.cmd) [untested]" },
	/* MINIX */
	{ "minix",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<MINIXFormat>(MINIXFormat::FormatSeparate); },
		"MINIX/ELKS executable, separate code/data" },
	{ "minix_sep" },
	{ "minix_separate" },
	{ "minix_split" },
	{ "elks" },
	{ "elks_sep" },
	{ "elks_separate" },
	{ "elks_split" },
	{ "minix_comb",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<MINIXFormat>(MINIXFormat::FormatCombined); },
		"MINIX/ELKS executable, combined code/data" },
	{ "minix_combined" },
	{ "elks_comb" },
	{ "elks_combined" },
	/* 68K */
	{ "tos",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_GEMDOS); },
		"GEMDOS/Atari TOS contiguous executable (.prg/.tos)" },
	{ "gemdos" },
	{ "prg" },
	{ "atari" },
	{ "st" },
	{ "cpm68k",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_CPM68K, CPM68KFormat::MAGIC_CONTIGUOUS); },
		"CP/M-68K contiguous executable (.68k)" },
	{ "cpm68k_cont" },
	{ "68k" },
	{ "68k_cont" },
	{ "601a" },
	{ "cpm68k_noncont",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_CPM68K, CPM68KFormat::MAGIC_NONCONTIGUOUS); },
		"CP/M-68K non-contiguous executable (.68k)" },
	{ "68k_noncont" },
	{ "601b" },
	{ "cdos68k",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_HUMAN68K); },
		"Concurrent DOS 68K contiguous executable (.68k) [untested]" },
	{ "cdos68k_c",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_CDOS68K, CPM68KFormat::MAGIC_CONTIGUOUS); },
		"Concurrent DOS 68K contiguous executable with crunched relocations (.68k) [untested]" },
	{ "cdos68k_crunched" },
	{ "cdos68kc" },
	{ "601c" },
	{ "zfile",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM68KFormat>(CPM68KFormat::SYSTEM_HUMAN68K); },
		"Human68k unrelocatable contiguous executable (.z)" },
	/* COFF */
	{ "cdos68k_coff",
		[]() -> std::shared_ptr<OutputFormat> { return COFFFormat::CreateWriter(COFFFormat::CDOS68K); }, /* TODO: also 386 COFF writer */
		"COFF (68000) executable for Concurrent DOS 68K (.68k) [untested]" },
	{ "djgpp",
		[]() -> std::shared_ptr<OutputFormat> { return COFFFormat::CreateWriter(COFFFormat::DJGPP); },
		"COFF (80386) executable for DJGPP 1.11 and later (.exe)" },
	{ "djgpp_coff" },
	{ "djgppv2" }, /* technically, 1.11 */
	{ "coff" },
	/* a.out */
	{ "aout",
		[]() -> std::shared_ptr<OutputFormat> { return AOutFormat::CreateWriter(AOutFormat::DJGPP1); },
		"32-bit a.out executable (80386) with ZMAGIC for GO32/DJGPP version 1 (.exe)" },
	{ "a.out" },
	{ "djgppv1" },
	{ "zmagic" },
	{ "pdos32",
		[]() -> std::shared_ptr<OutputFormat> { return AOutFormat::CreateWriter(AOutFormat::PDOS386); },
		"32-bit a.out executable (80386) with OMAGIC for PDOS/386 (.exe) (obsolete)" },
	{ "pdos386" },
	{ "omagic" },
	/* HU */
	{ "xfile",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<HUFormat>(); },
		"Human68k \"HU\" executable (.x) on the X68000" },
	{ "hu" },
	{ "human68k" },
	{ "h68k" },
	{ "x68000" },
	{ "x68k" },
	/* Classic Macintosh */
	{ "code",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<Apple::MacDriver>(Apple::MacDriver::TARGET_RESOURCE_FORK); },
		"Classic Macintosh 'CODE' resource executable, resource fork only" },
	{ "resource" },
	{ "rsrc" },
	{ "basilisk",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<Apple::MacDriver>(Apple::MacDriver::TARGET_DATA_FORK, Apple::MacDriver::PRODUCE_RESOURCE_FORK | Apple::MacDriver::PRODUCE_FINDER_INFO); },
		"Classic Macintosh 'CODE' resource executable with resource fork and Finder Info in separate .rsrc, .finf folders" },
	{ "macos",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<Apple::MacDriver>(Apple::MacDriver::TARGET_DATA_FORK, Apple::MacDriver::PRODUCE_RESOURCE_FORK | Apple::MacDriver::PRODUCE_FINDER_INFO | Apple::MacDriver::PRODUCE_APPLE_DOUBLE | Apple::MacDriver::PRODUCE_MAC_BINARY); },
		"Classic Macintosh 'CODE' resource executable, stored as AppleDouble format and the resource fork and Finder Info in separate .rsrc, .finf folders" },
	{ "apple" },
	{ "appledouble",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<Apple::MacDriver>(Apple::MacDriver::TARGET_DATA_FORK, Apple::MacDriver::PRODUCE_APPLE_DOUBLE); },
		"Classic Macintosh 'CODE' resource executable, stored as AppleDouble format" },
	{ "applesingle",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<Apple::MacDriver>(Apple::MacDriver::TARGET_APPLE_SINGLE); },
		"Classic Macintosh 'CODE' resource executable, stored as AppleSingle format" },
	/* Hunk */
	{ "amiga",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<HunkFormat>(); },
		"Amiga Hunk executable" },
	{ "hunk" },
	/* NE */
	{ "dos4",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateConsoleApplication(NEFormat::MSDOS4); },
		"Multitasking \"European\" MS-DOS 4.0 \"NE\" executable (.exe)" },
	{ "msdos4" },
	{ "dos_ne" },
	{ "win",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateConsoleApplication(NEFormat::Windows)->SimulateLinker(NEFormat::CompatibleWatcom); },
		"16-bit Windows \"NE\" executable (.exe), Watcom compatible" },
	{ "win16" },
	{ "windows" },
	{ "windows16" },
	{ "ne" },
	{ "win_dll",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateLibraryModule(NEFormat::Windows)->SimulateLinker(NEFormat::CompatibleWatcom); },
		"16-bit Windows \"NE\" dynamic library (.dll), Watcom compatible" },
	{ "win16_dll" },
	{ "windows_dll" },
	{ "windows16_dll" },
	{ "os2",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateConsoleApplication(NEFormat::OS2)->SimulateLinker(NEFormat::CompatibleWatcom); },
		"16-bit OS/2 \"NE\" console executable (.exe), Watcom compatible" },
	{ "os2v1" },
	{ "os2_16" },
	{ "os2_pm",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateGUIApplication(NEFormat::OS2)->SimulateLinker(NEFormat::CompatibleWatcom); },
		"16-bit OS/2 \"NE\" GUI (Presentation Manager) executable (.exe), Watcom compatible" },
	{ "os2v1_pm" },
	{ "os2_16_pm" },
	{ "os2_dll",
		[]() -> std::shared_ptr<OutputFormat> { return NEFormat::CreateLibraryModule(NEFormat::OS2)->SimulateLinker(NEFormat::CompatibleWatcom); },
		"16-bit OS/2 \"NE\" dynamic library (.dll), Watcom compatible" },
	{ "os2v1_dll" },
	{ "os2_16_dll" },
	/* LE/LX */
	{ "dos4g",
		[]() -> std::shared_ptr<OutputFormat> { return LEFormat::CreateConsoleApplication(LEFormat::DOS4G)->SimulateLinker(LEFormat::CompatibleWatcom); },
		"DOS/4G \"LE\" executable (.exe), Watcom compatible" },
	{ "dos_le" },
	{ "win386",
		[]() -> std::shared_ptr<OutputFormat> { return LEFormat::CreateConsoleApplication(LEFormat::Windows386)->SimulateLinker(LEFormat::CompatibleWatcom); },
		"Windows 386 \"LE\" virtual device driver (.386 or .vxd), Watcom compatible" },
	{ "win_vxd" },
	{ "vxd" },
	{ "le" },
	{ "os2v2",
		[]() -> std::shared_ptr<OutputFormat> { return LEFormat::CreateConsoleApplication(LEFormat::OS2)->SimulateLinker(LEFormat::CompatibleWatcom); },
		"32-bit OS/2 \"LX\" console executable (.exe), Watcom compatible" },
	{ "os2_32" },
	{ "lx" },
	{ "os2v2_pm",
		[]() -> std::shared_ptr<OutputFormat> { return LEFormat::CreateGUIApplication(LEFormat::OS2)->SimulateLinker(LEFormat::CompatibleWatcom); },
		"32-bit OS/2 \"LX\" GUI (Presentation Manager) executable (.exe), Watcom compatible" },
	{ "os2_32_pm" },
	{ "lx_pm" },
	{ "os2v2_dll",
		[]() -> std::shared_ptr<OutputFormat> { return LEFormat::CreateLibraryModule(LEFormat::OS2)->SimulateLinker(LEFormat::CompatibleWatcom); },
		"32-bit OS/2 \"LX\" dynamic library (.dll), Watcom compatible" },
	{ "os2_32_dll" },
	{ "lx_dll" },
	/* BW */
	{ "dos16m",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<BWFormat>(); },
		"DOS/16M \"BW\" executable (.exp)" },
	{ "bw" },
	/* MP/MQ */
	{ "pharlap",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<MPFormat>(false); },
		"Phar Lap 386|DOS-Extender \"MP\" executable (.exp)" },
	{ "mp" },
	{ "exp" },
	{ "pharlap_rex",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<MPFormat>(true); },
		"Phar Lap 386|DOS-Extender \"MQ\" relocatable executable (.rex)" },
	{ "mq" },
	{ "rex" },
	/* P2/P3 */
	{ "pharlap_extended",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<P3Format::Flat>(true); },
		"Phar Lap 386|DOS-Extender flat \"P3\" executable (.exp)" },
	{ "pharlap_ext" },
	{ "p3" },
	{ "pharlap_segmented",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<P3Format::MultiSegmented>(true); },
		"Phar Lap 386|DOS-Extender segmented \"P3\" executable (.exp) [untested]" },
	{ "pharlap_seg" },
	{ "p3_seg" },
	{ "pharlap16_extended",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<P3Format::Flat>(false); },
		"Phar Lap 386|DOS-Extender flat \"P2\" executable (.exp) [untested]" },
	{ "pharlap16_ext" },
	{ "p2" },
	{ "pharlap16_segmented",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<P3Format::MultiSegmented>(false); },
		"Phar Lap 386|DOS-Extender segmented \"P2\" executable (.exp) [untested]" },
	{ "pharlap16_seg" },
	{ "p2_seg" },
	/* Z8K */
	{ "cpm8k",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM8KFormat>(CPM8KFormat::MAGIC_NONSHARED); },
		"CP/M-8000 nonshared executable (.z8k)" },
	{ "cpm8k_nonshared" },
	{ "cpm8k_ns" },
	{ "z8k" },
	{ "z8k_nonshared" },
	{ "z8k_ns" },
	{ "ee03" },
	{ "cpm8k_split",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM8KFormat>(CPM8KFormat::MAGIC_SPLIT); },
		"CP/M-8000 split executable (.z8k)" },
	{ "cpm8k_split" },
	{ "cpm8k_sp" },
	{ "z8k" },
	{ "z8k_split" },
	{ "z8k_sp" },
	{ "ee0b" },
	{ "cpm8k_segmented",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CPM8KFormat>(CPM8KFormat::MAGIC_SEGMENTED); },
		"CP/M-8000 segmented executable (.z8k)" },
	{ "cpm8k_segmented" },
	{ "cpm8k_sg" },
	{ "z8k" },
	{ "z8k_segmented" },
	{ "z8k_sg" },
	{ "ee01" },
	/* PCOS */
	{ "pcos",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<PCOS::CMDFormat>(); },
		"Olivetti M20 PCOS cmd format (version 2)" }, // note: "version" here simply refers to the first byte in the header
	{ "pcos_cmd" },
	{ "pcos_sav",
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<PCOS::CMDFormat>(PCOS::CMDFormat::TYPE_SAV); },
		"Olivetti M20 PCOS cmd format (version 2)" }, // note: "version" here simply refers to the first byte in the header
	/* 6502 */
	{ "atari-com", // TODO: testing
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<AtariFormat>(); },
		"Atari 8-bit file format" },
	{ "cbm-prg", // TODO: testing
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<CommodoreFormat>(); },
		"Commodore 8-bit file format" },
	{ "apple-bin", // TODO: testing
		[]() -> std::shared_ptr<OutputFormat> { return std::make_shared<AppleFormat>(); },
		"Apple 8-bit file format" },
};

const size_t formats_size = sizeof(formats) / sizeof(formats[0]);

/**
 * @brief Searches for the associated file format
 */
std::shared_ptr<OutputFormat> FetchMainFormat(std::string text)
{
	output_format_type * last_format;
	for(size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
	{
		if(formats[i].produce)
		{
			/* the producer function is only defined for the first entry */
			last_format = &formats[i];
		}
		if(text == formats[i].format)
		{
			assert(last_format != nullptr);
			return last_format->produce();
		}
	}
	std::ostringstream message;
	message << "Fatal error: Unknown output format `" << text << "'";
	Linker::FatalError(message.str());
}

std::shared_ptr<OutputFormat> FetchFormat(std::string text)
{
	size_t pos = text.find("+");
	if(pos == std::string::npos)
		return FetchMainFormat(text);
	std::string format_name = text.substr(0, pos);
	std::shared_ptr<OutputFormat> format = FetchMainFormat(format_name);
	for(;;)
	{
		pos += 1;
		size_t next_pos = text.find("+", pos);
		std::string subformat = next_pos == std::string::npos ? text.substr(pos) : text.substr(pos, next_pos);
		if(!format->AddSupplementaryOutputFormat(subformat))
		{
			Linker::Error << "Error: Unknown output subformat `" << text << "', ignoring" << std::endl;
		}
		if(next_pos == std::string::npos)
			break;
		pos = next_pos;
	}
	return format;
}

static bool VerifyMacintoshResource(Reader& in, format_description& description)
{
	/* TODO */
	return true;
}

static bool VerifyDRPageRelocatable(Reader& in, format_description& description)
{
	/* TODO */
	return true;
}

static bool VerifyHPSystemManager(Reader& in, format_description& description)
{
	/* conflicts with Adam DOS32 dynamic library */
	in.Seek(description.offset + 2);
	char rest[2];
	in.ReadData(sizeof(rest), rest);
	return std::string(rest) != "L "; /* Adam dynamic library */
}

static bool VerifyMachOOrJava(Reader& in, format_description& description)
{
	/* Apple Universal Binary or Java class file */
	in.Seek(description.offset + 4);
	uint32_t value = in.ReadUnsigned(4, BigEndian);
	/* according to magic: */
	/* for Java class files, this is the version (minor.major), which is at least 0x002E */
	/* for big endian Mach-O, this is the number of architectures, which is currently at most 18 */
	if(value >= 43)
	{
		description.magic.type = FORMAT_JAVA;
		description.magic.description = "Java class file";
	}
	else
	{
		description.magic.type = FORMAT_MACHO;
		description.magic.description = "Mach-O universal binary";
	}
	return true;
}

static bool VerifyCPM3(Reader& in, format_description& description)
{
	/* TODO */
	return true;
}

static bool VerifyCPM86(Reader& in, format_description& description)
{
	int groups = 0;
	in.Seek(description.offset);
	for(int i = 0; i < 8; i++)
	{
		int type = in.ReadUnsigned(1, EndianType(0));
		if(type == 0)
			break;
		if(type > 9)
			return false;
		if(type == 9)
			type = 1;
		type = 1 << (type - 1);
		if((groups & type))
			return false;
		groups |= type;
		in.Skip(8);
	}
	/* must have code (type 1) or pure code (type 9) present (stored as bit 0) */
	return groups & 1;
}

static bool VerifyGSOS(Reader& in, format_description& description)
{
	in.Seek(description.offset + 0x0E);
	if(in.ReadUnsigned(1) != 4)
		return false; /* NUMLEN must be 4 for 32-bit values */

	uint64_t version = in.ReadUnsigned(1);
	if(version != 1 && version != 2)
		return false; /* unexpected VERSION value */

	in.Seek(description.offset + 0x20);
	uint64_t endian = in.ReadUnsigned(1);
	if(endian != 0 && endian != 1)
		return false; /* invalid NUMSEX */

	EndianType endian_type = endian == 1 ? BigEndian : LittleEndian;
	in.Seek(description.offset + 0x22);
	if(in.ReadUnsigned(1, endian_type) != 1)
		return false; /* invalid SEGNUM, must start with 1 */

	/* if these are all satisfied, there is not much else we can verify */
	return true;
}

static bool VerifyFLEX(Reader& in, format_description& description)
{
	/* TODO */
	return false;
}

static bool VerifyAIF(Reader& in, format_description& description)
{
	/* The ARM/RISC OS binary format has a special SWI instruction at offset 0x10.
	 * Other entries are possible as well, but we will only look for this. */
	char buffer[4];
	memset(buffer, 0, sizeof(buffer));
	in.Seek(description.offset + 0x10);
	in.ReadData(sizeof(buffer), buffer);
	return std::string(buffer) == "\x11\x00\x00\xEF";
}

// TODO: fix conflicts
static const struct format_magic format_magics[] =
{
	{ std::string("\x00\x00\x03\xF3", 4), 0, FORMAT_HUNK,    "Amiga Hunk executable" },
	{ std::string("\x00\x00\x03\xE7", 4), 0, FORMAT_HUNK,    "Amiga Hunk unit (object or library)" },
	{ std::string("\x00\x00\x01\x00", 4), 0, FORMAT_RSRC,    "Macintosh resource fork", VerifyMacintoshResource },
	{ std::string("\x00\x05\x16\x00", 4), 0, FORMAT_APPLE,   "Macintosh AppleSingle" },
	{ std::string("\x00\x05\x16\x07", 4), 0, FORMAT_APPLE,   "Macintosh AppleDouble" },
	{ std::string("\x00\x65", 2),         0, FORMAT_COFF,    "WDC65 COFF object file" },
	{ std::string("\x00", 1),             0, FORMAT_PRL,     "MP/M-80 page relocatable executable (.prl)", VerifyDRPageRelocatable },
	{ std::string("\x01\x00o65", 5),      0, FORMAT_O65,     "6502 binary relocation format (André Fachat, used by xa)" },
//	{ std::string("\x01\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX/RT lpd" }, // conflicts with CMD format
	{ std::string("\x01\x03"),            0, FORMAT_MINIX,   "MINIX/ELKS a.out executable" },
	{ std::string("\x01P"),               0, FORMAT_COFF,    "Motorola 68000 COFF executable (Concurrent DOS 68K)" },
	{ std::string("\x02\x06"),            0, FORMAT_XENIX,   "Big endian x.out (Xenix)" },
	{ std::string("\x02"),                0, FORMAT_FLEX,    "FLEX command file", VerifyFLEX },
	{ std::string("\x05\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX version 1 or overlay" },
	{ std::string("\x06\x02"),            0, FORMAT_XENIX,   "Little endian x.out (Xenix)" },
	{ std::string("\x07\x01"),            0, FORMAT_AOUT,    "Little endian OMAGIC a.out, combined code/data (old PDOS/386)" },
	{ std::string("\x08\x01"),            0, FORMAT_AOUT,    "Little endian NMAGIC a.out, separate code/data" },
	{ std::string("\x09\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX separate code/data" },
	{ std::string("\x0B\x01"),            0, FORMAT_AOUT,    "Little endian ZMAGIC a.out, on-demand paged (old DJGPP)" },
	{ std::string("\x0C\x01"),            0, FORMAT_AOUT,    "Little endian a.out, on-demand paged pure" },
	{ std::string("\x0E\x01"),            0, FORMAT_AOUT,    "Little endian a.out, readable on-demand paged pure" },
	{ std::string("\x11\x01"),            0, FORMAT_AOUT,    "Little endian CMAGIC a.out, core dump" },
	{ std::string("\x18\x01"),            0, FORMAT_AOUT,    "Little endian a.out, 2.11BSD overlay, combined code/data" },
	{ std::string("\x19\x01"),            0, FORMAT_AOUT,    "Little endian a.out, 2.11BSD overlay, separate code/data" },
	{ std::string("\x1F\x01"),            0, FORMAT_AOUT,    "Little endian a.out, System V overlay, separate code/data" },
	{ std::string("!<arch>\n"),           0, FORMAT_AR,      "UNIX archive" },
	{ std::string("Adam"),                0, FORMAT_ADAM,    "Adam Seychell's DOS32 DOS Extender format \"Adam\" executable" },
	{ std::string("BW"),                  0, FORMAT_BW,      "Rational Systems/Tenberry Software DOS/16M 16-bit protected mode \"BW\" executable (.exp)" },
	{ std::string("D3X1"),                0, FORMAT_D3X,     "Daniel Broca's D3X DOS Extender format \"D3X1\" executable" },
	{ std::string("DLL "),                0, FORMAT_ADAM,    "Adam Seychell's DOS32 DOS Extender format \"DLL \" dynamic library" },
	{ std::string("DL"),                  0, FORMAT_MZ,      "HP 100LX/200LX System Manager compliant executable (\"DL\") format (.exm), MZ variant", VerifyHPSystemManager },
	{ std::string("DX"),                  0, FORMAT_NE,      "Microsoft 16-bit new executable (\"NE\") for Windows, OS/2 (Rational Systems modification)" },
	{ std::string("Flat"),                0, FORMAT_LV,      "CandyMan's DX64 DOS Extender \"Flat\" executable" },
	{ std::string("HU"),                  0, FORMAT_HU,      "Human68k \"HU\" executable (.x)" },
	{ std::string("Joy!"),                0, FORMAT_PEF,     "Apple Preferred Executable Format (\"Joy!\") executable" },
	{ std::string("L\x01"),               0, FORMAT_COFF,    "Intel 80386 COFF executable (DJGPP)" },
	{ std::string("LE"),                  0, FORMAT_LE,      "Microsoft/IBM 32-bit linear executable (\"LE\") for Windows386 virtual drivers and DOS/4G" },
	{ std::string("LX"),                  0, FORMAT_LE,      "IBM 32-bit linear executable (\"LX\") for OS/2" },
	{ std::string("LC"),                  0, FORMAT_LE,      "DOS/32A compressed linear executable" }, /* ??? */
	{ std::string("LV\x00\x00", 4),       0, FORMAT_LV,      "CandyMan's DX64 DOS Extender \"LV\" executable" },
	{ std::string("MP"),                  0, FORMAT_MP,      "Phar Lap 386|DOS-Extender \"MP\" executable (.exp)" },
	{ std::string("MQ"),                  0, FORMAT_MP,      "Phar Lap 386|DOS-Extender \"MQ\" relocatable executable (.rex)" },
	{ std::string("MZ"),                  0, FORMAT_MZ,      "MS-DOS \"MZ\" executable (.exe)" },
	{ std::string("NE"),                  0, FORMAT_NE,      "Microsoft 16-bit new executable (\"NE\") for Windows, OS/2 and Multitasking (\"European\") MS-DOS 4" },
	{ std::string("P2"),                  0, FORMAT_P3,      "Phar Lap 386|DOS-Extender \"P2\" executable (.exp)" },
	{ std::string("P3"),                  0, FORMAT_P3,      "Phar Lap 386|DOS-Extender \"P3\" executable (.exp)" },
	{ std::string("PE\x00\x00", 4),       0, FORMAT_PE,      "Microsoft 32/64-bit portable executable (\"PE\") for Windows" },
	{ std::string("PL\x00\x00", 4),       0, FORMAT_PE,      "Microsoft 32/64-bit portable executable (\"PL\") for Windows (Phar Lap modification)" },
	{ std::string("PMW1"),                0, FORMAT_PMODEW,  "PMODE/W executable" },
	{ std::string("W3"),                  0, FORMAT_W3,      "Windows WIN386.EXE file" },
//	{ std::string("W4"),                  0, FORMAT_W4,      "Windows 95 WIN32.VXD file" }, /* TODO: unverified */
	{ std::string("XP\x01\x00", 4),       0, FORMAT_XP,      "Ergo OS/286 and OS/386 executable (.exp)" },
	{ std::string("ZM"),                  0, FORMAT_MZ,      "MS-DOS executable (.exe), old-style \"ZM\" variant" },
	{ std::string("Z\x80"),               0, FORMAT_COFF,    "Zilog Z80 COFF object file" },
	{ std::string("`\x1A"),               0, FORMAT_68K,     "CP/M-68K/Concurrent DOS 68K/GEMDOS/Atari TOS/Human68k contiguous executable (.68k/.prg/.tos/.z)" },
	{ std::string("`\x1B"),               0, FORMAT_68K,     "CP/M-68K non-contiguous executable (.68k)" },
	{ std::string("`\x1C"),               0, FORMAT_68K,     "Concurrent DOS 68K contiguous executable with crunched relocations (.68k)" },
	{ std::string("\x7F" "ELF"),          0, FORMAT_ELF,     "UNIX/Linux ELF file format" },
	{ std::string("\x80\x00", 2),         0, FORMAT_COFF,    "Zilog Z8000 COFF object file (GNU binutils)" },
	{ std::string("\x80"),                0, FORMAT_OMF,     "Intel Object Module Format object file" },
	{ std::string("\xA3\x86"),            0, FORMAT_AS86,    "as86 object format" },
	{ std::string("\xC7\x45\xC1\x53"),    0, FORMAT_GEOS,    "PC/GEOS Geode format (version 2)" },
	{ std::string("\xC7\x45\xCF\x53"),    0, FORMAT_GEOS,    "PC/GEOS Geode format (version 1)" },
	{ std::string("\xC9"),                0, FORMAT_CPM3,    "CP/M Plus for Intel 8080", VerifyCPM3 },
	{ std::string("\xCA\xFE\xBA\xBE"),    0, format_type(0), "", VerifyMachOOrJava }, /* Mach-O or Java */
	{ std::string("\xCC\x00", 2),         0, FORMAT_AOUT,    "Little endian QMAGIC a.out, header part of code segment" },
	{ std::string("\xCE\xFA\xED\xFE"),    0, FORMAT_MACHO,   "32-bit little endian Mach-O format" },
	{ std::string("\xCF\xFA\xED\xFE"),    0, FORMAT_MACHO,   "64-bit little endian Mach-O format" },
	{ std::string("\xEE\x00", 2),         0, FORMAT_Z8K,     "CP/M-8000 object file, segmented" },
	{ std::string("\xEE\x01"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, segmented" },
	{ std::string("\xEE\x02"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented" },
	{ std::string("\xEE\x03"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, non-shared code/data" },
	{ std::string("\xEE\x06"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented, shared code/data" }, /* undocumented */
	{ std::string("\xEE\x07"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, shared code/data" }, /* CP/M-8000 cannot execute it */
	{ std::string("\xEE\x0A"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented, split code/data" }, /* undocumented */
	{ std::string("\xEE\x0B"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, split code/data" },
	{ std::string("\xF0"),                0, FORMAT_OMF,     "Intel Object Module Format, library" },
	{ std::string("\xFE\xED\xFA\xCE"),    0, FORMAT_MACHO,   "32-bit big endian Mach-O format" },
	{ std::string("\xFE\xED\xFA\xCF"),    0, FORMAT_MACHO,   "64-bit big endian Mach-O format" },
	{ std::string("\xFF\x00", 2),         0, FORMAT_UZI280,  "UZI-280 executable" },
	{ std::string("\x00\xCC", 2),         2, FORMAT_AOUT,    "Big endian QMAGIC a.out, header part of code segment" },
//	{ std::string("\x01\x01"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX/RT lpd" }, // removed to keep symmetry with little endian form
	{ std::string("\x01\x05"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX version 1 or overlay" },
	{ std::string("\x01\x07"),            2, FORMAT_AOUT,    "Big endian OMAGIC a.out, combined code/data (old PDOS/386)" },
	{ std::string("\x01\x08"),            2, FORMAT_AOUT,    "Big endian NMAGIC a.out, separate code/data" },
	{ std::string("\x01\x09"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX separate code/data" },
	{ std::string("\x01\x0B"),            2, FORMAT_AOUT,    "Big endian ZMAGIC a.out, on-demand paged (old DJGPP)" },
	{ std::string("\x01\x0C"),            2, FORMAT_AOUT,    "Big endian a.out, on-demand paged pure" },
	{ std::string("\x01\x0E"),            2, FORMAT_AOUT,    "Big endian a.out, readable on-demand paged pure" },
	{ std::string("\x01\x11"),            2, FORMAT_AOUT,    "Big endian CMAGIC a.out, core dump" },
	{ std::string("\x01\x18"),            2, FORMAT_AOUT,    "Big endian a.out, 2.11BSD overlay, combined code/data" },
	{ std::string("\x01\x19"),            2, FORMAT_AOUT,    "Big endian a.out, 2.11BSD overlay, separate code/data" },
	{ std::string("\x01\x1F"),            2, FORMAT_AOUT,    "Big endian a.out, System V overlay, separate code/data" },
	{ std::string("TLOC"),                3, FORMAT_PCOS,    "Olivetti M20 PCOS file format" },
	{ std::string(""),                    0, FORMAT_CMD,     "CP/M-86 executable format (.cmd)", VerifyCPM86 },
	{ std::string(""),                    0, FORMAT_GSOS,    "Apple GS/OS object format", VerifyGSOS },
	{ std::string(""),                    0, FORMAT_AIF,     "ARM AIF format", VerifyAIF },
};

// only files that may appear in a static library // TODO
static const struct format_magic library_format_magics[] =
{
	{ std::string("\x00\x65", 2),         0, FORMAT_COFF,    "WDC65 COFF object file" },
//	{ std::string("\x01\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX/RT lpd" }, // conflicts with CMD format
	{ std::string("\x01P"),               0, FORMAT_COFF,    "Motorola 68000 COFF executable (Concurrent DOS 68K)" },
	{ std::string("\x05\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX version 1 or overlay" },
	{ std::string("\x07\x01"),            0, FORMAT_AOUT,    "Little endian OMAGIC a.out, combined code/data (old PDOS/386)" },
	{ std::string("\x08\x01"),            0, FORMAT_AOUT,    "Little endian NMAGIC a.out, separate code/data" },
	{ std::string("\x09\x01"),            0, FORMAT_AOUT,    "Little endian a.out, UNIX separate code/data" },
	{ std::string("\x0B\x01"),            0, FORMAT_AOUT,    "Little endian ZMAGIC a.out, on-demand paged (old DJGPP)" },
	{ std::string("\x0C\x01"),            0, FORMAT_AOUT,    "Little endian a.out, on-demand paged pure" },
	{ std::string("\x0E\x01"),            0, FORMAT_AOUT,    "Little endian a.out, readable on-demand paged pure" },
	{ std::string("\x11\x01"),            0, FORMAT_AOUT,    "Little endian CMAGIC a.out, core dump" },
	{ std::string("\x18\x01"),            0, FORMAT_AOUT,    "Little endian a.out, 2.11BSD overlay, combined code/data" },
	{ std::string("\x19\x01"),            0, FORMAT_AOUT,    "Little endian a.out, 2.11BSD overlay, separate code/data" },
	{ std::string("\x1F\x01"),            0, FORMAT_AOUT,    "Little endian a.out, System V overlay, separate code/data" },
	{ std::string("L\x01"),               0, FORMAT_COFF,    "Intel 80386 COFF executable (DJGPP)" },
	{ std::string("Z\x80"),               0, FORMAT_COFF,    "Zilog Z80 COFF object file" },
	{ std::string("`\x1A"),               0, FORMAT_68K,     "CP/M-68K/Concurrent DOS 68K/GEMDOS/Atari TOS/Human68k contiguous executable (.68k/.prg/.tos/.z)" },
	{ std::string("`\x1B"),               0, FORMAT_68K,     "CP/M-68K non-contiguous executable (.68k)" },
	{ std::string("`\x1C"),               0, FORMAT_68K,     "Concurrent DOS 68K contiguous executable with crunched relocations (.68k)" },
	{ std::string("\x7F" "ELF"),          0, FORMAT_ELF,     "UNIX/Linux ELF file format" },
	{ std::string("\x80\x00", 2),         0, FORMAT_COFF,    "Zilog Z8000 COFF object file (GNU binutils)" },
	{ std::string("\xA3\x86"),            0, FORMAT_AS86,    "as86 object format" },
	{ std::string("\xCC\x00", 2),         0, FORMAT_AOUT,    "Little endian QMAGIC a.out, header part of code segment" },
	{ std::string("\xCE\xFA\xED\xFE"),    0, FORMAT_MACHO,   "32-bit little endian Mach-O format" },
	{ std::string("\xCF\xFA\xED\xFE"),    0, FORMAT_MACHO,   "64-bit little endian Mach-O format" },
	{ std::string("\xEE\x00", 2),         0, FORMAT_Z8K,     "CP/M-8000 object file, segmented" },
	{ std::string("\xEE\x01"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, segmented" },
	{ std::string("\xEE\x02"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented" },
	{ std::string("\xEE\x03"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, non-shared code/data" },
	{ std::string("\xEE\x06"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented, shared code/data" }, /* undocumented */
	{ std::string("\xEE\x07"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, shared code/data" }, /* CP/M-8000 cannot execute it */
	{ std::string("\xEE\x0A"),            0, FORMAT_Z8K,     "CP/M-8000 object file, non-segmented, split code/data" }, /* undocumented */
	{ std::string("\xEE\x0B"),            0, FORMAT_Z8K,     "CP/M-8000 executable file, non-segmented, split code/data" },
	{ std::string("\xFE\xED\xFA\xCE"),    0, FORMAT_MACHO,   "32-bit big endian Mach-O format" },
	{ std::string("\xFE\xED\xFA\xCF"),    0, FORMAT_MACHO,   "64-bit big endian Mach-O format" },
	{ std::string("\xFF\x00", 2),         0, FORMAT_UZI280,  "UZI-280 executable" },
	{ std::string("\x00\xCC", 2),         2, FORMAT_AOUT,    "Big endian QMAGIC a.out, header part of code segment" },
//	{ std::string("\x01\x01"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX/RT lpd" }, // removed to keep symmetry with little endian form
	{ std::string("\x01\x05"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX version 1 or overlay" },
	{ std::string("\x01\x07"),            2, FORMAT_AOUT,    "Big endian OMAGIC a.out, combined code/data (old PDOS/386)" },
	{ std::string("\x01\x08"),            2, FORMAT_AOUT,    "Big endian NMAGIC a.out, separate code/data" },
	{ std::string("\x01\x09"),            2, FORMAT_AOUT,    "Big endian a.out, UNIX separate code/data" },
	{ std::string("\x01\x0B"),            2, FORMAT_AOUT,    "Big endian ZMAGIC a.out, on-demand paged (old DJGPP)" },
	{ std::string("\x01\x0C"),            2, FORMAT_AOUT,    "Big endian a.out, on-demand paged pure" },
	{ std::string("\x01\x0E"),            2, FORMAT_AOUT,    "Big endian a.out, readable on-demand paged pure" },
	{ std::string("\x01\x11"),            2, FORMAT_AOUT,    "Big endian CMAGIC a.out, core dump" },
	{ std::string("\x01\x18"),            2, FORMAT_AOUT,    "Big endian a.out, 2.11BSD overlay, combined code/data" },
	{ std::string("\x01\x19"),            2, FORMAT_AOUT,    "Big endian a.out, 2.11BSD overlay, separate code/data" },
	{ std::string("\x01\x1F"),            2, FORMAT_AOUT,    "Big endian a.out, System V overlay, separate code/data" },
};

void DetermineFormatFor(const format_magic * format_magics, size_t format_magics_count, std::vector<format_description>& descriptions, Reader& rd, uint32_t offset)
{
	rd.Seek(offset);
	char magic[8];
	rd.ReadData(sizeof magic, magic);
	uint32_t position = rd.Tell();
	if(position == uint32_t(-1))
		return;
	uint32_t bytes_read = position > offset ? position - offset : 0;
	if(bytes_read == 0)
		return;
	for(size_t i = 0; i < format_magics_count; i++)
	{
		if(bytes_read < format_magics[i].offset + format_magics[i].magic.size())
			continue;
//Linker::Debug << "\"" << std::string(magic + format_magics[i].offset, format_magics[i].magic.size()) << "\"" << std::endl;
		if(std::string(magic + format_magics[i].offset, format_magics[i].magic.size()) == format_magics[i].magic)
		{
//Linker::Debug << format_magics[i].description << std::endl;
			format_description description;
			description.magic = format_magics[i];
			description.offset = offset;
			if(format_magics[i].special_parse == nullptr || format_magics[i].special_parse(rd, description))
			{
				descriptions.push_back(description);
			}
		}
	}
	if(offset == 0)
	{
		std::vector<format_description> descriptions2 = descriptions;
		for(auto& description : descriptions2)
		{
			switch(description.magic.type)
			{
			case FORMAT_MZ:
				{
					rd.SeekEnd();
					uint32_t file_size = rd.Tell();
					rd.Seek(2);
					uint32_t image_size = rd.ReadUnsigned(2, LittleEndian);
					image_size = (rd.ReadUnsigned(2, LittleEndian) << 9) - (-image_size & 0x1FF);
					rd.Seek(0x3C);
					uint32_t new_header = rd.ReadUnsigned(4, LittleEndian);
					if(0 < new_header && new_header < file_size)
						DetermineFormat(descriptions, rd, new_header);
					if(0 < image_size && image_size < file_size && image_size != new_header)
						DetermineFormat(descriptions, rd, image_size);
					/* Watcom Win386 extender stores an MP executable here */
					rd.Seek(0x38);
					new_header = rd.ReadUnsigned(4, LittleEndian);
					if(0 < new_header && new_header < file_size)
						DetermineFormat(descriptions, rd, new_header);
				}
				break;
			case FORMAT_APPLE:
				/* store resource fork */
				break;
			//case FORMAT_CMD:
			//	/* check RSX records */
			//	break;
			//case FORMAT_CPM3:
			//	/* check RSX records */
			//	break;
			default:
				break;
			}
		}
	}
}

void DetermineFormat(std::vector<format_description>& descriptions, Reader& rd, uint32_t offset)
{
	DetermineFormatFor(format_magics, sizeof(format_magics) / sizeof(format_magics[0]), descriptions, rd, offset);
}

std::shared_ptr<Format> CreateFormat(Reader& rd, format_description& file_format, Archive::ArchiveFormat::file_reader_type * file_reader)
{
	switch(file_format.magic.type)
	{
	case FORMAT_68K:
		return std::make_shared<CPM68KFormat>();
	case FORMAT_ADAM:
		return std::make_shared<SeychellDOS32::AdamFormat>(); // TODO: test
	case FORMAT_AIF:
		return std::make_shared<ARM::AIFFormat>(); // TODO
	case FORMAT_AOUT:
		return std::make_shared<AOutFormat>(); // TODO: test dumper
	case FORMAT_APPLE:
		return std::make_shared<Apple::AppleSingleDouble>(); // TODO: test
	case FORMAT_APPLEII:
		return std::make_shared<AppleFormat>(); // TODO: test
	case FORMAT_AR:
		return std::make_shared<ArchiveFormat>(file_reader); // TODO: test
	case FORMAT_AS86:
		return std::make_shared<AS86ObjFormat>(); // TODO: test
	case FORMAT_ATARI:
		return std::make_shared<AtariFormat>(); // TODO: test
	case FORMAT_BW:
		return std::make_shared<BWFormat>(); // TODO: test dumper
	case FORMAT_COFF:
		return std::make_shared<COFFFormat>(); // TODO: test dumper
	case FORMAT_CMD:
		return std::make_shared<CPM86Format>();
	case FORMAT_CPM3:
		return std::make_shared<CPM3Format>(); // TODO: test
	case FORMAT_D3X:
		return std::make_shared<BrocaD3X::D3X1Format>(); // TODO: test
	case FORMAT_ELF:
		return std::make_shared<ELFFormat>(); // TODO: test dumper
	case FORMAT_FLAT:
		return std::make_shared<BinaryFormat>(); // TODO: test
	case FORMAT_FLEX:
		return std::make_shared<FLEXFormat>(); // TODO
	case FORMAT_GEOS:
		return std::make_shared<GEOS::GeodeFormat>(); // TODO
	case FORMAT_GSOS:
		return std::make_shared<GSOS_OMFFormat>(); // TODO
	case FORMAT_HU:
		return std::make_shared<HUFormat>(); // TODO
	case FORMAT_HUNK:
		return std::make_shared<HunkFormat>(); // TODO
	case FORMAT_JAVA:
		/* TODO */
		return nullptr;
	case FORMAT_LE:
		return std::make_shared<LEFormat>(); // TODO
	case FORMAT_LV:
		return std::make_shared<DX64::LVFormat>(); // TODO
	case FORMAT_MACHO:
		return std::make_shared<MachOFormat>(); // TODO
	case FORMAT_MINIX:
		return std::make_shared<MINIXFormat>(); // TODO
	case FORMAT_MP:
		return std::make_shared<MPFormat>(); // TODO
	case FORMAT_MZ:
		return std::make_shared<MZFormat>();
	case FORMAT_NE:
		return std::make_shared<NEFormat>(); // TODO
	case FORMAT_O65:
		return std::make_shared<O65::O65Format>(); // TODO
	case FORMAT_OMF:
		return std::make_shared<OMF::OMFFormat>(); // TODO
	case FORMAT_P3:
		return std::make_shared<P3FormatContainer>(); // TODO
	case FORMAT_PCOS:
		return std::make_shared<PCOS::CMDFormat>(); // TODO
	case FORMAT_PE:
		return std::make_shared<PEFormat>(); // TODO
	case FORMAT_PEF:
		return std::make_shared<Apple::PEFFormat>(); // TODO
	case FORMAT_PMODEW:
		return std::make_shared<PMODE::PMW1Format>(); // TODO
	case FORMAT_PRL:
		return std::make_shared<PRLFormat>(); // TODO
	case FORMAT_RSRC:
		return std::make_shared<Apple::ResourceFork>(); // TODO: test
	case FORMAT_UZI280:
		return std::make_shared<UZI280Format>(); // TODO
	case FORMAT_W3:
		/* TODO */
		return std::make_shared<W3Format>(); // TODO
	case FORMAT_W4:
		/* TODO */
		return nullptr;
	case FORMAT_XENIX:
		/* TODO */
		return std::make_shared<Xenix::XOutFormat>(); // TODO
	case FORMAT_XENIX_BOUT:
		return std::make_shared<Xenix::BOutFormat>(); // TODO
	case FORMAT_XP:
		/* TODO */
		return std::make_shared<Ergo::XPFormat>(); // TODO
	case FORMAT_Z8K:
		return std::make_shared<CPM8KFormat>(); // TODO: test
#if 0
	default:
		/* TODO */
		return nullptr;
#endif
	}
	Linker::FatalError("Internal error: invalid output format");
}

std::shared_ptr<Linker::Image> ReadArchiveFile(Linker::Reader& rd, offset_t size)
{
	std::vector<format_description> descriptions;
	offset_t offset = rd.Tell();
	DetermineFormat(descriptions, rd, offset);
	rd.Seek(offset);
	if(descriptions.size() == 0)
	{
		return Linker::Buffer::ReadFromFile(rd, size);
	}
	else
	{
		Linker::Reader wrd = rd.CreateWindow(offset, size);
		std::shared_ptr<Format> format = CreateFormat(wrd, descriptions[0]);
		format->ReadFile(wrd);
		return format;
	}
}

std::shared_ptr<Linker::Image> ReadLibraryFile(Linker::Reader& rd, offset_t size)
{
	std::vector<format_description> descriptions;
	offset_t offset = rd.Tell();
	DetermineFormatFor(library_format_magics, sizeof library_format_magics / sizeof library_format_magics[0], descriptions, rd, offset);
	rd.Seek(offset);
	if(descriptions.size() == 0)
	{
		return Linker::Buffer::ReadFromFile(rd, size);
	}
	else
	{
		Linker::Reader wrd = rd.CreateWindow(offset, size);
		std::shared_ptr<Format> format = CreateFormat(wrd, descriptions[0]);
		format->ReadFile(wrd);
		return format;
	}
}

