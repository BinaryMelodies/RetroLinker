#ifndef MZEXE_H
#define MZEXE_H

#include <iomanip>
#include <set>
#include <string>
#include <vector>
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/format.h"
#include "../linker/module.h"
#include "../linker/reader.h"
#include "../linker/section.h"
#include "../linker/segment.h"
#include "../linker/writer.h"
#include "../dumper/dumper.h"

namespace Microsoft
{
	/**
	 * @brief MZ .EXE format for MS-DOS
	 *
	 * The native .exe file format for MS-DOS, developed as an upgrade from the earlier flat binary .com file format.
	 * They are named after their identification code "MZ" that appears as the first two bytes of executables.
	 *
	 * HP 100LX/200LX System Manager modules (file extension .exm) use a variant of this format, with "MZ" replaced with "DL",
	 * executable and non-executable parts separated into separate segments, and the field at offset 0x1A storing the segment
	 * of the data segment.
	 *
	 * First appeared probably around MS-DOS 1.0 (they were absent from PC DOS 0.9), they were initially identified by
	 * the new file extension ".exe". Since MS-DOS 2.0, MZ executables may also have the extension ".com", as DOS only looks
	 * at the first two bytes to determine the file format.
	 */
	class MZFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		/* * * General members * * */

		/** @brief Type of magic number, usually "MZ" */
		enum magic_type
		{
			/** @brief The most common magic number "MZ" */
			MAGIC_MZ = 1,
			/** @brief According to some sources such as Ralf Brown's interrupt list, some early excutables started with the sequence "ZM" instead */
			MAGIC_ZM,
			/** @brief HP 100LX/200LX System Manager modules (.exm) use the magic number "DL" */
			MAGIC_DL,
		};

		/** @brief The magic number at the start of the executable file, usually "MZ" */
		char signature[2]; /* TODO: make parameter */

		/** @brief Size of last 512 byte block, 0 if full. Set by CalculateValues */
		uint16_t last_block_size;
		/** @brief Size of MZ image in 512 blocks, rounded up. Set by CalculateValues */
		uint16_t file_size_blocks; /* TODO: consider making file size a parameter */

		uint32_t GetFileSize() const;

		/** @brief Number of relocations. Updated by CalculateValues */
		uint16_t relocation_count;
		/** @brief Size of MZ header. Updated by CalculateValues */
		uint16_t header_size_paras; /* TODO: make header size a parameter */
		/** @brief Minimum required extra memory, in paragraphs */
		uint16_t min_extra_paras;
		/** @brief Maximum required extra memory, in paragraphs. Set by CalculateValues using extra_paras */
		uint16_t max_extra_paras;
		/** @brief Initial value for the stack segment (SS) */
		uint16_t ss;
		/** @brief Initial value for the stack (SP) */
		uint16_t sp;
		/** @brief Checksum */
		uint16_t checksum; /* TODO: fill when writing */
		/** @brief Entry point initial value for IP */
		uint16_t ip;
		/** @brief Initial value for the code segment (CS) */
		uint16_t cs;
		/** @brief Offset to first relocation. Updated by CalculateValues */
		uint16_t relocation_offset; /* TODO: make parameter */

		/** @brief Overlay number, should be 0 for main programs, not used for .exm files */
		uint16_t overlay_number;
		/** @brief Starting paragraph of program data, only required for .exm files */
		uint16_t data_segment;

		/**
		 * @brief Represents a relocation entry in the header, as a pair of 16-bit words
		 *
		 * Since only the linear offset of the relocation is actually imporant, the same relocation can be represented as various different pairs
		 */
		struct Relocation
		{
			/** @brief Segment of relocation */
			uint16_t segment;
			/** @brief Offset of relocation within segment */
			uint16_t offset;

			Relocation(uint16_t segment, uint16_t offset)
				: segment(segment), offset(offset)
			{
			}

			static Relocation FromLinear(uint32_t address);

			uint32_t GetOffset() const;

			bool operator ==(const Relocation& other) const;

			bool operator <(const Relocation& other) const;
		};

		/** @brief Address relocation offsets to paragraph fixups */
		std::vector<Relocation> relocations;

		/** @brief Concurrent DOS embedded program information, produced by PIFED */
		struct PIF
		{
			static constexpr uint32_t MAGIC_BEGIN = 0x0013EDC1;
			static constexpr uint32_t MAGIC_END = 0xEDC10013;
			static constexpr size_t SIZE = 19;

