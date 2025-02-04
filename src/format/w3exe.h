#ifndef W3EXE_H
#define W3EXE_H

#include <array>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace Microsoft
{
	/**
	 * @brief WIN386.EXE (TODO: not implemented)
	 */
	class W3Format : public virtual Linker::OutputFormat
	{
	public:
		class Entry
		{
		public:
			std::array<char, 8> filename;
			uint32_t file_offset;
			uint32_t unknown;
			// TODO: data contents
		};

		offset_t file_offset;
		uint16_t unknown;
		std::vector<Entry> entries;

//		void Clear() override;
//		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */

//		using Linker::OutputFormat::GetDefaultExtension;
//		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* W3EXE_H */
