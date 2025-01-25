#ifndef XPEXP_H
#define XPEXP_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace Ergo
{
	/**
	 * @brief Ergo OS/286 and OS/386 "XP" .exp file (Ergo was formerly A.I. Architects, then Eclipse)
	 */
	class XPFormat : public virtual Linker::LinkerManager
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

				ACCESS_CPL0 = 0x00,
				ACCESS_CPL1 = 0x20,
				ACCESS_CPL2 = 0x40,
				ACCESS_CPL3 = 0x60,
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
			void WriteFile(Linker::Writer& wr);
		};

		uint32_t offset = 0;
		uint32_t ldt_offset = 0;
		uint32_t image_offset = 0;
		uint32_t relocation_offset = 0; // TODO: unsure
		uint32_t minimum_extent = 0;
		uint32_t maximum_extent = 0;
		uint32_t unknown_field = 0;
		uint32_t gs, fs, ds, ss, cs, es, edi, esi, ebp, esp, ebx, edx, ecx, eax, eflags, eip;
		std::vector<Segment> ldt;
		std::shared_ptr<Linker::Writable> image;

		void Clear() override;
		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* XPEXP_H */
