#ifndef PEFEXE_H
#define PEFEXE_H

#include "macos.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace Linker
{
	class Position;
}

namespace Apple
{
	/**
	 * @brief PowerPC Classic Mac OS "PEF" file format
	 */
	class PEFFormat : public virtual Linker::SegmentManager
	{
	public:
		// TODO: untested
		/** @brief Pattern initialization data
		 *
		 * For pattern initialized data sections (data sections that are not unpacked), they are stored in the file as
		 * a sequence of pattern initialization data. This class represents a single pattern in a stream of patterns. */
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
			/** @brief Offset at which this pattern is stored in the file (only relevant for dumping) */
			offset_t file_offset;
			/** @brief The pattern type */
			opcode_type opcode;
			/** @brief Generic count parameter, used for repeating or zero filling data */
			uint32_t count;
			/** @brief Sequence of data to be used, possibly repeatedly */
			std::vector<uint8_t> common_data;
			/** @brief Equal sized sequences of data to be inserted between repeated common data */
			std::vector<std::vector<uint8_t>> custom_data;

			/** @brief Reads a variable length value as used by some patterns */
			static uint32_t ReadValue(Linker::Reader& rd);
			/** @brief Determines the required number of bytes to store this value, with an optional minimum size */
			static size_t GetValueSize(uint32_t value, size_t size_hint = 0);
			/** @brief Writes a variable length value, with an optional minimum size */
			static void WriteValue(Linker::Writer& wr, uint32_t value, size_t size_hint = 0);

			/** @brief Reads an initialization pattern and initializes this structure */
			void ReadFile(Linker::Reader& rd);
			/** @brief Writes the initialization pattern to a stream */
			void WriteFile(Linker::Writer& wr) const;
			/** @brief Size of the packed data, as stored in the file */
			offset_t CodeSize() const;
			/** @brief Size of the unpacked data, as loaded into memory */
			offset_t DataSize() const;
			/** @brief Expand the pattern into unpacked data and append it to the buffer */
			void ExpandData(Linker::Buffer& buffer) const;
		};

		class Section;
		class ImportedSymbol;
		class ImportedLibrary;

		/** @brief Represents a single relocated 32-bit word in memory
		 *
		 * Note that PEF files do not store these directly, instead these have to be generated from the relocation data.
		 */
		class Relocation
		{
		public:
			// add 32-bit section start to offset
			enum target_type
			{
				Section,
				Symbol,
			};
			target_type type = Section;
			bool compiled = true;
			// section: section number, symbol: symbol number
			uint32_t number = 0;
			// only for section targets
			std::weak_ptr<PEFFormat::Section> section;
			// only for symbol targets
			std::weak_ptr<ImportedSymbol> symbol;

			static inline Relocation ToSection(uint32_t section_number)
			{
				Relocation relocation;
				relocation.type = Section;
				relocation.number = section_number;
				return relocation;
			}

			static inline Relocation ToSection(std::weak_ptr<PEFFormat::Section> section)
			{
				Relocation relocation;
				relocation.type = Section;
				relocation.compiled = false;
				relocation.section = section;
				return relocation;
			}

			static inline Relocation ToSymbol(uint32_t symbol_number)
			{
				Relocation relocation;
				relocation.type = Symbol;
				relocation.number = symbol_number;
				return relocation;
			}

			static inline Relocation ToSymbol(std::weak_ptr<ImportedSymbol> symbol)
			{
				Relocation relocation;
				relocation.type = Symbol;
				relocation.compiled = false;
				relocation.symbol = symbol;
				return relocation;
			}
		};

		class RelocOpcode;

		/** @brief A representation of a state machine that generates the actual relocations using the relocation data in a file
		 *
		 * PEF files contain a sequence of instructions that express state changes to be applied to an internal "pseudo-microprocessor".
		 * This class can interpret these instructions and convert them to symbol relocations. The behavior is modelled on the description
		 * in Inside Macintosh: Mac OS Runtime Architectures, except no actual memory addresses are used, since this is not intended
		 * to be used as a loader. Addresses are instead replaced by section and symbol indices.
		 */
		class RelocationProcessor
		{
		public:
			const PEFFormat& pef_format;

			size_t reloc_instr_ptr = 0;
			uint32_t reloc_address = 0;
			uint32_t import_index = 0;
			uint32_t section_c = 0;
			uint32_t section_d = 0;
			uint32_t current_repeat_count = 0;

			std::vector<RelocOpcode>& reloc_opcodes;
			std::map<uint32_t, Relocation>& relocations;

			void Initialize();

			RelocationProcessor(const PEFFormat& pef_format, std::vector<RelocOpcode>& reloc_opcodes, std::map<uint32_t, Relocation>& relocations)
				: pef_format(pef_format), reloc_opcodes(reloc_opcodes), relocations(relocations)
			{
				Initialize();
			}

