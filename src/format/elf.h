#ifndef ELF_H
#define ELF_H

#include <sstream>
#include <vector>
#include "../common.h"
#include "../linker/format.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/reader.h"

namespace ELF
{
	/**
	 * @brief ELF object and executable format
	 *
	 * The latest and most widespread file format, developed for the UNIX operating system.
	 */
	class ELFFormat : public virtual Linker::InputFormat, public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void WriteFile(Linker::Writer& wr) override;

		Linker::OutputFormat * output_format;
		void SetupOptions(char special_char, Linker::OutputFormat * format) override;

		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;

		EndianType endiantype;
		Linker::Module * module;

		ELFFormat()
			: endiantype(::LittleEndian), module(nullptr), special_prefix_char('$'),
				option_segmentation(false), option_16bit(true), option_stack_section(false), option_heap_section(false), option_resources(false), option_libraries(false)
		{
		}

		ELFFormat(Linker::Module& module)
			: endiantype(::LittleEndian), module(&module), special_prefix_char('$'),
				option_segmentation(false), option_16bit(true), option_stack_section(false), option_heap_section(false), option_resources(false), option_libraries(false)
		{
		}

		char special_prefix_char;
			/* GNU assembler can use '$', NASM must use '?' */
		bool option_segmentation;
		bool option_16bit;
		bool option_linear;
		bool option_stack_section;
		bool option_heap_section;
		bool option_resources;
		bool option_libraries;
		size_t wordbytes;
		enum cpu_type
		{
			I386,
			M68K,
			ARM, /* TODO */
		} cpu;

		class Symbol
		{
		public:
			uint32_t name_offset;
			std::string name;
			offset_t value, size;
			uint8_t bind, type, other;
			uint16_t shndx;
			uint16_t sh_link;
			bool defined;
			bool unallocated;
			Linker::Location location;
			Linker::CommonSymbol specification;
		};

		class Section
		{
		public:
			uint32_t name_offset;
			std::string name;
			uint32_t type, link, info;
			offset_t flags;
			offset_t address, file_offset, size, align, entsize;
			Linker::Section * section;
			std::vector<Symbol> symbols;
		};
		std::vector<Section> sections;

		class Relocation
		{
		public:
			offset_t offset;
			uint32_t type;
			uint32_t symbol;
			int64_t addend;
			uint16_t sh_link, sh_info;
			bool addend_from_section_data;
		};
		std::vector<Relocation> relocations;

	private:
		/* symbols */
		std::string segment_prefix();
		std::string segment_of_prefix();
		std::string segment_at_prefix();
		std::string with_respect_to_segment_prefix();
		std::string segment_difference_prefix();
		std::string import_prefix();
		std::string segment_of_import_prefix();
		std::string export_prefix();

		/* sections */
		std::string resource_prefix();

	public:
		static const uint8_t ELFCLASS32 = 1;
		static const uint8_t ELFCLASS64 = 2;
		static const uint8_t ELFDATA2LSB = 1;
		static const uint8_t ELFDATA2MSB = 2;

		static const uint16_t EM_386 = 3;
		static const uint16_t EM_68K = 4;
		static const uint16_t EM_ARM = 40;

		static const uint32_t SHT_PROGBITS = 1;
		static const uint32_t SHT_SYMTAB = 2;
		static const uint32_t SHT_STRTAB = 3;
		static const uint32_t SHT_RELA = 4;
		static const uint32_t SHT_NOBITS = 8;
		static const uint32_t SHT_REL = 9;
		static const uint32_t SHT_GROUP = 10;

		static const offset_t SHF_WRITE = 0x0001;
		static const offset_t SHF_ALLOC = 0x0002;
		static const offset_t SHF_EXECINSTR = 0x0004;
		static const offset_t SHF_MERGE = 0x0010;
		static const offset_t SHF_GROUP = 0x0200;

		static const uint16_t SHN_UNDEF = 0;
		static const uint16_t SHN_ABS = 0xFFF1;
		static const uint16_t SHN_COMMON = 0xFFF2;
		static const uint16_t SHN_XINDEX = 0xFFFF;

		static const uint8_t STB_LOCAL = 0;
		static const uint8_t STB_GLOBAL = 1;

		static const offset_t R_386_8 = 22;
		static const offset_t R_386_PC8 = 23;
		static const offset_t R_386_16 = 20;
		static const offset_t R_386_PC16 = 21;
		static const offset_t R_386_32 = 1;
		static const offset_t R_386_PC32 = 2;
		/* extensions, see https://github.com/tkchia/build-ia16/blob/master/elf16-writeup.md (TODO: unsupported for now) */
		static const offset_t R_386_SEG16 = 45;
		static const offset_t R_386_SUB16 = 46;
		static const offset_t R_386_SUB32 = 47;
		static const offset_t R_386_SEGRELATIVE = 48;
		static const offset_t R_386_OZSEG16 = 80;
		static const offset_t R_386_OZRELSEG16 = 81;

		static const offset_t R_68K_8 = 3;
		static const offset_t R_68K_PC8 = 6;
		static const offset_t R_68K_16 = 2;
		static const offset_t R_68K_PC16 = 5;
		static const offset_t R_68K_32 = 1;
		static const offset_t R_68K_PC32 = 4;

		static const offset_t R_ARM_ABS8 = 8;
		static const offset_t R_ARM_ABS16 = 16;
		static const offset_t R_ARM_ABS32 = 2;
		static const offset_t R_ARM_REL32 = 3;
		static const offset_t R_ARM_CALL = 28;
		static const offset_t R_ARM_JUMP24 = 29;
		static const offset_t R_ARM_PC24 = 1;
		static const offset_t R_ARM_V4BX = 40;

		bool parse_imported_name(std::string reference_name, Linker::SymbolName& symbol);

		bool parse_exported_name(std::string reference_name, Linker::ExportedSymbol& symbol);

		void ReadFile(Linker::Reader& in) override;
	};

}

#endif /* ELF_H */
