#ifndef PCOS_H
#define PCOS_H

#include <array>
#include <memory>
#include <vector>
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"
#include "../dumper/dumper.h"

/* TODO: untested */

namespace PCOS
{
	/**
	 * @brief Olivetti M20 PCOS cmd/sav executable file format
	 *
	 * The native executable file format for the Olivetti M20 PCOS operating system, at least version 4.
	 * Earlier versions seem to use a different format.
	 * The PCOS also has a separate file format for abs executables.
	 * This is based on Christian Groessler's work.
	 */
	class CMDFormat : public virtual Linker::LinkerManager
	{
	public:
		/* * * General members * * */

		class MemoryBlock
		{
		public:
			enum block_type
			{
				TYPE_LOAD = 3,
				TYPE_OFFSET_RELOCATION = 4,
				TYPE_SEGMENT_RELOCATION = 5,
				TYPE_END = 6,
			};
			block_type type;

			virtual ~MemoryBlock();

			virtual uint16_t GetLength() const;
			virtual void ReadFile(Linker::Reader& rd, uint16_t length);
			virtual void WriteFile(Linker::Writer& wr) const;
			virtual std::unique_ptr<Dumper::Region> MakeRegion(std::string name, offset_t offset, unsigned display_width) const;
			virtual void AddFields(Dumper::Region& region) const;
			virtual void DumpContents(Dumper::Dumper& dump, offset_t file_offset) const;
			void Dump(Dumper::Dumper& dump, offset_t file_offset) const;

			static std::unique_ptr<MemoryBlock> ReadFile(Linker::Reader& rd);

			explicit MemoryBlock(int type)
				: type(block_type(type))
			{
			}
		};

		class LoadBlock : public MemoryBlock
		{
		public:
			LoadBlock()
				: MemoryBlock(TYPE_LOAD)
			{
			}

			uint32_t block_id;
			std::shared_ptr<Linker::Writable> image;

			uint16_t GetLength() const override;
			void ReadFile(Linker::Reader& rd, uint16_t length) override;
			void WriteFile(Linker::Writer& wr) const override;
			std::unique_ptr<Dumper::Region> MakeRegion(std::string name, offset_t offset, unsigned display_width) const override;
			void AddFields(Dumper::Region& region) const override;
		};

		struct relocation
		{
			uint8_t source = 0, target = 0;
		};

		class RelocationBlock : public MemoryBlock
		{
		public:
			RelocationBlock(int type)
				: MemoryBlock(type)
			{
			}

			std::vector<relocation> relocations;

			uint16_t GetLength() const override;
			void ReadFile(Linker::Reader& rd, uint16_t length) override;
			void WriteFile(Linker::Writer& wr) const override;
			void DumpContents(Dumper::Dumper& dump, offset_t file_offset) const override;
		};

		class UnknownBlock : public MemoryBlock
		{
		public:
			UnknownBlock(int type)
				: MemoryBlock(type)
			{
			}

			std::shared_ptr<Linker::Writable> image;

			uint16_t GetLength() const override;
			void ReadFile(Linker::Reader& rd, uint16_t length) override;
			void WriteFile(Linker::Writer& wr) const override;
			std::unique_ptr<Dumper::Region> MakeRegion(std::string name, offset_t offset, unsigned display_width) const override;
		};

		uint16_t file_header_size = 0x40;
		std::array<char, 6> linker_version;
		enum file_type : uint8_t
		{
			TYPE_CMD = 1,
			TYPE_SAV = 9,
		};
		file_type type = TYPE_CMD;
		uint32_t entry_point;
		uint16_t stack_size;
		uint16_t allocation_length;
		std::vector<std::unique_ptr<MemoryBlock>> blocks;

		CMDFormat(int type = TYPE_CMD)
			: type(file_type(type))
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		void CalculateValues() override;
	};
}

#endif /* PCOS_H */
