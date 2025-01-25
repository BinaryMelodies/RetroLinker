#ifndef AS86OBJ_H
#define AS86OBJ_H

#include <array>
#include "../common.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* as86 object format (input only) */
namespace AS86Obj
{
	/**
	 * @brief Output format for as86, used as an output by Bruce's C compiler from the dev86 package
	 */
	class AS86ObjFormat : public virtual Linker::InputFormat
	{
	public:
		class Symbol
		{
		public:
			uint16_t name_offset = 0;
			std::string name;
			uint16_t symbol_type = 0;
			int offset_size = 0;
			uint32_t offset = 0;
		};

		class ByteCode
		{
		public:
			virtual ~ByteCode();
			static std::unique_ptr<ByteCode> ReadFile(Linker::Reader& rd, int& relocation_size);
		};

		class RelocatorSize : public ByteCode
		{
		public:
			uint8_t size;
			RelocatorSize(uint8_t size)
				: size(size > 3 ? 3 : size)
			{
			}
		};

		class SkipBytes : public ByteCode
		{
		public:
			uint8_t count;
			SkipBytes(uint8_t count)
				: count(count)
			{
			}
		};

		class ChangeSegment : public ByteCode
		{
		public:
			uint8_t segment;
			ChangeSegment(uint8_t segment)
				: segment(segment)
			{
			}
		};

		class RawBytes : public ByteCode
		{
		public:
			std::unique_ptr<Linker::Buffer> buffer;
		};

		class SimpleRelocator : public ByteCode
		{
		public:
			uint32_t offset;
			uint8_t segment;
			bool ip_relative;
			SimpleRelocator(uint8_t type, uint32_t offset)
				: offset(offset), segment(type & 0xF), ip_relative((type & 0x20) != 0)
			{
			}
		};

		class SymbolRelocator : public ByteCode
		{
		public:
			uint32_t offset;
			uint16_t symbol_index;
			bool ip_relative;
			SymbolRelocator(uint8_t type, uint32_t offset, uint16_t symbol_index)
				: offset(offset), symbol_index(symbol_index), ip_relative((type & 0x20) != 0)
			{
			}
		};

		class Module
		{
		public:
			offset_t file_offset = 0;
			uint32_t code_offset = 0;
			uint32_t image_size = 0;
			uint16_t string_table_size = 0;
			struct version
			{
				uint8_t major, minor;
			};
			version module_version = { };
			uint32_t maximum_segment_size = 0;
			struct segment_size_list
			{
				uint32_t word = 0;
				struct offset
				{
					uint32_t& word;
					int index;

					operator int() const;
					offset& operator =(int value);
				};
				int operator[](int index) const;
				offset operator[](int index);
				segment_size_list& operator =(uint32_t value);
			};
			segment_size_list segment_sizes_word;
			typedef uint32_t Segment;
			std::array<Segment, 16> segment_sizes = { };
			std::vector<Symbol> symbols;
			offset_t string_table_offset = 0;
			std::string module_name;
			std::vector<std::unique_ptr<ByteCode>> data;
		};

		std::vector<Module> modules;

		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		/* TODO */
	};
}

#endif /* AS86OBJ_H */
