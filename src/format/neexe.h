#ifndef NEEXE_H
#define NEEXE_H

#include "mzexe.h"
#include "../common.h"
#include "../linker/linker.h"
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
	class NEFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatIs16bit() const override;

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		enum system_type
		{
			OS2 = 1, /* OS/2 1.0 - 1.3 */
			Windows, /* Windows 1.0 - 3.11 */
			MSDOS4, /* Multitasking MS-DOS 4.0 */
			Windows386,
			BorlandOSS,
			PharLap = 0x80,
		} system;

		enum program_flag_type
		{
			NODATA = 0,
			SINGLEDATA = 1, /* .DLL */
			MULTIPLEDATA = 2, /* .EXE */
			GLOBAL_INITIALIZATION = 4,
			PROTECTED_MODE_ONLY = 8,
			CPU_8086 = 0x10, /* TODO: this is not how all systems handle this */
			CPU_80286 = 0x20,
			CPU_80386 = 0x40,
			CPU_8087 = 0x80,
		} program_flags;

		enum application_flag_type
		{
			FULLSCREEN = 1,
			GUI_AWARE = 2,
			GUI = 3,
			FAMILY_APPLICATION = 8, /* first segment loads application */
			ERROR_IN_IMAGE = 0x20,
			LIBRARY = 0x80,
		} application_flags;

		bool IsLibrary() const;

		enum additional_flag_type
		{
			SUPPORT_LONGFILENAME = 1,
			WIN20_PROTECTED_MODE = 2,
			WIN20_PROPORTIONAL_FONTS = 4,
			FAST_LOAD_AREA = 8,
		} additional_flags;

		enum compatibility_type
		{
			CompatibleNone,
			CompatibleWatcom,
			CompatibleMicrosoft, /* TODO */
			CompatibleGNU, /* TODO */
		} compatibility;

		NEFormat(system_type system, unsigned program_flags, unsigned application_flags)
			: system(system), program_flags((program_flag_type)program_flags), application_flags((application_flag_type)application_flags), additional_flags((additional_flag_type)0), compatibility(CompatibleNone),
				linker_version{1, 0},
				heap_size(0), stack_size(0), sector_shift(9), code_swap_area_length(0),
//				stack(".stack"), heap(".heap"),
				imported_names_length(0)
		{
		}

		NEFormat * SimulateLinker(compatibility_type compatibility);

		static NEFormat * CreateConsoleApplication(system_type system = Windows);

		static NEFormat * CreateGUIApplication(system_type system = Windows);

		static NEFormat * CreateLibraryModule(system_type system = Windows);

		class Entry
		{
		public:
			enum entry_type
			{
				Unused,
				Fixed,
				Movable,
			} type;
			uint8_t segment;
			enum flag_type
			{
				Exported = 1,
				SharedData = 2,
			} flags;
			uint16_t offset;

			enum
			{
				WordCountShift = 3,

				INT_3Fh = 0x3FCD,
			};
			Entry()
				: type(Unused), segment(0), flags((flag_type)0), offset(0)
			{
			}

			Entry(unsigned type, uint8_t segment, unsigned flags, uint16_t offset)
				: type((entry_type)type), segment(segment), flags((flag_type)flags), offset(offset)
			{
			}

			offset_t GetEntrySize() const;

			uint8_t GetIndicatorByte() const;

			void WriteEntry(Linker::Writer& wr);
		};

		class Segment
		{
		public:
			Linker::Segment * image;
			offset_t data_offset;
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
			} flags;
			uint16_t movable_entry_index; /* for movable segments, each relocation targetting it needs one and only one entry */

			Segment(Linker::Segment * segment, unsigned flags)
				: image(segment), flags((flag_type)flags), movable_entry_index(0)
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
				} type;
				enum flag_type
				{
					Internal = 0,
					ImportOrdinal = 1,
					ImportName = 2,
					OSFixup = 3,
					Additive = 4,
				} flags;
				uint16_t offset;
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
				uint16_t target;
				Relocation()
					: type((source_type)0), flags((flag_type)0), offset(0), module(0), target(0)
				{
				}

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint16_t target)
					: type((source_type)type), flags((flag_type)flags), offset(offset), module(module), target(target)
				{
				}

				static source_type GetType(Linker::Relocation& rel);
			};
			std::map<uint16_t, Relocation> relocations;

			void AddRelocation(const Relocation& rel);
		};

		struct Name
		{
		public:
			std::string name;
			uint16_t ordinal;
		};

		class Resource
		{
		public:
			/* TODO */
		};

		/** @brief Version number */
		struct version
		{
			uint8_t major, minor;
		} linker_version, windows_version;

		uint16_t automatic_data;
		uint16_t heap_size, stack_size;
		uint16_t ss, sp, cs, ip;
		uint16_t sector_shift;

		uint32_t segment_table_offset;
		uint32_t resource_table_offset;
		uint32_t resident_name_table_offset;
		uint32_t module_reference_table_offset;
		uint32_t imported_names_table_offset;
		uint32_t entry_table_offset;
		uint32_t entry_table_length;
		uint16_t movable_entry_count;
		uint32_t nonresident_name_table_length;
		uint32_t nonresident_name_table_offset;
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
		uint16_t code_swap_area_length;

		Linker::Segment * stack, * heap;
		std::vector<Segment> segments;
		std::map<Linker::Segment *, size_t> segment_index;
		std::vector<Resource> resources;
		std::vector<Name> resident_names, nonresident_names;
		std::vector<uint16_t> module_references;
		std::map<std::string, uint16_t> module_reference_offsets;
		std::vector<std::string> imported_names;
		std::map<std::string, uint16_t> imported_name_offsets;
		uint16_t imported_names_length;

		std::vector<Entry> entries;

		/*std::string stub_file;*/
		std::string module_name;
		std::string program_name;

		bool option_capitalize_names; /* TODO: parametrize */
		enum memory_model_t
		{
			MODEL_SMALL,
			MODEL_LARGE,
		} memory_model;

		unsigned GetCodeSegmentFlags() const;
		unsigned GetDataSegmentFlags() const;
		void AddSegment(const Segment& segment);
		uint16_t FetchModule(std::string name);
		uint16_t FetchImportedName(std::string name);
		std::string MakeProcedureName(std::string name);
		uint16_t MakeEntry(Linker::Position value);
		uint16_t MakeEntry(uint16_t ordinal, Linker::Position value);
		uint8_t CountBundles(size_t entry_index);

		using LinkerManager::SetLinkScript;
		void SetModel(std::string model) override;
		void SetOptions(std::map<std::string, std::string>& options) override;
		void OnNewSegment(Linker::Segment * segment) override;
		Script::List * GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void CalculateValues() override;
		void WriteFile(Linker::Writer& wr) override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* NEEXE_H */
