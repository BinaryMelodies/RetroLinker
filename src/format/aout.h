#ifndef AOUT_H
#define AOUT_H

#include "mzexe.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: UNIX v1 a.out
	magic
	header_size + text_size + data_size
	symtab_size
	relocations_size
	bss_size
	0

TODO: PDP-11 a.out
*/

namespace AOut
{
	/**
	 * @brief UNIX/Linux a.out binary file format
	 *
	 * Introduced in the earliest versions of UNIX, it underwent several incompatible updates, including being extended to 32 bits.
	 * At the time of writing, almost all current UNIX platforms use the COFF or ELF format.
	 *
	 * Versions:
	 * - PDP-7 version (unimplemented)
	 * - UNIX v1 version (unimplemented)
	 * - 16-bit UNIX version (unimplemented)
	 * - 32-bit UNIX version (first introduced on the VAX)
	 *
	 * Variants:
	 * - OMAGIC (old magic), the code segment is writable
	 * - NMAGIC (new magic), the code segment is pure and can be shared (read-only)
	 * - ZMAGIC, demand paged
	 * - QMAGIC (TODO)
	 *
	 * Platforms:
	 * - early Linux (unimplemented) (obsolete)
	 * - early DJGPP before version 1.11
	 * - early PDOS/386 and PD-Windows (http://pdos.sourceforge.net/) (obsolete)
	 */
	class AOutFormat : public virtual Linker::InputFormat, public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */
		::EndianType endiantype = ::UndefinedEndian;

		/**
		 * @brief The target operating system type for parsing or generating binaries
		 */
		enum system_type
		{
			/** @brief Treat it as undetermined, intended for object files */
			UNSPECIFIED,
			/** @brief Treat it as a UNIX Version 1/2 binary (PDP-11 only) */
			UNIX_V1, /* TODO */
			/** @brief Treat it as a UNIX Version 3/4/5/6/7 binary */
			UNIX,
			/** @brief Treat it as a UNIX System III binary */
			SYSTEM_III,
			/** @brief Treat it as a UNIX System V binary (PDP-11 only) */
			SYSTEM_V,
			/**
			 * @brief Treat it as a Linux a.out binary, used before kernel 5.19 (Linux switched to ELF in 1.2)
			 *
			 * Linux supported OMAGIC, NMAGIC, ZMAGIC, QMAGIC binaries, the midmag field is in the native byte order and the machine type is stored in bits 16 to 23 of midmag
			 */
			LINUX, /* TODO: not fully supported */
			/**
			 * @brief Treat it as a FreeBSD a.out binary, used before 3.0
			 *
			 * FreeBSD supported OMAGIC, NMAGIC, ZMAGIC, QMAGIC binaries, the midmag field is preferred to be in the little endian byte order and the machine type is stored in bits 16 to 25 of midmag
			 */
			FREEBSD, /* TODO */
			/**
			 * @brief Treat it as a NetBSD a.out binary, used before 1.5
			 *
			 * FreeBSD supported OMAGIC, NMAGIC, ZMAGIC, QMAGIC binaries, the midmag field is preferred to be in the big endian byte order and the machine type is stored in bits 16 to 25 of midmag
			 */
			NETBSD, /* TODO */
			/**
			 * @brief Treat it as a DJGPP a.out binary, used before DJGPP 1.11
			 *
			 * Similar to Linux
			 */
			DJGPP1,
			/**
			 * @brief Treat it as a Public Domain OS a.out, a 32-bit DOS-like operating system (http://pdos.sourceforge.net/)
			 *
			 * PDOS only supports OMAGIC binaries
			 */
			PDOS386,
			/**
			 * @brief Treat it as an EMX a.out binary, should only be used with EMXAOutFormat
			 */
			EMX,
		};

		static constexpr system_type DEFAULT = system_type(-1);

		/**
		 * @brief The target operating system, it influences section offset and address choices
		 */
		system_type system;

