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

		/** @brief Represents a block of data in the file, also an end-of-block object is an instance of this type */
		class MemoryBlock
		{
		public:
			/** @brief Every block has a type field */
			enum block_type : uint8_t
			{
				/** @brief A block of data to be loaded into memory */
				TYPE_LOAD = 3,
				/** @brief A sequence of fixups for 16-bit memory offsets */
				TYPE_OFFSET_RELOCATION = 4,
				/** @brief A sequence of fixups for 16-bit memory segments */
				TYPE_SEGMENT_RELOCATION = 5,
				/** @brief Terminating block, must be the final one */
				TYPE_END = 6,
			};
			/** @brief The type of the block */
			block_type type;

			virtual ~MemoryBlock();

			/** @brief The length of the block, not including the type and length fields */
			virtual uint16_t GetLength() const;
			/** @brief Fills the contents of this object
			 *
			 * @param length The number of bytes in the block, not including the type and length fields
			 */
			virtual void ReadFile(Linker::Reader& rd, uint16_t length);
			/** @brief Writes the contents of the block to the file, including the type and length fields */
			virtual void WriteFile(Linker::Writer& wr) const;
			/** @brief Creates a region for displaying the block contents */
			virtual std::unique_ptr<Dumper::Region> MakeRegion(std::string name, offset_t offset, unsigned display_width) const;
			/** @brief Adds block specific fields */
			virtual void AddFields(Dumper::Region& region, CMDFormat& module) const;
			/** @brief Display block specific contents */
			virtual void DumpContents(Dumper::Dumper& dump, offset_t file_offset) const;
			/** @brief Displays the entire block */
			void Dump(Dumper::Dumper& dump, offset_t file_offset, CMDFormat& module) const;

			/** @brief Parses a block, including the type and length fields */
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

			/** @brief The first 4 bytes of the block */
			uint32_t block_id;
			/** @brief The memory resident part of the block */
			std::shared_ptr<Linker::Writable> image;

			uint16_t GetLength() const override;
			void ReadFile(Linker::Reader& rd, uint16_t length) override;
			void WriteFile(Linker::Writer& wr) const override;
			std::unique_ptr<Dumper::Region> MakeRegion(std::string name, offset_t offset, unsigned display_width) const override;
			void AddFields(Dumper::Region& region, CMDFormat& module) const override;
		};

		/** @brief A block containing a sequence of relocations between two blocks, either the offset or segment parts */
		class RelocationBlock : public MemoryBlock
		{
		public:
			/** @brief The block where these relocations must be applied */
			uint8_t source = 0;
			/** @brief The block referenced */
			uint8_t target = 0;
			/** @brief Sequence of offsets to 16-bit words that must be relocated */
			std::vector<uint16_t> offsets;

			RelocationBlock(int type)
				: MemoryBlock(type)
			{
			}

			uint16_t GetLength() const override;
			void ReadFile(Linker::Reader& rd, uint16_t length) override;
			void WriteFile(Linker::Writer& wr) const override;
			void AddFields(Dumper::Region& region, CMDFormat& module) const override;
			void DumpContents(Dumper::Dumper& dump, offset_t file_offset) const override;
		};

		/** @brief Represents the contents of a block whose format is not known or implemented */
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

		/** @brief Size of header, following the first 3 bytes */
		uint16_t file_header_size = 0x40;
		/** @brief String field following the first 7 bytes */
		std::array<char, 6> linker_version;
		/** @brief Executable types */
		enum file_type : uint8_t
		{
			/** @brief Executable which gets unloaded after execution */
			TYPE_CMD = 1,
			/** @brief Executable that is kept resident in memory */
			TYPE_SAV = 9,
		};
		/** @brief Type of executable */
		file_type type = TYPE_CMD;
		/** @brief 24-bit entry point, memory block and offset */
		uint32_t entry_point;
		/** @brief Size of stack */
		uint16_t stack_size;
		// TODO: exact meaning
		uint16_t allocation_length;
		/** @brief Sequence of blocks to be loaded */
		std::vector<std::unique_ptr<MemoryBlock>> blocks;

		CMDFormat(int type = TYPE_CMD)
			: type(file_type(type))
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		offset_t WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		void CalculateValues() override;
	};
}

#endif /* PCOS_H */
