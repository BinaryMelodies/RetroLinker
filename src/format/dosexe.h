#ifndef DOSEXE_H
#define DOSEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/segment_manager.h"
#include "../linker/section.h"
#include "mzexe.h"

namespace SeychellDOS32
{
	/**
	 * @brief Adam Seychell's DOS32 "Adam" executable format
	 */
	class AdamFormat : public virtual Linker::Format, public virtual Linker::OutputFormat
	{
	public:
		bool is_v35; /* based on Michael Tippach's research */

		AdamFormat(bool is_v35 = false)
			: is_v35(is_v35)
		{
			if(is_v35)
			{
				Linker::FatalError("Fatal error: 3.5 format unimplemented");
			}
		}

		bool is_dll = false;
		uint16_t minimum_dos_version = 0;
		uint16_t dlink_version = 0;
		uint32_t image_size = 0;
		uint32_t header_size = 0;
		uint32_t extra_memory_size = 0;
		uint32_t eip = 0;
		uint32_t esp = 0;
		std::set<uint32_t> relocations;
		uint32_t flags = 0;
		uint32_t relocation_start = 0;
		/** @brief Unknown field for version 3.5 */
		uint32_t last_header_field = 0;

		std::shared_ptr<Linker::Image> image;

		enum
		{
			FLAG_COMPRESSED = 0x0001,
			FLAG_DISPLAY_LOGO = 0x0002,
		};

		void CalculateValues() override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
};

namespace BrocaD3X
{
	/**
	 * @brief Daniel Broca's D3X executable format
	 */
	class D3X1Format : public virtual Linker::Format
	{
	public:
		uint32_t header_size = 0;
		uint32_t binary_size = 0;
		uint32_t extra_size = 0;
		uint32_t entry = 0;
		uint32_t stack_top = 0;

		D3X1Format()
			: header_size(24)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
};

namespace DX64
{
	/**
	 * @brief CandyMan's DX64 "Flat" and "LV" executable formats
	 */
	class LVFormat : public virtual Linker::Format
	{
	public:
		enum format_type
		{
			FORMAT_FLAT,
			FORMAT_LV,
		};

		char signature[4];
		uint32_t eip = 0;
		uint32_t esp = 0;
		uint32_t extra_memory_size = 0;
		std::shared_ptr<Linker::Image> image;

		explicit LVFormat()
		{
		}

		LVFormat(format_type type)
		{
			SetSignature(type);
		}

		void SetSignature(format_type type);

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
}

/* TODO: other formats? */

#endif /* DOSEXE_H */
