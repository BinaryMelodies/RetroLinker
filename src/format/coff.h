#ifndef COFF_H
#define COFF_H

#include <map>
#include "cpm68k.h"
#include "mzexe.h"
#include "../common.h"
#include "../linker/linker_manager.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace COFF
{
	/**
	 * @brief The UNIX COFF file format
	 *
	 * Originally introduced in UNIX System V, it replaced the previous a.out format.
	 * It was later adopted, often with extensions, on many UNIX and non-UNIX operating systems.
	 *
	 * The current implementation supports the following formats:
	 * - DJGPP version 1.11 or later, running on top of MS-DOS
	 * - Digital Research Concurrent DOS 68K
	 */
	class COFFFormat : public virtual Linker::InputFormat, public virtual Linker::LinkerManager, protected Microsoft::MZSimpleStubWriter
	{
		/* * * General members * * */
	public:
		/**
		 * @brief Represents the first 16-bit word of a COFF file
		 */
		enum cpu
		{
			CPU_UNKNOWN = 0,

			//// main supported types

			/** @brief Intel x86-32, introduced with the Intel 80386
			 *
			 * Value as used by UNIX, Microsoft and DJGPP
			 */
			CPU_I386  = 0x014C,

			/** @brief Motorola 68000 and compatibles
			 *
			 * Value as used by UNIX and Digital Research Concurrent DOS 68K
			 */
			CPU_M68K  = 0x0150,

			//// GNU binutils output

			/** @brief WDC 65C816
			 *
			 * Value as used by GNU binutils
			 */
			CPU_W65   = 0x6500,

			/** @brief Zilog Z80
			 *
			 * Value as used by GNU binutils
			 */
			CPU_Z80   = 0x805A,

			/** @brief Zilog Z8000
			 *
			 * Value as used by GNU binutils
			 */
			CPU_Z8K   = 0x8000,

			//// other CPU types, included for completeness sake

			/** @brief Intel x86-16, introduced with the Intel 8086
			 *
			 * Value as used by UNIX
			 */
			CPU_I86   = 0x0148,

			/** @brief National Semiconductor NS32000
			 *
			 * Value as used by UNIX
			 */
			CPU_NS32K = 0x0154,

			/** @brief IBM System/370
			 *
			 * Value as used by UNIX
			 */
			CPU_I370  = 0x0158,

			/** @brief MIPS architecture
			 *
			 * Value as used by UNIX
			 */
			CPU_MIPS  = 0x0160,

			/** @brief Motorola 88000
			 *
			 * Value as used by UNIX and also defined in ECOFF
			 */
			CPU_M88K  = 0x016D,

			/** @brief AT&T Bellmac 32, Western Electric 32000 and family
			 *
			 * Value as used by UNIX
			 */
			CPU_WE32K = 0x0170,

			/** @brief DEC VAX
			 *
			 * Value as used by UNIX
			 */
			CPU_VAX   = 0x0178,

			/** @brief AMD 29000
			 *
			 * Value as used by UNIX
			 */
			CPU_AM29K = 0x017A,

			/** @brief DEC Alpha
			 *
			 * Value as used by UNIX
			 */
			CPU_ALPHA = 0x0183,

			/** @brief IBM PowerPC, 32-bit
			 *
			 * Value as defined by XCOFF
			 */
			CPU_PPC   = 0x01DF,

			/** @brief IBM PowerPC, 64-bit
			 *
			 * Value as defined by XCOFF
			 */
			CPU_PPC64 = 0x01F7,

			/** @brief SHARC from Analog Devices */
			CPU_SHARC = 0x521C,

			//// Microsoft values

			/** @brief Intel i860
			 *
			 * Value as shown in early Microsoft documentation
			 */
			CPU_I860  = 0x014D,

			/** @brief Hitachi SuperH family
			 *
			 * Value as defined by Microsoft
			 */
			CPU_SH    = 0x01A2,

			/** @brief ARM, also known as ARM32 or AArch32; also represents Thumb
			 *
			 * Value as defined by Microsoft
			 */
			CPU_ARM   = 0x01C0,

			/** @brief Matsushita AM33
			 *
			 * Value as defined by Microsoft
			 */
			CPU_AM33  = 0x01D3,

			/** @brief Intel Itanium architecture, also known as IA-64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_IA64  = 0x0200,

			/** @brief Hewlett-Packard PA-RISC
			 *
			 * Value as defined by Microsoft
			 */
			CPU_HPPA  = 0x0290,

			/** @brief EFI bytecode
			 *
			 * Value as defined by Microsoft
			 */
			CPU_EFI   = 0x0EBC,

			/** @brief RISC-V 32
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV32 = 0x5032,

			/** @brief RISC-V 64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV64 = 0x5064,

			/** @brief RISC-V 128
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV128 = 0x5128,

			/** @brief x86-64, introduced by AMD
			 *
			 * Value as defined by Microsoft
			 */
			CPU_AMD64 = 0x8664,

			/** @brief Mitsubishi M32R
			 *
			 * Value as defined by Microsoft
			 */
			CPU_M32R  = 0x9041,

			/** @brief ARM64, also known as AArch64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_ARM64 = 0xAA64,

			//// overloaded values (greater or equal to 0x10000)
			// for now, there are no overloaded values
		};

		enum COFFVariantType
		{
			COFF = 1,
			ECOFF = 2,
			XCOFF32 = 3,
			XCOFF64 = 4,
			PECOFF = 5,
		};

		struct MachineType
		{
			cpu actual_cpu;
			::EndianType endian;
		};

		static const std::map<uint32_t, MachineType> MACHINE_TYPES;

		/** @brief The actual value of the magic number (COFF name: f_magic) */
		char signature[2] = { };

		/**
		 * @brief Retrieves the natural byte order for the architecture
		 */
		::EndianType GetEndianType() const;

		/**
		 * @brief A generic COFF relocation
		 */
		class Relocation
		{
		public:
			virtual ~Relocation();
			virtual offset_t GetAddress() const = 0;
			virtual size_t GetSize() const = 0;
			virtual size_t GetEntrySize() const = 0;
			virtual void WriteFile(Linker::Writer& wr) const = 0;
			virtual void FillEntry(Dumper::Entry& entry) const = 0;
		};

		/**
		 * @brief The standard UNIX COFF relocation format
		 */
		class UNIXRelocation : public Relocation
		{
		public:
			/** @brief No relocation */
			static constexpr uint16_t R_ABS = 0;
			/** @brief 16-bit virtual address of symbol */
			static constexpr uint16_t R_DIR16 = 1;
			/** @brief 16-bit relative address of symbol */
			static constexpr uint16_t R_REL16 = 2;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t R_DIR32 = 6;
			/** @brief (Microsoft COFF) 32-bit relative virtual address of symbol */
			static constexpr uint16_t R_DIR32NB = 7;
			/** @brief (Intel x86) 16-bit segment selector of symbol */
			static constexpr uint16_t R_SEG12 = 9;
			/** @brief (Microsoft, debugging) 16-bit section index */
			static constexpr uint16_t R_SECTION = 10;
			/** @brief (Microsoft, debugging) 32-bit offset from section start */
			static constexpr uint16_t R_SECREL = 11;
			/** @brief (Microsoft) CLR token */
			static constexpr uint16_t R_TOKEN = 12;
			/** @brief (Microsoft) 7-bit offset from section base */
			static constexpr uint16_t R_SECREL7 = 13;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t R_REL32 = 20;

			COFFVariantType coff_variant;
			cpu cpu_type;

			offset_t address = 0;
			uint32_t symbol_index = 0;
			uint16_t type = 0;
			uint32_t information = 0;

			UNIXRelocation(COFFVariantType coff_variant, cpu cpu_type)
				: coff_variant(coff_variant), cpu_type(cpu_type)
			{
			}

			void Read(Linker::Reader& rd);

			offset_t GetAddress() const override;

			size_t GetSize() const override;

			size_t GetEntrySize() const override;

			void WriteFile(Linker::Writer& wr) const override;

			void FillEntry(Dumper::Entry& entry) const override;
		};

		/**
		 * @brief A relocation, as stored by the Z80/Z8000 backend
		 */
		class ZilogRelocation : public Relocation
		{
		public:
			// https://github.com/aixoss/binutils/blob/master/include/coff/internal.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z80.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z8k.h

			static constexpr uint16_t R_Z80_IMM8  = 0x22;
			static constexpr uint16_t R_Z80_IMM16 = 0x01;
			static constexpr uint16_t R_Z80_IMM24 = 0x33;
			static constexpr uint16_t R_Z80_IMM32 = 0x17;
			static constexpr uint16_t R_Z80_OFF8  = 0x32;
			static constexpr uint16_t R_Z80_JR    = 0x02;

			static constexpr uint16_t R_Z8K_IMM4L = 0x23;
			static constexpr uint16_t R_Z8K_IMM4H = 0x24;
			static constexpr uint16_t R_Z8K_DISP7 = 0x25; /* djnz */
			static constexpr uint16_t R_Z8K_IMM8  = 0x22;
			static constexpr uint16_t R_Z8K_IMM16 = 0x01;
			static constexpr uint16_t R_Z8K_REL16 = 0x04;
			static constexpr uint16_t R_Z8K_IMM32 = 0x11;
			static constexpr uint16_t R_Z8K_JR    = 0x02; /* jr */
			static constexpr uint16_t R_Z8K_CALLR = 0x05; /* callr */

			static constexpr uint16_t R_W65_ABS8     = 0x01;
			static constexpr uint16_t R_W65_ABS16    = 0x02;
			static constexpr uint16_t R_W65_ABS24    = 0x03;
			static constexpr uint16_t R_W65_ABS8S8   = 0x04;
			static constexpr uint16_t R_W65_ABS8S16  = 0x05;
			static constexpr uint16_t R_W65_ABS16S8  = 0x06;
			static constexpr uint16_t R_W65_ABS16S16 = 0x07;
			static constexpr uint16_t R_W65_PCR8     = 0x08;
			static constexpr uint16_t R_W65_PCR16    = 0x09;
			static constexpr uint16_t R_W65_DP       = 0x0A;

			cpu cpu_type;

			/**
			 * @brief Address of the relocation (COFF name: r_vaddr)
			 */
			uint32_t address;
			/**
			 * @brief Index of symbol in symbol table (COFF name: r_symndx)
			 */
			uint32_t symbol_index;
			/**
			 * @brief (COFF name: r_offset)
			 */
			uint32_t offset;
			/**
			 * @brief Type of relocation (COFF name: r_type)
			 */
			uint16_t type;
			/**
			 * @brief unknown (COFF name: r_stuff)
			 */
			uint16_t data;

			ZilogRelocation(cpu cpu_type)
				: cpu_type(cpu_type)
			{
			}

			void Read(Linker::Reader& rd);

			offset_t GetAddress() const override;

			size_t GetSize() const override;

			size_t GetEntrySize() const override;

			void WriteFile(Linker::Writer& wr) const override;

			void FillEntry(Dumper::Entry& entry) const override;
		};

		/**
		 * @brief A COFF symbol
		 */
		class Symbol
		{
		public:
			/**
			 * @brief Symbol name (COFF name: n_name, if it fits inside field)
			 */
			std::string name;
			/**
			 * @brief The index of the symbol name within the string table, if not stored directly in the entry, 0 otherwise (COFF name: n_name)
			 */
			uint32_t name_index;
			/**
			 * @brief The actual value of the symbol (COFF name: n_value)
			 */
			uint32_t value;
			/**
			 * @brief The number of the section, with special values 0 (N_UNDEF), 1 (N_ABS) and 2 (N_DEBUG) (COFF name: n_scnum)
			 */
			uint16_t section_number;
			/**
			 * @brief The symbol type (COFF name: n_type)
			 */
			uint16_t type;
			/**
			 * @brief COFF name: n_sclass, typical values are 2 (C_EXT), 3 (C_STAT)
			 *
			 * The fields storage_class, section_number and value interact in non-obvious ways
			 */
			uint8_t storage_class;
			/**
			 * @brief Signifies how many extra entries are present, these should be skipped, 0 is a typical value (COFF name: n_numaux)
			 */
			uint8_t auxiliary_count;

			void Read(Linker::Reader& rd);

			bool IsExternal() const;
		};

		/**
		 * @brief A COFF section
		 */
		class Section
		{
		public:
			/**
			 * @brief The name of the section (COFF name: s_name)
			 */
			std::string name;
			/**
			 * @brief The physical address of the section (expected to be identical to the virtual address) (COFF name: s_paddr)
			 *
			 * For PE/COFF, this field is reinterpreted as the size of the section in memory
			 */
			offset_t physical_address = 0;
			/**
			 * @brief The virtual address of the section (COFF name: s_vaddr)
			 */
			offset_t address = 0;
			/**
			 * @brief The size of the section (COFF name: s_size)
			 */
			offset_t size = 0;
			/**
			 * @brief Offset of stored image data from COFF header start (COFF name: s_scnptr)
			 */
			offset_t section_pointer = 0;
			/**
			 * @brief Offset to COFF relocations (COFF name: s_relptr)
			 */
			offset_t relocation_pointer = 0;
			/**
			 * @brief unused (COFF name: s_lnnoptr)
			 */
			offset_t line_number_pointer = 0;
			/**
			 * @brief COFF relocation count (COFF name: s_nreloc)
			 */
			uint32_t relocation_count = 0;
			/**
			 * @brief unused (COFF name: s_nlnno)
			 */
			uint32_t line_number_count = 0;
			/**
			 * @brief COFF section flags, determines the type of the section (text, data, bss, etc.) (COFF name: s_flags)
			 */
			uint32_t flags = 0;

			/**
			 * @brief The stored image data
			 */
			std::shared_ptr<Linker::Image> image;

			/**
			 * @brief Collection of COFF relocations
			 */
			std::vector<std::unique_ptr<Relocation>> relocations;

			/** @brief COFF section flags */
			enum
			{
				/** @brief Section contains executable (COFF name: STYP_TEXT) */
				TEXT = 0x0020,
				/** @brief Section contains initialized data (COFF name: STYP_DATA) */
				DATA = 0x0040,
				/** @brief Section contains uninitialized data (COFF name: STYP_BSS) */
				BSS  = 0x0080,
			};

			void Clear();

			Section(uint32_t flags = 0, std::shared_ptr<Linker::Image> image = nullptr)
				: flags(flags), image(image)
			{
			}

			~Section()
			{
				Clear();
			}

			void ReadSectionHeader(Linker::Reader& rd, COFFVariantType coff_variant);

			void WriteSectionHeader(Linker::Writer& wr, COFFVariantType coff_variant);

			uint32_t ImageSize();
		};

		/**
		 * @brief The list of COFF sections
		 */
		std::vector<std::unique_ptr<Section>> sections;

		/**
		 * @brief Section count (COFF name: f_nscns)
		 */
		uint16_t section_count = 0;
		/**
		 * @brief Time stamp, unused (COFF name: f_timdat)
		 */
		uint32_t timestamp = 0;
		/**
		 * @brief Offset to the first symbol (COFF name: f_symptr)
		 */
		offset_t symbol_table_offset = 0;
		/**
		 * @brief The number of symbols (COFF name: f_nsyms)
		 */
		uint32_t symbol_count = 0;
		/**
		 * @brief The symbols stored inside the COFF file
		 */
		std::vector<std::unique_ptr<Symbol>> symbols;
		/**
		 * @brief The size of the optional header (COFF: f_opthdr)
		 */
		uint32_t optional_header_size = 0;
		/**
		 * @brief COFF flags, such as whether the file is executable (f_flags)
		 */
		uint16_t flags = 0;

		/**
		 * @brief An abstract class to represent the optional header
		 */
		class OptionalHeader
		{
		public:
			virtual ~OptionalHeader();
			/**
			 * @brief Returns size of optional header
			 */
			virtual uint32_t GetSize() = 0;
			virtual void ReadFile(Linker::Reader& rd) = 0;
			virtual void WriteFile(Linker::Writer& wr) = 0;
			/**
			 * @brief Retrieves any additional data from the file corresponding to this type of optional header
			 */
			virtual void PostReadFile(COFFFormat& coff, Linker::Reader& rd);
			/**
			 * @brief Stores any additional data in the file corresponding to this type of optional header
			 */
			virtual void PostWriteFile(COFFFormat& coff, Linker::Writer& wr);

			virtual void Dump(COFFFormat& coff, Dumper::Dumper& dump);
		};

		/**
		 * @brief The optional header instance used for reading/writing the COFF file
		 */
		std::unique_ptr<OptionalHeader> optional_header = nullptr;

		/**
		 * @brief Concurrent DOS 68K requires a special block of data to represent "crunched" relocations (see CPM68KWriter for more details)
		 */
		std::map<uint32_t, size_t> relocations; /* CDOS68K */

		/**
		 * @brief A simplified class to represent an optional header of unknown structure
		 */
		class UnknownOptionalHeader : public OptionalHeader
		{
		public:
			std::unique_ptr<Linker::Buffer> buffer = nullptr;

			UnknownOptionalHeader(offset_t size)
				: buffer(std::make_unique<Linker::Buffer>(size))
			{
			}

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

			void Dump(COFFFormat& coff, Dumper::Dumper& dump) override;
		};

		/**
		 * @brief A standard 28 byte a.out optional header, used by DJGPP
		 */
		class AOutHeader : public OptionalHeader
		{
		public:
			/**
			 * @brief Type of executable, most typically ZMAGIC (COFF name: magic)
			 */
			uint16_t magic = 0;
			/**
			 * @brief unused (COFF name: vstamp)
			 */
			uint16_t version_stamp = 0;

			/**
			 * @brief unused (COFF name: tsize)
			 */
			uint32_t code_size = 0;
			/**
			 * @brief unused (COFF name: dsize)
			 */
			uint32_t data_size = 0;
			/**
			 * @brief unused (COFF name: bsize)
			 */
			uint32_t bss_size = 0;
			/**
			 * @brief Initial value of eip (COFF name: entry)
			 */
			uint32_t entry_address = 0;
			/**
			 * @brief unused (COFF name: text_start)
			 */
			uint32_t code_address = 0;
			/**
			 * @brief unused (COFF name: data_start)
			 */
			uint32_t data_address = 0;

			AOutHeader(uint16_t magic = 0)
				: magic(magic)
			{
			}

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

		protected:
			virtual void DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region);

		public:
			void Dump(COFFFormat& coff, Dumper::Dumper& dump) override;
		};

		/**
		 * @brief Concurrent DOS 68K/FlexOS 386 optional header
		 * Concurrent DOS 68K uses the typical a.out header with two additional fields for the offset to relocations and the size of the stack
		 */
		class FlexOSAOutHeader : public AOutHeader
		{
		public:
			/**
			 * @brief The offset to the crunched relocation data within the file
			 */
			uint32_t relocations_offset = 0;
			/**
			 * @brief Size of stack for execution
			 */
			uint32_t stack_size = 0;

			/* TODO: magic not needed for CDOS68K? */

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

			void PostReadFile(COFFFormat& coff, Linker::Reader& rd) override;

			void PostWriteFile(COFFFormat& coff, Linker::Writer& wr) override;

		protected:
			void DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) override;
		};

		/**
		 * @brief GNU a.out optional header
		 * TODO
		 */
		class GNUAOutHeader : public OptionalHeader
		{
		public:
			/* Note: untested */
			uint32_t info = 0;
			uint32_t code_size = 0;
			uint32_t data_size = 0;
			uint32_t bss_size = 0;
			uint32_t symbol_table_size = 0;
			uint32_t entry_address = 0;
			uint32_t code_relocation_size = 0;
			uint32_t data_relocation_size = 0;

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& wr) override;

			void WriteFile(Linker::Writer& wr) override;

			void Dump(COFFFormat& coff, Dumper::Dumper& dump) override;
		};

		/**
		 * @brief 56 byte long header used on MIPS
		 *
		 * TODO: untested
		 */
		class MIPSAOutHeader : public AOutHeader
		{
		public:
			// https://web.archive.org/web/20140723105157/http://www-scf.usc.edu/~csci402/ncode/coff_8h-source.html
			/* bss_start */
			uint32_t bss_address;
			/* gpr_mask */
			uint32_t gpr_mask;
			/* cprmask */
			uint32_t cpr_mask[4];
			/* gp_value */
			uint32_t gp_value;

			MIPSAOutHeader(uint16_t magic = 0)
				: AOutHeader(magic)
			{
			}

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

		protected:
			void DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) override;
		};

		/**
		 * @brief ECOFF optional header
		 *
		 * TODO: untested
		 */
		class ECOFFAOutHeader : public OptionalHeader
		{
		public:
			static constexpr uint16_t OMAGIC = 0x0107;
			static constexpr uint16_t NMAGIC = 0x0108;
			static constexpr uint16_t ZMAGIC = 0x010B;
			/**
			 * @brief Type of executable
			 */
			uint16_t magic = 0;
			static constexpr uint16_t SYM_STAMP = 0x030D;
			/**
			 * @brief Object file version stamp (ECOFF name: vstamp)
			 */
			uint16_t version_stamp = SYM_STAMP;
			/**
			 * @brief Revision build of system tools (ECOFF name: bldrev)
			 */
			uint16_t build_revision = 0;

			/**
			 * @brief Size of code segment (ECOFF name: tsize)
			 */
			uint64_t code_size = 0;
			/**
			 * @brief Size of data segment (ECOFF name: dsize)
			 */
			uint64_t data_size = 0;
			/**
			 * @brief Size of bss segment (ECOFF name: bsize)
			 */
			uint64_t bss_size = 0;
			/**
			 * @brief Virtual address of execution start (ECOFF name: entry)
			 */
			uint64_t entry_address = 0;
			/**
			 * @brief Base address for code segment (ECOFF name: text_start)
			 */
			uint64_t code_address = 0;
			/**
			 * @brief Base address for data segment (ECOFF name: data_start)
			 */
			uint64_t data_address = 0;
			/**
			 * @brief Base address for bss segment (ECOFF name: bss_start)
			 */
			uint64_t bss_address = 0;
			/** @brief unused (ECOFF name: gprmask) */
			uint32_t gpr_mask = 0;
			/** @brief unused (ECOFF name: fprmask) */
			uint32_t fpr_mask = 0;
			/** @brief Initial global pointer value (ECOFF name: gp_value) */
			uint64_t global_pointer = 0;

			ECOFFAOutHeader(uint16_t magic = 0)
				: magic(magic)
			{
			}

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

			void Dump(COFFFormat& coff, Dumper::Dumper& dump) override;
		};

		/**
		 * @brief XCOFF optional header
		 *
		 * TODO: untested
		 */
		class XCOFFAOutHeader : public OptionalHeader
		{
		public:
			/** @brief False for XCOFF32, true for XCOFF64 */
			bool is64 = false;
			/**
			 * @brief Type of executable (XCOFF name: mflag)
			 */
			uint16_t magic = 0;
			/**
			 * @brief Object file version stamp (XCOFF name: vstamp)
			 */
			uint16_t version_stamp = 0;
			/**
			 * @brief Size of code segment (XCOFF name: tsize)
			 */
			uint64_t code_size = 0;
			/**
			 * @brief Size of data segment (XCOFF name: dsize)
			 */
			uint64_t data_size = 0;
			/**
			 * @brief Size of bss segment (XCOFF name: bsize)
			 */
			uint64_t bss_size = 0;
			/**
			 * @brief Virtual address of execution start (XCOFF name: entry)
			 */
			uint64_t entry_address = 0;
			/**
			 * @brief Base address for code segment (XCOFF name: text_start)
			 */
			uint64_t code_address = 0;
			/**
			 * @brief Base address for data segment (XCOFF name: data_start)
			 */
			uint64_t data_address = 0;
			/**
			 * @brief Address of TOC (XCOFF name: toc)
			 */
			uint64_t toc_address = 0;
			/**
			 * @brief Section number for entry point (XCOFF name: snentry)
			 */
			uint16_t entry_section = 0;
			/**
			 * @brief Section number for text (XCOFF name: sntext)
			 */
			uint16_t code_section = 0;
			/**
			 * @brief Section number for data (XCOFF name: sndata)
			 */
			uint16_t data_section = 0;
			/**
			 * @brief Section number for TOC (XCOFF name: sntoc)
			 */
			uint16_t toc_section = 0;
			/**
			 * @brief Section number for loader data (XCOFF name: snloader)
			 */
			uint16_t loader_section = 0;
			/**
			 * @brief Section number for bss (XCOFF name: snbss)
			 */
			uint16_t bss_section = 0;
			/**
			 * @brief Maximum alignment of text (XCOFF name: algntext)
			 */
			uint16_t code_align = 0;
			/**
			 * @brief Maximum alignment of data (XCOFF name: algndata)
			 */
			uint16_t data_align = 0;
			/**
			 * @brief Module type (XCOFF name: modtype)
			 */
			uint16_t module_type = 0;
			/**
			 * @brief CPU flags (XCOFF name: cpuflag)
			 */
			uint8_t cpu_flags = 0;
			/**
			 * @brief CPU type (XCOFF name: cputype)
			 */
			uint8_t cpu_type = 0;
			/**
			 * @brief Maximum stack size (XCOFF name: maxstack)
			 */
			uint64_t maximum_stack_size = 0;
			/**
			 * @brief Maximum data size (XCOFF name: maxdata)
			 */
			uint64_t maximum_data_size = 0;
			/**
			 * @brief Reserved for debugger (XCOFF name: debugger)
			 */
			uint32_t debugger_data = 0;
			/**
			 * @brief Requested code page size (XCOFF name: textpsize)
			 */
			uint8_t code_page_size = 0;
			/**
			 * @brief Requested data page size (XCOFF name: datapsize)
			 */
			uint8_t text_page_size = 0;
			/**
			 * @brief Requested stack page size (XCOFF name: stackpsize)
			 */
			uint8_t stack_page_size = 0;
			/**
			 * @brief Flags
			 */
			uint8_t flags = 0;
			/**
			 * @brief Section number for tdata (XCOFF name: sntdata)
			 */
			uint16_t tdata_section = 0;
			/**
			 * @brief Section number for tbss (XCOFF name: sntbss)
			 */
			uint16_t tbss_section = 0;
			/**
			 * @brief XCOFF64 specific flags (XCOFF64 name: x64flags)
			 */
			uint32_t xcoff64_flags = 0;
#if 0
			/**
			 * @brief Shared memory page size (XCOFF64 name: shmpsize)
			 */
			uint8_t shared_memory_page = 0;
#endif

			XCOFFAOutHeader(bool is64, uint16_t magic = 0)
				: is64(is64), magic(magic)
			{
			}

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

			void Dump(COFFFormat& coff, Dumper::Dumper& dump) override;
		};

		void Clear() override;

		void AssignMagicValue();

		COFFVariantType coff_variant = COFFVariantType(0);

		/**
		 * @brief The CPU type, reflected by the first 16-bit word of a COFF file
		 *
		 * The byte order has to be determined heuristically.
		 */
		cpu cpu_type = CPU_UNKNOWN;

		/**
		 * @brief The byte order
		 */
		::EndianType endiantype = ::EndianType(0);

		bool DetectCpuType(::EndianType expected);

		void DetectCpuType();

		void ReadFile(Linker::Reader& rd) override;

	protected:
		void ReadCOFFHeader(Linker::Reader& rd);
		void ReadOptionalHeader(Linker::Reader& rd);
		void ReadRestOfFile(Linker::Reader& rd);

	public:
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;

	protected:
		offset_t WriteFileContents(Linker::Writer& wr);

	public:
		void Dump(Dumper::Dumper& dump) override;

		/* * * Reader members * * */

		void SetupOptions(std::shared_ptr<Linker::OutputFormat> format) override;

		bool option_segmentation = false;

		bool FormatRequiresDataStreamFix() const override;

	private:
		/* symbols */
		std::string segment_prefix();

		std::string segment_of_prefix();

		/**
		 * @brief For Z8000 short segmented addresses
		 */
		std::string segmented_address_prefix();

