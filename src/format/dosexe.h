#ifndef DOSEXE_H
#define DOSEXE_H

#include <array>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/segment_manager.h"
#include "../linker/section.h"
#include "mzexe.h"

namespace SeychellDOS32
{
	/**
	 * @brief Adam Seychell's DOS32 "Adam" executable format
	 */
	class AdamFormat : public virtual Linker::Format, public virtual Linker::SegmentManager
	{
	public:
		enum format_type
		{
			FORMAT_UNSPECIFIED,
			/** @brief DOS32 version 3.3 */
			FORMAT_33,
			/** @brief DOS32 version 3.5 beta */
			FORMAT_35, /* based on Michael Tippach's and own research */
			/** @brief DX64 veresion */
			FORMAT_DX64,
		};
		format_type format;

		enum output_type
		{
			OUTPUT_EXE,
			OUTPUT_DLL
		};
		output_type output;

		AdamFormat(format_type format = FORMAT_UNSPECIFIED, output_type output = OUTPUT_EXE)
			: format(format), output(output)
		{
			if(output == OUTPUT_EXE)
				MakeApplication();
			else
				MakeLibrary();
		}

		enum relocation_type
		{
			Selector16,
			Offset32,
		};

		std::array<char, 4> signature;
		std::array<uint8_t, 2> minimum_dos_version;
		std::array<uint8_t, 2> dlink_version;
		/* header + program + relocations */
		uint32_t image_size = 0;
		/* header */
		uint32_t header_size = 0;
		/* program */
		uint32_t program_size = 0;
		/* program + zero data */
		uint32_t memory_size = 0;
		/* v3.5 only: program + relocations */
		uint32_t contents_size = 0;
		/* v3.3 only */
		uint32_t selector_relocation_count = 0;
		/* DX64 only */
		uint32_t offset_relocations_size = 0;
		uint32_t eip = 0;
		uint32_t esp = 0;
		std::vector<uint32_t> selector_relocations;
		std::vector<uint32_t> offset_relocations;
		/* v3.3: used internally, v3.5: used to import/export */
		std::map<uint32_t, relocation_type> relocations_map;
		uint32_t flags = 0;

		std::shared_ptr<Linker::Contents> image;

		enum
		{
			FLAG_COMPRESSED = 0x0001,
			FLAG_DISPLAY_LOGO = 0x0002,
			FLAG_4MB_HEAP_LIMIT = 0x0004, // 3.5 only
		};

		constexpr bool IsV35() const { return (dlink_version[1] > 0x03) || (dlink_version[1] == 0x03 && dlink_version[0] >= 0x50); }
		constexpr bool IsDLL() const { return signature[0] == 'D'; }

		void MakeApplication();
		void MakeLibrary();

		/* only for v3.5 */
		static uint32_t GetRelocationSize(uint32_t displacement, relocation_type type);

		void CalculateValues() override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		/* * * Writer members * * */

		mutable Microsoft::MZSimpleStubWriter stub;
		uint32_t stack_size = 0;

		class AdamOptionCollector : public Linker::OptionCollector
		{
		public:
			class OutputEnumeration : public Linker::Enumeration<output_type>
			{
			public:
				OutputEnumeration()
					: Enumeration(
						"EXE", OUTPUT_EXE,
						"DLL", OUTPUT_DLL)
				{
					descriptions = {
						{ OUTPUT_EXE, "executable" },
						{ OUTPUT_DLL, "shared library" },
					};
				}
			};

			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<Linker::ItemOf<OutputEnumeration>> output{"output", "Binary type"};
			Linker::Option<offset_t> stack{"stack", "Specify the stack size"};

			AdamOptionCollector()
			{
				InitializeFields(stub, output, stack);
			}
		};

		bool FormatSupportsSegmentation() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		/**
		 * @brief Create the required segments, if they have not already been allocated.
		 * The Adam format uses a single segment.
		 */
		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		/**
		 * @brief Link application according to script or memory model
		 */
		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
};

namespace BorcaD3X
{
	/**
	 * @brief Daniel Borca's D3X executable format
	 */
	class D3X1Format : public virtual Linker::Format
	{
	public:
		uint32_t header_size = 0;
		uint32_t binary_size = 0;
		uint32_t extra_size = 0;
		uint32_t entry = 0;
		uint32_t stack_top = 0;

		D3X1Format()
			: header_size(24)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
};

namespace DX64
{
	/**
	 * @brief CandyMan's DX64 "Flat" and "LV" executable formats
	 */
	class LVFormat : public virtual Linker::Format
	{
	public:
		enum format_type
		{
			FORMAT_FLAT,
			FORMAT_LV,
		};

		char signature[4];
		uint32_t eip = 0;
		uint32_t esp = 0;
		uint32_t extra_memory_size = 0;
		std::shared_ptr<Linker::Contents> image;

		explicit LVFormat()
		{
		}

		LVFormat(format_type type)
		{
			SetSignature(type);
		}

		void SetSignature(format_type type);

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
}

/* TODO: other formats? */

#endif /* DOSEXE_H */
