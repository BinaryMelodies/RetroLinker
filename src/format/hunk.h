#ifndef HUNK_H
#define HUNK_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/section.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace Amiga
{
	/**
	 * @brief AmigaOS/TRIPOS Hunk files
	 *
	 * Introduced for the TRIPOS system and then adopted for AmigaOS, a hunk file stores the binary executable for an Amiga application.
	 */
	class HunkFormat : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		class Hunk;
		class Module;

		struct Relocation
		{
			/** @brief Size of relocation in bytes */
			size_t size;
			enum relocation_type
			{
				/** @brief Relocation is an absolute reference */
				Absolute,
				/** @brief Relocation is PC-relative */
				SelfRelative,
				/** @brief Relocation relative to the data section */
				DataRelative,
				/** @brief Relocation is 26-bit PC-relative, for PowerPC */
				SelfRelative26,
			};
			relocation_type type;
			uint32_t offset;
			Relocation(size_t size, relocation_type type, uint32_t offset)
				: size(size), type(type), offset(offset)
			{
			}
			bool operator <(const Relocation& other) const;
		};

		/**
		 * @brief The smallest unit of a Hunk file, it starts with a type word. A HUNK_END or HUNK_BREAK block is represented by this.
		 */
		class Block
		{
		public:
			enum block_type
			{
				/** @brief First block inside an object file */
				HUNK_UNIT = 0x3E7,
				/** @brief Name of hunk */
				HUNK_NAME = 0x3E8,
				/** @brief First block of a code segment (hunk) containing Motorola 68k instructions */
				HUNK_CODE = 0x3E9,
				/** @brief First block of a data segment (hunk) */
				HUNK_DATA = 0x3EA,
				/** @brief First block of a bss segment (hunk) */
				HUNK_BSS = 0x3EB,
				/** @brief Block containing 32-bit relocations */
				HUNK_ABSRELOC32 = 0x3EC,
				/** @brief Block containing 16-bit PC-relative relocations */
				HUNK_RELRELOC16 = 0x3ED,
				/** @brief Block containing 8-bit PC-relative relocations */
				HUNK_RELRELOC8 = 0x3EE,
				/** @brief Block containing externally visible references */
				HUNK_EXT = 0x3EF,
				/** @brief Block containing a symbol table */
				HUNK_SYMBOL = 0x3F0,
				/** @brief Debug information */
				HUNK_DEBUG = 0x3F1,
				/** @brief Block terminates a hunk */
				HUNK_END = 0x3F2,
				/** @brief First block inside an executable or library file */
				HUNK_HEADER = 0x3F3,
				HUNK_OVERLAY = 0x3F5,
				HUNK_BREAK = 0x3F6,
				/** @brief Block containing 32-bit data section relative relocations */
				HUNK_DRELOC32 = 0x3F7,
				/** @brief Block containing 16-bit data section relative relocations */
				HUNK_DRELOC16 = 0x3F8,
				/** @brief Block containing 8-bit data section relative relocations */
				HUNK_DRELOC8 = 0x3F9,
				HUNK_LIB = 0x3FA,
				HUNK_INDEX = 0x3FB,
				/** @brief Block containing 32-bit relocations in a compactified form */
				HUNK_RELOC32SHORT = 0x3FC,
				/** @brief Block containing 32-bit PC-relative relocations */
				HUNK_RELRELOC32 = 0x3FD,
				/** @brief Block containing 8-bit relocations */
				HUNK_ABSRELOC16 = 0x3FE,
				/** @brief First block of a code segment (hunk) containing PowerPC instructions */
				HUNK_PPC_CODE = 0x4E9,
				/** @brief Block containing 26-bit PC-relative relocations inside 4-byte words */
				HUNK_RELRELOC26 = 0x4EC,

				/** @brief V37 Block containing 32-bit relocations in a compactified form, only found in executables */
				HUNK_V37_RELOC32SHORT = 0x3F7,
			};
			block_type type;
			/** @brief Required because V37 executables define HUNK_DRELOC32 as HUNK_RELOC32SHORT, must block types ignore this field and it is not stored */
			bool is_executable = false;
			Block(block_type type, bool is_executable = false)
				: type(type), is_executable(is_executable)
			{
			}
			virtual ~Block() = default;
			/**
			 * @brief Reads a single block from file
			 *
			 * @param rd The reader
			 * @param is_executable If file is executable, false if object or not known
			 * @return A parsed block, or nullptr if file ended
			 */
			static std::shared_ptr<Block> ReadBlock(Linker::Reader& rd, bool is_executable = false);
			/** @brief Reads the rest of the block after the type word */
			virtual void Read(Linker::Reader& rd);
			/** @brief Writes the entire block into a file */
			virtual void Write(Linker::Writer& wr) const;
			/** @brief Returns the size of the block as stored inside a file */
			virtual offset_t FileSize() const;
			virtual void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
			void AddCommonFields(Dumper::Region& region, unsigned index) const;
			virtual void AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
		};

		/** @brief Represents the first block inside of an object file or a hunk. This class is used for HUNK_UNIT and HUNK_NAME blocks. */
		class TextBlock : public Block
		{
		public:
			std::string name;

			TextBlock(block_type type, std::string name = "")
				: Block(type), name(name)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;
		};

		/** @brief Represents the first block inside of an executable file, a HUNK_HEADER */
		class HeaderBlock : public Block
		{
		public:
			std::vector<std::string> library_names;
			uint32_t table_size = 0;
			uint32_t first_hunk = 0;
			std::vector<uint32_t> hunk_sizes;

			HeaderBlock()
				: Block(HUNK_HEADER)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			void AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents the block that gives the memory loaded contents of the hunk (segment) */
		class RelocatableBlock : public Block
		{
		public:
			enum
			{
				FlagMask      = 0xE0000000,
				BitAdvisory   = 0x20000000,
				BitChipMem    = 0x40000000,
				BitFastMem    = 0x80000000,
				BitAdditional = 0xC0000000,
			};

			enum flag_type
			{
				LoadAny      = 0x00000000,
				LoadPublic   = 0x00000001, /* default, not stored */
				LoadChipMem  = 0x00000002,
				LoadFastMem  = 0x00000004,
				LoadLocalMem = 0x00000008,
				Load24BitDma = 0x00000010,
				LoadClear    = 0x00010000,
			};
			flag_type flags;
			bool loaded_with_additional_flags = false;

			RelocatableBlock(block_type type, uint32_t flags = LoadAny)
				: Block(type), flags(flag_type(flags | LoadPublic))
			{
			}

			/** @brief Returns the 32-bit longword that stores the size of the segment plus 2 flag bits */
			uint32_t GetSizeField() const;

			/** @brief Returns true if an additional 32-bit longword is needed for flags */
			bool RequiresAdditionalFlags() const;

			/** @brief The contents of an additional 32-bit longword is needed for flags */
			uint32_t GetAdditionalFlags() const;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

		protected:
			/** @brief The actual size of the segment data, as a count of 32-bit longwords */
			virtual uint32_t GetSize() const = 0;
			/** @brief Reads the segment data (if any) */
			virtual void ReadBody(Linker::Reader& rd, uint32_t longword_count);
			/** @brief Writes the segment data (if any) */
			virtual void WriteBody(Linker::Writer& wr) const;
		};

		/** @brief Represents the load block of a non-bss segment (hunk) HUNK_CODE/HUNK_DATA/HUNK_PPC_CODE, containing a memory image */
		class LoadBlock : public RelocatableBlock
		{
		public:
			std::shared_ptr<Linker::Image> image;

			LoadBlock(block_type type, uint32_t flags = LoadAny)
				: RelocatableBlock(type, flags)
			{
			}

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

		protected:
			uint32_t GetSize() const override;
			void ReadBody(Linker::Reader& rd, uint32_t longword_count) override;
			void WriteBody(Linker::Writer& wr) const override;
		};

		/** @brief Represents the declaration block of a HUNK_BSS segment (hunk) that needs to be allocated and filled with zero at load time */
		class BssBlock : public RelocatableBlock
		{
		public:
			/** @brief Size of memory in 4-byte longwords */
			uint32_t size;

			BssBlock(uint32_t size = 0, uint32_t flags = LoadAny)
				: RelocatableBlock(HUNK_BSS, flags), size(size)
			{
			}

			offset_t FileSize() const override;

			void AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

		protected:
			uint32_t GetSize() const override;
			void ReadBody(Linker::Reader& rd, uint32_t longword_count) override;
		};

		/** @brief Represents a block containing relocations */
		class RelocationBlock : public Block
		{
		public:
			struct RelocationData
			{
				uint32_t hunk;
				std::vector<uint32_t> offsets;
			};
			std::vector<RelocationData> relocations;

			RelocationBlock(block_type type, bool is_executable = false)
				: Block(type, is_executable)
			{
			}

			/** @brief Whether this is a HUNK_RELOC32SHORT block (needed because V37 gives it a different number) */
			bool IsShortRelocationBlock() const;

			size_t GetRelocationSize() const;

			Relocation::relocation_type GetRelocationType() const;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents a HUNK_EXT or HUNK_SYMBOL block */
		class SymbolBlock : public Block
		{
		public:
			class Unit
			{
			public:
				enum symbol_type
				{
					EXT_SYMB = 0,
					EXT_DEF = 1,
					EXT_ABS = 2,
					EXT_RES = 3,

					EXT_ABSREF32 = 129,
					EXT_COMMON = 130,
					EXT_RELREF16 = 131,
					EXT_RELREF8 = 132,
					EXT_DREF32 = 133,
					EXT_DREF16 = 134,
					EXT_DREF8 = 135,
					EXT_RELREF32 = 136,
					EXT_RELCOMMON = 137,
					EXT_ABSREF16 = 138,
					EXT_ABSREF8 = 139,
					EXT_RELREF26 = 229,
				};
				symbol_type type;
				std::string name;

				Unit(symbol_type type, std::string name)
					: type(type), name(name)
				{
				}

				static std::unique_ptr<Unit> ReadData(Linker::Reader& rd);

				virtual ~Unit() = default;
				/** @brief Read data after type and name */
				virtual void Read(Linker::Reader& rd);
				/** @brief Write entire unit */
				virtual void Write(Linker::Writer& wr) const;
				/** @brief Size of entire unit, including type and name fields */
				virtual offset_t FileSize() const;
				virtual void DumpContents(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
				virtual void AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
			};

			class Definition : public Unit
			{
			public:
				uint32_t value;

				Definition(symbol_type type, std::string name, uint32_t value = 0)
					: Unit(type, name), value(value)
				{
				}

				void Read(Linker::Reader& rd) override;
				/** @brief Write entire unit */
				void Write(Linker::Writer& wr) const override;
				/** @brief Size of entire unit, including type and name fields */
				offset_t FileSize() const override;
				void AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			};

			class References : public Unit
			{
			public:
				std::vector<uint32_t> references;

				References(symbol_type type, std::string name)
					: Unit(type, name)
				{
				}

				void Read(Linker::Reader& rd) override;
				/** @brief Write entire unit */
				void Write(Linker::Writer& wr) const override;
				/** @brief Size of entire unit, including type and name fields */
				offset_t FileSize() const override;
				void DumpContents(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			};

			class CommonReferences : public References
			{
			public:
				uint32_t size;

				CommonReferences(symbol_type type, std::string name, uint32_t size = 0)
					: References(type, name), size(size)
				{
				}

				void Read(Linker::Reader& rd) override;
				/** @brief Write entire unit */
				void Write(Linker::Writer& wr) const override;
				/** @brief Size of entire unit, including type and name fields */
				offset_t FileSize() const override;
				void AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			};

			std::vector<std::unique_ptr<Unit>> symbols;

			SymbolBlock(block_type type)
				: Block(type)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;
			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents a HUNK_DEBUG block */
		class DebugBlock : public Block
		{
		public:
			// TODO: untested
			std::shared_ptr<Linker::Image> image;

			DebugBlock()
				: Block(HUNK_DEBUG)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents a HUNK_OVERLAY block */
		class OverlayBlock : public Block
		{
		public:
			// TODO: untested
			uint32_t maximum_level = 2;

			struct OverlaySymbol
			{
				uint32_t offset;
				uint32_t res1, res2;
				uint32_t level;
				uint32_t ordinate;
				uint32_t first_hunk;
				uint32_t symbol_hunk;
				uint32_t symbol_offset;
			};

			std::vector<OverlaySymbol> overlay_data_table;

			OverlayBlock()
				: Block(HUNK_OVERLAY)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents a HUNK_LIB block */
		class LibraryBlock : public Block
		{
		public:
			// TODO: untested

			std::unique_ptr<Module> hunks;

			LibraryBlock()
				: Block(HUNK_LIB)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		class IndexBlock : public Block
		{
		public:
			// TODO: untested

			class Definition
			{
			public:
				uint16_t string_offset = 0;
				uint16_t symbol_offset = 0;
				uint16_t type = 0;

				static Definition Read(Linker::Reader& rd);
				void Write(Linker::Writer& wr) const;
			};

			class HunkEntry
			{
			public:
				uint16_t string_offset = 0;
				uint16_t hunk_size = 0; // in longwords
				uint16_t hunk_type = 0;
				std::vector<uint16_t> references;
				std::vector<Definition> definitions;

				static HunkEntry Read(Linker::Reader& rd);
				void Write(Linker::Writer& wr) const;
				offset_t FileSize() const;
			};

			class ProgramUnit
			{
			public:
				int16_t string_offset = 0;
				uint16_t first_hunk_offset = 0; // in longwords
				std::vector<HunkEntry> hunks;

				static ProgramUnit Read(Linker::Reader& rd);
				void Write(Linker::Writer& wr) const;
				offset_t FileSize() const;
			};

			std::vector<std::string> strings;
			std::vector<ProgramUnit> units;

			IndexBlock()
				: Block(HUNK_INDEX)
			{
			}

			offset_t StringTableSize() const;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief The smallest loadable unit of a Hunk file is the hunk, it roughly corresponds to a segment in other file formats */
		class Hunk
		{
		public:
			/** @brief The sequence of blocks the hunk is stored as */
			std::vector<std::shared_ptr<Block>> blocks;

			enum hunk_type : uint32_t
			{
				Undefined = 0,
				Invalid = uint32_t(-1),

				Code = Block::HUNK_CODE,
				CodePPC = Block::HUNK_PPC_CODE,
				Data = Block::HUNK_DATA,
				Bss = Block::HUNK_BSS,
			};
			hunk_type type = Undefined;

			// TODO: implement advisory flag

			LoadBlock::flag_type flags = LoadBlock::LoadPublic;
			/** @brief The memory image, if stored in a file (that is, a non-bss segment) */
			std::shared_ptr<Linker::Image> image;
			/** @brief Size of the memory image, if it is a zero filled segment (bss) */
			offset_t image_size = 0;

			std::string name;

			explicit Hunk()
				: type(Undefined), flags(LoadBlock::LoadAny), image(nullptr)
			{
			}

			Hunk(hunk_type type, std::string name = "image", unsigned flags = LoadBlock::LoadAny)
				: type(hunk_type(type)), flags(LoadBlock::flag_type(flags)), image(std::make_shared<Linker::Segment>(name))
			{
			}

			Hunk(hunk_type type, std::shared_ptr<Linker::Segment> segment, unsigned flags = LoadBlock::LoadAny)
				: type(hunk_type(type)), flags(LoadBlock::flag_type(flags)), image(segment)
			{
			}

			std::map<uint32_t, std::set<Relocation>> relocations;

			/** @brief Converts the data collected by the linker into hunk blocks */
			void ProduceBlocks(HunkFormat& fmt, Module& module);

			/** @brief The amount of memory this hunk will take up after loading */
			uint32_t GetMemorySize() const;

			/** @brief The amount of bytes this hunk will take up in the file */
			uint32_t GetFileSize() const;

			/** @brief Retrieves the size field of the first block, to be stored in a header block */
			uint32_t GetSizeField(HunkFormat& fmt, Module& module);

			/** @brief Appends a new block to the block sequence and updates the internal structure of the hunk, used for reading */
			void AppendBlock(std::shared_ptr<Block> block);
		};

		/** @brief Module is a program unit containing several hunks
		 *
		 * In this representation, a module will begin with a starting block, contain several hunks, and optionally be terminated by an ending block.
		 * This permits storing object files, libraries and executable files with overlays in a similar structure.
		 */
		class Module
		{
		public:
			/** @brief For an object unit, HUNK_UNIT, for an executable node, HUNK_HEADER, for a new type library, HUNK_LIB */
			std::shared_ptr<Block> start_block;
			/** @brief The hunks in this module. Hunks in a library are stored inside a HUNK_LIB */
			std::vector<Hunk> hunks;
			/** @brief A final block before the start of the next module
			 *
			 * An executable node can optionally be followed by a HUNK_OVERLAY.
			 * Every overlay is stored as a separate module, and two overlays are terminated by a HUNK_BREAK.
			 * A library must be terminated by a HUNK_INDEX.
			 */
			std::shared_ptr<Block> end_block;

			bool IsExecutable() const;
			offset_t ImageSize() const;
			void ReadFile(Linker::Reader& rd, std::shared_ptr<Block>& next_block, offset_t end);
			void WriteFile(Linker::Writer& wr) const;
			void Dump(Dumper::Dumper& dump, offset_t current_offset, unsigned index) const;

			/** @brief If a header block is available, checks the associated hunk size in the header, returns 0 otherwise */
			offset_t GetHunkSizeInHeader(uint32_t index) const;
		};

		/** @brief A collection of modules/program units
		 *
		 * For executable files, the first module is the main program, and further modules are overlays.
		 */
		std::vector<Module> modules;

		bool IsExecutable() const;

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		static std::string ReadString(uint32_t longword_count, Linker::Reader& rd);
		static std::string ReadString(Linker::Reader& rd, uint32_t& longword_count);
		static std::string ReadString(Linker::Reader& rd);
		static void WriteStringContents(Linker::Writer& wr, std::string name);
		static void WriteString(Linker::Writer& wr, std::string name);
		static offset_t MeasureString(std::string name);

		/* * * Writer members * * */

		class HunkOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::optional<std::string>> system{"system", "Target system version, determines generated hunk types, permitted options: v1, v37, v38, v39"};

			HunkOptionCollector()
			{
				InitializeFields(system);
			}
		};

		enum cpu_type
		{
			CPU_M68K = Block::HUNK_CODE,
			CPU_PPC = Block::HUNK_PPC_CODE,
		};
		cpu_type cpu = CPU_PPC;

		enum system_version
		{
			V1,
			/** @brief generate HUNK_V37_RELOC32SHORT */
			V37,
			/** @brief generate HUNK_RELOC32SHORT instead of HUNK_V37_RELOC32SHORT */
			V38, // TODO: not sure about name, so called because it is newer than V37 but older than V39
			/** @brief generate HUNK_RELRELOC32 */
			V39, // TODO: not yet implemented
		};
		system_version system = V1;

		HunkFormat() = default;

		HunkFormat(system_version system)
			: system(system)
		{
		}

		/* @brief Convenience map to make it easier to look up the indices of segments */
		std::map<std::shared_ptr<Linker::Segment>, size_t> segment_index;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		enum flags
		{
			/**
			 * @brief Section to be stored in fast memory
			 */
			FastMemory = Linker::Section::CustomFlag,
			/**
			 * @brief Section to be stored in chip memory
			 */
			ChipMemory = Linker::Section::CustomFlag << 1,
		};

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void AddHunk(const Hunk& hunk);

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;

	};

}

#endif /* HUNK_H */
