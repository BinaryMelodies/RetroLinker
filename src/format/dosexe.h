#ifndef DOSEXE_H
#define DOSEXE_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/section.h"
#include "mzexe.h"

namespace SeychellDOS32
{
	/**
	 * @brief Adam Seychell's DOS32 "Adam" executable format
	 */
	class AdamFormat : public virtual Linker::Format
	{
	public:
		bool is_v35; /* based on Michael Tippach's research */

		AdamFormat(bool is_v35 = false)
			: is_v35(is_v35)
		{
			if(is_v35)
			{
				Linker::Error << "Fatal Error: 3.5 format unimplemented" << std::endl;
				assert(false);
			}
		}

		bool is_dll;
		uint16_t minimum_dos_version;
		uint16_t dlink_version;
		uint32_t relocation_size;
		uint32_t header_size;
		uint32_t extra_memory_size;
		uint32_t eip;
		uint32_t esp;
		std::set<uint32_t> relocations;
		uint32_t flags;

		std::shared_ptr<Linker::Writable> image;

		enum
		{
			FLAG_COMPRESSED = 0x0001,
			FLAG_DISPLAY_LOGO = 0x0002,
		};

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
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
		uint32_t header_size;
		uint32_t binary_size;
		uint32_t extra_size;
		uint32_t entry;
		uint32_t stack_top;

		D3X1Format()
			: header_size(24)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};
};

namespace DX64
{
	/**
	 * @brief CandyMan's DX64 "Flat" executable format
	 */
	class FlatFormat : public virtual Linker::Format
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief CandyMan's DX64 "LV" executable format
	 */
	class LVFormat : public virtual Linker::Format
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};
}

/* TODO: other formats? */

#endif /* DOSEXE_H */
