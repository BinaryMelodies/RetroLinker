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
	class AppleFormat : public Binary::GenericBinaryFormat
	{
	public:
		/* TODO: untested */

		/* TODO: enable setting the base address as a parameter */
		/* TODO: SYS files are pure binary loaded at 0x2000 */

		AppleFormat(uint64_t default_base_address = 0x0803, std::string default_extension = ".bin")
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief EXE file for Atari 400/800
	 */
	class AtariFormat : public GenericBinaryFormat
	{
	public:
		/* TODO: untested */

		/* TODO: enable setting the base address, default should be ??? */

		/* exe, obj, com are also used */
		AtariFormat(uint64_t default_base_address = 0, std::string default_extension = ".xex")
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		struct Segment
		{
		public:
			uint16_t header;
			bool header_optional;
			uint16_t address;
			std::shared_ptr<Linker::Writable> image;

			Segment(bool header_optional = true)
				: header(0xFFFF), header_optional(header_optional), address(0), image(nullptr)
			{
			}

			Segment(uint16_t header)
				: header(header), header_optional(false), address(0), image(nullptr)
			{
			}

//			~Segment()
//			{
//				if(image)
//					delete image;
//			}

			offset_t GetSize() const;

			void ReadFile(Linker::Reader& rd);

			void WriteFile(Linker::Writer& wr);
		};

		std::vector<std::unique_ptr<Segment>> segments;

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

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void ProcessModule(Linker::Module& module) override;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
	};

	/**
	 * @brief PRG file for Commodore PET/VIC-20/64
	 */
	class CommodoreFormat : public GenericBinaryFormat
	{
	public:
		/* TODO */

		/** @brief Address at which the BASIC program should start */
		static const uint16_t BASIC_START = 0x0801;

		enum
		{
			BASIC_SYS = 0x9E, /* BASIC token */
		};

		std::shared_ptr<Linker::Segment> loader; /* loader routine in BASIC */

		void Clear() override;

		~CommodoreFormat()
		{
			Clear();
		}

		void SetupDefaultLoader();

		void ProcessModule(Linker::Module& module) override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	class PRLFormat;

	/**
	 * @brief CP/M Plus .com file format
	 */
	class CPM3Format : public GenericBinaryFormat
	{
	public:
		/* TODO: untested */

		uint8_t preinit_code[10] = { 0xC9 }; /* z80 return instruction */
		bool loader_active = true;
		uint8_t rsx_count = 0;
		struct rsx_record
		{
			std::string rsx_file_name;

			std::string name;
			uint16_t offset = 0;
			uint16_t length = 0; /* only used for reading */
			bool nonbanked_only = false;
			std::shared_ptr<PRLFormat> module;

			void OpenAndPrepare();
		};
		std::vector<rsx_record> rsx_table;

		void Clear() override;

		~CPM3Format()
		{
			Clear();
		}

		void SetOptions(std::map<std::string, std::string>& options) override;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		void CalculateValues() override;
	};

	/**
	 * @brief FLEX .cmd file format
	 */
	class FLEXFormat : public GenericBinaryFormat
	{
	public:
		/* TODO: enable setting the base address, default should be ??? */

		struct Segment
		{
		public:
			uint16_t address;
			uint16_t size; /* it is supposed to be at most 255, but we can store larger segments by cutting them into pieces */
			std::shared_ptr<Linker::Writable> image;

			void WriteFile(Linker::Writer& wr);
		};

		std::vector<std::unique_ptr<Segment>> segments;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void WriteFile(Linker::Writer& wr) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

	/**
	 * @brief MP/M .prl file format
	 */
	class PRLFormat : public GenericBinaryFormat
	{
	public:
		/* TODO: untested */

		uint16_t zero_fill = 0;
		uint16_t load_address = 0;
		uint16_t csbase = 0;

		bool suppress_relocations = false;

		std::set<uint16_t> relocations;

		PRLFormat(uint64_t default_base_address = 0, std::string default_extension = ".prl")
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		bool ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution) override;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;
		void WriteWithoutHeader(Linker::Writer& wr);

		void Dump(Dumper::Dumper& dump) override;
	};

	/**
	 * @brief UZI/UZI280 file formats
	 */
	class UZIFormat : public GenericBinaryFormat
	{
	public:
		/* TODO */
		bool uzi180_header;
		uint16_t entry;

		void ProcessModule(Linker::Module& module) override;

		void WriteFile(Linker::Writer& wr) override;

		using GenericBinaryFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};

	/**
	 * @brief UZI280 file format
	 */
	class UZI280Format : public GenericBinaryFormat
	{
	public:
		std::shared_ptr<Linker::Writable> code, data;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		/* TODO: apparently both .code and .data are loaded at 0x0100 */

		void WriteFile(Linker::Writer& wr) override;

		using GenericBinaryFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};
}

#endif /* _8BITEXE_H */
