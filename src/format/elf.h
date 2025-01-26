#ifndef ELF_H
#define ELF_H

#include <sstream>
#include <vector>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/format.h"
#include "../linker/linker_manager.h"
#include "../linker/module.h"
#include "../linker/reader.h"

namespace ELF
{
	/**
	 * @brief ELF object and executable format
	 *
	 * The latest and most widespread file format, developed for the UNIX operating system.
	 */
	class ELFFormat : public virtual Linker::InputFormat, public virtual Linker::LinkerManager
	{
	public:
		/* * * General members * * */

		static constexpr uint8_t ELFCLASSNONE = 0;
		static constexpr uint8_t ELFCLASS32 = 1;
		static constexpr uint8_t ELFCLASS64 = 2;

		static constexpr uint8_t ELFDATANONE = 0;
		static constexpr uint8_t ELFDATA2LSB = 1;
		static constexpr uint8_t ELFDATA2MSB = 2;

		static constexpr uint8_t EV_NONE = 0;
		static constexpr uint8_t EV_CURRENT = 1;

		static constexpr uint8_t ELFOSABI_NONE = 0;

		static constexpr offset_t SHF_WRITE = 0x0001;
		static constexpr offset_t SHF_ALLOC = 0x0002;
		static constexpr offset_t SHF_EXECINSTR = 0x0004;
		static constexpr offset_t SHF_MERGE = 0x0010;
		static constexpr offset_t SHF_STRINGS = 0x0020;
		static constexpr offset_t SHF_INFO_LINK = 0x0040;
		static constexpr offset_t SHF_LINK_ORDER = 0x0080;
		static constexpr offset_t SHF_OS_NONCONFORMING = 0x0100;
		static constexpr offset_t SHF_GROUP = 0x0200;
		static constexpr offset_t SHF_TLS = 0x0400;
		static constexpr offset_t SHF_COMPRESSED = 0x0800;
		/* GNU specific */
		static constexpr offset_t SHF_GNU_RETAIN = 0x00200000;
		static constexpr offset_t SHF_GNU_MBIND = 0x01000000;
		static constexpr offset_t SHF_EXCLUDE = 0x80000000;
		/* OS/2 specific */
		static constexpr offset_t SHF_BEGIN = 0x01000000;
		static constexpr offset_t SHF_END = 0x02000000;

		static constexpr uint16_t SHN_UNDEF = 0;
		static constexpr uint16_t SHN_LORESERVE = 0xFF00;
		static constexpr uint16_t SHN_ABS = 0xFFF1;
		static constexpr uint16_t SHN_COMMON = 0xFFF2;
		static constexpr uint16_t SHN_XINDEX = 0xFFFF;

		static constexpr uint8_t STB_LOCAL = 0;
		static constexpr uint8_t STB_GLOBAL = 1;
		static constexpr uint8_t STB_WEAK = 2;
		static constexpr uint8_t STB_ENTRY = 12; /* IBM OS/2 */

		static constexpr uint8_t STT_NOTYPE = 0;
		static constexpr uint8_t STT_OBJECT = 1;
		static constexpr uint8_t STT_FUNC = 2;
		static constexpr uint8_t STT_SECTION = 3;
		static constexpr uint8_t STT_FILE = 4;
		static constexpr uint8_t STT_COMMON = 5;
		static constexpr uint8_t STT_TLS = 6;
		static constexpr uint8_t STT_IMPORT = 11; /* IBM OS/2 */

		static constexpr uint8_t STV_DEFAULT = 0;
		static constexpr uint8_t STV_INTERNAL = 1;
		static constexpr uint8_t STV_HIDDEN = 2;
		static constexpr uint8_t STV_PROTECTED = 3;

		static constexpr offset_t R_386_8 = 22;
		static constexpr offset_t R_386_PC8 = 23;
		static constexpr offset_t R_386_16 = 20;
		static constexpr offset_t R_386_PC16 = 21;
		static constexpr offset_t R_386_32 = 1;
		static constexpr offset_t R_386_PC32 = 2;
		/* extensions, see https://github.com/tkchia/build-ia16/blob/master/elf16-writeup.md (TODO: unsupported for now) */
		static constexpr offset_t R_386_SEG16 = 45;
		static constexpr offset_t R_386_SUB16 = 46;
		static constexpr offset_t R_386_SUB32 = 47;
		static constexpr offset_t R_386_SEGRELATIVE = 48;
		static constexpr offset_t R_386_OZSEG16 = 80;
		static constexpr offset_t R_386_OZRELSEG16 = 81;

