#ifndef FORMATS_H
#define FORMATS_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "linker/format.h"
#include "linker/reader.h"

struct output_format_type
{
	std::string format;
	std::shared_ptr<Linker::OutputFormat> (* produce)();
	std::string documentation;
};

extern output_format_type formats[];
extern const size_t formats_size;

/**
 * @brief Descriptor to return when checking the signature of a binary file
 */
enum format_type
{
	FORMAT_68K, // .68k used by CP/M-68K, Concurrent DOS 68K, GEMDOS/Atari TOS, Human68k
	FORMAT_ADAM, // Adam Seychell's DOS32 DOS extender
	FORMAT_AIF, // ARM image format /* TODO: not implemented */
	FORMAT_AOUT, // UNIX a.out
	FORMAT_APPLE, // Macintosh AppleSingle/AppleDouble, used to store Macintosh files on other systems
	FORMAT_APPLEII, // Apple ][ binary format
	FORMAT_ATARI, // Atari 8-bit binary format
	FORMAT_BW, // DOS/16M .exp file
	FORMAT_COFF, // UNIX COFF
	FORMAT_CMD, // CP/M-86
	FORMAT_CPM3, // CP/M-80 Plus format
	FORMAT_D3X, // Daniel Broca's D3X DOS extender
	FORMAT_ELF, // UNIX ELF
	FORMAT_FLAT, // flat unstructured file
	FORMAT_FLEX, // FLEX .cmd (6800 or 6809 CPUs)
	FORMAT_GEOS, // PC/GEOS Geode format
	FORMAT_GSOS, // Apple IIgs GS/OS format
	FORMAT_HU, // Human68k .x
	FORMAT_HUNK, // Amiga hunk
	FORMAT_JAVA, // Java class file
	FORMAT_LE, // Linear executable (OS/2, DOS/4G)
	FORMAT_LV, // CandyMan's DX64 DOS extender, LV/Float format
	FORMAT_MACHO, // Mach-O format
	FORMAT_MINIX, // MINIX a.out
	FORMAT_MP, // Phar Lap executable .exp and relocatable executable, .rex
	FORMAT_MZ, // MS-DOS .exe
	FORMAT_NE, // 16-bit Windows .exe
	FORMAT_O65, // 6502 binary relocation format
	FORMAT_OMF, // Intel Object file
	FORMAT_P3, // Phar Lap new executable .exp
	FORMAT_PE, // 32-bit Windows .exe
	FORMAT_PEF, // Classic Macintosh PowerPC executable
	FORMAT_PMODEW, // PMODE/W executable
	FORMAT_PRL, // MP/M-80 relocatable
	FORMAT_RSRC, // Classic Macintosh resource, possibly containing an executable
	FORMAT_UZI280, // UZI-280 executable
	FORMAT_W3, // Windows unique file
	FORMAT_W4, // Windows unique file
	FORMAT_XENIX, // Xenix segmented executable
	FORMAT_XP, // OS/286 or OS/386 executable
	FORMAT_Z8K, // CP/M-8000
};

struct format_description;

struct format_magic
{
	std::string magic;
	unsigned offset;
	format_type type;
	std::string description;
	bool (* special_parse)(Linker::Reader& in, format_description& description);
};

struct format_description
{
	format_magic magic;
	uint32_t offset;
};

/**
 * @brief Converts a string representation into an output format
 *
 * Most formats are identified by an identifier, but some of them (such as the Classic Macintosh) support a <tt>+</tt> separated list of formats, the first of which is the main format, followed by supplementary formats.
 */
std::shared_ptr<Linker::OutputFormat> FetchFormat(std::string text);

/**
 * @brief Collects all the possible file formats, this includes skipping over MZ stubs for protected mode DOS or Windows 3.x executables
 */
void DetermineFormat(std::vector<format_description>& descriptions, Linker::Reader& rd, uint32_t offset = 0);

std::shared_ptr<Linker::Format> CreateFormat(Linker::Reader& rd, format_description& file_format);

#endif /* FORMATS_H */
