#ifndef _8BITEXE_H
#define _8BITEXE_H

#include <algorithm>
#include "binary.h"
#include "gsos.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/reader.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

/* a collection of various simple 8-bit formats */
/* TODO: should this be reorganized? */

namespace Binary
{
	/**
	 * @brief BIN file for Apple ][
	 */
	class AppleFormat : public Binary::GenericBinaryFormat
	{
	public:
		bool dos33_header;

		/* TODO: enable setting the base address as a parameter */

		AppleFormat(uint64_t default_base_address = 0x0803, std::string default_extension = "", bool dos33_header = true)
			: GenericBinaryFormat(default_base_address, default_extension), dos33_header(dos33_header)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};

	/**
	 * @brief SOS file for Apple ///
	 */
	class SOSFormat : public Binary::GenericBinaryFormat
	{
	public:
		// TODO: untested

		std::shared_ptr<Linker::Contents> optional_header;

		/* TODO: enable setting the base address as a parameter */

		SOSFormat(uint64_t default_base_address = 0x9000, std::string default_extension = "") // TODO: what would be a good default address? this is lifted from the SOS reference manual
			: GenericBinaryFormat(default_base_address, default_extension)
		{
		}

		offset_t ImageSize() const override;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};

	/**
	 * @brief This is not actually a file format, but an interface to permit generating multiple binary outputs for Apple ][ binaries.
	 *
	 * Depending on the target disk file system and tools, there are a variety of ways to prepare a binary file for Apple ][.
	 * The ones currently implemented are:
	 * - If the target file system is an Apple DOS 3.3 system, a BIN binary is prefixed with a 4 byte header. This is achieved
	 * by selecting target to be TARGET_BIN or TARGET_DOS33.
	 * - If the target file system is a ProDOS system, the file type and auxiliary file type must also be stored. This is achieved
	 * in two ways:
	 * - Either by adding a suffix of the form #TTAAAA where TT is the hexadecimal value of the file type and AAAA is the hexadecimal
	 * value of the auxiliary file type (this is referred to as NuLib2 Attribute Preservation String or NAPS in CiderPress documents)
	 * - Or by bundling the data fork alongside the ProDOS file information in an AppleSingle file.
	 */
	class AppleDriver : public Apple::GSOutputDriver
	{
	public:
		std::shared_ptr<Linker::OutputFormat> data_fork;

		enum header_format_t
		{
			/** @brief For a BIN file, there may or may not be a DOS 3.3 header (if no "+naps" is specified) */
			HEADER_BIN,
			/** @brief For a BIN file, the DOS 3.3 header is included (even if "+naps" is specified) */
			HEADER_DOS33,
			/** @brief For a BIN file, there is no DOS 3.3 header (even if no "+naps" is specified) */
			HEADER_RAW,
		};
		header_format_t header;

		file_type_t file_type;
		/* TODO: enable setting the base address as a parameter */

		AppleDriver(file_type_t file_type = FILE_TYPE_BIN, header_format_t header = HEADER_BIN)
			: GSOutputDriver(TARGET_DATA_FORK, 0),
			header(header),
			file_type(file_type)
		{
		}

		AppleDriver(file_type_t file_type, target_format_t target)
			: GSOutputDriver(target, 0),
			header(HEADER_BIN),
			file_type(file_type)
		{
		}

	protected:
		void OnContainerCreated() override;
		void OnCalculateValues() override;
		void OnReadFile(Linker::Reader& rd) override;
		offset_t OnWriteFile(Linker::Writer& wr) const override;
		void OnDump(Dumper::Dumper& dump) const override;

	public:
		void SetAppleSingleDoubleVersion(offset_t version);

		class DriverOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::optional<offset_t>> asver{"asver", "Version of the AppleSingle/AppleDouble container (recognized values: 1, 2)"};
			Linker::Option<std::optional<offset_t>> adver{"adver", "Version of the AppleSingle/AppleDouble container (recognized values: 1, 2)"};

			DriverOptionCollector()
			{
				InitializeFields(asver, adver);
			}
		};

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		void ReadFile(Linker::Reader& rd) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;
		void Dump(Dumper::Dumper& dump) const override;

		uint8_t GetFileType() const;
		uint16_t GetAuxiliaryFileType() const;

		bool UseDOS33Header() const
		{
			if(file_type != FILE_TYPE_BIN || target != OutputDriver::TARGET_DATA_FORK)
			{
				return false;
			}

			switch(header)
			{
			case HEADER_BIN:
				// only apply header if there is no NAPS suffix
				return (produce & OutputDriver::PRODUCE_NAPS_SUFFIX) == 0;
			case HEADER_DOS33:
				return true;
			default:
			case HEADER_RAW:
				return false;
			}
		}

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
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
			std::shared_ptr<Linker::Contents> image;
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
		class BASICLine : public Linker::Format
		{
		public:
			uint16_t line_address = 0;
			uint16_t line_number = 0;
			uint16_t next_address = 0;
			std::vector<uint8_t> tokens;