		static constexpr offset_t R_68K_8 = 3;
		static constexpr offset_t R_68K_PC8 = 6;
		static constexpr offset_t R_68K_16 = 2;
		static constexpr offset_t R_68K_PC16 = 5;
		static constexpr offset_t R_68K_32 = 1;
		static constexpr offset_t R_68K_PC32 = 4;

		static constexpr offset_t R_ARM_ABS8 = 8;
		static constexpr offset_t R_ARM_ABS16 = 16;
		static constexpr offset_t R_ARM_ABS32 = 2;
		static constexpr offset_t R_ARM_REL32 = 3;
		static constexpr offset_t R_ARM_CALL = 28;
		static constexpr offset_t R_ARM_JUMP24 = 29;
		static constexpr offset_t R_ARM_PC24 = 1;
		static constexpr offset_t R_ARM_V4BX = 40;

		static constexpr offset_t DT_NULL = 0;
		static constexpr offset_t DT_NEEDED = 1;
		static constexpr offset_t DT_PLTRELSZ = 2;
		static constexpr offset_t DT_PLTGOT = 3;
		static constexpr offset_t DT_HASH = 4;
		static constexpr offset_t DT_STRTAB = 5;
		static constexpr offset_t DT_SYMTAB = 6;
		static constexpr offset_t DT_RELA = 7;
		static constexpr offset_t DT_RELASZ = 8;
		static constexpr offset_t DT_RELAENT = 9;
		static constexpr offset_t DT_STRSZ = 10;
		static constexpr offset_t DT_SYMENT = 11;
		static constexpr offset_t DT_INIT = 12;
		static constexpr offset_t DT_FINI = 13;
		static constexpr offset_t DT_SONAME = 14;
		static constexpr offset_t DT_RPATH = 15;
		static constexpr offset_t DT_SYMBOLIC = 16;
		static constexpr offset_t DT_REL = 17;
		static constexpr offset_t DT_RELSZ = 18;
		static constexpr offset_t DT_RELENT = 19;
		static constexpr offset_t DT_PLTREL = 20;
		static constexpr offset_t DT_DEBUG = 21;
		static constexpr offset_t DT_TEXTREL = 22;
		static constexpr offset_t DT_JMPREL = 23;
		static constexpr offset_t DT_BIND_NOW = 24;
		static constexpr offset_t DT_INIT_ARRAY = 25;
		static constexpr offset_t DT_FINI_ARRAY = 26;
		static constexpr offset_t DT_INIT_ARRAYSZ = 27;
		static constexpr offset_t DT_FINI_ARRAYSZ = 28;
		static constexpr offset_t DT_RUNPATH = 29;
		static constexpr offset_t DT_FLAGS = 30;
		static constexpr offset_t DT_ENCODING = 31;
		static constexpr offset_t DT_PREINIT_ARRAY = 32;
		static constexpr offset_t DT_PREINIT_ARRAYSZ = 33;
		static constexpr offset_t DT_SYMTAB_SHNDX = 34;
		static constexpr offset_t DT_RELRSZ = 35;
		static constexpr offset_t DT_RELR = 36;
		static constexpr offset_t DT_RELRENT = 37;
		/* IBM OS/2 specific */
		static constexpr offset_t DT_EXPORT = 0x60000001;
		static constexpr offset_t DT_EXPORTSZ = 0x60000002;
		static constexpr offset_t DT_EXPENT = 0x60000003;
		static constexpr offset_t DT_IMPORT = 0x60000004;
		static constexpr offset_t DT_IMPORTSZ = 0x60000005;
		static constexpr offset_t DT_IMPENT = 0x60000006;
		static constexpr offset_t DT_IT = 0x60000007;
		static constexpr offset_t DT_ITPRTY = 0x60000008;
		static constexpr offset_t DT_INITTERM = 0x60000009;
		static constexpr offset_t DT_STACKSZ = 0x6000000A;
		/* GNU binutils */
		static constexpr offset_t DT_GNU_FLAGS1 = 0x6FFFFDF4;
		static constexpr offset_t DT_GNU_PRELINKED = 0x6FFFFDF5;
		static constexpr offset_t DT_GNU_CONFLICTSZ = 0x6FFFFDF6;
		static constexpr offset_t DT_GNU_LIBLISTSZ = 0x6FFFFDF7;
		static constexpr offset_t DT_CHECKSUM = 0x6FFFFDF8;
		static constexpr offset_t DT_PLTPADSZ = 0x6FFFFDF9;
		static constexpr offset_t DT_MOVEENT = 0x6FFFFDFA;
		static constexpr offset_t DT_MOVESZ = 0x6FFFFDFB;
		static constexpr offset_t DT_FEATURE = 0x6FFFFDFC;
		static constexpr offset_t DT_POSTFLAG_1 = 0x6FFFFDFD;
		static constexpr offset_t DT_SYMINSZ = 0x6FFFFDFE;
		static constexpr offset_t DT_SYMINENT = 0x6FFFFDFF;
		static constexpr offset_t DT_GNU_HASH = 0x6FFFFEF5;
		static constexpr offset_t DT_TLSDESC_PLT = 0x6FFFFEF6;
		static constexpr offset_t DT_TLSDESC_GOT = 0x6FFFFEF7;
		static constexpr offset_t DT_GNU_CONFLICT = 0x6FFFFFEF8;
		static constexpr offset_t DT_GNU_LIBLIST = 0x6FFFFEF9;
		static constexpr offset_t DT_CONFIG = 0x6FFFFEFA;
		static constexpr offset_t DT_DEPAUDIT = 0x6FFFFEFB;
		static constexpr offset_t DT_AUDIT = 0x6FFFFEFC;
		static constexpr offset_t DT_PLTPAD = 0x6FFFFEFD;
		static constexpr offset_t DT_MOVETAB = 0x6FFFFEFE;
		static constexpr offset_t DT_SYMINFO = 0x6FFFFEFF;
		static constexpr offset_t DT_VERSYM = 0x6FFFFFF0;
		static constexpr offset_t DT_RELACOUNT = 0x6FFFFFF9;
		static constexpr offset_t DT_RELCOUNT = 0x6FFFFFFA;
		static constexpr offset_t DT_FLAGS_1 = 0x6FFFFFFB;
		static constexpr offset_t DT_VERDEF = 0x6FFFFFFC;
		static constexpr offset_t DT_VERDEFNUM = 0x6FFFFFFD;
		static constexpr offset_t DT_VERNEED = 0x6FFFFFFE;
		static constexpr offset_t DT_VERNEEDNUM = 0x6FFFFFFF;
		static constexpr offset_t DT_AUXILIARY = 0x7FFFFFFD;
		static constexpr offset_t DT_USED = 0x7FFFFFFE;
		static constexpr offset_t DT_FILTER = 0x7FFFFFFFF;