		/**
		 * @brief The low 16 bits of the midmag field, it determines how sections should be loaded into memory, in a system dependent way
		 */
		enum magic_type
		{
			/** @brief Object file or impure executable: The text, data and bss sections are loaded contiguously */
			OMAGIC = 0x0107,
			/** @brief Same as OMAGIC */
			MAGIC_IMPURE = OMAGIC,

			/** @brief Pure executable: The data section starts on a page boundary, making it possible to have a write protected text section */
			NMAGIC = 0x0108,
			/** @brief Same as NMAGIC */
			MAGIC_ROTEXT = NMAGIC,
			/** @brief Same as NMAGIC */
			MAGIC_PURE = NMAGIC,

			/** @brief Demand paged executable: Like NMAGIC, the data section starts on a page boundary, but the text section also starts on a page boundary */
			ZMAGIC = 0x010B,
			/** @brief Same as ZMAGIC */
			MAGIC_DEMAND_PAGED = ZMAGIC,

			/** @brief The header is included in the text segment, but the first page is unmapped */
			QMAGIC = 0x00CC,
			/** @brief Same as QMAGIC */
			MAGIC_UNMAPPED_ZERO = QMAGIC,

			// PDP-11 specific

			/** @brief UNIX Version 1 magic number, collides with Version 7 overlay magic number */
			MAGIC_V1 = 0x0105 | 0xFFFF0000,

			/** @brief Overlay, since UNIX Version 7 (PDP-11 only, TODO: double check if not supported for VAX) */
			MAGIC_OVERLAY = 0x0105,

			/** @brief Code and data are in different address spaces (PDP-11 only) */
			MAGIC_SEPARATE = 0x0109,

			/** @brief 2.9BSD/2.11BSD (PDP-11 only) */
			MAGIC_AUTO_OVERLAY_NONSEPARATE = 0x0118,

			/** @brief 2.9BSD/2.11BSD (PDP-11 only) */
			MAGIC_AUTO_OVERLAY_SEPARATE = 0x0119,
		};
		/**
		 * @brief The low 16 bits of the midmag field
		 */
		magic_type magic;

		/**
		 * @brief Default magic number associated with the system
		 *
		 * This information is based on studying the executables included with that software.
		 * PDOS/386 uses OMAGIC, whereas DJGPP uses ZMAGIC.
		 */
		static constexpr magic_type GetDefaultMagic(system_type system)
		{
			switch(system)
			{
			default:
			case DJGPP1:
				return ZMAGIC;
			case PDOS386:
				return OMAGIC;
			}
		}

		/**
		 * @brief All fields in the header must appear in native byte order, except for FreeBSD and NetBSD where:
		 * - Under FreeBSD, the midmag field is expected to be in little endian byte order, but it recognizes NetBSD headers with a big endian midmag field
		 * - Under NetBSD, the midmag field is expected to be in big endian byte order, but if the CPU field is missing, it can be read in little endian byte order */
		::EndianType midmag_endiantype;

	protected:
		AOutFormat(system_type system, magic_type magic)
			: system(system), magic(magic)
		{
		}

	public:
		AOutFormat()
			: system(UNSPECIFIED), magic(magic_type(0))
		{
		}

		/** Represents a CPU/Machine type field, typically 8 to 10 bits wide */
		uint16_t mid_value = 0;

		/** Represents the high 8 bits of the midmag field, typically 6 to 8 bits wide */
		uint8_t flags = 0;

		static constexpr uint16_t MID_UNKNOWN = 0x000;
		static constexpr uint16_t MID_68010 = 0x001;
		static constexpr uint16_t MID_68020 = 0x002;
		static constexpr uint16_t MID_PC386 = 0x064; // mentioned for Linux and NetBSD
		static constexpr uint16_t MID_I386 = 0x086; // mentioned for FreeBSD and NetBSD
		static constexpr uint16_t MID_ARM6 = 0x08F; // mentioned for FreeBSD and NetBSD
		static constexpr uint16_t MID_MIPS1 = 0x097; // mentioned for Linux and NetBSD
		static constexpr uint16_t MID_MIPS2 = 0x098; // mentioned for Linux and NetBSD
		static constexpr uint16_t MID_HP200 = 0x0C8; // mentioned for FreeBSD and NetBSD
		static constexpr uint16_t MID_HP300 = 0x12C; // mentioned for FreeBSD and NetBSD
		static constexpr uint16_t MID_HPUX800 = 0x20B; // mentioned for FreeBSD and NetBSD
		static constexpr uint16_t MID_HPUX = 0x20C; // mentioned for FreeBSD and NetBSD