			/* TODO: requires testing */
			uint16_t maximum_extra_paragraphs;
			uint16_t minimum_extra_paragraphs;
			uint8_t flags;
			uint8_t lowest_used_interrupt;
			uint8_t highest_used_interrupt;
			uint8_t com_port_usage;
			uint8_t lpt_port_usage;
			uint8_t screen_usage;

			void SetDefaults();

			void ReadFile(Linker::Reader& rd);

			void WriteFile(Linker::Writer& wr);

			void Dump(Dumper::Dumper& dump, offset_t file_offset);
		};

		/** @brief Concurrent DOS program information entry, allocated only if present */
		PIF * pif;

		/** @brief The program image, placed after the MZ header */
		Linker::Writable * image;

		magic_type GetSignature() const;

		void SetSignature(magic_type magic);

		void Initialize() override;

		void Clear() override;

		MZFormat()
		{
			Initialize();
		}

		~MZFormat()
		{
			Clear();
		}

		void SetFileSize(uint32_t size);

		uint32_t GetHeaderSize();

		uint32_t GetPifOffset() const;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		void CalculateValues() override;

		/* * * Writer members * * */

		/** @brief Represents the memory model of the running executable, which is the way in which the segments are set up during execution */
		enum memory_model_t
		{
			/** @brief Default model, same as small */
			MODEL_DEFAULT,
			/** @brief Tiny model, code and data segment are the same */
			MODEL_TINY,
			/** @brief Small model, separate code and data segments */
			MODEL_SMALL,
			/** @brief Compact model, separate code and multiple data segments */
			MODEL_COMPACT,
			/** @brief Large model, every section is a separate segment */
			MODEL_LARGE,
		};
		/** @brief Memory model of generated executable */
		memory_model_t memory_model;

		/* filled in automatically */
		/** @brief Required maximum extra paragraphs after bss */
		uint16_t extra_paras;

		/** @brief Total size of bss and stack */
		uint32_t zero_fill;

		/** @brief User provided alignment value for header size */
		uint32_t option_header_align;

		/** @brief User provided alignment value for file align */
		uint32_t option_file_align;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		using LinkerManager::SetLinkScript;

		void SetModel(std::string model) override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(Linker::Segment * segment) override;

		/**
		 * @brief Create the required segments, if they have not already been allocated.
		 * The MZ format uses a single segment.
		 */
		void CreateDefaultSegments();

		Script::List * GetScript(Linker::Module& module);

		/**
		 * @brief Link application according to script or memory model ()
		 */
		void Link(Linker::Module& module);

#if 0
		/**
		 * @brief Link application according to large model
		 */
		void LinkLarge(Linker::Module& module);
#endif

		void ProcessModule(Linker::Module& module) override;

		uint32_t GetDataSize() const;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	class MZSimpleStubWriter
	{
	public:
		std::string stub_file;
		bool stub_file_valid;
		std::ifstream stub;

		MZSimpleStubWriter(std::string stub_file = "")
			: stub_file(stub_file), stub_file_valid(true), stub_size(-1)
		{
		}

		offset_t stub_size;

		bool OpenAndCheckValidFile();

		offset_t GetStubImageSize();

		void WriteStubImage(std::ostream& out);

		void WriteStubImage(Linker::Writer& wr);

		~MZSimpleStubWriter()
		{
			if(stub.is_open())
			{
				stub.close();
			}
		}
	};

	class MZStubWriter
	{
	public:
		std::string stub_file;
		bool stub_file_valid;
		std::ifstream stub;

		MZStubWriter(std::string stub_file = "")
			: stub_file(stub_file), stub_file_valid(true), original_file_size(-1)
		{
		}

		uint32_t original_file_size;
		uint32_t stub_file_size;
		uint16_t stub_reloc_count;
		uint32_t original_header_size;
		uint32_t stub_header_size;
		uint16_t original_reloc_offset;
		uint16_t stub_reloc_offset;

		bool OpenAndCheckValidFile();

		offset_t GetStubImageSize();

		void WriteStubImage(std::ostream& out);

		void WriteStubImage(Linker::Writer& wr);

		~MZStubWriter()
		{
			if(stub.is_open())
			{
				stub.close();
			}
		}
	};

}

#endif /* MZEXE_H */