		uint8_t file_class = 0;
		EndianType endiantype = ::LittleEndian;

		uint8_t data_encoding = 0;
		size_t wordbytes = 4;

		enum cpu_type
		{
			EM_NONE = 0,
			EM_M32 = 1,
			EM_SPARC = 2,
			EM_386 = 3,
			EM_68K = 4,
			EM_88K = 5,
			EM_IAMCU = 6,
			EM_860 = 7,
			EM_MIPS = 8,
			EM_S370 = 9,
			EM_MIPS_RS3_LE = 10,
			EM_OLD_SPARCV9 = 11, // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h

			EM_PARISC = 15,

			EM_VPP500 = 17,
			EM_SPARC32PLUS = 18,
			EM_960 = 19,
			EM_PPC = 20,
			EM_PPC64 = 21,
			EM_S390 = 22,
			EM_SPU = 23,

			EM_V800 = 36,
			EM_FR20 = 37,
			EM_RH32 = 38,
			EM_MCORE = 39,
			EM_RCE = EM_MCORE,
			EM_ARM = 40,
			EM_ALPHA = 41,
			EM_SH = 42,
			EM_SPARCV9 = 43,
			EM_TRICORE = 44,
			EM_ARC = 45,
			EM_H8_300 = 46,
			EM_H8_300H = 47,
			EM_H8S = 48,
			EM_H8_500 = 49,
			EM_IA_64 = 50,
			EM_MIPS_X = 51,
			EM_COLDFIRE = 52,
			EM_68HC12 = 53,
			EM_MMA = 54,
			EM_PCP = 55,
			EM_NCPU = 56,
			EM_NDR1 = 57,
			EM_STARCORE = 58,
			EM_ME16 = 59,
			EM_ST100 = 60,
			EM_TINYJ = 61,
			EM_X86_64 = 62,
			EM_PDSP = 63,
			EM_PDP10 = 64,
			EM_PDP11 = 65,
			EM_FX66 = 66,
			EM_ST9PLUS = 67,
			EM_ST7 = 68,
			EM_68HC16 = 69,
			EM_68HC11 = 70,
			EM_68HC08 = 71,
			EM_68HC05 = 72,
			EM_SVX = 73,
			EM_ST19 = 74,
			EM_VAX = 75,
			EM_CRIS = 76,
			EM_JAVELIN = 77,
			EM_FIREPATH = 78,
			EM_ZSP = 79,
			EM_MMIX = 80,
			EM_HUANY = 81,
			EM_PRISM = 82,
			EM_AVR = 83,
			EM_FR30 = 84,
			EM_D10V = 85,
			EM_D30V = 86,
			EM_V850 = 87,
			EM_M32R = 88,
			EM_MN10300 = 89,
			EM_MN10200 = 90,
			EM_PJ = 91,
			EM_OPENRISC = 92,
			EM_ARC_COMPACT = 93,
			EM_ARC_A5 = EM_ARC_COMPACT,
			EM_XTENSA = 94,
			EM_SCORE_OLD = 95,
			EM_VIDEOCORE = 95,
			EM_TMM_GPP = 96,
			EM_NS32K = 97,
			EM_TPC = 98,
			EM_PJ_OLD = 99,
			EM_SNP1K = 99,
			EM_ST200 = 100,
			EM_IP2K = 101,
			EM_MAX = 102,
			EM_CR = 103,
			EM_F2MC16 = 104,
			EM_MSP430 = 105,
			EM_BLACKFIN = 106,
			EM_SE_C33 = 107,
			EM_SEP = 108,
			EM_ARCA = 109,
			EM_UNICORE = 110,
			EM_EXCESS = 111,
			EM_DXP = 112,
			EM_ALTERA_NIOS32 = 113,
			EM_CRX = 114,
			EM_CR16_OLD = 115,
			EM_XGATE = 115,
			EM_C166 = 116,
			EM_M16C = 117,
			EM_DSPIC30F = 118,
			EM_CE = 119,
			EM_M32C = 120,