		// according to a.out.h
		static constexpr uint16_t MID_LINUX_OLDSUN2 = 0x000;
		static constexpr uint16_t MID_LINUX_SPARC = 0x003;

		// according to imgact_aout.h
		static constexpr uint16_t MID_FREEBSD_SPARC = 0x08C;

		// according to aout_mids.h
		static constexpr uint16_t MID_NETBSD_M68K = 0x087;
		static constexpr uint16_t MID_NETBSD_M68K4K = 0x088;
		static constexpr uint16_t MID_NETBSD_NS32532K = 0x089;
		static constexpr uint16_t MID_NETBSD_SPARC = 0x08A;
		static constexpr uint16_t MID_NETBSD_PMAX = 0x08B;
		static constexpr uint16_t MID_NETBSD_VAX1K = 0x08C;
		static constexpr uint16_t MID_NETBSD_ALPHA = 0x08D;
		static constexpr uint16_t MID_NETBSD_MIPS = 0x08E;
		static constexpr uint16_t MID_NETBSD_M680002K = 0x090;
		static constexpr uint16_t MID_NETBSD_SH3 = 0x091;
		static constexpr uint16_t MID_NETBSD_POWERPC64 = 0x094;
		static constexpr uint16_t MID_NETBSD_POWERPC = 0x095;
		static constexpr uint16_t MID_NETBSD_VAX = 0x096;
		static constexpr uint16_t MID_NETBSD_M88K = 0x099;
		static constexpr uint16_t MID_NETBSD_HPPA = 0x09A;
		static constexpr uint16_t MID_NETBSD_SH5_64 = 0x09B;
		static constexpr uint16_t MID_NETBSD_SPARC64 = 0x09C;
		static constexpr uint16_t MID_NETBSD_X86_64 = 0x09D;
		static constexpr uint16_t MID_NETBSD_SH5_32 = 0x09E;
		static constexpr uint16_t MID_NETBSD_IA64 = 0x09F;
		static constexpr uint16_t MID_NETBSD_AARCH64 = 0x0B7;
		static constexpr uint16_t MID_NETBSD_OR1K = 0x0B8;
		static constexpr uint16_t MID_NETBSD_RISCV = 0x0B9;

		static constexpr uint16_t MID_BFD_ARM = 0x067;

		enum word_size_t
		{
			WordSize16 = 2,
			WordSize32 = 4,
		};

		/** @brief Number of bytes in a machine word (2 or 4), typically also determines the size of the header (16 or 32 bytes) */
		word_size_t word_size = word_size_t(0);

		/** @brief Retrieves the size of the header for the current settings */
		constexpr uint32_t GetHeaderSize() const
		{
			return word_size * 8;
		}

		enum cpu_type
		{
			UNKNOWN,
			M68K,
			SPARC,
			SPARC64,
			I386,
			AMD64,
			ARM,
			AARCH64,
			MIPS,
			PARISC,
			PDP11,
			NS32K,
			VAX,
			ALPHA,
			SUPERH,
			SUPERH64,
			PPC,
			PPC64,
			M88K,
			IA64,
			OR1K,
			RISCV,
			SYS360,
			SYS390_64,
		};
		cpu_type cpu = UNKNOWN;

		/** @brief Returns the default endian type for the currently set CPU */
		::EndianType GetEndianType() const;

		/** @brief Returns the expected word size for the currently set CPU */
		word_size_t GetWordSize() const;