			void Advance(uint32_t offset)
			{
				reloc_address += offset;
			}

			void AddRelocation(Relocation relocation)
			{
				relocations[reloc_address] = relocation;
				Advance(4);
			}

			void AddSection(uint32_t section_number)
			{
				AddRelocation(Relocation::ToSection(section_number));
			}

			void AddSectionC()
			{
				AddSection(section_c);
			}

			void AddSectionD()
			{
				AddSection(section_d);
			}

			void AddSymbol()
			{
				AddRelocation(Relocation::ToSymbol(import_index));
				import_index ++;
			}

			/** @brief Step back this amount of 16-bit halfwords in the instruction stream */
			void Regress(uint32_t block_count);

			/** @brief Repeat a previous sequence of instructions
			 *
			 * This function is intended to be called repeatedly each time the sequence of instructions
			 * is executed. The processor keeps track of how many times the repetition has taken place
			 * and stops iterating once the internal count reaches zero.
			 *
			 * @param[in] block_count The number of 16-bit halfwords consisting of the block (not counting the repetition instruction).
			 * Note that some instructions have a length of 32 bits, which means block_count has to count them as 2.
			 * @param[in] repeat_count The number of times the block needs to be repeated. This does not include the initial repetition.
			 */
			void Repeat(uint32_t block_count, uint32_t repeat_count)
			{
				// unless this is the last iteration of this block
				if(current_repeat_count != 1)
				{
					// jump over this (already processed) instruction and the block
					reloc_instr_ptr --;
					Regress(block_count);
				}

				if(current_repeat_count == 0)
				{
					// iteration starts
					current_repeat_count = repeat_count;
				}
				else
				{
					current_repeat_count --;
				}
			}

			void GenerateRelocations();
		};

		/** @brief Represents a single 16-bit or 32-bit opcode that encodes relocations */
		class RelocOpcode
		{
		public:
			/* the values are chosen so that they can be bitwise or'd to the opcode word */
			enum opcode_type
			{
				SmInvalid = -1,
				//LgInvalid = -2,
				BySectDWithSkip = 0x0000,
				BySectC = 0x4000,
				BySectD = 0x4200,
				TVector12 = 0x4400,
				TVector8 = 0x4600,
				VTable8 = 0x4800,
				ImportRun = 0x4A00,
				SmByImport = 0x6000,
				SmSetSectC = 0x6200,
				SmSetSectD = 0x6400,
				SmBySection = 0x6600,
				IncrPosition = 0x8000,
				SmRepeat = 0x9000,
				SetPosition = 0xA000,
				LgByImport = 0xA400,
				LgRepeat = 0xB000,
				LgBySection = 0xB400,
				LgSetSectC = 0xB440,
				LgSetSectD = 0xB480,
			};
			/** @brief Offset of opcode within file (only used for dumping) */
			uint32_t offset = 0; // within file
			/** @brief Opcode type */
			opcode_type opcode = SmInvalid;

			/** @brief A value parameter
			 *
			 * Depending on the type of the operation, this may encode a byte skip value (BySectDWithSkip), an offset
			 * value (IncrPosition, SetPosition), a section or symbol index (ByImport, SetSect, BySection) or
			 * the size of the block to repeat in 16-bit halfwords (Repeat) */
			uint32_t value = 0;

			/*union
			{
				uint32_t run_length; // BySectDWithSkip, BySect*, TVector*, VTable8, ImportRun
				uint32_t repeat_count; // *Repeat
			};*/
			/** @brief A repetition parameter
			 *
			 * Depending on the type of the operation, this may encode a run_length (BySectDWithSkip, BySecton, TVector, VTable8, ImportRun)
			 * or a repetition count (Repeat) */
			uint32_t repeat = 0;

			RelocOpcode(opcode_type opcode = SmInvalid)
				: opcode(opcode)
			{
			}

			RelocOpcode(opcode_type opcode, uint32_t value, uint32_t repeat)
				: opcode(opcode), value(value), repeat(repeat)
			{
			}

			/** @brief Reads and initializes a single relocation opcode record */
			void ReadFile(Linker::Reader& rd);
			/** @brief Returns the bit sequence that this opcode is stored as in the file
			 *
			 * For 16-bit instructions, the value is stored as a zero extended value in the least significant bits, for 32-bit instructions,
			 * it takes up the entire word. */
			uint32_t GetWord() const;
			/** @brief Returns the number of bytes required to encode this opcode */
			offset_t CodeSize() const;
			/** @brief Outputs a single relocation opcode to the file */
			void WriteFile(Linker::Writer& wr)
			{
				wr.WriteWord(CodeSize(), GetWord());
			}
			/** @brief Executes the opcode to possibly produce some relocation information and/or alter the state of the pseudo-microprocessor */
			void GenerateRelocations(RelocationProcessor& processor) const;