			EM_TSK3000 = 131,
			EM_RS08 = 132,
			EM_SHARC = 133,
			EM_ECOG2 = 134,
			EM_SCORE7 = 135,
			EM_DSP24 = 136,
			EM_VIDEOCORE3 = 137,
			EM_LATTICEMICO32 = 138,
			EM_SE_CE17 = 139,
			EM_TI_C6000 = 140,
			EM_TI_C2000 = 141,
			EM_TI_C5500 = 142,
			EM_TI_ARP32 = 143,
			EM_TI_PRU = 144,

			EM_MMDSP_PLUS = 160,
			EM_CYPRESS_M8C = 161,
			EM_R32C = 162,
			EM_TRIMEDIA = 163,
			EM_QDSP6 = 164,
			EM_8051 = 165,
			EM_STXP7X = 166,
			EM_NDS32 = 167,
			EM_ECOG1 = 168,
			EM_ECOG1X = EM_ECOG1,
			EM_MAXQ30 = 169,
			EM_XIMO16 = 170,
			EM_MANIK = 171,
			EM_CRAYNV2 = 172,
			EM_RX = 173,
			EM_METAG = 174,
			EM_MCST_ELBRUS = 175,
			EM_ECOG16 = 176,
			EM_CR16 = 177,
			EM_ETPU = 178,
			EM_SLE9X = 179,
			EM_L10M = 180,
			EM_K10M = 181,
			EM_INTEL182 = 182,
			EM_AARCH64 = 183,
			EM_ARM184 = 184,
			EM_AVR32 = 185,
			EM_STM8 = 186,
			EM_TILE64 = 187,
			EM_TILEPRO = 188,
			EM_MICROBLAZE = 189,
			EM_CUDA = 190,
			EM_TILEGX = 191,
			EM_CLOUDSHIELD = 192,
			EM_COREA_1ST = 193,
			EM_COREA_2ND = 194,
			EM_ARC_COMPACT2 = 195,
			EM_OPEN8 = 196,
			EM_RL78 = 197,
			EM_VIDEOCORE5 = 198,
			EM_78KOR = 199,
			EM_56800EX = 200,
			EM_BA1 = 201,
			EM_BA2 = 202,
			EM_XCORE = 203,
			EM_MCHP_PIC = 204,
			EM_INTELGT = 205, // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
			EM_INTEL206 = 206,
			EM_INTEL207 = 207,
			EM_INTEL208 = 208,
			EM_INTEL209 = 209,
			EM_KM32 = 210,
			EM_KMX32 = 211,
			EM_KMX16 = 212,
			EM_KMX8 = 213,
			EM_KVARC = 214,
			EM_CDP = 215,
			EM_COGE = 216,
			EM_COOL = 217,
			EM_NORC = 218,
			EM_CSR_KALIMBA = 219,
			EM_Z80 = 220,
			EM_VISIUM = 221,
			EM_FT32 = 222,
			EM_MOXIE = 223,
			EM_AMDGPU = 224,

