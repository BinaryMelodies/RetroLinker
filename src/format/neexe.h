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
			enum flag_type : uint16_t
			{
				Data = 1, Code = 0,
				Allocated = 2,
				Loaded = 4, /* RealMode = 4 */ /* TODO */
				Iterated = 8, /* TODO */
				Movable = 0x10, Fixed = 0,
				Shareable = 0x20,
				Preload = 0x40, LoadOnCall = 0,
				ExecuteOnly = 0x80|Code, ReadOnly = 0x80|Data,
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
				/** @brief The type of the field that needs to be updated in the binary image */
				enum source_type
				{
					/** @brief Lower 8 bits of value */
					Offset8 = 0,
					/** @brief A 16-bit segment selector value (or paragraph in real mode) */
					Selector16 = 2,
					/** @brief A 32-bit far pointer, 16-bit segment and 16-bit offset value */
					Pointer32 = 3,
					/** @brief A 16-bit offset within its preferred segment */
					Offset16 = 5,
					/** @brief A 48-bit far pointer, 16-bit segment and 32-bit offset value */
					Pointer48 = 11,
					/** @brief A 32-bit offset within its preferred segment */
					Offset32 = 13,
				};
				/** @brief The type of relocation */
				source_type type = source_type(0);
				/** @brief Encodes what the type of the target is for this relocation */
				enum flag_type
				{
					/** @brief Mask to get the type of the target */
					TargetTypeMask = 3,
					/** @brief An internal segment:offset, or if segment is 0xFF, offset is the entry number */
					Internal = 0,
					/** @brief An imported module:ordinal where module is the module number */
					ImportOrdinal = 1,
					/** @brief An imported module:offset where module is the module number and offset is an offset to the name */
					ImportName = 2,
					/** @brief This is actually an 80x87 instruction sequence that needs to be fixed up for 80x87 emulation */
					OSFixup = 3,
					/** @brief Whether the relocation has an added in the image or it is the first element in a relocation chain
					 *
					 * If set, the value stored in the image has to be added to the target value.
					 * If cleared, the value stored in the image is part of a relocation chain, providing the offset to the following relocation.
					 * The chain is closed if the value 0xFFFF is present.
					 * All other properties of the relocation are the same as the first member of the chain.
					 */
					Additive = 4,
				};
				/** @brief The type of target */
				flag_type flags = flag_type(0);
				/** @brief The offset to the relocation, or if the relocation is chained, a list of offsets (this is not possible for Offset8) */
				std::vector<uint16_t> offsets;
				/** @brief This field is a segment (1 based) or module (1 based) or 80x87 instruction reference, depending on the flags */
				uint16_t module = 0;
				enum
				{
					FIARQQ = 1, FJARQQ = 1,
					FISRQQ = 2, FJSRQQ = 2,
					FICRQQ = 3, FJCRQQ = 3,
					FIERQQ = 4,
					FIDRQQ = 5,
					FIWRQQ = 6,
				};
				/** @brief This field is an offset or ordinal, depending on the flags */
				uint16_t target = 0;

				/** @brief Convenience field that stores the module name */
				std::string module_name;
				/** @brief Convenience field that stores the imported procedure name, if imported by name, also the name for an exported entry */
				std::string import_name;
				/** @brief Convenience field that stores the actual segment for an entry */
				uint8_t actual_segment = 0;
				/** @brief Convenience field that stores the actual offset for an entry */
				uint16_t actual_offset = 0;

				Relocation() = default;

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint16_t target)
					: type(source_type(type)), flags(flag_type(flags)), module(module), target(target)
				{
					offsets.push_back(offset);
				}

				static source_type GetType(Linker::Relocation& rel);
				size_t GetSize() const;
			};
			std::vector<Relocation> relocations;
			/** @brief Used internally during output generation */
			std::map<uint16_t, Relocation> relocations_map;

			void AddRelocation(const Relocation& rel);
			void Dump(Dumper::Dumper& dump, unsigned index, bool isos2);
		};

		/** @brief Represents a resource entry
		 *
		 * For Windows executables, resources have very similar properties to segments, and for OS/2 executables, resources are also segments
		 */
		class Resource : public Segment
		{
		public:
			enum
			{
				// Windows resource types
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

				// OS/2 resource types
				OS2_POINTER = 1,
				OS2_BITMAP = 2,
				OS2_MENU = 3,
				// TODO: other OS/2 resource types
			};

			/** @brief Type identifier, used by both Windows and OS/2
			 *
			 * Under Windows, if bit 15 is cleared, it references a string
			 */
			uint16_t type_id = 0;
			/** @brief Type name, used only by Windows */
			std::string type_id_name;
			/** @brief Resource dentifier, used by both Windows and OS/2
			 *
			 * Under Windows, if bit 15 is cleared, it references a string
			 */
			uint16_t id = 0;
			/** @brief Resource name, used only by Windows */
			std::string id_name;
			/** @brief Reserved field, named so in Microsoft documentation */
			uint16_t handle = 0;
			/** @brief Reserved field, named so in Microsoft documentation */
			uint16_t usage = 0;

			void Dump(Dumper::Dumper& dump, unsigned index, bool isos2);
		};

		/** @brief Windows executables bundle their resources by resource type */
		class ResourceType
		{
		public:
			uint16_t type_id = 0;
			std::string type_id_name;
			std::vector<Resource> resources;
		};

		/** @brief Represents an entry into the binary, typically DLL exported procedures */
		class Entry
		{
		public:
			/** @brief This field represents the type of the entry */
			enum entry_type
			{
				/** @brief The entry is unused, the other fields are meaningless */
				Unused,
				/** @brief The entry references a fixed segment */
				Fixed,
				/** @brief The entry references a movable segment */
				Movable,
			};
			/** @brief The type of entry, based on the first byte in an entry bundle */
			entry_type type = Unused;
			/** @brief The number of the segment, 1 based */
			uint8_t segment = 0;
			/** @brief Flags present in an entry */
			enum flag_type : uint8_t
			{
				/** @brief Set if the entry is exported */
				Exported = 1,
				/** @brief Set if the data segment used by the entry is global, used in SINGLEDATA modules (libraries) */
				SharedData = 2,
			};
			flag_type flags = flag_type(0);
			/** @brief Offset within the segment */
			uint16_t offset = 0;

			// informational purposes
			enum export_type
			{
				/** @brief The entry is not exported, the Exported bit is not set */
				NotExported,
				/** @brief The entry is exported by name, it is referenced in the resident name table */
				ExportByName,
				/** @brief The entry is exported by ordinal, it is referenced in the nonresident name table */
				ExportByOrdinal,
			};
			/** @brief Whether the entry is exported. This is not actually stored in the entry table and its value is ignored during program generation */
			export_type export_state = NotExported;
			/** @brief The name of an exported entry. This is not actually stored in the entry table and its value is ignored during program generation */
			std::string entry_name;

			enum
			{
				WordCountShift = 3,

				/** @brief Byte code for interrupt call that must be placed in the entry field */
				INT_3Fh = 0x3FCD,
			};

			/** @brief Convenience field to signify this entry is part of the same bundle as the previous one */
			mutable bool same_bundle = false; // mutable because this isn't actually part of the entry state but the entire entry table

			Entry() = default;

			Entry(unsigned type, uint8_t segment, unsigned flags, uint16_t offset)
				: type(entry_type(type)), segment(segment), flags(flag_type(flags)), offset(offset)
			{
			}

			/** @brief Returns the size of an entry as stored in the file, without the first two bytes of the bundle */
			offset_t GetEntrySize() const;

			/** @brief Retrieves the segment indicator byte. For Fixed entries, this the same as the segment number */
			uint8_t GetIndicatorByte() const;

			/** @brief Reads an entry within a bundle */
			static Entry ReadEntry(Linker::Reader& rd, uint8_t indicator_byte);
			/** @brief Writes an entry within a bundle */
			void WriteEntry(Linker::Writer& wr);
		};

		/** @brief Represents an imported module in the module reference table */
		class ModuleReference
		{
		public:
			/** @brief Offset to the module name within the imported names table */
			uint16_t name_offset = 0;
			/** @brief The name of the module, not actually used during program generation */
			std::string name;

			ModuleReference(uint16_t name_offset, std::string name = "")
				: name_offset(name_offset), name(name)
			{
			}
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

		/** @brief On OS/2, this is a sequence of resources, for non-OS/2 targets, this must be compiled into resource_types */
		std::vector<Resource> resources;

		/** @brief For non-OS/2 targets, the resources get organized according to their types */
		std::vector<ResourceType> resource_types;

		/** @brief For non-OS/2 targets, this is a list of all the resource type strings and resource strings */
		std::vector<std::string> resource_strings;

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

		std::vector<ModuleReference> module_references;

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
