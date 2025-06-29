#ifndef BFLT_H
#define BFLT_H

#include <vector>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace BFLT
{
	/**
	 * @brief BFLT Binary flat format
	 *
	 * A simple flat file format used by uClinux
	 */
	class BFLTFormat : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		::EndianType endian_type = ::UndefinedEndian;
		uint32_t format_version = 4;
		uint32_t image_offset = 0;
		uint32_t bss_size = 0;
		uint32_t stack_size = 0;
		uint32_t relocation_offset = 0;

		std::shared_ptr<Linker::Image> code, data;

		/**
		 * @brief Represents a relocation within the image
		 */
		struct Relocation
		{
			enum relocation_type
			{
				Text = 0,
				Data = 1,
				Bss = 2,
			} type;
			offset_t offset;
		};

		std::vector<Relocation> relocations;
		uint32_t flags = 0;
		static constexpr uint32_t FLAG_RAM = 0x0001;
		static constexpr uint32_t FLAG_GOTPIC = 0x0002;
		static constexpr uint32_t FLAG_GZIP = 0x0004;

		BFLTFormat(uint32_t format_version = 4)
			: format_version(format_version)
		{
		}

		void Clear() override;

		~BFLTFormat()
		{
			Clear();
		}

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Writer members * * */

		std::shared_ptr<Linker::Segment> bss, stack;

		void SetModel(std::string model) override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void ProcessModule(Linker::Module& module) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* BFLT_H */