			EM_RISCV = 243,
			// https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
			EM_LANAI = 244,
			EM_CEVA = 245,
			EM_CEVA_X2 = 246,
			EM_BPF = 247,
			EM_GRAPHCORE_IPU = 248,
			EM_IMG1 = 249,
			EM_NFP = 250,
			EM_VE = 251,
			EM_CSKY = 252,
			EM_ARC_COMPACT3_64 = 253,
			EM_MCS6502 = 254,
			EM_ARC_COMPACT3 = 255,
			EM_KVX = 256,
			EM_65816 = 257,
			EM_LOONGARCH = 258,
			EM_KF32 = 259,
			EM_U16_U8CORE = 260,
			EM_TACHYUM = 261,
			EM_56800EF = 262,

			// apparently used by BeOS for the AT&T Hobbit
			EM_HOBBIT = 925,

			// https://llvm-mos.org/wiki/ELF_specification
			EM_MOS = 6502,
		};
		cpu_type cpu = EM_NONE;

		uint8_t header_version = 0;
		uint8_t osabi = 0;
		uint8_t abi_version = 0;

		enum file_type
		{
			ET_NONE = 0,
			ET_REL = 1,
			ET_EXEC = 2,
			ET_DYN = 3,
			ET_CORE = 4,
		};
		file_type object_file_type = ET_NONE;
		uint16_t file_version = 0;

		offset_t entry = 0;
		offset_t program_header_offset = 0;
		offset_t section_header_offset = 0;
		uint32_t flags = 0;
		uint16_t elf_header_size = 0;
		uint16_t program_header_entry_size = 0;
		uint16_t section_header_entry_size = 0;
		uint16_t section_name_string_table = 0;