			/* BASIC token */
			enum Token
			{
				// using http://fileformats.archiveteam.org/wiki/Commodore_BASIC_tokenized_file
				END = 0x80,
				FOR = 0x81,
				NEXT = 0x82,
				DATA = 0x83,
				INPUT_HASH = 0x84,
				INPUT = 0x85,
				DIM = 0x86,
				READ = 0x87,
				LET = 0x88,
				GOTO = 0x89,
				RUN = 0x8A,
				IF = 0x8B,
				RESTORE = 0x8C,
				GOSUB = 0x8D,
				RETURN = 0x8E,
				REM = 0x8F,
				STOP = 0x90,
				ON = 0x91,
				WAIT = 0x92,
				LOAD = 0x93,
				SAVE = 0x94,
				VERIFY = 0x95,
				DEF = 0x96,
				POKE = 0x97,
				PRINT_HASH = 0x98,
				PRINT = 0x99,
				CONST = 0x9A,
				LIST = 0x9B,
				CLR = 0x9C,
				CMD = 0x9D,
				SYS = 0x9E,
				OPEN = 0x9F,
				CLOSE = 0xA0,
				GET = 0xA1,
				NEW = 0xA2,
				TAB_PAREN = 0xA3,
				TO = 0xA4,
				FN = 0xA5,
				SPC_PAREN = 0xA6,
				THEN = 0xA7,
				NOT = 0xA8,
				STEP = 0xA9,
				_PLUS = 0xAA,
				_MINUS = 0xAB,
				_MULTIPLY = 0xAC,
				_DIVIDE = 0xAD,
				_POWER = 0xAE,
				AND = 0xAF,
				OR = 0xB0,
				_GREATER = 0xB1,
				_EQUAL = 0xB2,
				_LESS = 0xB3,
				SGN = 0xB4,
				INT = 0xB5,
				ABS = 0xB6,
				USR = 0xB7,
				FRE = 0xB8,
				POS = 0xB9,
				SQR = 0xBA,
				RND = 0xBB,
				LOG = 0xBC,
				EXP = 0xBD,
				COS = 0xBE,
				SIN = 0xBF,
				TAN = 0xC0,
				ATN = 0xC1,
				PEEK = 0xC2,
				LEN = 0xC3,
				STR_DOLLAR = 0xC4,
				VAL = 0xC5,
				ASC = 0xC6,
				CHR_DOLLAR = 0xC7,
				LEFT_DOLLAR = 0xC8,
				RIGHT_DOLLAR = 0xC9,
				MID_DOLLAR = 0xCA,
				GO = 0xCB,
				_PI = 0xFF,
			};

			void AddToken(Token token);
			void AddString(std::string text);
			void AddDecimal(int value);
			offset_t ImageSize() const override; // including header

			void ReadFile(Linker::Reader& rd) override;
			using Linker::Format::WriteFile;
			offset_t WriteFile(Linker::Writer& wr) const override;
			void CalculateValues();
			void Dump(Dumper::Dumper& dump) const override;
			void Dump(Dumper::Dumper& dump, std::optional<uint16_t> line_index, int display_flags) const;
		};

		class BASICFile : public Linker::Format
		{
		public:
			uint16_t load_address = 0;
			std::vector<BASICLine> lines;
			uint16_t end_address = 0;

			void ReadFile(Linker::Reader& rd) override;
			using Linker::Format::WriteFile;
			offset_t ImageSize() const override;
			offset_t WriteFile(Linker::Writer& wr) const override;
			void CalculateValues();
			void Dump(Dumper::Dumper& dump) const override;
			void Dump(Dumper::Dumper& dump, int display_flags) const;
		};

		/** @brief Address at which the BASIC program should start for a Commodore PET */
		static const uint16_t PET_BASIC_START = 0x0401;
		/** @brief Address at which the BASIC program should start for a Commodore VIC-20 */
		static const uint16_t VIC_BASIC_START = 0x1001;
		/** @brief Address at which the BASIC program should start for a Commodore 64 */
		static const uint16_t C64_BASIC_START = 0x0801;

		class CommodoreOptionCollector : public Linker::OptionCollector
		{
		public:
			class SystemTypeEnumeration : public Linker::Enumeration<uint16_t>
			{
			public:
				SystemTypeEnumeration()
					: Enumeration(
						"PET", PET_BASIC_START,
						"VIC20", VIC_BASIC_START,
						"VIC-20", VIC_BASIC_START,
						"VIC", VIC_BASIC_START,
						"C64", C64_BASIC_START)
				{
					descriptions = {
						{ PET_BASIC_START, "Commodore PET" },
						{ VIC_BASIC_START, "Commodore VIC-20" },
						{ C64_BASIC_START, "Commodore 64" },
					};
				}
			};

			Linker::Option<Linker::ItemOf<SystemTypeEnumeration>> sys{"sys", "Target Commodore system type", C64_BASIC_START};
			Linker::Option<std::optional<offset_t>> load_address{"load_address", "Load address for BASIC code"};

			CommodoreOptionCollector()
			{
				InitializeFields(sys, load_address);
			}
		};

		uint16_t load_address = 0;
		std::shared_ptr<Linker::Contents> loader;

		uint16_t GetLoadAddress() const;

		void Clear() override;

		~CommodoreFormat()
		{
			Clear();
		}

		void SetupDefaultLoader();
		uint16_t GetImagePaddingSize() const;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		void ProcessModule(Linker::Module& module) override;

		void ReadFile(Linker::Reader& rd) override;
		void CalculateValues() override;
		offset_t ImageSize() const override;
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
		class CPM3OptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::optional<std::vector<std::string>>> rsx_file_names{"rsx", "List of filenames to append as Resident System Extensions"};

			CPM3OptionCollector()
			{
				InitializeFields(rsx_file_names);
			}
		};

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
			std::shared_ptr<Linker::Contents> image;

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
		class PRLOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<bool> banked{"banked", "Generated .SPR file for banked CP/M 3"};

			PRLOptionCollector()
			{
				InitializeFields(banked);
			}
		};

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

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
		void SetOptions(std::map<std::string, std::string>& options) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		bool ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution) override;

		void ProcessModule(Linker::Module& module) override;

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
		std::shared_ptr<Linker::Contents> code, data;

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
