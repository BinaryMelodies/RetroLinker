#ifndef ARCH_H
#define ARCH_H

#include <array>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/format.h"
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
			virtual std::shared_ptr<Linker::Image> ReadFile(Linker::Reader& rd, offset_t size) = 0;
		};

		offset_t file_offset = 0;
		offset_t file_size = offset_t(-1);
		std::shared_ptr<FileReader> file_reader = nullptr;

		typedef std::shared_ptr<Linker::Image> file_reader_type(Linker::Reader& rd, offset_t size);
		typedef std::shared_ptr<Linker::Image> file_reader1_type(Linker::Reader& rd);

		void SetFileReader(std::shared_ptr<FileReader> file_reader);
		void SetFileReader(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd, offset_t size));
		void SetFileReader(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd));

		ArchiveFormat(std::shared_ptr<FileReader> file_reader = nullptr)
			: file_reader(file_reader)
		{
		}

		ArchiveFormat(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd, offset_t size));
		ArchiveFormat(std::shared_ptr<Linker::Image> (* file_reader)(Linker::Reader& rd));

		class File
		{
		public:
			std::string name;
			offset_t extended_name_offset = 0;
			uint64_t modification = 0;
			uint32_t owner_id = 0, group_id = 0;
			uint32_t mode;
			uint64_t size;
			std::shared_ptr<Linker::Image> contents;
		};

		std::vector<File> files;

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::ModuleCollector& linker, std::string file_name, bool is_library = false) const override;
		void CalculateValues() override;
		// TODO
	};
}

#endif /* ARCH_H */