		uint32_t code_size = 0;
		uint32_t data_size = 0;
		uint32_t bss_size = 0;
		uint32_t symbol_table_size = 0;
		uint32_t entry_address = 0;
		// only for 16-bit format
		uint16_t reserved = 0;
		uint16_t relocations_suppressed = 0;
		// only for 32-bit format
		uint32_t code_relocation_size = 0;
		uint32_t data_relocation_size = 0;
		// AT&T specifi
		uint16_t environment_stamp = 0;

		/**
		 * @brief Represents an a.out relocation, unifying 16-bit and 32-bit formats
		 */
		class Relocation
		{
		public:
			/** @brief Offset of relocation within segment */
			uint32_t address = 0;
			/** @brief Symbol number, only valid for external relocations */
			uint32_t symbol = 0;
			/** @brief Set to true if the relocation is PC-relative */
			bool relative = false;
			/**
			 * @brief Size of relocation, in bytes
			 *
			 * Must be 2 for 16-bit a.out
			 */
			size_t size = 0;
			/** @brief Type of target */
			enum segment_type
			{
				/** @brief Target is undefined */
				Undefined,
				/** @brief Target is external, symbol contains symbol number */
				External,
				/** @brief Target is an absolute value */
				Absolute,
				/** @brief Target is the text segment */
				Text,
				/** @brief Target is the data segment */
				Data,
				/** @brief Target is the bss segment */
				Bss,
				/** @brief Represents a file name symbol */
				FileName,
				/** @brief Relocation entry is invalid */
				Illegal,
			};
			/** @brief Represents the type of relocation target */
			segment_type segment = Undefined;
			/** @brief For invalid bit combinations, stores the bits that are expected to be 0 */
			uint32_t literal_entry = 0;

			static Relocation ReadFile16Bit(Linker::Reader& rd, uint16_t offset);
			void WriteFile16Bit(Linker::Writer& wr) const;

			static Relocation ReadFile32Bit(Linker::Reader& rd);
			void WriteFile32Bit(Linker::Writer& wr) const;
		};

		std::vector<Relocation> code_relocations, data_relocations;

		std::shared_ptr<Linker::Contents> code, data, bss;

	private:
		bool AttemptFetchMagic(uint8_t signature[4]);

		bool CheckFileSizes(Linker::Reader& rd, offset_t image_size);

		bool AttemptReadFile(Linker::Reader& rd, uint8_t signature[4], offset_t image_size);

	public:
		class Symbol
		{
		public:
			std::string name;
			uint16_t unknown = 0;
			uint16_t name_offset = 0;
			uint16_t type = 0;
			uint16_t value = 0;
		};

		std::vector<Symbol> symbols;

		mutable uint32_t page_size = 0; // GetPageSize will set this if it is 0, and so it needs to be declared mutable

		uint32_t GetPageSize() const;
		uint32_t GetTextOffset() const;
		uint32_t GetTextAddress() const;
		uint32_t GetDataOffsetAlign() const;
		uint32_t GetDataAddressAlign() const;

		void ReadHeader(Linker::Reader& rd);

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		void WriteHeader(Linker::Writer& wr) const;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		/* * * Reader members * * */

		static std::shared_ptr<AOutFormat> CreateReader(word_size_t word_size, ::EndianType endiantype, system_type system = UNSPECIFIED);

		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		/* * * Writer members * * */

		static std::shared_ptr<AOutFormat> CreateWriter(system_type system, magic_type magic);
		static std::shared_ptr<AOutFormat> CreateWriter(system_type system);

		class AOutOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};

			AOutOptionCollector()
			{
				InitializeFields(stub);
			}
		};

		// for old DJGPP executables
		mutable Microsoft::MZSimpleStubWriter stub;

		static std::vector<Linker::OptionDescription<void> *> ParameterNames;
		std::vector<Linker::OptionDescription<void> *> GetLinkerScriptParameterNames() override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		std::shared_ptr<Linker::Segment> GetCodeSegment();

		std::shared_ptr<Linker::Segment> GetDataSegment();

		std::shared_ptr<Linker::Segment> GetBssSegment();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};

}

#endif /* AOUT_H */
