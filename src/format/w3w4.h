#ifndef W3EXE_H
#define W3EXE_H

#include <array>
#include "leexe.h"
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
			std::string filename;
			uint32_t file_offset;
			uint32_t header_size;
			std::shared_ptr<LEFormat> contents;
		};

		offset_t file_offset;
		struct version
		{
			uint8_t major, minor;
		};
		version system_version;
		std::vector<Entry> entries;
		uint32_t file_end;

//		void Clear() override;
//		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */

//		using Linker::OutputFormat::GetDefaultExtension;
//		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief WMM32.VXD (TODO: not implemented)
	 */
	class W4Format : public virtual Linker::OutputFormat
	{
	public:
		// as documented in https://github.com/JHRobotics/patcher9x/blob/main/doc/VXDLIB_UTF8.txt

		class Chunk
		{
		public:
			uint32_t file_offset = 0;
			uint32_t length = 0;
			std::shared_ptr<Linker::Buffer> contents = nullptr;
		};

		offset_t file_offset;
		struct version
		{
			uint8_t major, minor;
		};
		version system_version;
		uint16_t chunk_size;
		std::vector<Chunk> chunks;
		uint32_t file_end;
		W3Format w3format;

		std::shared_ptr<Linker::Buffer> DecompressW4();

//		void Clear() override;
//		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */

//		using Linker::OutputFormat::GetDefaultExtension;
//		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* W3EXE_H */
