#ifndef HUNK_H
#define HUNK_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
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

		/**
		 * @brief The smallest unit of a Hunk file, it starts with a type word
		 */
		class Block
		{
		public:
			enum block_type
			{
				/** @brief First block inside an object file */
				HUNK_UNIT = 0x3E7,
				HUNK_NAME = 0x3E8,
				/** @brief First block of a code segment (hunk) containing Motorola 68k instructions */
				HUNK_CODE = 0x3E9,
				/** @brief First block of a data segment (hunk) */
				HUNK_DATA = 0x3EA,
				/** @brief First block of a bss segment (hunk) */
				HUNK_BSS = 0x3EB,
				/** @brief Block containing relocation */
				HUNK_RELOC32 = 0x3EC,
				HUNK_RELOC16 = 0x3ED,
				HUNK_RELOC8 = 0x3EE,
				HUNK_EXT = 0x3EF,
				HUNK_SYMBOL = 0x3F0,
				HUNK_DEBUG = 0x3F1,
				/** @brief Block terminates a hunk */
				HUNK_END = 0x3F2,
				/** @brief First block inside an executable or library file */
				HUNK_HEADER = 0x3F3,
				HUNK_OVERLAY = 0x3F5,
				HUNK_BREAK = 0x3F6,
				HUNK_DRELOC32 = 0x3F7,
				HUNK_DRELOC16 = 0x3F8,
				HUNK_DRELOC8 = 0x3F9,
				HUNK_LIB = 0x3FA,
				HUNK_INDEX = 0x3FB,
				HUNK_RELOC32SHORT = 0x3FC,
				HUNK_RELRELOC32 = 0x3FD,
				HUNK_ABSRELOC16 = 0x3FE,
				/** @brief First block of a code segment (hunk) containing PowerPC instructions */
				HUNK_PPC_CODE = 0x4E9,
				HUNK_RELRELOC26 = 0x4EC,
			};
			block_type type;
			Block(block_type type)
				: type(type)
			{
			}
			virtual ~Block() = default;
			/** @brief Reads a single block from file */
			static std::shared_ptr<Block> ReadBlock(Linker::Reader& rd);
			/** @brief Reads the rest of the block after the type word */
			virtual void Read(Linker::Reader& rd);
			/** @brief Writes the entire block into a file */
			virtual void Write(Linker::Writer& wr) const;
			/** @brief Returns the size of the block as stored inside a file */
			virtual offset_t FileSize() const;
			virtual void Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
			void AddCommonFields(Dumper::Region& region, unsigned index) const;
			virtual void AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
		};

		/** @brief Represents the first block inside of an object file */
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

		/** @brief Represents the first block inside of an executable or library file */
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

			void Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			void AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		/** @brief Represents the initial block of a hunk (segment) */
		class InitialHunkBlock : public Block
		{
		public:
			enum
			{
				FlagMask      = 0xC0000000,
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

			InitialHunkBlock(block_type type, uint32_t flags = LoadAny)
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

			void AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

		protected:
			/** @brief The actual size of the segment data, as a count of 32-bit longwords */
			virtual uint32_t GetSize() const = 0;
			/** @brief Reads the segment data (if any) */
			virtual void ReadBody(Linker::Reader& rd, uint32_t longword_count);
			/** @brief Writes the segment data (if any) */
			virtual void WriteBody(Linker::Writer& wr) const;
		};

		/** @brief Represents the initial block of a non-bss segment (hunk), containing a memory image */
		class LoadBlock : public InitialHunkBlock
		{
		public:
			std::shared_ptr<Linker::Image> image;

			LoadBlock(block_type type, uint32_t flags = LoadAny)
				: InitialHunkBlock(type, flags)
			{
			}

			void Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

		protected:
			uint32_t GetSize() const override;
			void ReadBody(Linker::Reader& rd, uint32_t longword_count) override;
			void WriteBody(Linker::Writer& wr) const override;
		};

		/** @brief Represents the initial block of a bss segment (hunk) that needs to be allocated and filled with zero at load time */
		class BssBlock : public InitialHunkBlock
		{
		public:
			/** @brief Size of memory in 4-byte longwords */
			uint32_t size;

			BssBlock(uint32_t size = 0, uint32_t flags = LoadAny)
				: InitialHunkBlock(HUNK_BSS, flags), size(size)
			{
			}

			offset_t FileSize() const override;

			void AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;

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

			RelocationBlock(block_type type)
				: Block(type)
			{
			}

			size_t GetRelocationSize() const;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

			void Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
		};

		class ExternalBlock : public Block
		{
		public:
			// TODO: untested
			class SymbolData
			{
			public:
				enum symbol_type
				{
					EXT_SYMB = 0,
					EXT_DEF = 1,
					EXT_ABS = 2,
					EXT_RES = 3,

					EXT_REF32 = 129,
					EXT_COMMON = 130,
					EXT_REF16 = 131,
					EXT_REF8 = 132,
					EXT_DREF32 = 133,
					EXT_DREF16 = 134,
					EXT_DREF8 = 135,
					EXT_RELREF26 = 229,
				};
				symbol_type type;
				std::string name;

				SymbolData(symbol_type type, std::string name)
					: type(type), name(name)
				{
				}

				static std::unique_ptr<SymbolData> ReadData(Linker::Reader& rd);

				virtual ~SymbolData() = default;
				/** @brief Read data after type and name */
				virtual void Read(Linker::Reader& rd);
				/** @brief Write entire unit */
				virtual void Write(Linker::Writer& wr) const;
				/** @brief Size of entire unit, including type and name fields */
				virtual offset_t FileSize() const;
				virtual void AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const;
			};

			class Definition : public SymbolData
			{
			public:
				uint32_t value;

				Definition(symbol_type type, std::string name, uint32_t value = 0)
					: SymbolData(type, name), value(value)
				{
				}

				void Read(Linker::Reader& rd) override;
				/** @brief Write entire unit */
				void Write(Linker::Writer& wr) const override;
				/** @brief Size of entire unit, including type and name fields */
				offset_t FileSize() const override;
				void AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
			};

			std::vector<std::unique_ptr<SymbolData>> symbols;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;
			void Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const override;
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

			struct Relocation
			{
				size_t size;
				uint32_t offset;
				Relocation(size_t size, uint32_t offset)
					: size(size), offset(offset)
				{
				}
				bool operator <(const Relocation& other) const;
			};

			std::map<uint32_t, std::set<Relocation>> relocations;

			/** @brief Converts the data collected by the linker into hunk blocks */
			void ProduceBlocks();

			/** @brief The amount of memory this hunk will take up after loading */
			uint32_t GetMemorySize() const;

			/** @brief The amount of bytes this hunk will take up in the file */
			uint32_t GetFileSize() const;

			/** @brief Retrieves the size field of the first block, to be stored in a header block */
			uint32_t GetSizeField();

			/** @brief Appends a new block to the block sequence and updates the internal structure of the hunk, used for reading */
			void AppendBlock(std::shared_ptr<Block> block);
		};

		/** @brief Every file is supposed to start with either a HUNK_UNIT or a HUNK_BLOCK, which is stored here */
		std::shared_ptr<Block> start_block;
		std::vector<Hunk> hunks;

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		/** @brief If a header block is available, checks the associated hunk size in the header, returns 0 otherwise */
		offset_t GetHunkSizeInHeader(uint32_t index) const;

		static std::string ReadString(uint32_t longword_count, Linker::Reader& rd);
		static std::string ReadString(Linker::Reader& rd, uint32_t& longword_count);
		static std::string ReadString(Linker::Reader& rd);
		static void WriteStringContents(Linker::Writer& wr, std::string name);
		static void WriteString(Linker::Writer& wr, std::string name);
		static offset_t MeasureString(std::string name);

		/* * * Writer members * * */

		enum cpu_type
		{
			CPU_M68K = Block::HUNK_CODE,
			CPU_PPC = Block::HUNK_PPC_CODE,
		};
		cpu_type cpu = CPU_PPC;

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
