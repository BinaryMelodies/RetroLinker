#ifndef LEEXE_H
#define LEEXE_H

#include <array>
#include <vector>
#include "mzexe.h"
#include "neexe.h"
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/options.h"
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
			/** @brief No emulation is attempted */
			CompatibleNone,
			/** @brief Attempts to mimic the output of the Watcom linker */
			CompatibleWatcom,
			/** @brief Attempts to mimic the output of the Microsoft linker (TODO: unimplemented) */
			CompatibleMicrosoft,
			/** @brief Attempts to mimic the output of the Borland linker (TODO: unimplemented) */
			CompatibleBorland,
			// /** @brief Attempts to mimic the output of the GNU linker, this is undefined for the LE output */
			// CompatibleGNU,
		};

		/** @brief Represents an LE/LX object (a relocatable section of memory) as stored in the object table, alongside associated information */
		class Object
		{
		public:
			std::shared_ptr<Linker::Image> image;
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

		/**
		 * @brief An image instance that is a collection of other images, conceptually pages
		 *
		 * When parsing an LE/LX binary image, the contents of objects might not appear contiguously, but instead as a collection of pages (typically 4 KiB in size).
		 * This class implements the Image interface but data crossing page boundaries can also be accessed.
		 */
		class PageSet : public Linker::Image
		{
		public:
			std::shared_ptr<LEFormat> file;
			std::vector<uint32_t> pages;

			PageSet() = delete;

			PageSet(std::shared_ptr<LEFormat> file)
				: file(file)
			{
			}

			offset_t ImageSize() const override;
			using Linker::Image::WriteFile;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset = 0) const override;
		};

		/**
		 * @brief An image instance for a single page within a complete object image
		 *
		 * When writing an LE/LX binary image, the object data needs to be split up into sections called pages (typically 4 KiB in size).
		 * This class implements the Image interface but instead gives a restricted window into the full data image, to be stored as a single page image.
		 */
		class SegmentPage : public Linker::Image
		{
		public:
			std::shared_ptr<Linker::Image> image;
			offset_t offset = 0;
			offset_t size = 0;

			SegmentPage(std::shared_ptr<Linker::Image> image, offset_t offset, offset_t size)
				: image(image), offset(offset), size(size)
			{
			}

			offset_t ImageSize() const override;
			using Linker::Image::WriteFile;
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset = 0) const override;
			std::shared_ptr<const Linker::ActualImage> AsImage() const override;
		};

		/**
		 * @brief A data structure to represent an LE/LX iterated page, consisting of data produced from a repetition of a certain pattern
		 */
		class IteratedPage : public Linker::Image
		{
		public:
			struct IterationRecord
			{
				uint16_t count = 0;
				std::vector<uint8_t> data;
			};
			std::vector<IterationRecord> records;

			offset_t ImageSize() const override;
			using Linker::Image::WriteFile;
			static std::shared_ptr<IteratedPage> ReadFromFile(Linker::Reader& rd, uint16_t size);
			offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset = 0) const override;

			/**
			 * @brief An image instance where the iterated page data can be accessed as the series of bytes it generates
			 */
			class View : public Linker::ActualImage
			{
			public:
				IteratedPage& iterated_page;
				size_t size;

				View(IteratedPage& iterated_page, size_t size)
					: iterated_page(iterated_page), size(size)
				{
				}

				size_t ReadData(size_t bytes, offset_t offset, void * buffer) const override;

				offset_t ImageSize() const override;
				using Linker::Image::WriteFile;
				offset_t WriteFile(Linker::Writer& wr, offset_t count, offset_t offset = 0) const override;
			};
		};

		/** @brief Represents a page (usually of 4 KiB) within an LE/LX object */
		class Page
		{
		public:
			/** @brief LX (and presumably LE) page types */
			enum page_type
			{
				Preload = 0,
				Iterated = 1,
				Invalid = 2,
				ZeroFilled = 3,
				Range = 4,
				Compressed = 5,
			};

			/**
			 * @brief (LX only) Offset of the page within the file
			 *
			 * It is stored with a page_offset_shift in the page table.
			 * In the LE format, the page_number field serves a similar purpose.
			 */
			uint32_t offset = 0;

			/** @brief (LX only) The size of the page as stored in the file
			 *
			 * For legal physical pages, the remainder of the page is filled with zeroes.
			 * For iterated pages, the page is filled with the iterated record data.
			 */
			uint16_t size = 0;

			/**
			 * @brief The type of the page
			 *
			 * For LE, documentation contradict each other, but this is typically set to 0.
			 */
			uint16_t type = 0;

			page_type GetPageType(const LEFormat& fmt) const;

			/** @brief The offset to the first fixup from the fixup record table, as stored in the fixup page table */
			uint32_t fixup_offset = 0;

			/** @brief Represents a relocation record associated to this page */
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
					Chained = 8,
					Target32 = 0x10,
					Additive32 = 0x20,
					Module16 = 0x40,
					Ordinal8 = 0x80,
				};
				flag_type flags = flag_type(0);
				uint16_t module = 0;
				uint32_t target = 0;
				uint32_t addition = 0;

				// informational purposes
				/** @brief Convenience field that stores the module name, not used for generation */
				std::string module_name;
				/** @brief Convenience field that stores the imported procedure name, if imported by name, also the name for an exported entry, not used for generation */
				std::string import_name;
				/** @brief Convenience field that stores the actual object number for an entry, not used for generation */
				uint16_t actual_object = 0;
				/** @brief Convenience field that stores the actual offset for an entry, not used for generation */
				uint32_t actual_offset = 0;

				Relocation()
					: Writer(::LittleEndian)
				{
				}

				Relocation(unsigned type, unsigned flags, uint16_t offset, uint16_t module, uint32_t target = 0, uint32_t addition = 0)
					: Writer(::LittleEndian), type(source_type(type)), flags(flag_type(flags)), module(module), target(target), addition(addition)
				{
					sources.push_back(Chain{offset});
				}

				static source_type GetType(Linker::Relocation& rel);

				bool IsExternal() const;

				bool IsSelectorOrOffset() const;

				bool ComesBefore() const;

				size_t GetSourceSize() const;

				/** @brief For chained relocations, this contains an element of the relocation chain */
				struct ChainLink
				{
					uint32_t target = 0;
					uint16_t source = 0;
				};

				/**
				 * @brief A source location entry in the relocation record
				 *
				 * Apart from containing the source offset within the relocation record, the 32-bit data as stored in the page image in the file may also be the start of a chain of entries.
				 * If no chaining is enabled, this is just a wrapper around the source value.
				 */
				struct Chain
				{
					/** @brief Offset within page where relocation should be applied to */
					uint16_t source = 0;
					/** @brief If the relocations are chained, this contains the base address to which other fields are relocated to */
					uint32_t base_address = 0;
					/** @brief If the relocations are chained, this contains the following link entries */
					std::vector<ChainLink> chains;
				};

				/** @brief The sequence of sources corresponding to the same relocation record */
				std::vector<Chain> sources;

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

				static Relocation ReadFile(Linker::Reader& rd, Page& page);
				void WriteFile(Linker::Writer& wr) const;
			};
			std::map<uint16_t, Relocation> relocations;
			uint32_t checksum = 0;
			std::shared_ptr<Linker::Image> image;

			Page()
			{
			}

			static Page LEPage(uint8_t type);

			static Page LXPage(uint32_t offset, uint16_t size, uint8_t type);

		protected:
			void FillDumpRegion(Dumper::Dumper& dump, Dumper::Region& page_region, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void FillDumpRelocations(Dumper::Dumper& dump, Dumper::Block& page_block, const LEFormat& fmt) const;
			void DumpPhysicalPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpIteratedPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpInvalidPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpZeroFilledPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpPageRange(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpCompressedPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;
			void DumpUnknownPage(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t object_number, uint32_t page_index) const;

		public:
			void Dump(Dumper::Dumper& dump, const LEFormat& fmt, uint32_t page_index) const;
		};

		/** @brief Stores an OS/2 resource */
		class Resource
		{
		public:
			uint16_t type = 0, name = 0;
			uint32_t size = 0;
			uint16_t object = 0;
			uint32_t offset = 0;
		};

		/** @brief A name and ordinal pair, as used for resident and non-resident names */
		struct Name
		{
		public:
			std::string name;
			uint16_t ordinal = 0;
		};

		/** @brief Represents an entry into the binary, typically DLL exported procedures */
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

				ForwarderByOrdinal = 1,
			};
			flag_type flags = flag_type(0);
			uint32_t offset = 0;

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
			/** @brief Convenience field that stores the module name for a forwarder entry, not used for generation */
			std::string module_name;
			/** @brief Convenience field that stores the imported procedure name for a forwarder entry, if imported by name, also the name for an exported entry, not used for generation */
			std::string import_name;

			Entry()
				: Writer(::LittleEndian)
			{
			}

			Entry(unsigned type)
				: Writer(::LittleEndian), type(entry_type(type))
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

		/** @brief Stores a module directive in the module directive table */
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
			I586 = 0x04,
			I860_N10 = 0x20,
			I860_N11 = 0x21,
			MIPS1 = 0x40,
			MIPS2 = 0x41,
			MIPS3 = 0x42,
		};
		cpu_type cpu = I386;

		enum system_type
		{
			/** @brief Runs on OS/2 2.0+ */
			OS2 = 1,
			/** @brief Reserved for Windows, unimplemented */
			Windows,
			/** @brief Reserved for DOS 4.x, unimplemented */
			MSDOS4,
			/** @brief Windows 386 (only virtual device drivers supported) */
			Windows386,
			/** @brief IBM Microkernel Personality Neutral, unimplemented */
			Neutral,

			/** @brief Used by DOS/4G, the numerical value is chosen so that it gets stored as OS/2 in the binary */
			DOS4G = 0x10001,
		};
		system_type system = system_type(0);

		/** @brief Represents the target image type */
		enum output_type
		{
			/** @brief A graphical executable image, conventionally taking the suffix `.exe` */
			OUTPUT_GUI,
			/** @brief A console executable image, conventionally taking the suffix `.exe` */
			OUTPUT_CON,
			/** @brief A dynamically linked library, conventionally taking the suffix `.dll` */
			OUTPUT_DLL,
			/** @brief A physical device driver, conventionally taking the suffix `.sys` */
			OUTPUT_PDD,
			/** @brief A virtual device driver, conventionally taking the suffix `.sys` */
			OUTPUT_VDD,
		};
		/*** @brief Whether to generate an executable or a library */
		output_type output;

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
		ResourceFile vxd_version_info_resource = ResourceFile(ResourceFile::System_Windows_3x);
		uint16_t vxd_device_id = 0;
		uint16_t vxd_ddk_version = 0;

		std::vector<Object> objects;
		std::vector<ModuleDirective> module_directives;

		std::vector<Page> pages;
		/**
		 * @brief (LE only) The object page map table contents
		 *
		 * The available documentation contradict each other, but each object page seems to map to a physical page offset within the file.
		 * In the LX format, the offset field of the object page table serves a similar purpose.
		 */
		std::vector<std::tuple<uint32_t, Page::page_type>> page_map_table;

		uint32_t ObjectPageToPhysicalPage(uint32_t index) const;

		std::map<uint16_t, std::map<uint16_t, Resource>> resources;
		std::vector<Name> resident_names, nonresident_names;
		std::vector<Entry> entries;
		std::vector<std::string> imported_modules, imported_procedures;

		offset_t file_size = offset_t(-1);

		/** @brief Configures the values for system/output type */
		void SetTargetDefaults();

		explicit LEFormat()
			: last_page_size(0)
		{
		}

		LEFormat(bool extended_format)
			: last_page_size(0)
		{
			if(extended_format)
				signature[1] = 'X';
		}

		LEFormat(system_type system, output_type output)
			: system(system_type(system)), output(output), last_page_size(0)
		{
		}

		LEFormat(system_type system, output_type output, bool extended_format)
			: system(system_type(system)), output(output), last_page_size(0)
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

		offset_t GetPageOffset(uint32_t physical_page_number) const;
		offset_t GetPageSize(uint32_t physical_page_number) const;

		/* * * Writer members * * */

		class LEOptionCollector : public Linker::OptionCollector
		{
		public:
			class SystemEnumeration : public Linker::Enumeration<system_type>
			{
			public:
				SystemEnumeration()
					: Enumeration(
						"OS2", OS2,
						"WINDOWS", Windows,
						"DOS", MSDOS4,
						"WIN386", Windows386,
						"WINDOWS386", Windows386,
						"VXD", Windows386,
						"IBM", Neutral,
						"DOS4G", DOS4G,
						"DOS4GW", DOS4G)
				{
					descriptions = {
						{ OS2, "OS/2 2.0+" },
						{ Windows, "Reserved for Windows (not supported)" },
						{ MSDOS4, "Reserved for Multitasking MS-DOS 4.0 (not supported)" },
						{ Windows386, "Windows 386 (virtual device driver only)" },
						{ Neutral, "IBM Microkernel Personality Neutral (not supported)" },
						{ DOS4G, "Rational Systems (Tenberry Software) DOS/4G or DOS/4GW executable" },
					};
				}
			};

			class OutputTypeEnumeration : public Linker::Enumeration<output_type>
			{
			public:
				OutputTypeEnumeration()
					: Enumeration(
						"GUI", OUTPUT_GUI,
						"PM", OUTPUT_GUI,
						"WINDOWS", OUTPUT_GUI,
						"CONSOLE", OUTPUT_CON,
						"DLL", OUTPUT_DLL,
						"PDD", OUTPUT_PDD,
						"SYS", OUTPUT_PDD,
						"VDD", OUTPUT_VDD,
						"VXD", OUTPUT_VDD)
				{
					descriptions = {
						{ OUTPUT_GUI, "creates a graphical executable (Presentation Manager for OS/2)" },
						{ OUTPUT_CON, "creates a console (text mode) executable" },
						{ OUTPUT_DLL, "creates a dynamic linking library (DLL)" },
						{ OUTPUT_PDD, "creates a physical device driver" },
						{ OUTPUT_VDD, "creates a virtual device driver" },
					};
				}
			};

			class CompatibilityEnumeration : public Linker::Enumeration<compatibility_type>
			{
			public:
				CompatibilityEnumeration()
					: Enumeration(
						"NONE", CompatibleNone,
						"WATCOM", CompatibleWatcom,
						"MS", CompatibleMicrosoft,
						"BORLAND", CompatibleBorland)
						//"GNU", CompatibleGNU
				{
					descriptions = {
						{ CompatibleNone, "default operation" },
						{ CompatibleWatcom, "mimics the Watcom linker (not implemented)" },
						{ CompatibleMicrosoft, "mimics the Microsoft linker (not implemented)" },
						{ CompatibleBorland, "mimics the Borland linker (not implemented)" },
						/*{ CompatibleGNU, "mimics the GNU linker (not implemented)" },*/
					};
				}
			};

			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<Linker::ItemOf<SystemEnumeration>> system{"system", "Target system type"};
			Linker::Option<Linker::ItemOf<OutputTypeEnumeration>> type{"type", "Type of binary"};
			Linker::Option<Linker::ItemOf<CompatibilityEnumeration>> compat{"compat", "Mimics the behavior of another linker"};
			Linker::Option<bool> le{"le", "Original linear executable (LE)"};
			Linker::Option<bool> lx{"lx", "Extended linear executable (LX)"};

			LEOptionCollector()
			{
				InitializeFields(stub, system, type, compat, le, lx);
			}
		};

		static std::shared_ptr<LEFormat> CreateConsoleApplication(system_type system = OS2);

		static std::shared_ptr<LEFormat> CreateGUIApplication(system_type system = OS2);

		static std::shared_ptr<LEFormat> CreateLibraryModule(system_type system = OS2);

		static std::shared_ptr<LEFormat> CreateDeviceDriver(system_type system = OS2, bool virtual_device_driver = true);

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

		static std::vector<Linker::OptionDescription<void> *> ParameterNames;
		std::vector<Linker::OptionDescription<void> *> GetLinkerScriptParameterNames() override;
		std::shared_ptr<Linker::OptionCollector> GetOptions() override;
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