			void Dump(Dumper::Dumper& dump, const PEFFormat& pef_format, uint32_t opcode_index, int display_options = 0) const;
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
			// convenience field to determine index of symbol in table
			uint32_t section_number = uint32_t(-1);

			std::shared_ptr<Linker::Contents> image;
			std::vector<PatternInitialization> patterns;
			// only appearing in sections with relocations
			std::map<uint32_t, Relocation> relocations;
			std::vector<RelocOpcode> reloc_opcodes;
			bool contains_relocations = false;
			uint16_t reserved_a = 0;
			uint32_t reloc_instr_size = 0;
			uint32_t first_reloc_offset = 0;

			Section() = default;
			Section(section_type section_kind, share_type share_kind, std::shared_ptr<Linker::Contents> image)
				: section_kind(section_kind), share_kind(share_kind), image(image)
			{
				alignment = ExpectedAlignment();
			}

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
		uint32_t loader_section_offset = 0; // duplicate value of sections[*]->container_offset for sections[*]->section_kind == Section::Loader

		static constexpr uint32_t NoSection = uint32_t(-1);
		static constexpr uint32_t Absolute = uint32_t(-2);
		static constexpr uint32_t Reexported = uint32_t(-3);

		/** @brief Represents a reference to some data, stored as an offset into a section pair */
		struct Reference
		{
			std::weak_ptr<Section> section_pointer;
			uint32_t section = NoSection;
			uint32_t offset = 0;

			void SetPosition(PEFFormat& pef_format, const Linker::Position& position);

			bool IsPresent() const { return section != NoSection; }
			void StoreSectionIndex()
			{
				auto actual_section = section_pointer.lock();
				section = actual_section ? actual_section->section_number : NoSection;
			}
		};
		Reference main_symbol, init_symbol, term_symbol;

		/** @brief Represents a string stored in the loader section string table */
		class Name
		{
		public:
			uint32_t name_offset = 0;
			std::string name = "";
			std::string LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd);
			std::string LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd, uint16_t length);
			void StoreNameString(PEFFormat& pef_format);
			void StoreNameStringNoNull(PEFFormat& pef_format);

			Name() = default;

