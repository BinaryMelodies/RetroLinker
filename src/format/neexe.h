#ifndef NEEXE_H
#define NEEXE_H

#include <array>
#include "mzexe.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/linker_manager.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace Microsoft
{
	/**
	 * @brief NE .EXE new executable file format
	 *
	 * A segmented 16-bit format First introduced for Windows, it supported multiple segments, resources, dynamic libraries among others.
	 * It was later also adopted for the following targets:
	 * - 16-bit Windows applications
	 * - 16-bit OS/2 applications
	 * - a Multitasking variant of MS-DOS referred to as European MS-DOS 4.0
	 */
	class NEFormat : public virtual Linker::LinkerManager, protected Microsoft::MZStubWriter, public std::enable_shared_from_this<NEFormat>
	{
	public:
		/* * * General members * * */

		/** @brief A name and ordinal pair, as used for resident and non-resident names */
		struct Name
		{
		public:
			std::string name;
			uint16_t ordinal = 0;
		};

		/** @brief Represents an NE segment as stored in the segment table and segment data */
		class Segment
		{
		public:
			std::shared_ptr<Linker::Image> image;
			offset_t data_offset = 0;
			/** @brief Size of segment as stored in the file, only used during reading */
			offset_t image_size = 0;
			enum flag_type
			{
				Data = 1, Code = 0,
				Allocated = 2,
				Loaded = 4, /* RealMode = 4 */ /* TODO */
				Iterated = 8, /* TODO */
				Movable = 0x10, Fixed = 0,
				Shareable = 0x20,
				Preload = 0x40, LoadOnCall = 0,
				ExecuteOnly = 0x80, ReadOnly = 0x80|Data,
				Relocations = 0x0100,
				DebugInfo = 0x0200,
				Discardable = 0x1000,
			};
			flag_type flags = flag_type(0);
			/** @brief Size of segment as stored in memory */
			uint32_t total_size = 0;
			/** @brief Entry number for movable segments (field not present in segment table)
			 *
			 * When generating a file, for movable segments, each relocation targetting it needs one and only one entry
			 */
			uint16_t movable_entry_index = 0;

			Segment()
			{
			}

			Segment(std::shared_ptr<Linker::Segment> segment, unsigned flags)
				: image(segment), flags(flag_type(flags))
			{
			}

			enum
			{
				PrivilegeLevelShift = 10,
			};

			class Relocation
			{
			public:
				enum source_type
				{
					Offset8 = 0,
					Selector16 = 2,
					Pointer32 = 3,
					Offset16 = 5,
					Pointer48 = 11,
					Offset32 = 13,
				};
				source_type type = source_type(0);
				enum flag_type
				{
					Internal = 0,
					ImportOrdinal = 1,
					ImportName = 2,
					OSFixup = 3,
					Additive = 4,
				};
				flag_type flags = flag_type(0);
				uint16_t offset = 0;
				union
				{
					uint16_t module;
					uint8_t segment;
				};
				enum
				{
					FIARQQ = 1, FJARQQ = 1,
					FISRQQ = 2, FJSRQQ = 2,
					FICRQQ = 3, FJCRQQ = 3,
					FIERQQ = 4,
					FIDRQQ = 5,
					FIWRQQ = 6,
				};
				uint16_t target = 0;
				Relocation()
					: module(0)
				{
				}

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint16_t target)
					: type(source_type(type)), flags(flag_type(flags)), offset(offset), module(module), target(target)
				{
				}

				static source_type GetType(Linker::Relocation& rel);
				size_t GetSize() const;
			};
			std::map<uint16_t, Relocation> relocations;

			void AddRelocation(const Relocation& rel);
		};

		/** @brief Represents an entry into the binary, typically DLL exported procedures */
		class Entry
		{
		public:
			enum entry_type
			{
				Unused,
				Fixed,
				Movable,
			};
			entry_type type = Unused;
			uint8_t segment = 0;
			enum flag_type
			{
				Exported = 1,
				SharedData = 2,
			};
			flag_type flags = flag_type(0);
			uint16_t offset = 0;

			enum
			{
				WordCountShift = 3,

				INT_3Fh = 0x3FCD,
			};

			Entry()
			{
			}

			Entry(unsigned type, uint8_t segment, unsigned flags, uint16_t offset)
				: type(entry_type(type)), segment(segment), flags(flag_type(flags)), offset(offset)
			{
			}

			offset_t GetEntrySize() const;

			uint8_t GetIndicatorByte() const;

			static Entry ReadEntry(Linker::Reader& rd, uint8_t indicator_byte);
			void WriteEntry(Linker::Writer& wr);
		};

		class Resource : public Segment
		{
		public:
			enum Windows
			{
				RT_CURSOR = 0x0001,
				RT_BITMAP = 0x0002,
				RT_ICON = 0x0003,
				RT_MENU = 0x0004,
				RT_DIALOG = 0x0005,
				RT_STRING = 0x0006,
				RT_FONTDIR = 0x0007,
				RT_FONT = 0x0008,
				RT_ACCELERATOR = 0x0009,
				RT_RCDATA = 0x000A,
				RT_MESSAGETABLE = 0x000B,
				RT_GROUP_CURSOR = 0x000C,
				RT_GROUP_ICON = 0x000E,
				RT_VERSION = 0x0010,
				RT_DLGINCLUDE = 0x0011,
				RT_PLUGPLAY = 0x0013,
				RT_VXD = 0x0014,
				RT_ANICURSOR = 0x0015,
				RT_ANIICON = 0x0016,
				RT_HTML = 0x0017,
				RT_MANIFEST = 0x0018,

				OS2_POINTER = 1,
				OS2_BITMAP = 2,
				OS2_MENU = 3,
			};

			uint16_t type_id = 0;
			std::string type_id_name;
			uint16_t flags = 0;
			uint16_t id = 0;
			std::string id_name;
			uint16_t handle = 0;
			uint16_t usage = 0;
		};

		class ResourceType
		{
		public:
			uint16_t type_id = 0;
			std::string type_id_name;
			std::vector<Resource> resources;
		};

		/** @brief The signature, almost always "NE" */
		std::array<char, 2> signature{'N', 'E'};

		/** @brief Version number */
		struct version
		{
			uint8_t major, minor;
		};
		/** @brief Version of the linker */
		version linker_version{1, 0};

		uint32_t crc32 = 0;

		enum program_flag_type : uint8_t
		{
			/** @brief There are no automatic data segments */
			NODATA = 0,
			/** @brief There is a single, shared automatic segment, used for libraries (DLL)
			 *
			 * Cannot be set with MULTIPLEDATA at the same time
			 */
			SINGLEDATA = 1,
			/** @brief There is an automatic data segment for each task instance, used for applications (EXE)
			 *
			 * Cannot be set with SINGLEDATA at the same time
			 */
			MULTIPLEDATA = 2,

			GLOBAL_INITIALIZATION = 4,
			PROTECTED_MODE_ONLY = 8,
			CPU_8086 = 0x10, /* this is not how all systems handle this */
			CPU_80286 = 0x20,
			CPU_80386 = 0x40,
			CPU_8087 = 0x80,
		};
		/** @brief Properties of the program during execution */
		program_flag_type program_flags = program_flag_type(0);

		enum application_flag_type : uint8_t
		{
			/** @brief Application can only run in full screen mode
			 *
			 * This is typically the case for programs that directly access video memory
			 */
			FULLSCREEN = 1,
			/** @brief Application is well behaved when running in windowed mode
			 *
			 * This is typically the case for programs that are text/console only but use BIOS and/or DOS calls to access the screen, as these can be translated by the system to windowing functions
			 */
			GUI_AWARE = 2,
			/** @brief Graphical application */
			GUI = 3,

			FAMILY_APPLICATION = 8, /* first segment loads application */
			ERROR_IN_IMAGE = 0x20,

			/** @brief Set for library modules (DLL) */
			LIBRARY = 0x80,
		};
		/** @brief Properties of the application */
		application_flag_type application_flags = application_flag_type(0);

		/** @brief Automatic data segment number, starting from 1, only used for SINGLEDATA/MULTIPLEDATA */
		uint16_t automatic_data = 0;

		/** @brief Size of initial stack, added to the size of the automatic data segment */
		uint16_t stack_size = 0;
		/** @brief Size of initial heap, added to the size of the automatic data segment */
		uint16_t heap_size = 0;

		uint16_t ip = 0, cs = 0, sp = 0, ss = 0;

		// The following fields are listed in the order the parts appearing in the binary instead of the fields in the header

		/** @brief Offset of segment table
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t segment_table_offset = 0;

		std::vector<Segment> segments;

		/** @brief Offset of resource table
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t resource_table_offset = 0;

		uint16_t resource_count = 0;

		/** @brief Specifies the shift count to get the actual offsets for the resource data */
		uint16_t resource_shift = 0;

		/** @brief For non-OS/2 targets, this must be compiled into resource_types */
		std::vector<Resource> resources;

		std::vector<ResourceType> resource_types;

		/** @brief Offset of resident name table, containing this module's name and the names and ordinals of procedures exported by name
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t resident_name_table_offset = 0;

		std::vector<Name> resident_names;

		/** @brief Offset of module reference table, containing 16-bit offsets to the names of the imported modules in the imported names table
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t module_reference_table_offset = 0;

		std::vector<uint16_t> module_references;

		/** @brief Offset of imported names table, containing the names of imported modules and procedures imported by name
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t imported_names_table_offset = 0;

		std::vector<std::string> imported_names;

		/** @brief Offset of non-resident names table, containing this module's description and the names of procedures exported by ordinal
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t nonresident_name_table_offset = 0;
		uint32_t nonresident_name_table_length = 0;

		std::vector<Name> nonresident_names;

		/** @brief Offset of entry table
		 *
		 * The image contains a 16-bit offset from the start of the NE header, but this value stores it from the beginning of the file
		 */
		uint32_t entry_table_offset = 0;

		/** @brief Size of entry table in bytes */
		uint32_t entry_table_length = 0;

		/** @brief Number of movable entries in the entry table */
		uint16_t movable_entry_count = 0;

		std::vector<Entry> entries;

		/** @brief Specifies the shift count to get the actual offsets of segments */
		uint16_t sector_shift = 9;

		enum system_type : uint8_t
		{
			/** @brief Used for OS/2 1.0 - 1.3 */
			OS2 = 1,
			/** @brief Used for Windows 1.0 - 3.11 */
			Windows,
			/** @brief Signifies Multitasking MS-DOS 4.0 */
			MSDOS4,
			Windows386,
			BorlandOSS,
			PharLap = 0x80,
		};
		system_type system = system_type(0);

		enum additional_flag_type : uint8_t
		{
			SUPPORT_LONGFILENAME = 1,
			WIN20_PROTECTED_MODE = 2,
			WIN20_PROPORTIONAL_FONTS = 4,
			FAST_LOAD_AREA = 8,
		};
		additional_flag_type additional_flags = additional_flag_type(0);

		union
		{
			/* Windows */
			uint16_t fast_load_area_offset;
			/* OS/2 ? */
			uint16_t return_thunks_offset;
		};
		union
		{
			/* Windows */
			uint16_t fast_load_area_length;
			/* OS/2 ? */
			uint16_t segment_reference_thunks_offset;
		};
		uint16_t code_swap_area_length = 0;

		version windows_version{0, 0};

		offset_t file_size = offset_t(-1);

		bool IsLibrary() const;

		bool IsOS2() const;

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		explicit NEFormat()
		{
		}

		NEFormat(system_type system, unsigned program_flags, unsigned application_flags)
			: program_flags(program_flag_type(program_flags)), application_flags(application_flag_type(application_flags)), system(system),
			fast_load_area_offset(0), fast_load_area_length(0)
		{
		}

		/* * * Writer members * * */

		std::shared_ptr<Linker::Segment> stack, heap;
		std::map<std::shared_ptr<Linker::Segment>, size_t> segment_index;
		std::map<std::string, uint16_t> module_reference_offsets;
		std::map<std::string, uint16_t> imported_name_offsets;
		uint16_t imported_names_length = 0;

		/*std::string stub_file;*/
		std::string module_name;
		std::string program_name;

		bool option_capitalize_names = false; /* TODO: parametrize */
		enum memory_model_t
		{
			MODEL_SMALL,
			MODEL_LARGE,
		};
		memory_model_t memory_model = MODEL_SMALL;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		enum compatibility_type
		{
			CompatibleNone,
			CompatibleWatcom,
			CompatibleMicrosoft, /* TODO */
			CompatibleGNU, /* TODO */
		};
		compatibility_type compatibility = CompatibleNone;

		std::shared_ptr<NEFormat> SimulateLinker(compatibility_type compatibility);

		static std::shared_ptr<NEFormat> CreateConsoleApplication(system_type system = Windows);

		static std::shared_ptr<NEFormat> CreateGUIApplication(system_type system = Windows);

		static std::shared_ptr<NEFormat> CreateLibraryModule(system_type system = Windows);

		unsigned GetCodeSegmentFlags() const;
		unsigned GetDataSegmentFlags() const;
		void AddSegment(const Segment& segment);
		uint16_t FetchModule(std::string name);
		uint16_t FetchImportedName(std::string name);
		std::string MakeProcedureName(std::string name);
		uint16_t MakeEntry(Linker::Position value);
		uint16_t MakeEntry(uint16_t ordinal, Linker::Position value);
		uint8_t CountBundles(size_t entry_index);

		void SetModel(std::string model) override;
		void SetOptions(std::map<std::string, std::string>& options) override;
		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void CalculateValues() override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* NEEXE_H */
