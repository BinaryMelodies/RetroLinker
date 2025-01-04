#ifndef LEEXE_H
#define LEEXE_H

#include "mzexe.h"
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace Microsoft
{
	/**
	 * @brief LE/LX .EXE linear executable file format
	 *
	 * Introduced first for 32-bit virtual device drivers for 16-bit versions of Windows, it was adopted and extended for 32-bit versions of OS/2 as well as a few DOS extenders.
	 * It has two main variants, the LE and the LX executable formats.
	 *
	 * The following platforms are supported:
	 * - Windows/386 device drivers, LE format (not tested or fully supported)
	 * - LE executables for the DOS/4G DOS extender
	 * - LX executables for 32-bit versions of OS/2
	 */
	class LEFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		bool FormatSupportsSegmentation() const override;

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		::EndianType endiantype;

		enum system_type
		{
			OS2 = 1, /* OS/2 2.0+ */
			Windows, /* Windows 386 */
			MSDOS4, /* ? */
			Windows386, /* ? */

			DOS4G = -1, /* not a real system type */
		} system;

		bool IsOS2() const;

		enum
		{
			PreProcessInitialization = 0x00000004,
			NoInternalFixup = 0x00000010,
			NoExternalFixup = 0x00000020,
			FullScreen = 0x00000100,
			GUIAware = 0x00000200,
			GUI = 0x00000300,
			ErrorInImage = 0x00002000,
			Library = 0x00008000,
			ProtectedMemoryLibrary = 0x00018000,
			PhysicalDriver = 0x00020000,
			VirtualDriver = 0x00028000,
			PerProcessTermination = 0x40000000,
		};
		uint32_t module_flags;

		bool IsLibrary() const;

		bool IsDriver() const;

		bool extended_format;

		enum cpu_type
		{
			I386 = 2,
		} cpu;

		enum compatibility_type
		{
			CompatibleNone,
			CompatibleWatcom,
			CompatibleMicrosoft, /* TODO??? */
			CompatibleGNU, /* TODO: emx extender */
		} compatibility;

	protected:
		LEFormat(unsigned system, unsigned module_flags, bool extended_format)
			: endiantype(::LittleEndian), system((system_type)system), module_flags(module_flags), extended_format(extended_format),
				cpu(I386), compatibility(CompatibleNone), imported_procedure_names_length(0)
		{
		}

	public:
		static LEFormat * CreateConsoleApplication(system_type system = OS2);

		static LEFormat * CreateGUIApplication(system_type system = OS2);

		static LEFormat * CreateLibraryModule(system_type system = OS2);

		LEFormat * SimulateLinker(compatibility_type compatibility);

		/*std::string stub_file;*/
		std::string program_name, module_name;

		uint32_t page_count;
		uint32_t eip_object, eip_value, esp_object, esp_value;
		union
		{
			uint32_t last_page_size; /* LE */
			uint32_t page_offset_shift; /* LX */
		};
		uint32_t fixup_section_size, loader_section_size;
		uint32_t object_table_offset, object_page_table_offset, object_iterated_pages_offset;
		uint32_t resource_table_offset, resource_table_entry_count, resident_name_table_offset;
		uint32_t entry_table_offset, fixup_page_table_offset, fixup_record_table_offset;
		uint32_t imported_module_table_offset, imported_procedure_table_offset;
		uint32_t data_pages_offset, nonresident_name_table_offset, nonresident_name_table_size;
		uint32_t automatic_data, stack_size, heap_size;

		class Object
		{
		public:
			std::shared_ptr<Linker::Segment> image;
			enum flag_type
			{
				Readable = 0x0001,
				Writable = 0x0002,
				Execable = 0x0004,
				Resource = 0x0008,
				Discardable = 0x0010,
				Shared = 0x0020,
				PreloadPages = 0x0040,
				InvalidPages = 0x0080,
				ZeroFilledPages = 0x0100,
				Resident = 0x0200,
				ResidentContiguous = 0x0300,
				ResidentLongLockable = 0x0400,
				Alias16_16 = 0x1000, /* x86 */
				BigSegment = 0x2000, /* x86 */
				Conforming = 0x4000, /* x86 */
				IOPrivilege = 0x8000, /* x86 */
			} flags;
			uint32_t page_table_index;
			uint32_t page_entry_count;
			uint32_t data_pages_offset;

			Object(std::shared_ptr<Linker::Segment> segment, unsigned flags)
				: image(segment), flags((flag_type)flags)
			{
			}
		};

		void AddRelocation(Object& object, unsigned type, unsigned flags, size_t offset, uint16_t module, uint32_t target = 0, uint32_t addition = 0);

		class Page
		{
		public:
			union
			{
				struct
				{
					uint32_t fixup_table_index;
					uint8_t type;
				} le;
				struct
				{
					uint32_t offset;
					uint16_t size;
					uint16_t flags;
				} lx;
			};

			enum
			{
				/* LE types */
				NoRelocations = 0,
				Relocations = 3,

				/* LX flags */
				Preload = 0,
				Iterated,
				Invalid,
				ZeroFilled,
				Range,
			};

			uint32_t fixup_offset;

			class Relocation : public Linker::Writer
			{
			public:
				enum source_type
				{
					Offset8 = 0,
					Selector16 = 2,
					Pointer32 = 3,
					Offset16 = 5,
					Pointer48 = 6,
					Offset32 = 7,
					Relative32 = 8,

					SourceTypeMask = 0x0F,

					Alias = 0x10,
					SourceList = 0x20,
				} type;
				enum flag_type
				{
					Internal = 0,
					ImportOrdinal = 1,
					ImportName = 2,
					Entry = 3,

					FlagTypeMask = 3,

					Additive = 4,
					Target32 = 0x10,
					Additive32 = 0x20,
					Module16 = 0x40,
					Ordinal8 = 0x80,
				} flags;
				uint16_t module;
				uint32_t target;
				uint32_t addition;

				Relocation()
					: Writer(::LittleEndian), type(), flags(), module(0), target(0), addition(0)
				{
				}

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint32_t target = 0, uint32_t addition = 0)
					: Writer(::LittleEndian), type((source_type)type), flags((flag_type)flags), module(module), target(target), addition(addition)
				{
					sources.push_back(offset);
				}

				static source_type GetType(Linker::Relocation& rel);

				bool IsExternal() const;

				bool IsSelectorOrOffset() const;

				bool ComesBefore() const;

				size_t GetSourceSize() const;

			private:
				std::vector<uint16_t> sources;

			public:
				/* do not call this */
				void DecrementSingleSourceOffset(size_t amount);

				bool IsSelector() const;
				bool IsSourceList() const;
				bool IsAdditive() const;
				size_t GetTargetSize() const;
				size_t GetAdditiveSize() const;
				size_t GetModuleSize() const;
				size_t GetOrdinalSize() const;

				void CalculateSizes(compatibility_type compatibility);

				size_t GetSize() const;

				void WriteFile(Linker::Writer& wr, compatibility_type compatibility);
			};
			std::map<uint16_t, Relocation> relocations;

			Page()
				: lx{0, 0, 0}, fixup_offset(0)
			{
			}

		protected:
			Page(uint16_t fixup_table_index, uint8_t type)
				: le{fixup_table_index, type}, fixup_offset(0)
			{
			}

			Page(uint32_t offset, uint16_t size, uint8_t flags)
				: lx{offset, size, flags}, fixup_offset(0)
			{
			}

		public:
			static Page LEPage(uint16_t fixup_table_index, uint8_t type);

			static Page LXPage(uint32_t offset, uint16_t size, uint8_t flags);
		};

		class Resource
		{
			/* TODO */
		};

		struct Name
		{
		public:
			std::string name;
			uint16_t ordinal;
		};

		class Entry : public Linker::Writer
		{
		public:
			enum entry_type
			{
				Unused,
				Entry16,
				CallGate286,
				Entry32,
				Forwarder,
			} type;
			uint16_t object;
			enum flag_type
			{
				Exported = 1,
				SharedData = 2,
			} flags;
			uint32_t offset;

			Entry()
				: Writer(::LittleEndian), type(Unused), object(0), flags((flag_type)0), offset(0)
			{
			}

			Entry(unsigned type, uint16_t object, unsigned flags, uint32_t offset)
				: Writer(::LittleEndian), type((entry_type)type), object(object), flags((flag_type)flags), offset(offset)
			{
			}

			bool SameBundle(const Entry& other) const;

			offset_t GetEntryHeadSize() const;

			offset_t GetEntryBodySize() const;

			void WriteEntryHead(Linker::Writer& wr);

			void WriteEntryBody(Linker::Writer& wr);
		};

		static const uint32_t page_size = 0x1000;

		std::shared_ptr<Linker::Segment> stack, heap;
		std::vector<Object> objects;
		std::map<std::shared_ptr<Linker::Segment>, size_t> object_index;
		std::vector<Page> pages;
		std::vector<Resource> resources;
		std::vector<Name> resident_names, nonresident_names;
		std::vector<Entry> entries;
		std::vector<std::string> imported_modules, imported_procedures;
		std::map<std::string, uint32_t> imported_procedure_name_offsets;
		offset_t imported_procedure_names_length;

		unsigned GetDefaultObjectFlags() const;
		void AddObject(const Object& object);
		uint16_t FetchImportedModuleName(std::string name);
		uint16_t FetchImportedProcedureName(std::string name);
		uint16_t MakeEntry(Linker::Position value);
		uint16_t MakeEntry(uint16_t index, Linker::Position value);
		uint8_t CountBundles(size_t entry_index);

		using LinkerManager::SetLinkScript;
		void SetOptions(std::map<std::string, std::string>& options) override;
		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;
		Script::List * GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void CalculateValues() override;
		void WriteFile(Linker::Writer& wr) override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

	};

}

#endif /* LEEXE_H */
