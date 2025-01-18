#ifndef O65_H
#define O65_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writable.h"
#include "../linker/writer.h"

/* o65 object format (input only) */
namespace O65
{
	/**
	 * @brief Output format for the 6502 assembler xa
	 *
	 * Documented here: http://www.6502.org/users/andre/o65/fileformat.html
	 */
	class O65Format : public virtual Linker::InputFormat
	{
	public:
		/** @brief A object file typically contains a single module */
		class Module
		{
		public:
			/** @brief Additional entries in the header */
			struct header_option
			{
				/** @brief Filename of object file */
				static const int TYPE_FILENAME = 0x00;
				/** @brief OS type */
				static const int TYPE_SYSTEM = 0x01;
				/** @brief Name of assembler that created this file */
				static const int TYPE_ASSEMBLER = 0x02;
				/** @brief Name of author */
				static const int TYPE_AUTHOR = 0x03;
				/** @brief Creation date in a human readable format */
				static const int TYPE_CREATION = 0x04;

				/** @brief Type of header entry */
				uint8_t type;
				/** @brief Header entry contents */
				std::vector<uint8_t> data;

				header_option(uint8_t type)
					: type(type), data()
				{
				}

				header_option(uint8_t type, std::vector<uint8_t>& data)
					: type(type), data(data)
				{
				}
			};

			/** @brief Relocation entries */
			struct relocation
			{
				/** @brief The bitmask in the relocation type corresponding to the relocation type */
				static const int RELOC_TYPE_MASK = 0xE0;
				/** @brief Low 8 bits of a value */
				static const int RELOC_TYPE_LOW = 0x20;
				/** @brief High 8 bits (bits 8 to 15) of a value */
				static const int RELOC_TYPE_HIGH = 0x40;
				/** @brief 16-bit value */
				static const int RELOC_TYPE_WORD = 0x80;
				/** @brief High 8 bits (bits 16 to 23) of a far address */
				static const int RELOC_TYPE_SEG = 0xA0;
				/** @brief 24-bit address */
				static const int RELOC_TYPE_SEGADDR = 0xC0;

				/** @brief The bitmask in the relocation type corresponding to the segment number */
				static const int RELOC_SEGMENT_MASK = 0x1F;
				/** @brief Reference does not belong to a segment, instead it is an external reference */
				static const int RELOC_SEGMENT_UNDEFINED = 0x00;
				/** @brief Reference is a global value */
				static const int RELOC_SEGMENT_ABSOLUTE = 0x01;
				/** @brief Reference belongs to the text segment */
				static const int RELOC_SEGMENT_TEXT = 0x02;
				/** @brief Reference belongs to the data segment */
				static const int RELOC_SEGMENT_DATA = 0x03;
				/** @brief Reference belongs to the bss segment */
				static const int RELOC_SEGMENT_BSS = 0x04;
				/** @brief Reference belongs to the zero segment (6502/65c816 specific) */
				static const int RELOC_SEGMENT_ZERO = 0x05;

				/** @brief The type (bits 4 to 7) and segment (bits 0 to 3) of the relocation */
				uint8_t type_segment;
				/** @brief (optional) For undefined references (RELOC_SEGMENT_UNDEFINED), the index of the external symbol in the undefined_symbols */
				offset_t symbol_index;
				/** @brief (optional) The unrelocated parts of the reference */
				offset_t value;

				relocation()
					: type_segment(0), symbol_index(0), value(0)
				{
				}

				relocation(uint8_t type_segment, offset_t value = 0, offset_t symbol_index = 0)
					: type_segment(type_segment), symbol_index(symbol_index), value(value)
				{
				}
			};

			struct exported_global
			{
				/** @brief Name of the exported symbol */
				std::string name;
				/** @brief Segment identifier */
				uint8_t segment_id;
				/** @brief Symbol value */
				offset_t value;

				exported_global(std::string_view name, uint8_t segment_id, offset_t value)
					: name(name), segment_id(segment_id), value(value)
				{
				}
			};