			Name(std::string name)
				: name(name)
			{
			}
		};

		class ImportedLibrary;

		enum symbol_class_type
		{
			Code = 0,
			Data = 1,
			TVect = 2,
			TOC = 3,
			Glue = 4,
		};
		static inline constexpr bool IsValidSymbolClass(int value)
		{
			switch(value)
			{
			case Code:
			case Data:
			case TVect:
			case TOC:
			case Glue:
				return true;
			default:
				return false;
			}
		}

		class ImportedSymbol : public Name
		{
		public:
			symbol_class_type symbol_class = TVect;
			uint8_t flags = 0;
			// convenience field to determine index of symbol in table
			uint32_t symbol_number = uint32_t(-1);

			ImportedSymbol() = default;
			ImportedSymbol(std::string name) : Name(name) { }

			std::weak_ptr<ImportedLibrary> library; // back link to library that includes it
		};

		class ImportedLibrary : public Name
		{
		public:
			uint32_t old_imp_version = 0;
			uint32_t current_version = 0;
			uint32_t imported_symbol_count = 0;
			uint32_t first_imported_symbol = 0;
			std::vector<std::shared_ptr<ImportedSymbol>> imported_symbols;
			// only used for generation
			std::map<std::string, std::shared_ptr<ImportedSymbol>> named_imported_symbols;
			uint8_t options = 0;
			uint8_t reserved_a = 0;
			uint16_t reserved_b = 0;

			ImportedLibrary() = default;
			ImportedLibrary(std::string name) : Name(name) { }

			std::shared_ptr<ImportedSymbol> GetImportByName(std::string name)
			{
				auto symbol_iter = named_imported_symbols.find(name);
				if(symbol_iter == named_imported_symbols.end())
				{
					auto symbol = std::make_shared<ImportedSymbol>(name);
					imported_symbols.push_back(symbol);
					named_imported_symbols[name] = symbol;
					return symbol;
				}
				else
				{
					return symbol_iter->second;
				}
			}
		};
		std::vector<std::shared_ptr<ImportedLibrary>> imported_libraries;
		// only used for generation
		std::map<std::string, std::shared_ptr<ImportedLibrary>> named_imported_libraries;
		std::vector<std::shared_ptr<ImportedSymbol>> imported_symbols;
		std::vector<uint32_t> reloc_section_indexes;
		uint32_t reloc_instr_offset = 0;
		uint32_t loader_strings_offset = 0;
		uint32_t export_hash_offset = 0;
		std::vector<RelocOpcode> relocs_area;
		uint32_t relocs_area_size = 0;
		std::vector<std::string> loader_string_table;
		uint32_t loader_string_table_size = 0;

		struct ExportedSymbol;

		struct HashTableEntry
		{
			uint16_t chain_count = 0;
			uint32_t first_index = 0;

			// used only for generation
			std::vector<std::shared_ptr<ExportedSymbol>> chain;
		};
		std::vector<HashTableEntry> hash_table;

		struct ExportedSymbol : public Name, public Reference
		{
			uint16_t symbol_length = 0;
			uint16_t hash_value = 0;
			symbol_class_type symbol_class = Data;

			ExportedSymbol() = default;

			ExportedSymbol(std::string name)
				: Name(name)
			{
			}

			ExportedSymbol(std::string name, symbol_class_type symbol_class)
				: Name(name), symbol_class(symbol_class)
			{
			}

			using Name::LoadNameString;
			std::string LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd);
		};
		std::vector<std::shared_ptr<ExportedSymbol>> exported_symbols;

		static uint32_t ComputeHashWord(std::string name);
		static uint32_t HashTableIndex(uint32_t hash_word, uint32_t export_hash_table_power);
		static uint8_t ComputeHashTableExponent(uint32_t hash_table_size);

		std::shared_ptr<ImportedLibrary> FetchImportLibrary(std::string name)
		{
			auto library_iter = named_imported_libraries.find(name);
			if(library_iter == named_imported_libraries.end())
			{
				auto library = std::make_shared<ImportedLibrary>(name);
				imported_libraries.push_back(library);
				named_imported_libraries[name] = library;
				return library;
			}
			else
			{
				return library_iter->second;
			}
		}

		static constexpr uint32_t LoaderHeaderSize = 56;
		static constexpr uint32_t LibraryDescriptionSize = 28;
		uint32_t GetLibraryDescriptionsSize() const
		{
			return LibraryDescriptionSize * imported_libraries.size();
		}
		uint32_t GetSymbolTablesSize() const
		{
			uint32_t size = 0;
			for(auto library : imported_libraries)
			{
				size += 4 * library->imported_symbols.size();
			}
			return size;
		}
		uint32_t GetRelocationHeadersSize() const
		{
			return 12 * reloc_section_indexes.size();
		}
		uint32_t GetRelocationAreaSize() const
		{
			return relocs_area_size;
		}
		uint32_t GetLoaderStringAreaSize() const
		{
			return loader_string_table_size;
		}
		uint32_t GetExportHashTableSize() const
		{
			return 4 * hash_table.size();
		}
		uint32_t GetExportKeyTableSize() const
		{
			return 4 * exported_symbols.size();
		}
		uint32_t GetExportSymbolTableSize() const
		{
			return 10 * exported_symbols.size();
		}

		uint32_t GetMinimumRelocInstrOffset() const
		{
			return LoaderHeaderSize
					+ GetLibraryDescriptionsSize()
					+ GetSymbolTablesSize()
					+ GetRelocationHeadersSize();
		}

		uint32_t GetLoaderSectionSize() const
		{
			return
				std::max({LoaderHeaderSize
					+ GetLibraryDescriptionsSize()
					+ GetSymbolTablesSize()
					+ GetRelocationHeadersSize(),
					reloc_instr_offset + GetRelocationAreaSize(),
					loader_strings_offset + GetLoaderStringAreaSize(),
					export_hash_offset
						+ GetExportHashTableSize()
						+ GetExportKeyTableSize()
						+ GetExportSymbolTableSize()});
		}

		// temporary structure to map linker segments to PEF sections
		std::map<std::shared_ptr<Linker::Segment>, std::shared_ptr<Section>> segment_to_section_map;

		bool FormatSupportsLibraries() const override;
		bool FormatSupportsResources() const override;

		void ReadLoaderSection(Linker::Reader& rd);
		void WriteLoaderSection(Linker::Writer& wr) const;

		void ReadFile(Linker::Reader& rd) override;
		void CalculateValues() override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void SortImports();
		void ProcessRelocations(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
	};

	class PEFOutputDriver : public MacintoshOutputDriver
	{
	public:
		// TODO: very preliminary

		PEFOutputDriver(target_format_t target = TARGET_DATA_FORK)
			: MacintoshOutputDriver(target)
		{
		}

		PEFOutputDriver(target_format_t target, int produce)
			: MacintoshOutputDriver(target, produce)
		{
		}

		bool FormatSupportsResources() const override;

	private:
		/** COFF format */
		std::shared_ptr<PEFFormat> data_fork;
		/** Direct access to the Mac OS resource fork */
		std::shared_ptr<MacintoshResourceFileFormat> resource_fork;
		std::shared_ptr<FinderInfo> finder_info;

		std::map<std::string, std::string> options;
		std::string model;
		std::string script_file;
		std::map<std::string, std::string> script_options;
	};
}

#endif /* PEFEXE_H */
