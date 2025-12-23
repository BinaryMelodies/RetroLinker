#ifndef CPM8K_H
#define CPM8K_H

#include <map>
#include <set>
#include <string>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace DigitalResearch
{
	/**
	 * @brief CP/M-8000 .z8k file format
	 */
	class CPM8KFormat : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		/**
		 * @brief Represents a segment within the module
		 */
		class Segment
		{
		public:
			/**
			 * @brief Each segment has an associated number. For 0xFF, the linker can assign a value. For segmented executables, this is the segment number that will be used
			 */
			uint8_t number = 0xFF;
			/**
			 * @brief The type of a segment
			 */
			enum segment_type
			{
				BSS = 1,
				STACK,
				CODE,
				RODATA,
				DATA,
				MIXED,
				MIXED_PROTECTABLE,
			};
			/**
			 * @brief The type of the segment
			 */
			segment_type type = segment_type(0);
			/**
			 * @brief Length of segment in bytes
			 */
			uint16_t length = 0;
			/**
			 * @brief Storage for segment
			 */
			std::shared_ptr<Linker::Contents> image = nullptr;

			void Clear();

			bool IsPresent() const;
		};

		struct Relocation
		{
			/**
			 * @brief The source segment of the relocation
			 */
			uint8_t segment;
			enum relocation_type
			{
				/** @brief A 16-bit offset to segment */
				SEG_OFFSET = 1,
				/** @brief A 16-bit segmented address of segment */
				SEG_SHORT_SEGMENTED = 2,
				/** @brief A 32-bit segmented address of segment */
				SEG_LONG_SEGMENTED = 3,
				/** @brief A 16-bit offset to external item */
				EXT_OFFSET = 5,
				/** @brief A 16-bit segmented address of external item */
				EXT_SHORT_SEGMENTED = 6,
				/** @brief A 32-bit segmented address of external item */
				EXT_LONG_SEGMENTED = 7,
			};
			relocation_type type;
			/**
			 * @brief Source offset of relocation
			 */
			uint16_t offset;
			/**
			 * @brief The segment or symbol number that the relocation references
			 */
			uint16_t target;

			constexpr bool IsExternal() const
			{
				return type == EXT_OFFSET || type == EXT_SHORT_SEGMENTED || type == EXT_LONG_SEGMENTED;
			}

			constexpr size_t GetRelocationSize() const
			{
				switch(type)
				{
				case SEG_OFFSET:
				case SEG_SHORT_SEGMENTED:
				case EXT_OFFSET:
				case EXT_SHORT_SEGMENTED:
					return 2;
				case SEG_LONG_SEGMENTED:
				case EXT_LONG_SEGMENTED:
					return 4;
				default:
					return 0;
				}
			}

			static Relocation ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr) const;
		};

		struct Symbol
		{
			/* TODO */
		};

		enum magic_type
		{
			MAGIC_SEGMENTED_OBJECT = 0xEE00,
			MAGIC_SEGMENTED        = 0xEE01,
			MAGIC_NONSHARED_OBJECT = 0xEE02,
			MAGIC_NONSHARED        = 0xEE03,
			MAGIC_SHARED_OBJECT    = 0xEE06,
			MAGIC_SHARED           = 0xEE07,
			MAGIC_SPLIT_OBJECT     = 0xEE0A,
			MAGIC_SPLIT            = 0xEE0B,
		};

		/** @brief The magic number at the beginning of the executable file */
		char signature[2];
		/** @brief Number of segments in the segment_array */
		uint16_t segment_count = 0;
		/** @brief Total number of bytes in all the segments combined */
		uint32_t total_size = 0;
		/** @brief Total size of relocations */
		uint32_t relocation_size = 0;
		/** @brief Total size of symbols */
		uint32_t symbol_table_size = 0;

		std::vector<Segment> segments;
		std::vector<Relocation> relocations;
		std::vector<Symbol> symbols;

		magic_type GetSignature() const;

		void SetSignature(magic_type magic);

		void Clear() override;

		CPM8KFormat(magic_type magic = MAGIC_NONSHARED)
		{
			SetSignature(magic);
		}

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;

		void CalculateValues() override;

		/* * * Writer members * * */

		/** @brief Segment to collect bss */
		std::shared_ptr<Linker::Segment> bss_segment;

		bool FormatSupportsSegmentation() const override;

		std::vector<std::shared_ptr<Linker::Segment>>& Segments();

		unsigned GetSegmentNumber(std::shared_ptr<Linker::Segment> segment);

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		bool IsCombined() const;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* CPM8K_H */
