#ifndef LEEXE_H
#define LEEXE_H

#include <array>
#include "mzexe.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
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
	class LEFormat : public virtual Linker::SegmentManager, public std::enable_shared_from_this<LEFormat>
	{
	public:
		/* * * General members * * */

		enum compatibility_type
		{
			CompatibleNone,
			CompatibleWatcom,
			CompatibleMicrosoft, /* TODO??? */
			CompatibleGNU, /* TODO: emx extender */
		};

		class Object
		{
		public:
			std::shared_ptr<Linker::Segment> image;
			enum flag_type
			{
				Readable = 0x0001,
				Writable = 0x0002,
				Executable = 0x0004,
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
			};
			uint32_t size = 0;
			uint32_t address = 0;
			flag_type flags = flag_type(0);
			uint32_t page_table_index = 0;
			uint32_t page_entry_count = 0;
			uint32_t data_pages_offset = 0;

			Object()
			{
			}

			Object(std::shared_ptr<Linker::Segment> segment, unsigned flags)
				: image(segment), flags(flag_type(flags))
			{
			}
		};

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

			uint32_t fixup_offset = 0;

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
				};
				source_type type = source_type(0);
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
				};
				flag_type flags = flag_type(0);
				uint16_t module = 0;
				uint32_t target = 0;
				uint32_t addition = 0;

				Relocation()
					: Writer(::LittleEndian)
				{
				}

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint32_t target = 0, uint32_t addition = 0)
					: Writer(::LittleEndian), type(source_type(type)), flags(flag_type(flags)), module(module), target(target), addition(addition)
				{
					sources.push_back(Chained{offset});
				}

				static source_type GetType(Linker::Relocation& rel);

				bool IsExternal() const;

				bool IsSelectorOrOffset() const;

				bool ComesBefore() const;

				size_t GetSourceSize() const;

			private:
				struct Chain
				{
					uint32_t target = 0;
					uint16_t source = 0;
				};

				struct Chained
				{
					uint16_t source = 0;
					std::vector<Chain> chains;
				};

				std::vector<Chained> sources;

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

				uint16_t GetFirstSource() const;

				void CalculateSizes(compatibility_type compatibility);

				size_t GetSize() const;

				static Relocation ReadFile(Linker::Reader& rd);
				void WriteFile(Linker::Writer& wr) const;
			};
			std::map<uint16_t, Relocation> relocations;
			uint32_t checksum = 0;

			Page()
				: lx{0, 0, 0}
			{
			}

		protected:
			Page(uint16_t fixup_table_index, uint8_t type)
				: le{fixup_table_index, type}
			{
			}

			Page(uint32_t offset, uint16_t size, uint8_t flags)
				: lx{offset, size, flags}
			{
			}

		public:
			static Page LEPage(uint16_t fixup_table_index, uint8_t type);

			static Page LXPage(uint32_t offset, uint16_t size, uint8_t flags);
		};

		class Resource
		{
		public:
			uint16_t type = 0, name = 0;
			uint32_t size = 0;
			uint16_t object = 0;
			uint32_t offset = 0;
		};

		struct Name
		{
		public:
			std::string name;
			uint16_t ordinal = 0;
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
			};
			entry_type type = Unused;
			uint16_t object = 0;
			enum flag_type
			{
				Exported = 1,
				SharedData = 2,
			};
			flag_type flags = flag_type(0);
			uint32_t offset = 0;

			Entry()
				: Writer(::LittleEndian)
			{
			}

			Entry(unsigned type, uint16_t object, unsigned flags, uint32_t offset)
				: Writer(::LittleEndian), type(entry_type(type)), object(object), flags(flag_type(flags)), offset(offset)
			{
			}

			mutable bool same_bundle = false; // mutable because this isn't actually part of the entry state but the entire entry table

			bool SameBundle(const Entry& other) const;

			offset_t GetEntryHeadSize() const;

			offset_t GetEntryBodySize() const;

			static Entry ReadEntryHead(Linker::Reader& rd, uint8_t type);
			static Entry ReadEntry(Linker::Reader& rd, uint8_t type, LEFormat::Entry& head);

			void WriteEntryHead(Linker::Writer& wr) const;

			void WriteEntryBody(Linker::Writer& wr) const;
		};

		class ModuleDirective
		{
		public:
			enum directive_number : uint16_t
			{
				VerifyRecordDirective = 0x8001,
				LanguageInformationDirective = 0x0002,
				CoProcessorRequiredSupportTable = 0x0003,
				ThreadStateInitializationDirective = 0x0004,
				CSetBrowseInformation = 0x0005,
			};
			static constexpr uint16_t ResidentFlagMask = 0x8000;
			directive_number directive = directive_number(0);
			uint16_t length = 0;
			offset_t offset = 0;

			bool IsResident() const;
		};

		::EndianType endiantype = ::LittleEndian;

		/** @brief The signature, almost always "LE" or "LX" */
		std::array<char, 2> signature{'L', 'E'};

		bool IsExtendedFormat() const;

		uint32_t format_level = 0;

		/* https://faydoc.tripod.com/formats/exe-LE.htm */
		enum cpu_type
		{
			I286 = 0x01,
			I386 = 0x02, /* only value used */
			I486 = 0x03,
			I860_N10 = 0x20,
			I860_N11 = 0x21,
			MIPS1 = 0x40,
			MIPS2 = 0x41,
			MIPS3 = 0x42,
		};
		cpu_type cpu = I386;

		enum system_type
		{
			OS2 = 1, /* OS/2 2.0+ */
			Windows, /* Windows 386 */
			MSDOS4, /* ? */
			Windows386, /* ? */

			DOS4G = -1, /* not a real system type */
		};
		system_type system = system_type(0);

		uint32_t module_version = 0;

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
		uint32_t module_flags = 0;

		uint32_t page_count = 0, page_size = 0x1000;
		uint32_t eip_object = 0, eip_value = 0, esp_object = 0, esp_value = 0;
		union
		{
			uint32_t last_page_size; /* LE */
			uint32_t page_offset_shift; /* LX */
		};
		uint32_t fixup_section_size = 0, fixup_section_checksum = 0;
		uint32_t loader_section_size = 0, loader_section_checksum = 0;
		uint32_t object_table_offset = 0, object_page_table_offset = 0, object_iterated_pages_offset = 0;
		uint32_t resource_table_offset = 0, resource_table_entry_count = 0, resident_name_table_offset = 0;
		uint32_t entry_table_offset = 0, module_directives_offset = 0, fixup_page_table_offset = 0, fixup_record_table_offset = 0;
		uint32_t imported_module_table_offset = 0, imported_procedure_table_offset = 0;
		uint32_t per_page_checksum_offset = 0;
		uint32_t data_pages_offset = 0, preload_page_count = 0;
		uint32_t nonresident_name_table_offset = 0, nonresident_name_table_size = 0, nonresident_name_table_checksum = 0;
		uint32_t automatic_data = 0;
		uint32_t debug_info_offset = 0, debug_info_size = 0, instance_preload_page_count = 0, instance_demand_page_count = 0;
		uint32_t stack_size = 0, heap_size = 0;

		uint32_t vxd_version_info_resource_offset = 0;
		uint32_t vxd_version_info_resource_length = 0;
		uint16_t vxd_device_id = 0;
		uint16_t vxd_ddk_version = 0;

		std::vector<Object> objects;
		std::vector<ModuleDirective> module_directives;

		std::vector<Page> pages;
		std::map<uint16_t, std::map<uint16_t, Resource>> resources;
		std::vector<Name> resident_names, nonresident_names;
		std::vector<Entry> entries;
		std::vector<std::string> imported_modules, imported_procedures;

		explicit LEFormat()
			: last_page_size(0)
		{
		}

		LEFormat(unsigned system, unsigned module_flags, bool extended_format)
			: system(system_type(system)), module_flags(module_flags), last_page_size(0)
		{
			if(extended_format)
				signature[1] = 'X';
		}

		bool IsLibrary() const;

		bool IsDriver() const;

		bool IsOS2() const;

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		/* * * Writer members * * */

		static std::shared_ptr<LEFormat> CreateConsoleApplication(system_type system = OS2);

		static std::shared_ptr<LEFormat> CreateGUIApplication(system_type system = OS2);

		static std::shared_ptr<LEFormat> CreateLibraryModule(system_type system = OS2);

		std::shared_ptr<LEFormat> SimulateLinker(compatibility_type compatibility);

		mutable MZStubWriter stub;

		compatibility_type compatibility = CompatibleNone;

		/*std::string stub_file;*/
		std::string program_name, module_name;

		bool FormatSupportsSegmentation() const override;

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		Resource& AddResource(Resource& resource);

		void GetRelocationOffset(Object& object, size_t offset, size_t& page_index, uint16_t& page_offset);
		void AddRelocation(Object& object, unsigned type, unsigned flags, size_t offset, uint16_t module, uint32_t target = 0, uint32_t addition = 0);

		std::shared_ptr<Linker::Segment> stack, heap;
		std::map<std::shared_ptr<Linker::Segment>, size_t> object_index;
		std::map<std::string, uint32_t> imported_procedure_name_offsets;
		offset_t imported_procedure_names_length = 0;

		unsigned GetDefaultObjectFlags() const;
		void AddObject(const Object& object);
		uint16_t FetchImportedModuleName(std::string name);
		uint16_t FetchImportedProcedureName(std::string name);
		uint16_t MakeEntry(Linker::Position value);
		uint16_t MakeEntry(uint16_t index, Linker::Position value);
		uint8_t CountBundles(size_t entry_index) const;

		void SetOptions(std::map<std::string, std::string>& options) override;
		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void CalculateValues() override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;

	};

}

#endif /* LEEXE_H */
