#ifndef MZEXE_H
#define MZEXE_H

#include <iomanip>
#include <set>
#include <string>
#include <vector>
#include "../common.h"
#include "../linker/format.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/reader.h"
#include "../linker/section.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
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
	class MZFormat : public virtual Linker::SegmentManager
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
		char signature[2] = { 'M', 'Z' }; /* TODO: make parameter */

		/** @brief Size of last 512 byte block, 0 if full. Set by CalculateValues */
		uint16_t last_block_size = 0;
		/** @brief Size of MZ image in 512 blocks, rounded up. Set by CalculateValues */
		uint16_t file_size_blocks = 0; /* TODO: consider making file size a parameter */

		offset_t ImageSize() const override;

		/** @brief Number of relocations. Updated by CalculateValues */
		uint16_t relocation_count = 0;
		/** @brief Size of MZ header. Updated by CalculateValues */
		uint16_t header_size_paras = 0; /* TODO: make header size a parameter */
		/** @brief Minimum required extra memory, in paragraphs */
		uint16_t min_extra_paras = 0;
		/** @brief Maximum required extra memory, in paragraphs. Set by CalculateValues using extra_paras */
		uint16_t max_extra_paras = 0;
		/** @brief Initial value for the stack segment (SS) */
		uint16_t ss = 0;
		/** @brief Initial value for the stack (SP) */
		uint16_t sp = 0;
		/** @brief Checksum */
		uint16_t checksum = 0; /* TODO: fill when writing */
		/** @brief Entry point initial value for IP */
		uint16_t ip = 0;
		/** @brief Initial value for the code segment (CS) */
		uint16_t cs = 0;
		/** @brief Offset to first relocation. Updated by CalculateValues */
		uint16_t relocation_offset = 0; /* TODO: make parameter */

		/** @brief Overlay number, should be 0 for main programs, not used for .exm files */
		uint16_t overlay_number = 0;
		/** @brief Starting paragraph of program data, only required for .exm files */
		uint16_t data_segment = 0;

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

			void WriteFile(Linker::Writer& wr) const;

			void Dump(Dumper::Dumper& dump, offset_t file_offset) const;
		};

		/** @brief Concurrent DOS program information entry, allocated only if present */
		std::unique_ptr<PIF> pif;

		/** @brief The program image, placed after the MZ header */
		std::shared_ptr<Linker::Contents> image;

		magic_type GetSignature() const;

		void SetSignature(magic_type magic);

		void Clear() override;

		~MZFormat()
		{
			Clear();
		}

		void SetFileSize(uint32_t size);

		uint32_t GetHeaderSize() const;

		uint32_t GetPifOffset() const;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Writer members * * */

		class MZOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::optional<offset_t>> header_align{"header_align", "Aligns the end of the header to a specific boundary, must be power of 2"};
			Linker::Option<std::optional<offset_t>> file_align{"file_align", "Aligns the end of the file to a specific boundary, must be power of 2"};
			Linker::Option<offset_t> stack{"stack", "Specify the stack size"};

			MZOptionCollector()
			{
				InitializeFields(header_align, file_align, stack);
			}
		};

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
		memory_model_t memory_model = MODEL_DEFAULT;

		/* filled in automatically */
		/** @brief Stack size requested at the command line */
		uint16_t stack_size = 0;

		/** @brief Required maximum extra paragraphs after bss */
		uint16_t extra_paras = 0;

		/** @brief Total size of bss and stack */
		uint32_t zero_fill = 0;

		/** @brief User provided alignment value for header size */
		uint32_t option_header_align = 0x10;

		/** @brief User provided alignment value for file align */
		uint32_t option_file_align = 1;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		static std::vector<Linker::OptionDescription<void>> MemoryModelNames;

		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;

		void SetModel(std::string model) override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		/**
		 * @brief Create the required segments, if they have not already been allocated.
		 * The MZ format uses a single segment.
		 */
		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		/**
		 * @brief Link application according to script or memory model
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

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

	class MZSimpleStubWriter
	{
	public:
		std::string filename;
		bool valid = true;
		std::ifstream stream;

		MZSimpleStubWriter(std::string filename = "")
			: filename(filename)
		{
		}

		offset_t size = -1;

		bool OpenAndCheckValidFile();

		offset_t GetStubImageSize();

		void WriteStubImage(std::ostream& out);

		void WriteStubImage(Linker::Writer& wr);

		~MZSimpleStubWriter()
		{
			if(stream.is_open())
			{
				stream.close();
			}
		}
	};

	class MZStubWriter
	{
	public:
		std::string filename;
		bool valid = true;
		std::ifstream stream;

		MZStubWriter(std::string filename = "")
			: filename(filename)
		{
		}

		uint32_t original_file_size = -1;
		uint32_t stub_file_size = 0;
		uint16_t stub_reloc_count = 0;
		uint32_t original_header_size = 0;
		uint32_t stub_header_size = 0;
		uint16_t original_reloc_offset = 0;
		uint16_t stub_reloc_offset = 0;

		bool OpenAndCheckValidFile();

		offset_t GetStubImageSize();

		void WriteStubImage(std::ostream& out);

		void WriteStubImage(Linker::Writer& wr);

		~MZStubWriter()
		{
			if(stream.is_open())
			{
				stream.close();
			}
		}
	};

}

#endif /* MZEXE_H */
