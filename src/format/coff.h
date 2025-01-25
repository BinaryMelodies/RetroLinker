#ifndef COFF_H
#define COFF_H

#include <map>
#include "cpm68k.h"
#include "mzexe.h"
#include "../common.h"
#include "../linker/linker.h"
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
			// main supported types
			CPU_I386  = 0x014C,
			CPU_M68K  = 0x0150,
			// GNU binutils output
			CPU_W65   = 0x6500,
			CPU_Z80   = 0x805A,
			CPU_Z8K   = 0x8000,
			// other CPU types, included for completeness sake
			CPU_I86   = 0x0148,
			CPU_NS32K = 0x0154,
			CPU_I370  = 0x0158, // IBM 370
			CPU_MIPS  = 0x0160,
			CPU_M88K  = 0x016D,
			CPU_WE32K = 0x0170,
			CPU_VAX   = 0x0178,
			CPU_AM29K = 0x017A,
			CPU_ALPHA = 0x0183,
			CPU_PPC   = 0x01DF,
			CPU_PPC64 = 0x01F7,
			CPU_SHARC = 0x521C,
			// overloaded values
			// for now, none
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
			virtual offset_t GetAddress() = 0;
			virtual size_t GetSize() = 0;
			virtual void FillEntry(Dumper::Entry& entry) = 0;
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

			static const uint16_t R_Z80_IMM8  = 0x22;
			static const uint16_t R_Z80_IMM16 = 0x01;
			static const uint16_t R_Z80_IMM24 = 0x33;
			static const uint16_t R_Z80_IMM32 = 0x17;
			static const uint16_t R_Z80_OFF8  = 0x32;
			static const uint16_t R_Z80_JR    = 0x02;

			static const uint16_t R_Z8K_IMM4L = 0x23;
			static const uint16_t R_Z8K_IMM4H = 0x24;
			static const uint16_t R_Z8K_DISP7 = 0x25; /* djnz */
			static const uint16_t R_Z8K_IMM8  = 0x22;
			static const uint16_t R_Z8K_IMM16 = 0x01;
			static const uint16_t R_Z8K_REL16 = 0x04;
			static const uint16_t R_Z8K_IMM32 = 0x11;
			static const uint16_t R_Z8K_JR    = 0x02; /* jr */
			static const uint16_t R_Z8K_CALLR = 0x05; /* callr */

			static const uint16_t R_W65_ABS8     = 0x01;
			static const uint16_t R_W65_ABS16    = 0x02;
			static const uint16_t R_W65_ABS24    = 0x03;
			static const uint16_t R_W65_ABS8S8   = 0x04;
			static const uint16_t R_W65_ABS8S16  = 0x05;
			static const uint16_t R_W65_ABS16S8  = 0x06;
			static const uint16_t R_W65_ABS16S16 = 0x07;
			static const uint16_t R_W65_PCR8     = 0x08;
			static const uint16_t R_W65_PCR16    = 0x09;
			static const uint16_t R_W65_DP       = 0x0A;

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

			offset_t GetAddress() override;

			size_t GetSize() override;

			void FillEntry(Dumper::Entry& entry) override;
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
			 */
			uint32_t physical_address = 0;
			/**
			 * @brief The virtual address of the section (COFF name: s_vaddr)
			 */
			uint32_t address = 0;
			/**
			 * @brief The size of the section (COFF name: s_size)
			 */
			uint32_t size = 0;
			/**
			 * @brief Offset of stored image data from COFF header start (COFF name: s_scnptr)
			 */
			uint32_t section_pointer = 0;
			/**
			 * @brief Offset to COFF relocations (COFF name: s_relptr)
			 */
			uint32_t relocation_pointer = 0;
			/**
			 * @brief unused (COFF name: s_lnnoptr)
			 */
			uint32_t line_number_pointer = 0;
			/**
			 * @brief COFF relocation count (COFF name: s_nreloc)
			 */
			uint16_t relocation_count = 0;
			/**
			 * @brief unused (COFF name: s_nlnno)
			 */
			uint16_t line_number_count = 0;
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

			void ReadSectionHeader(Linker::Reader& rd);

			void WriteSectionHeader(Linker::Writer& wr);

			uint32_t ActualDataSize();
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
		uint32_t symbol_table_offset = 0;
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
		 * @brief A standard 28 byte a.out optional header, used by DJGPP
		 *
		 * unimplemented
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
			/* TODO */

		protected:
			void DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) override;
		};

		void Clear() override;

		void AssignMagicValue();

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

		offset_t WriteFile(Linker::Writer& wr) override;

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

		void GenerateModule(Linker::Module& module) const;

	public:
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;

		/* * * Writer members * * */

	public:
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

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

}

#endif /* COFF_H */