			/** @brief Bits 0 to 1 in mode_word describing the file alignment */
			static const int MODE_ALIGN_MASK = 0x0003;
			/** @brief File alignment is byte */
			static const int MODE_ALIGN_BYTE = 0x0000;
			/** @brief File alignment is 2 byte word */
			static const int MODE_ALIGN_WORD = 0x0001;
			/** @brief File alignment is 4 byte long word */
			static const int MODE_ALIGN_LONG = 0x0002;
			/** @brief File alignment is 256 byte page/block */
			static const int MODE_ALIGN_PAGE = 0x0003;

			/** @brief Bits 4 to 7 in mode_word describing CPU instruction set */
			static const int MODE_CPU_MASK = 0x00F0;
			static const int MODE_CPU_6502 = 0x0000;
			static const int MODE_CPU_65C02 = 0x0010;
			static const int MODE_CPU_65SC02 = 0x0020;
			static const int MODE_CPU_65CE02 = 0x0030;
			static const int MODE_CPU_NMOS = 0x0040;
			static const int MODE_CPU_65816 = 0x0050;

			/** @brief Set if bss should be cleared on load */
			static const int MODE_BSS_ZERO = 0x0200;
			/** @brief Set if another module follows */
			static const int MODE_CHAIN = 0x0400;
			/** @brief Set if code, data and bss are contiguous */
			static const int MODE_SIMPLE = 0x0800;
			/** @brief Set for non-executable object modules */
			static const int MODE_OBJ = 0x1000;
			/** @brief Set if address/offset fields are 32-bit wide instead of 16-bit */
			static const int MODE_SIZE = 0x2000;
			/** @brief Set if file is page relocatable (256 bytes) instead of byte relocatable */
			static const int MODE_PAGE_RELOC = 0x4000;
			/** @brief Set for 65816 mode */
			static const int MODE_65816 = 0x8000;

			/** @brief The file mode */
			uint16_t mode_word = 0;

			/** @brief Base address of code segment */
			offset_t code_base = 0;
			/** @brief Code segment contents */
			std::shared_ptr<Linker::Writable> code_image = nullptr;
			/** @brief Base address of data segment */
			offset_t data_base = 0;
			/** @brief Data segment contents */
			std::shared_ptr<Linker::Writable> data_image = nullptr;
			/** @brief Base address of bss segment */
			offset_t bss_base = 0;
			/** @brief Bss segment size */
			offset_t bss_size = 0;
			/** @brief Base address of zero segment */
			offset_t zero_base = 0;
			/** @brief Zero segment size */
			offset_t zero_size = 0;
			/** @brief Stack segment size */
			offset_t stack_size = 0;

			/** @brief Additional header field entries */
			std::vector<header_option> header_options;
			/** @brief Undefined references appearing in file */
			std::vector<std::string> undefined_references;

			/** @brief Relocations within code segment */
			std::map<offset_t, relocation> code_relocations;
			/** @brief Relocations within data segment */
			std::map<offset_t, relocation> data_relocations;

			/** @brief Exported global symbols */
			std::vector<exported_global> exported_globals;

			~Module()
			{
				Clear();
			}

			void Clear();

			/** @brief Whether file can only be relocated according to a 256 byte page (MODE_PAGE_RELOC is set in mode_word) */
			bool IsPageRelocatable() const;
			/** @brief Whether another module follows this one (MODE_CHAIN is set in mode_word */
			bool IsChained() const;
		private:
			/** @brief Signals that another module follows this one (sets MODE_CHAIN in mode_word) */
			void SetChained();
			friend class O65Format;
		public:
			/** @brief Return the size of address/offset values in the file, in bytes (2 or 4) */
			int GetWordSize() const;
			/** @brief Reads a module dependent unsigned word, the size of which is given by GetWordSize() */
			offset_t ReadUnsigned(Linker::Reader& rd) const;
			/** @brief Writes a module dependent word, the size of which is given by GetWordSize() */
			void WriteWord(Linker::Writer& wr, offset_t value) const;

			void ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr);
			void GenerateModule(Linker::Module& module, Linker::Reader& rd) const;
		};

	protected:
		std::vector<std::unique_ptr<Module>> modules;

	public:
		offset_t GetModuleCount();
		std::unique_ptr<O65Format::Module>& GetModule(offset_t index);
		std::unique_ptr<O65Format::Module>& AddModule();

		~O65Format()
		{
			Clear();
		}

		void Clear() override;

		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
	};
}

#endif /* O65_H */
