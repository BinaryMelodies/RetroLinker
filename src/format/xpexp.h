#ifndef XPEXP_H
#define XPEXP_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/options.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"
#include "mzexe.h"

/* TODO: unimplemented */

namespace Ergo
{
	/**
	 * @brief Ergo OS/286 and OS/386 "XP" .exp file (Ergo was formerly A.I. Architects, then Eclipse)
	 */
	class XPFormat : public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		struct Segment
		{
		public:
			uint32_t base = 0;
			uint32_t limit = 0;
			enum
			{
				ACCESS_DATA = 0xF2, // writable ring 3 data
				ACCESS_CODE = 0xFA, // readable ring 3 code

				ACCESS_A = 0x01,
				ACCESS_W = 0x02, // data only
				ACCESS_R = 0x02, // code only
				ACCESS_E = 0x04, // data only
				ACCESS_C = 0x04, // code only

				ACCESS_X = 0x08,
				ACCESS_S = 0x10,

				ACCESS_CPL0 = 0x00,
				ACCESS_CPL1 = 0x20,
				ACCESS_CPL2 = 0x40,
				ACCESS_CPL3 = 0x60,

				ACCESS_P = 0x80,

				ACCESS_TYPE_EMPTY = 0x00,
				ACCESS_TYPE_TSS16_A = 0x01,
				ACCESS_TYPE_LDT = 0x02,
				ACCESS_TYPE_TSS16_B = 0x03,
				ACCESS_TYPE_CALLGATE16 = 0x04,
				ACCESS_TYPE_TASKGATE = 0x05,
				ACCESS_TYPE_INTGATE16 = 0x06,
				ACCESS_TYPE_TRAPGATE16 = 0x07,

				ACCESS_TYPE_TSS32_A = 0x09,

				ACCESS_TYPE_TSS32_B = 0x0B,
				ACCESS_TYPE_CALLGATE32 = 0x0C,

				ACCESS_TYPE_INTGATE32 = 0x0E,
				ACCESS_TYPE_TRAPGATE32 = 0x0F,
			};
			uint8_t access = 0;
			enum
			{
				FLAG_ALIAS = 0x10,
				FLAG_WINDOW = 0x20,
				FLAG_32BIT = 0x40,
				FLAG_PAGES = 0x80, // instead of bytes, for limit size
			};
			uint8_t flags = 0;

			Segment()
			{
			}

			Segment(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
				: base(base), limit(limit), access(access), flags(flags)
			{
			}

			static Segment ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr) const;
			void Dump(Dumper::Dumper& dump, const XPFormat& xp, unsigned index) const;
		};

		uint32_t ldt_offset = 0;
		uint32_t image_offset = 0;
		uint32_t relocation_offset = 0; // TODO: unsure
		uint32_t relocation_count = 0; // TODO: replace with std::vector<Relocation> once the relocation format is known
		uint32_t minimum_extent = 0;
		uint32_t maximum_extent = 0;
		uint32_t unknown_field = 0;
		uint32_t gs = 0, fs = 0, ds = 0, ss = 0, cs = 0, es = 0, edi = 0, esi = 0, ebp = 0, esp = 0, ebx = 0, edx = 0, ecx = 0, eax = 0, eflags = 0, eip = 0;
		std::vector<Segment> ldt;
		std::shared_ptr<Linker::Image> image;

		void Clear() override;
		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		offset_t ImageSize() const override;
		void Dump(Dumper::Dumper& dump) const override;

		/* * * Writer members * * */
		class XPOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<bool> dual_selector{"dualsel", "Always generate pairs of code/data selectors"};
			Linker::Option<bool> no_intermediate_selector{"nointer", "Do not generate selectors for segments inside groups"};

			XPOptionCollector()
			{
				InitializeFields(stub, dual_selector, no_intermediate_selector);
			}
		};

		mutable Microsoft::MZStubWriter stub;

		enum Wordsize
		{
			Unknown = 0,
			Word = 2,
			DWord = 4,
		};
		Wordsize wordsize = Unknown;

		/**
		 * @brief Generate code/data selector pairs for all segments
		 *
		 * Normally, only certain segments get both selectors generated.
		 * This flag makes all segments appear twice, once as code and once as data.
		 */
		bool option_create_selector_pairs = false;
		/**
		 * @brief No selectors are generated for segments in groups.
		 */
		bool option_no_intermediate_selectors = false; // TODO: groups are not yet implemented

		/** @brief Represents the memory model of the running executable, which is the way in which the segments are set up during execution */
		enum memory_model_t
		{
			/** @brief Default model, same as small */
			MODEL_DEFAULT,
			/** @brief Small model, separate code and data segments */
			MODEL_SMALL,
			/** @brief Compact model, separate code and multiple data segments */
			MODEL_COMPACT,
		};
		/** @brief Memory model of generated executable */
		memory_model_t memory_model = MODEL_DEFAULT;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatIsProtectedMode() const override;
		static std::vector<Linker::OptionDescription<void>> MemoryModelNames;

		std::vector<Linker::OptionDescription<void>> GetMemoryModelNames() override;

		void SetModel(std::string model) override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		/**
		 * @brief Create the required segments, if they have not already been allocated.
		 * The XP format uses a single segment.
		 */
		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		/**
		 * @brief Link application according to script or memory model
		 */
		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		std::shared_ptr<Linker::Segment> GetImageSegment();

		class Group
		{
		public:
			size_t first_section;
			size_t section_count;

			offset_t GetStartAddress(XPFormat * format) const;
			offset_t GetLength(XPFormat * format) const;
		};
		std::vector<Group> section_groups;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};
}

#endif /* XPEXP_H */