		class SectionContents : public Linker::Image
		{
		public:
			virtual void AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index);
			virtual void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index);
		};

		class Symbol
		{
		public:
			uint32_t name_offset = 0;
			std::string name;
			offset_t value = 0, size = 0;
			uint8_t bind = 0, type = 0, other = 0;
			uint32_t shndx = 0;
			uint32_t sh_link = 0;
			bool defined = false;
			bool unallocated = false;
			Linker::Location location;
			Linker::CommonSymbol specification;
		};

		class SymbolTable : public SectionContents
		{
		public:
			size_t wordbytes;
			offset_t entsize;
			/* used for SHT_SYMTAB and SHT_DYNSYM */
			std::vector<Symbol> symbols;

			SymbolTable(size_t wordbytes, offset_t entsize)
				: wordbytes(wordbytes), entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class StringTable : public SectionContents
		{
		public:
			offset_t size;
			/* used for SHT_STRTAB */
			std::vector<std::string> strings;

			StringTable(offset_t size)
				: size(size)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class Array : public SectionContents
		{
		public:
			offset_t entsize;
			/* used for SHT_SYMTAB_SHNDX, SHT_INIT_ARRAY/SHT_FINI_ARRAY/SHT_PREINIT_ARRAY, SHT_GROUP */
			std::vector<offset_t> array;

			Array(offset_t entsize)
				: entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class SectionGroup : public Array
		{
		public:
			offset_t flags = 0;

			SectionGroup(offset_t entsize)
				: Array(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class IndexArray : public Array
		{
		public:
			IndexArray(offset_t entsize)
				: Array(entsize)
			{
			}

			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class Relocation
		{
		public:
			offset_t offset = 0;
			uint32_t type = 0;
			uint32_t symbol = 0;
			int64_t addend = 0;
			uint32_t sh_link = 0, sh_info = 0;
			bool addend_from_section_data = false;

			size_t GetSize(cpu_type cpu) const;
			std::string GetName(cpu_type cpu) const;
		};

		class Relocations : public SectionContents
		{
		public:
			size_t wordbytes;
			offset_t entsize;
			/* used for SHT_REL, SHT_RELA */
			std::vector<Relocation> relocations;

			Relocations(size_t wordbytes, offset_t entsize)
				: wordbytes(wordbytes), entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class HashTable : public SectionContents
		{
		public:
			std::vector<uint32_t> buckets;
			std::vector<uint32_t> chains;

			static uint32_t Hash(const std::string& name);

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class DynamicObject
		{
		public:
			offset_t tag;
			offset_t value;
			std::string name;

			DynamicObject()
				: tag(0), value(0)
			{
			}

			DynamicObject(offset_t tag, offset_t value)
				: tag(tag), value(value)
			{
			}
		};

		class DynamicSection : public SectionContents
		{
		public:
			size_t wordbytes;
			offset_t entsize;
			/* used for SHT_DYNAMIC */
			std::vector<DynamicObject> dynamic;

			DynamicSection(size_t wordbytes, offset_t entsize)
				: wordbytes(wordbytes), entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class Note
		{
		public:
			std::string name;
			std::string descriptor;
			offset_t type;
		};

		class NotesSection : public SectionContents
		{
		public:
			offset_t size;

			/* used for SHT_NOTE */
			std::vector<Note> notes;

			NotesSection(offset_t size)
				: size(size)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		class VersionRequirement
		{
		public:
			class Auxiliary
			{
			public:
				uint32_t hash = 0;
				uint16_t flags = 0;
				uint16_t other = 0;
				uint32_t name_offset = 0;
				std::string name;
				uint32_t offset_next_entry = 0;
			};

			uint16_t version = 0;
			uint32_t file_name_offset = 0;
			std::string file_name;
			uint32_t offset_auxiliary_array = 0;
			std::vector<Auxiliary> auxiliary_array;
			uint32_t offset_next_entry = 0;
		};

		class VersionRequirements : public SectionContents
		{
		public:
			std::vector<VersionRequirement> requirements;

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		/* IBM OS/2 extension */
		class IBMSystemInfo : public SectionContents
		{
		public:
			enum system_type : uint32_t
			{
				EOS_NONE = 0,
				EOS_PN = 1,
				EOS_SVR4 = 2,
				EOS_AIX = 3,
				EOS_OS2 = 4,
			};
			system_type os_type = EOS_NONE;
			uint32_t os_size = 0;
			bool IsOS2Specific() const;
			struct os2_specific
			{
				/* OS/2 specific */
				enum os2_session : uint8_t
				{
					OS2_SES_NONE = 0,
					OS2_SES_FS = 1,
					OS2_SES_PM = 2,
					OS2_SES_VIO = 3,
				};
				os2_session sessiontype = OS2_SES_NONE;
				uint8_t sessionflags;
			} os2;
			/* unspecified */
			std::vector<uint8_t> os_specific;

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		/* IBM OS/2 extension */
		class IBMImportEntry
		{
		public:
			uint32_t ordinal = 0, name_offset = 0;
			std::string name;
			enum import_type
			{
				IMP_IGNORED = 0,
				IMP_STR_IDX = 1,
				IMP_DT_IDX = 2,
			};
			import_type type = IMP_IGNORED;
			uint32_t dll = 0;
			std::string dll_name;
		};

		/* IBM OS/2 extension */
		class IBMImportTable : public SectionContents
		{
		public:
			offset_t entsize;
			std::vector<IBMImportEntry> imports;

			IBMImportTable(offset_t entsize)
				: entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		/* IBM OS/2 extension */
		class IBMExportEntry
		{
		public:
			uint32_t ordinal = 0, symbol_index = 0, name_offset = 0;
			std::string name;
			uint32_t sh_link = 0, sh_info = 0;
		};

		/* IBM OS/2 extension */
		class IBMExportTable : public SectionContents
		{
		public:
			offset_t entsize;
			std::vector<IBMExportEntry> exports;

			IBMExportTable(offset_t entsize)
				: entsize(entsize)
			{
			}

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

		/* IBM OS/2 extension */
		class IBMResource
		{
		public:
			uint32_t type = 0;
			uint32_t ordinal = 0;
			uint32_t name_offset = 0;
			std::string name;
			uint32_t data_offset = 0;
			uint32_t data_size = 0;
			std::shared_ptr<Linker::Image> data;
		};

		/* IBM OS/2 extension */
		class IBMResourceCollection : public SectionContents
		{
		public:
#if 0
			uint32_t offset = 0;
			uint32_t size = 0;
#endif

			offset_t offset = 0;

			uint16_t version = 0;
			uint16_t flags = 0;
			uint32_t name_offset = 0;
			std::string name;
			uint32_t item_array_offset = 0;
			uint32_t item_array_entry_size = 0;
			uint32_t header_size = 0;
			uint32_t string_table_offset = 0;
			uint32_t locale_offset = 0;

			char16_t country[2] = { };
			char16_t language[2] = { };

			std::vector<IBMResource> resources;

			offset_t ImageSize() override;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) override;
			void AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index) override;
		};

#if 0
		/* IBM OS/2 extension */
		class IBMResourceFile
		{
		public:
			uint8_t file_class = 0;
			EndianType endiantype = ::LittleEndian;

			uint8_t data_encoding = 0;
			size_t wordbytes = 4;

			uint8_t version = 0;

			uint32_t header_size = 0;
			uint32_t resource_collection_offset = 0;
			std::vector<IBMResourceCollection> resource_collections;
		};
#endif

		class Section
		{
		public:
			uint32_t name_offset = 0;
			std::string name;
			enum section_type
			{
				SHT_NULL = 0,
				SHT_PROGBITS = 1,
				SHT_SYMTAB = 2,
				SHT_STRTAB = 3,
				SHT_RELA = 4,
				SHT_HASH = 5,
				SHT_DYNAMIC = 6,
				SHT_NOTE = 7,
				SHT_NOBITS = 8,
				SHT_REL = 9,
				SHT_SHLIB = 10, /* reserved */
				SHT_DYNSYM = 11,
				SHT_INIT_ARRAY = 14,
				SHT_FINI_ARRAY = 15,
				SHT_PREINIT_ARRAY = 16,
				SHT_GROUP = 17,
				SHT_SYMTAB_SHNDX = 18,
				/* IBM OS/2 specific */
				SHT_OS = 0x60000001,
				SHT_IMPORTS = 0x60000002,
				SHT_EXPORTS = 0x60000003,
				SHT_RES = 0x60000004,

				SHT_OLD_OS = 12,
				SHT_OLD_IMPORTS = 13,
				SHT_OLD_EXPORTS = 14,
				SHT_OLD_RES = 15,

				/* GNU extensions */
				SHT_GNU_INCREMENTAL_INPUTS = 0x6FFF4700,
				SHT_GNU_ATTRIBUTES = 0x6FFFFFF5,
				SHT_GNU_HASH = 0x6FFFFFF6,
				SHT_GNU_LIBLIST = 0x6FFFFFF7,
				SHT_SUNW_verdef = 0x6FFFFFFD,
				SHT_SUNW_verneed = 0x6FFFFFFE,
				SHT_SUNW_versym = 0x6FFFFFFF,
			};
			section_type type = SHT_NULL;
			uint32_t link = 0, info = 0;
			offset_t flags = 0;
			offset_t address = 0, file_offset = 0, size = 0, align = 0, entsize = 0;

			std::shared_ptr<Linker::Image> contents;

			std::shared_ptr<Linker::Section> GetSection();
			const std::shared_ptr<Linker::Section> GetSection() const;

			std::shared_ptr<SymbolTable> GetSymbolTable();
			const std::shared_ptr<SymbolTable> GetSymbolTable() const;

#if 0
			std::shared_ptr<StringTable> GetStringTable();
#endif
			std::shared_ptr<Array> GetArray();

			std::shared_ptr<Relocations> GetRelocations();
			const std::shared_ptr<Relocations> GetRelocations() const;

			std::shared_ptr<DynamicSection> GetDynamicSection();

#if 0
			std::shared_ptr<NotesSection> GetNotesSection();

			std::shared_ptr<IBMSystemInfo> GetIBMSystemInfo();
#endif

			std::shared_ptr<IBMImportTable> GetIBMImportTable();
			std::shared_ptr<IBMExportTable> GetIBMExportTable();

			bool GetFileSize() const;

			void Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index);

			static std::shared_ptr<Linker::Section> ReadProgBits(Linker::Reader& rd, offset_t file_offset, const std::string& name, offset_t size);
			static std::shared_ptr<Linker::Section> ReadNoBits(const std::string& name, offset_t size);
			static std::shared_ptr<SymbolTable> ReadSymbolTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize, uint32_t section_link, size_t wordbytes);
			static std::shared_ptr<Relocations> ReadRelocations(Linker::Reader& rd, Section::section_type type, offset_t file_offset, offset_t section_size, offset_t entsize, uint32_t section_link, uint32_t section_info, size_t wordbytes);
			static std::shared_ptr<StringTable> ReadStringTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size);
			static std::shared_ptr<Array> ReadArray(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize);
			static std::shared_ptr<SectionGroup> ReadSectionGroup(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize);
			static std::shared_ptr<IndexArray> ReadIndexArray(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize);
			static std::shared_ptr<HashTable> ReadHashTable(Linker::Reader& rd, offset_t file_offset);
			static std::shared_ptr<DynamicSection> ReadDynamic(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize, size_t wordbytes);
			static std::shared_ptr<NotesSection> ReadNote(Linker::Reader& rd, offset_t file_offset, offset_t section_size);
			static std::shared_ptr<VersionRequirements> ReadVersionRequirements(Linker::Reader& rd, offset_t file_offset, offset_t section_link, offset_t section_info);
			static std::shared_ptr<IBMSystemInfo> ReadIBMSystemInfo(Linker::Reader& rd, offset_t file_offset);
			static std::shared_ptr<IBMImportTable> ReadIBMImportTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize);
			static std::shared_ptr<IBMExportTable> ReadIBMExportTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize);
			static std::shared_ptr<IBMResourceCollection> ReadIBMResourceCollection(Linker::Reader& rd, offset_t file_offset);
		};
		std::vector<Section> sections;

		class Segment
		{
		public:
			enum segment_type
			{
				PT_NULL = 0,
				PT_LOAD = 1,
				PT_DYNAMIC = 2,
				PT_INTERP = 3,
				PT_NOTE = 4,
				PT_SHLIB = 5,
				PT_PHDR = 6,
				PT_TLS = 7,
				/* IBM OS/2 specific */
				PT_OS = 0x60000001,
				PT_RES = 0x60000002,

				PT_OLD_OS = 7,
				PT_OLD_RES = 9,
				/* SUN, GNU, OpenBSD extensions */
				PT_SUNW_EH_FRAME = 0x6474E550,
				PT_GNU_STACK = 0x6474E551,
				PT_GNU_RELRO = 0x6474E552,
				PT_GNU_PROPERTY = 0x6474E553,
				PT_GNU_SFRAME = 0x6474E554,
				PT_GNU_MBIND_LO = 0x6474E555,
				PT_GNU_MBIND_HI = 0x6474F554,
				PT_OPENBSD_MUTABLE = 0x65A3DBE5,
				PT_OPENBSD_RANDOMIZE = 0x65A3DBE6,
				PT_OPENBSD_WXNEEDED = 0x65A3DBE7,
				PT_OPENBSD_NOBTCFI = 0x65A3DBE8,
				PT_OPENBSD_BOOTDATA = 0x65A41BE6,
			};
			segment_type type;
			uint32_t flags = 0;
			offset_t offset = 0, vaddr = 0, paddr = 0, filesz = 0, memsz = 0, align = 0;

			struct Part
			{
			public:
				enum part_type
				{
					/** @brief A section or part of a section */
					Section = 1,
					/** @brief Some other area inside the binary, including one of the headers */
					Block = 2,
				};
				part_type type;

				uint16_t index;
				offset_t offset, size;
				Part(part_type type, uint16_t index, offset_t offset, offset_t size)
					: type(type), index(index), offset(offset), size(size)
				{
				}

				offset_t GetOffset(ELFFormat& fmt);
				offset_t GetActualSize(ELFFormat& fmt);
			};

			std::vector<Part> parts;
		};
		std::vector<Segment> segments;

		class Block
		{
		public:
			offset_t offset = 0;
			offset_t size = 0;
			std::shared_ptr<Linker::Image> image;
			Block(offset_t offset = 0, offset_t size = 0)
				: offset(offset), size(size)
			{
			}
		};
		/** @brief Collection of blocks of data that do not belong to any section */
		std::vector<Block> blocks;

		/* AT&T Hobbit BeOS specific */

		/** @brief Resources for the prototype BeOS Development Release for the AT&T Hobbit
		 *
		 * The layout is based on Steve White's resource-extractor.
		 */
		struct HobbitBeOSResource
		{
			char type[4];
			offset_t unknown1, offset, size, unknown2;
			std::shared_ptr<Linker::Buffer> image;
		};
		offset_t hobbit_beos_resource_offset = 0;
		std::vector<HobbitBeOSResource> hobbit_beos_resources;

		ELFFormat()
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		offset_t WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		/**
		 * @brief Attempts to heuristically determine if it is in a beta OS/2 format
		 */
		bool IsOldOS2Format() const;

		/* * * Reader members * * */

		bool option_16bit = true;
		bool option_linear = false;

		void SetupOptions(std::shared_ptr<Linker::OutputFormat> format) override;

	private:
		void GenerateModule(Linker::Module& module) const;

	public:
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		void CalculateValues() override;
	};

}

#endif /* ELF_H */
