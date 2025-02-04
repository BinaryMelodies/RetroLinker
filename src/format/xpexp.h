#ifndef XPEXP_H
#define XPEXP_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace Ergo
{
	/**
	 * @brief Ergo OS/286 and OS/386 "XP" .exp file (Ergo was formerly A.I. Architects, then Eclipse)
	 */
	class XPFormat : public virtual Linker::SegmentManager
	{
	public:
		std::string stub_file; // TODO

		struct Segment
		{
		public:
			uint16_t limit;
			uint32_t base;
			enum
			{
				ACCESS_DATA = 0x92,
				ACCESS_CODE = 0x9A,

				ACCESS_E = 0x08,
				ACCESS_S = 0x10,

				ACCESS_CPL0 = 0x00,
				ACCESS_CPL1 = 0x20,
				ACCESS_CPL2 = 0x40,
				ACCESS_CPL3 = 0x60,

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
			uint8_t access;
			enum
			{
				FLAG_ALIAS = 0x10,
				FLAG_WINDOW = 0x20,
				FLAG_32BIT = 0x40,
				FLAG_PAGES = 0x80, // instead of bytes, for limit size
			};
			uint8_t flags;

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
		uint32_t gs, fs, ds, ss, cs, es, edi, esi, ebp, esp, ebx, edx, ecx, eax, eflags, eip;
		std::vector<Segment> ldt;
		std::shared_ptr<Linker::Image> image;

		void Clear() override;
		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		offset_t ImageSize() override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* XPEXP_H */
