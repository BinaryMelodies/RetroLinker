#ifndef _8BITEXE_H
#define _8BITEXE_H

#include <algorithm>
#include "binary.h"
#include "../common.h"
#include "../dumper/dumper.h"
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

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};

	/**
	 * @brief EXE file for Atari 400/800
	 */
	class AtariFormat : public GenericBinaryFormat
	{
	public:
		/* exe, obj, com are also used */
		AtariFormat(uint64_t default_base_address = 0, std::string default_extension = ".xex")
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		/**
		 * @brief Represents a loadable segment in the binary
		 */
		struct Segment
		{
		public:
			/**
			 * @brief Types of segments, represented by their signature values
			 */
			enum segment_type : uint16_t
			{
				/** @brief Lowest currently defined signature to check on reading */
				SIGNATURE_LOW = 0xFFFA,
				/** @brief SpartaDOS X fixed-address segment (not implemented) */
				SDX_FIXED = 0xFFFA,
				/** @brief SpartaDOS X required symbols (not implemented) */
				SDX_SYMREQ = 0xFFFB,
				/** @brief SpartaDOS X symbol definitions (not implemented) */
				SDX_SYMDEF = 0xFFFC,
				/** @brief SpartaDOS X fixup information (not implemented) */
				SDX_FIXUPS = 0xFFFD,
				/** @brief SpartaDOS X RAM allocation block (not implemented) */
				SDX_RAMALLOC = 0xFFFE,
				/** @brief SpartaDOS X position independent (not implemented) */
				SDX_POSIND = 0xFFFE,
				/** @brief Atari segment type */
				ATARI_SEGMENT = 0xFFFF,
			};
			/**
			 * @brief Header type, Atari DOS uses only 0xFFFF, signature only obligatory for the first segment
			 */
			segment_type header_type = ATARI_SEGMENT;
			/**
			 * @brief Set if placing header type is optional, also set when signature is absent in file when reading
			 */
			bool header_type_optional = false;
			/**
			 * @brief Address at which segment must be loaded
			 */
			uint16_t address = 0;
			/**
			 * @brief Only used for SDX_RAMALLOC/SDX_POSIND, SDX_SYMREQ
			 */
			uint8_t block_number = 0;
			enum control_byte_type : uint8_t
			{
				/** @brief allocate in conventional RAM */
				CB_CONVRAM = 0x00,
				/** @brief allocate in system extended area */
				CB_SYSEXTAREA = 0x02,
				/** @brief (SDX 4.47+) allocate in program extended area */
				CB_PROGEXTAREA = 0x04,
				/** @brief (SDX 4.43+) page aligned */
				CB_PAGEALIGNED = 0x40,
				/** @brief SDX_RAMALLOC instead of SDX_POSIND */
				CB_RAMALLOC = 0x80,
			};
			/**
			 * @brief Only used for SDX_RAMALLOC/SDX_POSIND
			 */
			control_byte_type control_byte = control_byte_type(0);
			/**
			 * @brief Only used for SDX_RAMALLOC/SDX_POSIND
			 */
			uint16_t size = 0;
			/**
			 * @brief Only used for SDX_SYMREQ, SDX_SYMDEF
			 */
			char symbol_name[8] = { };
			/**
			 * @brief The binary data in the segment
			 */
			std::shared_ptr<Linker::Image> image;
			/**
			 * @brief Relocations, only used for SDX_SYMREQ and SDX_FIXUPS
			 */
			std::set<uint16_t> relocations; // TODO: multiple blocks?

			Segment(bool header_type_optional = true)
				: header_type(ATARI_SEGMENT), header_type_optional(header_type_optional)
			{
			}

			Segment(uint16_t header_type)
				: header_type(segment_type(header_type)), header_type_optional(false)
			{
			}

			/**
			 * @brief Retrieves the number of bytes in the segment body
			 */
			offset_t GetSize() const;

			/**
			 * @brief Reads a segment from a file into this object
			 */
			void ReadFile(Linker::Reader& rd);

			/**
			 * @brief Writes the segment into a file
			 */
			void WriteFile(Linker::Writer& wr) const;

			/**
			 * @brief Read relocations
			 */
			void ReadRelocations(Linker::Reader& rd);

			/**
			 * @brief Writes relocations
			 */
			void WriteRelocations(Linker::Writer& wr) const;
		};

		/**
		 * @brief Sequence of segments
		 */
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

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
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

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

	class PRLFormat;

	/**
	 * @brief CP/M Plus .com file format
	 */
	class CPM3Format : public GenericBinaryFormat
	{
	public:
		/** @brief Pre-initialization code to be executed before fully loading program */
		uint8_t preinit_code[10] = { 0xC9 }; /* z80 return instruction */
		/** @brief Whether loader should be active, even if no RSXs are attached */
		bool loader_active = true;
		/** @brief A single RSX record */
		struct rsx_record
		{
			/** @brief Name of RSX file to load, only used for writing */
			std::string rsx_file_name;
			/** @brief Name of RSX file, as stored inside RSX */
			std::string name;
			/** @brief Offset to RSX block */
			uint16_t offset = 0;
			/** @brief Length of RSX module, only used for reading */
			uint16_t length = 0;
			/** @brief Whether RSX is only loaded on non-banked systems */
			bool nonbanked_only = false;
			/** @brief The actual RSX data, stored in PRLFormat (on disk, without the header) */
			std::shared_ptr<PRLFormat> module;

			/** @brief Reads RSX file and prepares fields */
			void OpenAndPrepare();
		};
		/** @brief The attached RSX records */
		std::vector<rsx_record> rsx_table;

		void Clear() override;

		CPM3Format()
			: GenericBinaryFormat(0x0100, ".com")
		{
		}

		~CPM3Format()
		{
			Clear();
		}

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

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
			std::shared_ptr<Linker::Image> image;

			void WriteFile(Linker::Writer& wr) const;
		};

		std::vector<std::unique_ptr<Segment>> segments;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

	/**
	 * @brief MP/M .prl file format
	 */
	class PRLFormat : public GenericBinaryFormat
	{
	public:
		/** @brief Additional memory to allocate, similar to .bss */
		uint16_t zero_fill = 0;
		/** @brief Address to be loaded at, only used for .OVL files */
		uint16_t load_address = 0; // TODO: implement
		/** @brief Code segment length, only used for .SPR files in banked systems */
		uint16_t cslen = 0; // TODO: implement

		/** @brief Do not include relocations, only used for .OVL files */
		bool suppress_relocations;

		/** @brief On a banked BIOS, align the data segment of the .SPR on a page boundary and store the length of the code segment in the header */
		bool option_banked_bios = false; // TODO: make flag, implement behavior

		/** @brief Offsets to bytes referencing pages that must be relocated */
		std::set<uint16_t> relocations;

		/**
		 * @brief The format of the generated binary
		 *
		 * This controls the difference between the layout of the generated binary, as well as defaults such as base address and file extension.
		 * The only ones that actually require special treatment are SPR and OVL files, the other file types are included for convenience.
		 */
		enum application_type
		{
			/** @brief Unspecified */
			APPL_UNKNOWN,
			/** @brief MP/M-80 .prl executable file */
			APPL_PRL,
			/** @brief CP/M-80 Plus .rsx resident system extension file (unsupported) */
			APPL_RSX,
			/** @brief CP/M-80 2 .rsm resident system extension file (unsupported) */
			APPL_RSM,
			/** @brief MP/M-80 .rsp resident system process file (unsupported) */
			APPL_RSP,
			/** @brief MP/M-80 .brs banked resident system process file (unsupported) */
			APPL_BRS,
			/** @brief MP/M-80 .spr system module file (unsupported) */
			APPL_SPR,
			/** @brief CP/M-80 .ovl overlay (unsupported) */
			APPL_OVL,
		};
		/** @brief Target application type */
		application_type application;

		static uint16_t GetDefaultBaseAddress(application_type application);
		static std::string GetDefaultApplicationExtension(application_type application);

		PRLFormat(application_type application = APPL_PRL)
			:
				GenericBinaryFormat(GetDefaultBaseAddress(application), GetDefaultApplicationExtension(application)),
				suppress_relocations(application == APPL_OVL),
				application(application)
		{
		}

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		bool ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution) override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		/** @brief Read without header, only needed for RSX files stored inside a CP/M 3 .COM file */
		void ReadWithoutHeader(Linker::Reader& rd, uint16_t image_size);

		/** @brief Write without header, only needed for RSX files stored inside a CP/M 3 .COM file */
		void WriteWithoutHeader(Linker::Writer& wr) const;

		void Dump(Dumper::Dumper& dump) const override;
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

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};

	/**
	 * @brief UZI280 file format
	 */
	class UZI280Format : public GenericBinaryFormat
	{
	public:
		std::shared_ptr<Linker::Image> code, data;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		/* TODO: apparently both .code and .data are loaded at 0x0100 */

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) const override;
	};
}

#endif /* _8BITEXE_H */
