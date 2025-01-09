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

		void SetupOptions(std::shared_ptr<Linker::OutputFormat> format) override;

	private:
		void GenerateModule(Linker::Module& module) const;

	public:
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;

		EndianType endiantype = ::LittleEndian;

		ELFFormat()
		{
		}

		bool option_16bit = true;
		bool option_linear = false;

		size_t wordbytes;

		enum cpu_type
		{
			EM_386 = 3,
			EM_68K = 4,
			EM_ARM = 40,
		};
		cpu_type cpu;

		uint8_t header_version;
		uint8_t osabi;
		uint8_t abi_version;

		enum file_type
		{
			ET_NONE = 0,
			ET_REL = 1,
			ET_EXEC = 2,
			ET_DYN = 3,
			ET_CORE = 4,
		};
		file_type object_file_type;
		uint16_t file_version;

		offset_t entry;
		offset_t program_header_offset;
		offset_t section_header_offset;
		uint32_t flags;
		uint16_t elf_header_size;
		uint16_t program_header_entry_size;
		uint16_t section_header_entry_size;
		uint16_t section_name_string_table;

		class Segment
		{
		public:
			uint32_t type = 0, flags = 0;
			offset_t offset = 0, vaddr = 0, paddr = 0, filesz = 0, memsz = 0, align = 0;

			class Block
			{
			public:
				virtual ~Block();
				virtual offset_t GetSize() const = 0;
			};

			class Section : public Block
			{
			public:
				uint16_t index;
				uint32_t offset, size;
				Section(uint16_t index, uint32_t offset, uint32_t size)
					: index(index), offset(offset), size(size)
				{
				}
				offset_t GetSize() const override;
			};

			class Data : public Block
			{
			public:
				std::shared_ptr<Linker::Writable> image;
				offset_t GetSize() const override;
			};

			std::vector<std::shared_ptr<Block>> blocks;
		};
		std::vector<Segment> segments;

		class Symbol
		{
		public:
			uint32_t name_offset = 0;
			std::string name;
			offset_t value = 0, size = 0;
			uint8_t bind = 0, type = 0, other = 0;
			uint16_t shndx = 0;
			uint16_t sh_link = 0;
			bool defined = false;
			bool unallocated = false;
			Linker::Location location;
			Linker::CommonSymbol specification;
		};

		class Section
		{
		public:
			uint32_t name_offset = 0;
			std::string name;
			enum section_type
			{
				SHT_NULL = 0,
				SHT_PROGBITS = 1,
				SHT_SYMTAB = 2,
				SHT_STRTAB = 3,
				SHT_RELA = 4,
				SHT_HASH = 5,
				SHT_DYNAMIC = 6,
				SHT_NOTE = 7,
				SHT_NOBITS = 8,
				SHT_REL = 9,
				SHT_SHLIB = 10, /* reserved */
				SHT_DYNSYM = 11,
				SHT_INIT_ARRAY = 14,
				SHT_FINI_ARRAY = 15,
				SHT_PREINIT_ARRAY = 16,
				SHT_GROUP = 17,
				SHT_SYMTAB_SHNDX = 18,
				/* IBM OS/2 specific */
				SHT_OS = 0x60000001,
				SHT_IMPORTS = 0x60000002,
				SHT_EXPORTS = 0x60000003,
				SHT_RES = 0x60000004,
				/* GNU extensions */
				SHT_GNU_INCREMENTAL_INPUTS = 0x6FFF4700,
				SHT_GNU_ATTRIBUTES = 0x6FFFFFF5,
				SHT_GNU_HASH = 0x6FFFFFF6,
				SHT_GNU_LIBLIST = 0x6FFFFFF7,
				SHT_SUNW_verdef = 0x6FFFFFFD,
				SHT_SUNW_verneed = 0x6FFFFFFE,
				SHT_SUNW_versym = 0x6FFFFFFF,
			};
			section_type type = SHT_NULL;
			uint32_t link = 0, info = 0;
			offset_t flags = 0;
			offset_t address = 0, file_offset = 0, size = 0, align = 0, entsize = 0;
			std::shared_ptr<Linker::Section> section;
			std::vector<Symbol> symbols;
		};
		std::vector<Section> sections;

		class Relocation
		{
		public:
			offset_t offset = 0;
			uint32_t type = 0;
			uint32_t symbol = 0;
			int64_t addend = 0;
			uint16_t sh_link = 0, sh_info = 0;
			bool addend_from_section_data = false;
		};
		std::vector<Relocation> relocations;

		static constexpr uint8_t ELFCLASS32 = 1;
		static constexpr uint8_t ELFCLASS64 = 2;

		static constexpr uint8_t ELFDATA2LSB = 1;
		static constexpr uint8_t ELFDATA2MSB = 2;

		static constexpr uint8_t EV_NONE = 0;
		static constexpr uint8_t EV_CURRENT = 1;

		static constexpr uint8_t ELFOSABI_NONE = 0;

		static constexpr offset_t SHF_WRITE = 0x0001;
		static constexpr offset_t SHF_ALLOC = 0x0002;
		static constexpr offset_t SHF_EXECINSTR = 0x0004;
		static constexpr offset_t SHF_MERGE = 0x0010;
		static constexpr offset_t SHF_STRINGS = 0x0020;
		static constexpr offset_t SHF_INFO_LINK = 0x0040;
		static constexpr offset_t SHF_LINK_ORDER = 0x0080;
		static constexpr offset_t SHF_OS_NONCONFORMING = 0x0100;
		static constexpr offset_t SHF_GROUP = 0x0200;
		static constexpr offset_t SHF_TLS = 0x0400;
		static constexpr offset_t SHF_COMPRESSED = 0x0800;
		/* GNU specific */
		static constexpr offset_t SHF_GNU_RETAIN = 0x00200000;
		static constexpr offset_t SHF_GNU_MBIND = 0x01000000;
		static constexpr offset_t SHF_EXCLUDE = 0x80000000;
		/* OS/2 specific */
		static constexpr offset_t SHF_BEGIN = 0x01000000;
		static constexpr offset_t SHF_END = 0x02000000;

		static constexpr uint16_t SHN_UNDEF = 0;
		static constexpr uint16_t SHN_ABS = 0xFFF1;
		static constexpr uint16_t SHN_COMMON = 0xFFF2;
		static constexpr uint16_t SHN_XINDEX = 0xFFFF;

		static constexpr uint8_t STB_LOCAL = 0;
		static constexpr uint8_t STB_GLOBAL = 1;

		static constexpr offset_t R_386_8 = 22;
		static constexpr offset_t R_386_PC8 = 23;
		static constexpr offset_t R_386_16 = 20;
		static constexpr offset_t R_386_PC16 = 21;
		static constexpr offset_t R_386_32 = 1;
		static constexpr offset_t R_386_PC32 = 2;
		/* extensions, see https://github.com/tkchia/build-ia16/blob/master/elf16-writeup.md (TODO: unsupported for now) */
		static constexpr offset_t R_386_SEG16 = 45;
		static constexpr offset_t R_386_SUB16 = 46;
		static constexpr offset_t R_386_SUB32 = 47;
		static constexpr offset_t R_386_SEGRELATIVE = 48;
		static constexpr offset_t R_386_OZSEG16 = 80;
		static constexpr offset_t R_386_OZRELSEG16 = 81;

		static constexpr offset_t R_68K_8 = 3;
		static constexpr offset_t R_68K_PC8 = 6;
		static constexpr offset_t R_68K_16 = 2;
		static constexpr offset_t R_68K_PC16 = 5;
		static constexpr offset_t R_68K_32 = 1;
		static constexpr offset_t R_68K_PC32 = 4;

		static constexpr offset_t R_ARM_ABS8 = 8;
		static constexpr offset_t R_ARM_ABS16 = 16;
		static constexpr offset_t R_ARM_ABS32 = 2;
		static constexpr offset_t R_ARM_REL32 = 3;
		static constexpr offset_t R_ARM_CALL = 28;
		static constexpr offset_t R_ARM_JUMP24 = 29;
		static constexpr offset_t R_ARM_PC24 = 1;
		static constexpr offset_t R_ARM_V4BX = 40;

		void ReadFile(Linker::Reader& in) override;
	};

}

#endif /* ELF_H */
