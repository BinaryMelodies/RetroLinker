#ifndef AS86OBJ_H
#define AS86OBJ_H

#include <array>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/format.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: not fully implemented */

namespace AS86Obj
{
	/**
	 * @brief Object format of Introl-C, output format for as86, used as an output by Bruce's C compiler from the dev86 package
	 */
	class AS86ObjFormat : public virtual Linker::InputFormat
	{
	public:
		static inline constexpr int GetSize(int size) { return size < 3 ? size : 4; }

		class Symbol
		{
		public:
			static constexpr uint16_t Absolute = 0x0010;
			static constexpr uint16_t Relative = 0x0020;
			static constexpr uint16_t Imported = 0x0040;
			static constexpr uint16_t Exported = 0x0080;
			static constexpr uint16_t Entry = 0x0100;
			static constexpr uint16_t Common = 0x2000;

			uint16_t name_offset = 0;
			std::string name;
			uint16_t symbol_type = 0;
			uint8_t segment = 0;
			int offset_size = 0;
			uint32_t offset = 0;
			offset_t symbol_definition_offset = 0; // convenience entry
		};

		class ByteCode
		{
		public:
			virtual ~ByteCode();
			virtual offset_t GetLength() const;
			virtual offset_t GetMemorySize() const = 0;
			virtual void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const = 0;
			virtual void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const;
			static std::unique_ptr<ByteCode> ReadFile(Linker::Reader& rd, int& relocation_size);
		};

		class RelocatorSize : public ByteCode
		{
		public:
			uint8_t size;
			RelocatorSize(uint8_t size)
				: size(GetSize(size > 3 ? 3 : size))
			{
			}

			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
		};

		class SkipBytes : public ByteCode
		{
		public:
			uint8_t count;
			SkipBytes(uint8_t count)
				: count(count)
			{
			}

			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
			void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const override;
		};

		class ChangeSegment : public ByteCode
		{
		public:
			uint8_t segment;
			ChangeSegment(uint8_t segment)
				: segment(segment)
			{
			}

			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
			void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const override;
		};

		class RawBytes : public ByteCode
		{
		public:
			std::shared_ptr<Linker::Buffer> buffer;

			offset_t GetLength() const override;
			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
			void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const override;
		};

		class SimpleRelocator : public ByteCode
		{
		public:
			uint32_t offset;
			uint8_t segment;
			bool ip_relative;
			int relocation_size;
			SimpleRelocator(uint8_t type, uint32_t offset, int relocation_size)
				: offset(offset), segment(type & 0xF), ip_relative((type & 0x20) != 0), relocation_size(GetSize(relocation_size))
			{
			}

			offset_t GetLength() const override;
			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
			void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const override;
		};

		class SymbolRelocator : public ByteCode
		{
		public:
			uint32_t offset;
			uint16_t symbol_index;
			std::string symbol_name;
			bool ip_relative;
			int relocation_size;
			int offset_size;
			int index_size;
			SymbolRelocator(uint8_t type, uint32_t offset, uint16_t symbol_index, int relocation_size, int offset_size, int index_size)
				: offset(offset), symbol_index(symbol_index), ip_relative((type & 0x20) != 0), relocation_size(GetSize(relocation_size)), offset_size(GetSize(offset_size)), index_size(GetSize(index_size))
			{
			}

			offset_t GetLength() const override;
			offset_t GetMemorySize() const override;
			void Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const override;
			void Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const override;
		};

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

		class Module
		{
		public:
			offset_t file_offset = 0;
			offset_t module_size = 0;
			uint32_t code_offset = 0;
			uint32_t image_size = 0;
			uint16_t string_table_size = 0;
			struct version
			{
				uint8_t major, minor;
			};
			version module_version = { };
			uint32_t maximum_segment_size = 0;
			segment_size_list segment_sizes_word;
			std::array<uint32_t, 16> segment_sizes = { };
			std::vector<Symbol> symbols;
			offset_t string_table_offset = 0;
			std::string module_name;
			std::vector<std::unique_ptr<ByteCode>> data;

			void Dump(Dumper::Dumper& dump, unsigned index);
		};

		offset_t file_size = offset_t(-1);

		enum cpu_type
		{
			/* represented here in big-endian format */
			CPU_I8086 = 0xA086,
			CPU_I80386 = 0xA386,
			CPU_MC6809 = 0x5331, /* S1 */
		};
		cpu_type cpu = cpu_type(0);

		std::vector<Module> modules;

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

	protected:
		static std::string GetDefaultSectionName(unsigned index);
		static std::shared_ptr<Linker::Section> GetDefaultSection(unsigned index);

	public:
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
	};
}

#endif /* AS86OBJ_H */
