#ifndef PEFEXE_H
#define PEFEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace Apple
{
	/**
	 * @brief PowerPC Classic Mac OS "PEF" file format
	 */
	class PEFFormat : public virtual Linker::SegmentManager
	{
	public:
		// TODO: untested
		class PatternInitialization
		{
		public:
			enum opcode_type
			{
				Zero = 0,
				BlockCopy = 1,
				RepeatedBlock = 2,
				InterleaveRepeatBlockWithBlockCopy = 3,
				InterleaveRepeatBlockWithZero = 4,
			};
			offset_t file_offset;
			opcode_type opcode;
			uint32_t count;
			std::vector<uint8_t> common_data;
			std::vector<std::vector<uint8_t>> custom_data;

			static uint32_t ReadValue(Linker::Reader& rd);
			static size_t GetValueSize(uint32_t value, size_t size_hint = 0);
			static void WriteValue(Linker::Writer& wr, uint32_t value, size_t size_hint = 0);

			void ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr) const;
			offset_t CodeSize() const;
			offset_t DataSize() const;
			void ExpandData(Linker::Buffer& buffer) const;
		};

		class Relocation
		{
		public:
			// TODO
		};

		class Section
		{
		public:
			enum section_type
			{
				Code = 0,
				UnpackedData = 1,
				PatternInitializedData = 2,
				Constant = 3,
				Loader = 4,
				Debug = 5,
				ExecutableData = 6,
				Exception = 7,
				Traceback = 8,
			};
			enum share_type
			{
				ProcessShare = 1,
				GlobalShare = 4,
				ProtectedShare = 5,
			};
			static constexpr uint32_t NoNameOffset = uint32_t(-1);
			uint32_t name_offset = NoNameOffset;
			std::string name = "";
			uint32_t default_address = 0;
			uint32_t total_size = 0;
			uint32_t unpacked_size = 0;
			uint32_t packed_size = 0;
			uint32_t container_offset = 0;
			section_type section_kind = Code;
			share_type share_kind = ProcessShare;
			uint8_t alignment = 0;
			uint8_t reserved = 0;

			std::shared_ptr<Linker::Contents> image;
			std::vector<PatternInitialization> patterns;
			std::vector<Relocation> relocations;

			bool IsInstantiated() const
			{
				switch(section_kind)
				{
				case Code:
				case UnpackedData:
				case PatternInitializedData:
				case Constant:
				case ExecutableData:
					return true;
				default:
					return false;
				}
			}

			offset_t ExpectedAlignment() const
			{
				switch(section_kind)
				{
				case Code:
				case UnpackedData:
				case Constant:
				case ExecutableData:
					return 16;
				case PatternInitializedData:
				case Loader:
					return 4;
				default:
					return 1;
				}
			}

			void ReadHeader(Linker::Reader& rd);
			void ReadFile(PEFFormat& pef_format, Linker::Reader& rd);
			size_t GetImageSize(PEFFormat& pef_format);
			void CalculateValues(PEFFormat& pef_format);
			void WriteHeader(Linker::Writer& wr) const;
			void WriteFile(const PEFFormat& pef_format, Linker::Writer& wr) const;
		};

		// container header information

		// values are stored as the bigendian 32-bit word
		enum cpu_type
		{
			M68K = 0x6D36386B, // 'm68k'
			PPC  = 0x70777063, // 'pwpc'
		};
		cpu_type architecture = PPC;
		uint32_t format_version = 1;
		uint32_t date_time_stamp = 0;
		uint32_t old_def_version = 0;
		uint32_t old_imp_version = 0;
		uint32_t current_version = 0;
		uint32_t reserved = 0;
		uint16_t inst_section_count = 0;
		std::vector<std::shared_ptr<Section>> sections;
		std::vector<std::string> section_name_table;
		uint32_t section_name_table_end = 0;

		static constexpr uint32_t ContainerHeaderSize = 40;
		static constexpr uint32_t SectionHeaderSize = 28;

		uint32_t GetSectionNameTableOffset() const
		{
			return ContainerHeaderSize + SectionHeaderSize * sections.size();
		}

		// loader section information
		struct SymbolReference
		{
			static constexpr uint32_t NoSection = uint32_t(-1);
			uint32_t section = NoSection;
			uint32_t name_offset = 0;
			std::string name;
		};
		SymbolReference main_symbol, init_symbol, term_symbol;

		class ImportedSymbol
		{
		public:
			enum class_type
			{
				Code,
				Data,
				TVect,
				TOC,
				Glue,
			};
			class_type symbol_class;
			uint8_t flags;
			uint32_t name_offset;
			std::string name;
		};

		class ImportedLibrary
		{
		public:
			uint32_t name_offset;
			std::string name;
			uint32_t old_imp_version;
			uint32_t current_version;
			uint32_t imported_symbol_count;
			uint32_t first_imported_symbol;
			std::vector<ImportedSymbol> imported_symbols;
			uint8_t options;
			uint8_t reserved_a;
			uint16_t reserved_b;
		};
		std::vector<ImportedLibrary> imported_libraries;
		std::vector<ImportedSymbol> imported_symbols;
		std::vector<uint32_t> reloc_section_indexes;
		uint32_t reloc_instr_offset = 0;
		uint32_t loader_strings_offset = 0;
		uint32_t export_hash_offset = 0;
		uint32_t export_hash_table_power = 0;

		class ExportedSymbol
		{
			// TODO
		};
		std::vector<ExportedSymbol> exported_symbols;

		static constexpr uint32_t LoaderHeaderSize = 56;
		uint32_t GetLibraryDescriptionsSize() const
		{
			return 24 * imported_libraries.size();
		}
		uint32_t GetSymbolTablesSize() const
		{
			uint32_t size = 0;
			for(auto& library : imported_libraries)
			{
				size += 4 * library.imported_symbols.size();
			}
			return size;
		}
		uint32_t GetRelocationHeadersSize() const
		{
			return 12 * reloc_section_indexes.size();
		}
		uint32_t GetRelocationAreaSize() const
		{
			// TODO
			return 0;
		}
		uint32_t GetLoaderStringAreaSize() const
		{
			// TODO
			return 0;
		}
		uint32_t GetExportHashTableSize() const
		{
			// TODO
			return 0;
		}
		uint32_t GetExportKeyTableSize() const
		{
			// TODO
			return 0;
		}
		uint32_t GetExportSymbolTableSize() const
		{
			// TODO
			return 0;
		}
		uint32_t GetLoaderSectionSize() const
		{
			return LoaderHeaderSize
				+ GetLibraryDescriptionsSize()
				+ GetSymbolTablesSize()
				+ GetRelocationHeadersSize()
				+ GetRelocationAreaSize()
				+ GetLoaderStringAreaSize()
				+ GetExportHashTableSize()
				+ GetExportKeyTableSize()
				+ GetExportSymbolTableSize();
		}

		void ReadLoaderSection(Linker::Reader& rd);
		void WriteLoaderSection(Linker::Writer& wr) const;

		void ReadFile(Linker::Reader& rd) override;
		void CalculateValues() override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */
	};
}

#endif /* PEFEXE_H */
