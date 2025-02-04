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

		class Block
		{
		public:
			enum block_type
			{
				HUNK_UNIT = 0x3E7,
				HUNK_CODE = 0x3E9,
				HUNK_DATA = 0x3EA,
				HUNK_BSS = 0x3EB,
				HUNK_RELOC32 = 0x3EC,
				HUNK_END = 0x3F2,
				HUNK_HEADER = 0x3F3,
				HUNK_PPC_CODE = 0x4E9,
			};
			block_type type;
			Block(block_type type)
				: type(type)
			{
			}
			virtual ~Block() = default;
			static std::shared_ptr<Block> ReadBlock(Linker::Reader& rd);
			virtual void Read(Linker::Reader& rd);
			virtual void Write(Linker::Writer& wr) const;
			virtual offset_t FileSize() const;
		};

		class UnitBlock : public Block
		{
		public:
			std::string name;

			UnitBlock()
				: Block(HUNK_UNIT)
			{
			}

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;
		};

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
		};

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

			InitialHunkBlock(block_type type, uint32_t flags = LoadAny)
				: Block(type), flags(flag_type(flags | LoadPublic))
			{
			}

			uint32_t GetSizeField() const;

			bool RequiresAdditionalFlags() const;

			uint32_t GetAdditionalFlags() const;

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;

		protected:
			virtual uint32_t GetSize() const = 0;
			virtual void ReadBody(Linker::Reader& rd, uint32_t longword_count);
			virtual void WriteBody(Linker::Writer& wr) const;
		};

		class LoadBlock : public InitialHunkBlock
		{
		public:
			std::shared_ptr<Linker::Image> image;

			LoadBlock(block_type type, uint32_t flags = LoadAny)
				: InitialHunkBlock(type, flags)
			{
			}

		protected:
			uint32_t GetSize() const override;
			void ReadBody(Linker::Reader& rd, uint32_t longword_count) override;
			void WriteBody(Linker::Writer& wr) const override;
		};

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

		protected:
			uint32_t GetSize() const override;
			void ReadBody(Linker::Reader& rd, uint32_t longword_count) override;
		};

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

			void Read(Linker::Reader& rd) override;
			void Write(Linker::Writer& wr) const override;
			offset_t FileSize() const override;
		};

		class Hunk
		{
		public:
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
			std::shared_ptr<Linker::Image> image;
			offset_t image_size = 0; // only relevant for bss

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

			std::map<uint32_t, std::vector<uint32_t> > relocations;

			void ProduceBlocks();

			uint32_t GetMemorySize() const;

			uint32_t GetSizeField();

			void AppendBlock(std::shared_ptr<Block> block);
		};

		std::shared_ptr<Block> start_block;
		std::vector<Hunk> hunks;

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		static std::string ReadString(Linker::Reader& rd, uint32_t& longword_count);
		static std::string ReadString(Linker::Reader& rd);
		static void WriteString(Linker::Writer& wr, std::string name);
		static offset_t MeasureString(std::string name);

		/* * * Writer members * * */

		enum cpu_type
		{
			CPU_M68K = Block::HUNK_CODE,
			CPU_PPC = Block::HUNK_PPC_CODE,
		};
		cpu_type cpu = CPU_PPC;

		std::map<std::shared_ptr<Linker::Segment>, size_t> segment_index; /* makes it easier to look up the indices of segments */

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