#if 0
		// TODO: can this be used?
		std::string segment_difference_prefix();
#endif

		enum
		{
			/* section number */
			N_UNDEF = 0,
			N_ABS = 0xFFFF,
			N_DEBUG = 0xFFFE,

			/* storage class */
			C_EXT = 2,
			C_STAT = 3,
			C_LABEL = 6,
		};

	public:
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		/* * * Writer members * * */

		/**
		 * @brief Represents the type of target system, which will determine the CPU type and several other fields
		 */
		enum format_type
		{
			/**
			 * @brief An unspecified value, probably will not work
			 */
			GENERIC,
			/**
			 * @brief DJGPP COFF executable
			 */
			DJGPP,
			/**
			 * @brief Concurrent DOS 68K executable (untested but confident)
			 */
			CDOS68K,
			/**
			 * @brief FlexOS 386 executable (unknown)
			 */
			CDOS386,
		};
		/**
		 * @brief A representation of the format to generate
		 */
		format_type type = GENERIC;

		/**
		 * @brief Suppress relocation generation, only relevant for Concurrent DOS 68K, since the other target formats do not store relocations
		 */
		bool option_no_relocation = false;

		/**
		 * @brief Size of MZ stub, only used for DJGPP COFF executables
		 */
		uint32_t stub_size = 0;

		/**
		 * @brief Concurrent DOS 68K and FlexOS 386: The stack segment, not stored as part of any section
		 */
		std::shared_ptr<Linker::Segment> stack;
		/**
		 * @brief Entry address, gets stored in optional header later
		 */
		uint32_t entry_address = 0; /* TODO */
		/**
		 * @brief Concurrent DOS 68K: Offset to relocations
		 */
		uint32_t relocations_offset = 0;

		COFFFormat(format_type type = GENERIC)
			: type(type)
		{
		}

		~COFFFormat()
		{
			Clear();
		}

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		static std::shared_ptr<COFFFormat> CreateWriter(format_type type);

		void SetOptions(std::map<std::string, std::string>& options) override;

		/** @brief COFF file header flags, most of these are obsolete, we only use them as precombined flag sets */
		enum
		{
			FLAG_NO_RELOCATIONS = 0x0001,
			FLAG_EXECUTABLE = 0x0002,
			FLAG_NO_LINE_NUMBERS = 0x0004,
			FLAG_NO_SYMBOLS = 0x0008,
			FLAG_32BIT_LITTLE_ENDIAN = 0x0100,
			FLAG_32BIT_BIG_ENDIAN = 0x0200,

			/**
			 * @brief Stored as the magic of the a.out header
			 */
			ZMAGIC = 0x010B,
		};

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		/** @brief Return the segment stored inside the section, note that this only works for binary generation */
		std::shared_ptr<Linker::Segment> GetSegment(std::unique_ptr<Section>& section);

		std::shared_ptr<Linker::Segment> GetCodeSegment();

		std::shared_ptr<Linker::Segment> GetDataSegment();

		std::shared_ptr<Linker::Segment> GetBssSegment();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

}

#endif /* COFF_H */
