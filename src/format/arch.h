#ifndef ARCH_H
#define ARCH_H

#include <array>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: not fully implemented */

namespace Archive
{
	/**
	 * @brief UNIX archive format
	 */
	class ArchiveFormat : public virtual Linker::InputFormat, public virtual Linker::OutputFormat
	{
	public:
		class FileReader
		{
		public:
			virtual ~FileReader();
			virtual std::shared_ptr<Linker::Writable> ReadFile(Linker::Reader& rd, offset_t size) = 0;
		};

		offset_t file_offset = 0;
		offset_t file_size = offset_t(-1);
		std::shared_ptr<FileReader> file_reader = nullptr;

		class File
		{
		public:
			std::string name;
			uint64_t modification = 0;
			uint32_t owner_id = 0, group_id = 0;
			uint32_t mode;
			uint64_t size;
			std::shared_ptr<Linker::Writable> contents;
		};

		std::vector<File> files;

		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		void CalculateValues() override;
		// TODO
	};
}

#endif /* ARCH_H */
