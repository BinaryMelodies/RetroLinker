#ifndef _8BITEXE_H
#define _8BITEXE_H

#include <algorithm>
#include "binary.h"
#include "../common.h"
#include "../linker/module.h"
#include "../linker/reader.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

/* TODO: this is all pretty much preliminary */
/* TODO: combine into binary.h? */

namespace Binary
{
	/**
	 * @brief BIN file for Apple ][
	 */
	class AppleFormat : public Binary::BinaryFormat
	{
	public:
		/* TODO: untested */

		/* TODO: enable setting the base address as a parameter */
		/* TODO: SYS files are pure binary loaded at 0x2000 */

		AppleFormat(uint64_t default_base_address = 0x0803, std::string default_extension = ".bin")
			: BinaryFormat(default_base_address, default_extension)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief EXE file for Atari 400/800
	 */
	class AtariFormat : public BinaryFormat
	{
	public:
		/* TODO: untested */

		/* TODO: enable setting the base address, default should be ??? */

		/* exe, obj, com are also used */
		AtariFormat(uint64_t default_base_address = 0, std::string default_extension = ".xex")
			: BinaryFormat(default_base_address, default_extension)
		{
		}

		struct Segment
		{
		public:
			uint16_t header;
			bool header_optional;
			uint16_t address;
			Linker::Writable * image;

			Segment(bool header_optional = true)
				: header(0xFFFF), header_optional(header_optional), address(0), image(nullptr)
			{
			}

			Segment(uint16_t header)
				: header(header), header_optional(false), address(0), image(nullptr)
			{
			}

			~Segment()
			{
				if(image)
					delete image;
			}

			offset_t GetSize() const;

			void ReadFile(Linker::Reader& rd);

			void WriteFile(Linker::Writer& wr);
		};

		std::vector<Segment *> segments;

		/** @brief Address which contains a loader between to execute between loading segments */
		static const uint16_t LOADER_ADDRESS = 0x02E2;

		/** @brief Address which contains the actual entry address after loading
		 *
		 * Note the indirection, this is not the entry point of the program.
		 */
		static const uint16_t ENTRY_ADDRESS = 0x02E0;

		/** @brief An entry point is present if the memory address at EntryAddress has been filled by a segment */
		bool HasEntryPoint() const;
		/** @brief Attaches a new segment that contains the entry point */
		void AddEntryPoint(uint16_t entry);

		void OnNewSegment(Linker::Segment * segment) override;

		void ProcessModule(Linker::Module& module) override;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief PRG file for Commodore PET/VIC-20/64
	 */
	class CommodoreFormat : public BinaryFormat
	{
	public:
		/* TODO */

		/** @brief Address at which the BASIC program should start */
		static const uint16_t BASIC_START = 0x0801;

		enum
		{
			BASIC_SYS = 0x9E, /* BASIC token */
		};

		Linker::Segment * loader; /* loader routine in BASIC */

		void Initialize() override;

		void Clear() override;

		CommodoreFormat()
		{
			Initialize();
		}

		~CommodoreFormat()
		{
			Clear();
		}

		void SetupDefaultLoader();

		void ProcessModule(Linker::Module& module) override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief CP/M Plus .com file format
	 */
	class CPM3Format : public BinaryFormat
	{
	public:
		/* TODO: untested */

		uint8_t preinit_code[10];
		bool loader_active;
		uint8_t rsx_count;
		struct rsx_record
		{
			std::string name;
			uint16_t offset;
			uint16_t length; /* only used for reading */
			bool nonbanked_only;
			BinaryFormat * module;
		};
		std::vector<rsx_record> rsx_table;

		void Initialize() override;

		void Clear() override;

		CPM3Format()
		{
			Initialize();
		}

		~CPM3Format()
		{
			Clear();
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief FLEX .cmd file format
	 */
	class FLEXFormat : public BinaryFormat
	{
	public:
		/* TODO: enable setting the base address, default should be ??? */

		struct Segment
		{
		public:
			uint16_t address;
			uint16_t size; /* it is supposed to be at most 255, but we can store larger segments by cutting them into pieces */
			Linker::Writable * image;

			void WriteFile(Linker::Writer& wr);
		};

		std::vector<Segment *> segments;

		void OnNewSegment(Linker::Segment * segment) override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief MP/M .prl file format
	 */
	class PRLFormat : public BinaryFormat
	{
	public:
		/* TODO: untested */

		uint16_t zero_fill;

		bool suppress_relocations;

		std::set<uint16_t> relocations;

		PRLFormat(uint64_t default_base_address = 0, std::string default_extension = ".prl")
			: BinaryFormat(default_base_address, default_extension)
		{
		}

		void OnNewSegment(Linker::Segment * segment) override;

		bool ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief UZI/UZI280 file formats
	 */
	class UZIFormat : public BinaryFormat
	{
	public:
		/* TODO */
		bool uzi180_header;
		uint16_t entry;

		void ProcessModule(Linker::Module& module) override;

		void WriteFile(Linker::Writer& wr) override;

		using BinaryFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};

	/**
	 * @brief UZI280 file format
	 */
	class UZI280Format : public BinaryFormat
	{
	public:
		Linker::Writable * code, * data;

		void OnNewSegment(Linker::Segment * segment) override;

		/* TODO: apparently both .code and .data are loaded at 0x0100 */

		void WriteFile(Linker::Writer& wr) override;

		using BinaryFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};
}

#endif /* _8BITEXE_H */
