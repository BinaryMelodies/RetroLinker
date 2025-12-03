#ifndef PEEXE_H
#define PEEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/options.h"
#include "../linker/segment_manager.h"
#include "coff.h"
#include "mzexe.h"

namespace Microsoft
{
	/**
	 * @brief Microsoft PE .EXE portable executable file format
	 */
	class PEFormat : public COFF::COFFFormat
	{
	public:
		char pe_signature[4];

		/** @brief Represents a version entry with major and minor versions */
		struct version_type
		{
			uint16_t major, minor;
		};

		/** @brief The PE optional header as specified by Microsoft */
		class PEOptionalHeader : public AOutHeader
		{
		public:
			/** @brief Magic number for ROM images, according to Microsoft */
			static constexpr uint16_t ROM32 = 0x0107;
			/** @brief Magic number for 32-bit binaries, also signifies 4-byte entries in the file (PE32) */
			static constexpr uint16_t EXE32 = 0x010B;
			/** @brief Magic number for 64-bit binaries, also signifies 8-byte entries in the file (PE32+) */
			static constexpr uint16_t EXE64 = 0x020B;

			/** @brief Conventional image base for 32-bit Windows executables */
			static constexpr offset_t Win32Base = 0x00400000;
			/** @brief Conventional image base for 32-bit Windows dynamic linking libraries */
			static constexpr offset_t Dll32Base = 0x10000000;
			/** @brief Conventional image base for Windows CE executables */
			static constexpr offset_t WinCEBase = 0x00010000;
			/** @brief Conventional image base for 64-bit Windows executables */
			static constexpr offset_t Win64Base = 0x140000000;
			/** @brief Conventional image base for 64-bit Windows dynamic linking libraries */
			static constexpr offset_t Dll64Base = 0x180000000;

			/**
			 * @brief Preferred base address of image, all relative virtual addresses are calculate relative to this value
			 */
			offset_t image_base = 0;
			/**
			 * @brief Section alignment
			 */
			uint32_t section_align = 0;
			/**
			 * @brief File alignment
			 */
			uint32_t file_align = 0;
			/**
			 * @brief Required operating system version
			 */
			version_type os_version = { };
			/**
			 * @brief Binary image version
			 */
			version_type image_version = { };
			/**
			 * @brief Version of the subsystem
			 */
			version_type subsystem_version = { };
			/**
			 * @brief Reserved value
			 */
			uint32_t win32_version = 0;
			/**
			 * @brief Size of the entire image, including headers
			 */
			uint32_t total_image_size = 0;
			/**
			 * @brief Cumulative size of all the headers, including the MZ stub
			 */
			uint32_t total_headers_size = 0;
			/**
			 * @brief Checksum
			 */
			uint32_t checksum = 0;
			/** Available Windows subsystems */
			enum SubsystemType : uint16_t
			{
				Unknown = 0,
				Native = 1,
				WindowsGUI = 2,
				WindowsCUI = 3,
				OS2CUI = 5,
				POSIXCUI = 7,
				NativeWin95 = 8,
				WinCEGUI = 9,
				EFIApplication = 10,
				EFIBootServiceDriver = 11,
				EFIRuntimeDriver = 12,
				EFIROM = 13,
				Xbox = 14,
				WindowsBootApplication = 16,
			};
			/**
			 * @brief The Windows subsystem this program runs on
			 */
			SubsystemType subsystem = Unknown;
			/**
			 * @brief DLL flags (in PE terminology, characteristics)
			 */
			uint16_t flags = 0;
			/**
			 * @brief How much of stack should be reserved at launch
			 */
			offset_t reserved_stack_size = 0;
			/**
			 * @brief How many pages of stack are actually available at launch
			 */
			offset_t committed_stack_size = 0;
			/**
			 * @brief How much of heap should be reserved at launch
			 */
			offset_t reserved_heap_size = 0;
			/**
			 * @brief How many pages of heap are actually available at launch
			 */
			offset_t committed_heap_size = 0;
			/**
			 * @brief Reserved
			 */
			uint32_t loader_flags = 0;

			/** @brief A data directory entry */
			struct DataDirectory
			{
				uint32_t address = 0, size = 0;
			};

			enum
			{
				DirExportTable,
				DirImportTable,
				DirResourceTable,
				DirExceptionTable,
				DirCertificateTable,
				DirBaseRelocationTable,
				DirDebug,
				DirArchitecture, // reserved
				DirGlobalPointer,
				DirTLSTable,
				DirLoadConfigTable,
				DirBoundImport,
				DirIAT,
				DirDelayImportDescriptor,
				DirCLRRuntimeHeader,
				DirReserved,
				DirTotalCount,
			};

			/**
			 * @brief PE specific areas in the file, each one has a specific purpose
			 */
			std::vector<DataDirectory> data_directories;

			/** @brief Whether the file is in the PE32+ (64-bit) format */
			bool Is64Bit() const;

			PEOptionalHeader()
				: AOutHeader()
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			/** @brief Converts a virtual address into an image base relative virtual address */
			uint32_t AddressToRVA(offset_t address) const;
			/** @brief Converts an image base relative virtual address into a virtual address */
			offset_t RVAToAddress(uint32_t rva, bool suppress_on_zero = false) const;

		protected:
			void DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const override;
		};

		/** @brief Retrieves the optional header */
		PEOptionalHeader& GetOptionalHeader();
		/** @brief Retrieves the optional header */
		const PEOptionalHeader& GetOptionalHeader() const;

		/** @brief Whether the file is in the PE32+ (64-bit) format */
		bool Is64Bit() const;

		/** @brief Converts a virtual address into an image base relative virtual address */
		uint32_t AddressToRVA(offset_t address) const;
		/** @brief Converts an image base relative virtual address into a virtual address */
		offset_t RVAToAddress(uint32_t rva, bool suppress_on_zero = false) const;
		/** @brief Converts an image base relative virtual address into a file offset */
		offset_t RVAToFileOffset(uint32_t rva) const;

		/** @brief A section, with the PE extensions */
		class Section : public COFF::COFFFormat::Section
		{
		public:
			// TODO: other flags
			/** @brief The section is discardable */
			static constexpr uint32_t DISCARDABLE = 0x02000000;
			/** @brief The section contains executable code */
			static constexpr uint32_t EXECUTE = 0x20000000;
			/** @brief The section contents are readable to the program */
			static constexpr uint32_t READ = 0x40000000;
			/** @brief The section contents are writable to the program */
			static constexpr uint32_t WRITE = 0x80000000;

			Section(uint32_t flags = 0, std::shared_ptr<Linker::Image> image = nullptr)
				: COFFFormat::Section(flags, image)
			{
			}

			/** @brief The COFF s_paddr field is redefined to contain the size of the section as loaded into memory */
			constexpr offset_t& virtual_size() { return physical_address; }
			/** @brief The COFF s_paddr field is redefined to contain the size of the section as loaded into memory */
			constexpr const offset_t& virtual_size() const { return physical_address; }

			void ReadSectionData(Linker::Reader& rd, const COFFFormat& coff_format) override;
			void WriteSectionData(Linker::Writer& wr, const COFFFormat& coff_format) const override;
			uint32_t ImageSize(const COFFFormat& coff_format) const override;
			void Dump(Dumper::Dumper& dump, const COFFFormat& format, unsigned section_index) const override;

			/** @brief Reads the contents of the section in the file */
			virtual void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt);
			/** @brief Writes the contents of the section to the file */
			virtual void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const;
			/** @brief Retrieves the size of the section, as stored in the file */
			virtual uint32_t ImageSize(const PEFormat& fmt) const;
			/** @brief Retrieves the size of the section, as loaded into memory */
			virtual uint32_t MemorySize(const PEFormat& fmt) const;

			size_t ReadData(size_t bytes, size_t offset, void * buffer) const;
		};

		/** @brief Wrapper around COFFFormat::sections to return PEFormat::Section pointers instead of COFFFormat::Section pointers */
		struct PESections
		{
			const PEFormat * pe;
			struct iterator
			{
				std::vector<std::shared_ptr<COFFFormat::Section>>::iterator coff_iterator;
				bool operator !=(const iterator& other) const;
				iterator& operator++();
				std::shared_ptr<PEFormat::Section> operator*() const;
			};
			struct const_iterator
			{
				std::vector<std::shared_ptr<COFFFormat::Section>>::const_iterator coff_iterator;
				bool operator !=(const const_iterator& other) const;
				const_iterator& operator++();
				std::shared_ptr<const PEFormat::Section> operator*() const;
			};
			iterator begin();
			const_iterator begin() const;
			iterator end();
			const_iterator end() const;
		};

		/** @brief Access the sections as PEFormat::Section */
		const PESections Sections() const;
		/** @brief Access the sections as PEFormat::Section */
		PESections Sections();

		/**
		 * @brief Attempts to find the section:offset pair that encompasses this memory range
		 *
		 * This function can be used to find the data corresponding to some relative virtual address, once loaded into sections.
		 * To fill the entire memory range, this function should be called repeatedly as it crosses section boundaries to retrieve all the parts of the memory.
		 *
		 * @param rva The relative virtual address of the start of the range, relative to the image base as specified in the optional header
		 * @param bytes The number of bytes in this range
		 * @param found_section The section that contains this relative virtual address
		 * @param section_offset The offset within the section that has this relative virtual address
		 * @return The largest number of bytes of this range that still belongs to this section, or 0 if the address does not belong to any section data
		 */
		size_t MapRVAToSectionData(uint32_t rva, size_t bytes, std::shared_ptr<Section>& found_section, size_t& section_offset) const;

		/** @brief Loads a sequence of bytes from the filled section data */
		size_t ReadData(size_t bytes, uint32_t rva, void * buffer) const;
		/** @brief Loads an unsigned word from the filled section data */
		uint64_t ReadUnsigned(size_t bytes, uint32_t rva, ::EndianType endiantype) const;
		/** @brief Loads a signed word from the filled section data */
		uint64_t ReadSigned(size_t bytes, uint32_t rva, ::EndianType endiantype) const;
		/** @brief Loads a sequence of bytes terminated by a specific character from the filled section data */
		std::string ReadASCII(uint32_t rva, char terminator, size_t maximum = size_t(-1)) const;

		/** @brief Represents a resource inside the image */
		class Resource
		{
		public:
			// TODO: this is just a sketch

			/** @brief Represents a resource identifier, either a string or a 32-bit value */
			typedef std::variant<std::string, uint32_t> Reference;

			/** @brief The first level of the resource tree corresponds to the resource type */
			static constexpr size_t Level_Type = 0;
			/** @brief The second level of the resource tree corresponds to the resource name */
			static constexpr size_t Level_Name = 1;
			/** @brief The third level of the resource tree corresponds to the language (locale) for the resource */
			static constexpr size_t Level_Language = 2;
			/** @brief Total number of levels used */
			static constexpr size_t Level_Count = 3;

			/** @brief The sequence of IDs that identifies this resource, conventionally corresponding to the resource type, name and language */
			std::vector<Reference> full_identifier;
			/** @brief The relative virtual address of the resource data */
			uint32_t data_rva = 0;
			/** @brief Codepage of the resource */
			uint32_t codepage = 0;
			/** @brief Reserved entry in the resource table */
			uint32_t reserved = 0;
		};

		class ResourceDirectory
		{
		public:
			// TODO: this is just a sketch

			/** @brief Represents a single entry in the resource directory */
			template <typename Key>
				class Entry
			{
			public:
				/** @brief The value that identifiers the resource at the current level */
				Key identifier;
				/** @brief The contents at this level, either the resource itself, or another level of resource directory */
				std::variant<std::shared_ptr<Resource>, std::shared_ptr<ResourceDirectory>> content;
				/** @brief The relative virtual address of the contents */
				uint32_t content_rva = 0;
			};

			/** @brief Resource directory characteristics */
			uint32_t flags = 0;
			uint32_t timestamp = 0;
			version_type version = { };
			/** @brief Entries that are identified via a string */
			std::vector<Entry<std::string>> name_entries;
			/** @brief Entries that are identified via a number */
			std::vector<Entry<uint32_t>> id_entries;

			/** @brief Inserts a resource into the directory
			 *
			 * @param resource The resource to add, it contains the full identification required to select the write directory entry
			 * @param level The level at which this directory resides, required to determine the identifier component
			 */
			void AddResource(std::shared_ptr<Resource>& resource, size_t level = 0);
		};

		/** @brief Represents an `.rsrc` resource section in the binary */
		class ResourcesSection : public Section, public ResourceDirectory
		{
		public:
			ResourcesSection()
				: Section(DATA | READ)
			{
				name = ".rsrc";
			}

			using Section::ReadSectionData;
			using Section::WriteSectionData;
			using Section::ImageSize;

			bool IsPresent() const;
			void Generate(PEFormat& fmt);
			void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt) override;
			void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const override;
			uint32_t ImageSize(const PEFormat& fmt) const override;
			uint32_t MemorySize(const PEFormat& fmt) const override;

			/** @brief Parses the contents of the directory, after all the section data for the file is loaded */
			void ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size);
			/** @brief Displays the directory contents */
			void DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const;
		};

		/** @brief The resources in this file */
		std::shared_ptr<ResourcesSection> resources = std::make_shared<ResourcesSection>();

		/** @brief A collection of the imported names for a specific dynamic linking library */
		class ImportedLibrary
		{
		public:
			typedef uint16_t Ordinal;

			/** @brief Represents an entry in the hint-name table, corresponding to imports by name */
			class Name
			{
			public:
				/** @brief The hint (ordinal) corresponding to this entry */
				uint16_t hint;
				/** @brief The name by which the entry is identified */
				std::string name = "";
				/** @brief The relative virtual address of the name of the entry */
				uint32_t rva = 0;

				Name(std::string name, uint16_t hint = 0)
					: hint(hint), name(name)
				{
				}
			};

			/** @brief Represents a single entry in the import directory, either an ordinal or a name */
			typedef std::variant<Ordinal, Name> ImportTableEntry;

			/** @brief Relative virtual address for the lookup table, preserved during execution time */
			uint32_t lookup_table_rva = 0;
			/** @brief Relative virtual address for the address table, used to access the imported functions, but has the same layout as the lookup table in the image */
			uint32_t address_table_rva = 0;
			uint32_t timestamp = 0;
			uint32_t forwarder_chain = 0;
			/** @brief Name of the dynamic linking library included */
			std::string name;
			/** @brief Relative virtual address of the name */
			uint32_t name_rva = 0;

			/** @brief List of imported entries */
			std::vector<ImportTableEntry> import_table;
			/** @brief A convenience field to quickly access the imported entry index via its name, must be kept synchronized with import_table */
			std::map<std::string, size_t> imports_by_name;
			/** @brief A convenience field to quickly access the imported entry index via its ordinal (only for import by ordinal, not for hints), must be kept synchronized with import_table */
			std::map<Ordinal, size_t> imports_by_ordinal;

			/** @brief Adds a new imported entry by name and hint, unless an entry by the same name already exists, in which case it does nothing */
			void AddImportByName(std::string entry_name, uint16_t hint);
			/** @brief Adds a new imported entry by ordinal, unless an entry by the same ordinal already exists, in which case it does nothing */
			void AddImportByOrdinal(uint16_t ordinal);

			/** @brief Retrieves the virtual address of an already registered entry that is imported by name */
			offset_t GetImportByNameAddress(const PEFormat& fmt, std::string name);
			/** @brief Retrieves the virtual address of an already registered entry that is imported by ordinal */
			offset_t GetImportByOrdinalAddress(const PEFormat& fmt, uint16_t ordinal);

			ImportedLibrary(std::string name)
				: name(name)
			{
			}
		};

		/** @brief Represents an `.idata` section for imported DLLs in the binary */
		class ImportsSection : public Section
		{
		public:
			/** @brief The sequence of all import directories, one for each DLL */
			std::vector<ImportedLibrary> libraries;
			/** @brief A convenience field to access the DLL index by its name */
			std::map<std::string, size_t> library_indexes;

			/** @brief Relative virtual address of the import address table, to be stored in the optional header */
			uint32_t address_table_rva;
			/** @brief Size of the import address table, to be stored in the optional header */
			uint32_t address_table_size;

			ImportsSection()
				: Section(DATA | READ | WRITE)
			{
				name = ".idata";
			}

			using Section::ReadSectionData;
			using Section::WriteSectionData;
			using Section::ImageSize;

			bool IsPresent() const;
			void Generate(PEFormat& fmt);
			void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt) override;
			void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const override;
			uint32_t ImageSize(const PEFormat& fmt) const override;
			uint32_t MemorySize(const PEFormat& fmt) const override;

			/** @brief Parses the contents of the directory, after all the section data for the file is loaded */
			void ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size);
			/** @brief Displays the directory contents */
			void DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const;
		};

		/** @brief The collection of imports in the file */
		std::shared_ptr<ImportsSection> imports = std::make_shared<ImportsSection>();

		/** @brief Represents a single exported entry in the file */
		class ExportedEntry
		{
		public:
			/** @brief Encompasses the information necessary to encode a forwarder exported reference
			 *
			 * Forwarder exported references contain the name of a different library and the name or ordinal to one of its entries.
			 * It enables to reexport an already existing entry from a different library via the current library.
			 */
			class Forwarder
			{
			public:
				/** @brief The name of the library whose entry is being reexported, this field will not appear directly in the image */
				std::string dll_name;
				/** @brief The name or ordinal to the entry that is being reexported, this field will not appear directly in the image */
				std::variant<std::string, uint32_t> reference;
				/** @brief A string representation of the forwarder, to appear in the image */
				std::string reference_name;
				/** @brief The relative virtual address of the reference name */
				uint32_t rva;
			};

			/** @brief Represents a name by which an exported entry is identified */
			class Name
			{
			public:
				/** @brief The name, as a string */
				std::string name;
				/** @brief The relative virtual address of the name string */
				uint32_t rva = 0;

				Name(std::string name)
					: name(name)
				{
				}
			};

			/** @brief Represents the associated value of the entry: either the relative virtual address of the object being exported, or a forwarder entry */
			std::variant<uint32_t, Forwarder> value;
			/** @brief An optional name by which the exported entry is identified, otherwise its position in the export table (its ordinal) can be used only */
			std::optional<Name> name;

			ExportedEntry(uint32_t rva)
				: value(rva), name()
			{
			}

			ExportedEntry(uint32_t rva, std::string name)
				: value(rva), name(name)
			{
			}

			ExportedEntry(Forwarder fwd)
				: value(fwd), name()
			{
			}
		};

		/** @brief Represents an `.edata` section for the exported entries in the binary */
		class ExportsSection : public Section
		{
		public:
			uint32_t flags = 0;
			uint32_t timestamp = 0;
			version_type version = { };
			/** @brief The name of this library */
			std::string dll_name;
			/** @brief The relative virtual address of the library name string */
			uint32_t dll_name_rva = 0;
			/** @brief The starting ordinal for the export table */
			uint32_t ordinal_base = 1;
			/** @brief Relative virtual address of the export address table, containing the exported entries for each successive ordinal value */
			uint32_t address_table_rva = 0;
			/** @brief Relative virtual address of the name pointer table, containing the relative virtual addresses of the names of each entry exported by name */
			uint32_t name_table_rva = 0;
			/** @brief Relative virtual address of the ordinal table, containing the ordinals for each entry exported by name */
			uint32_t ordinal_table_rva = 0;
			/** @brief Sequence of entries, the first entry corresponds to ordinal_base */
			std::vector<std::optional<std::shared_ptr<ExportedEntry>>> entries;
			/** @brief A collection of exports by name, with each string being associated with its ordinal */
			std::map<std::string, uint32_t> named_exports;

			/** @brief Sets a specific entry, corresponding to some ordinal */
			void SetEntry(uint32_t ordinal, std::shared_ptr<ExportedEntry> entry);
			/** @brief Adds an entry to an unused ordinal (for speed, we access the highest taken ordinal and assign this entry to the next ordinal) */
			void AddEntry(std::shared_ptr<ExportedEntry> entry);

			ExportsSection()
				: Section(DATA | READ)
			{
				name = ".edata";
				// TODO: flags and other fields
			}

			using Section::ReadSectionData;
			using Section::WriteSectionData;
			using Section::ImageSize;

			bool IsPresent() const;
			void Generate(PEFormat& fmt);
			void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt) override;
			void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const override;
			uint32_t ImageSize(const PEFormat& fmt) const override;
			uint32_t MemorySize(const PEFormat& fmt) const override;

			/** @brief Parses the contents of the directory, after all the section data for the file is loaded */
			void ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size);
			/** @brief Displays the directory contents */
			void DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const;
		};

		/** @brief The collection of exported symbols */
		std::shared_ptr<ExportsSection> exports = std::make_shared<ExportsSection>();

		/** @brief Represents a base relocation, as stored in the image */
		class BaseRelocation
		{
		public:
			/** @brief The type of the relocation, partly dependent on the current CPU */
			enum relocation_type
			{
				/** @brief No relocation */
				RelAbsolute = 0,
				/** @brief The most significant 16 bits of the address */
				RelHigh = 1,
				/** @brief The least significant 16 bits of the address */
				RelLow = 2,
				/** @brief A full 32-bit address */
				RelHighLow = 3,
				/** @brief The most significant 16 bits of the address, with the currently assumed least significant 16 bits stored in the next relocation entry in the base relocation table */
				RelHighAdj = 4,
				/** @brief MIPS specific */
				RelMIPSJmpAddr = 5,
				/** @brief ARM specific */
				RelARMMov32 = 5,
				/** @brief RISC-V specific */
				RelRISCVHigh20 = 5,
				/** @brief Thumb (ARM) specific */
				RelThumbMov32 = 7,
				/** @brief RISC-V specific */
				RelRISCVLow12I = 7,
				/** @brief RISC-V specific */
				RelRISCVLow12S = 8,
				/** @brief LoongArch specific */
				RelLoongArch32MarkLA = 8,
				/** @brief LoongArch specific */
				RelLoongArch64MarkLA = 8,
				/** @brief MIPS specific */
				RelMIPSJmpAddr16 = 9,
				/** @brief A full 64-bit address */
				RelDir64 = 10,
			};
			/** @brief The type of the relocation, stored as the 4 most significant bits of a 16-bit base relocation entry */
			relocation_type type;
			/** @brief The offset of the relocation within the page corresponding to this base relocation block, stored as the 12 least significant bits of a 16-bit base relocation entry */
			uint16_t offset;
			/** @brief For relocation type HighAdj, the 16-bit value stored in the following base relocation entry */
			uint16_t parameter = 0;

			/** @brief The number of base relocation entries required to store this base relocation, typically 1 */
			size_t GetEntryCount(const PEFormat * format) const;
			/** @brief The number of bytes impacted by this relocation (typically 2 or 4 or 8) */
			size_t GetRelocationSize(const PEFormat * format) const;
		};

		/** @brief A block of base relocations, corresponding to a single page in the image */
		class BaseRelocationBlock
		{
		public:
			/** @brief Pages are 4 KiB long */
			static constexpr uint32_t PAGE_SIZE = 0x1000;
			/** @brief The relative virtual address of the page this block corresponds to, with the least significant 12 bits set to 0 */
			uint32_t page_rva;
			/** @brief Number of bytes in this block, including the initial 8 bytes for the relative virtual address of the page and the size of the block */
			uint32_t block_size;
			/** @brief Sequence of relocations in this block, filled in by the linker once all the relocations have been collected */
			std::vector<BaseRelocation> relocations_list;
			/** @brief Collection of relocations, accessed via their page offset */
			std::map<uint16_t, BaseRelocation> relocations_map;

			BaseRelocationBlock(uint32_t page_rva = 0)
				: page_rva(page_rva & ~(PAGE_SIZE - 1))
			{
			}
		};

		/** @brief Represents a `.reloc` section for the base relocations for the binary */
		class BaseRelocationsSection : public Section
		{
		public:
			/** @brief Sequence of relocation blocks, filled in by the linker once all the relocations have been collected */
			std::vector<std::shared_ptr<BaseRelocationBlock>> blocks_list;
			/** @brief Collection of relocation blocks, accessed via the relative virtual address of the page */
			std::map<uint32_t, std::shared_ptr<BaseRelocationBlock>> blocks_map;

			BaseRelocationsSection()
				: Section(DATA | DISCARDABLE | READ)
			{
				name = ".reloc";
			}

			using Section::ReadSectionData;
			using Section::WriteSectionData;
			using Section::ImageSize;

			bool IsPresent() const;
			void Generate(PEFormat& fmt);
			void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt) override;
			void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const override;
			uint32_t ImageSize(const PEFormat& fmt) const override;
			uint32_t MemorySize(const PEFormat& fmt) const override;

			/** @brief Parses the contents of the directory, after all the section data for the file is loaded */
			void ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size);
			/** @brief Displays the directory contents */
			void DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const;
		};

		/** @brief The collection of base relocations */
		std::shared_ptr<BaseRelocationsSection> base_relocations = std::make_shared<BaseRelocationsSection>();

		/** @brief Adds a base relocation at the specified relative virtual address
		 *
		 * @param rva The relative virtual address where the relocation appears, to be split into the page address and the page offset
		 * @param type The type of the base relocation
		 * @param low_ref Only used for HIGHADJ relocations, the parameter to be stored after the base relocation entry
		 */
		void AddBaseRelocation(uint32_t rva, BaseRelocation::relocation_type type, uint16_t low_ref = 0);

		mutable MZStubWriter stub;

		void ReadFile(Linker::Reader& rd) override;

		::EndianType GetMachineEndianType() const;
		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

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
			/** @brief Attempts to mimic the output of the GNU (MinGW) linker, this is undefined for the LE output */
			CompatibleGNU,
		};

		compatibility_type compatibility = CompatibleNone;

		/** @brief Represents settings and assumptions about the target */
		enum target_type
		{
			/** @brief Windows 95/98/ME */
			TargetWin9x = 1,
			/** @brief Windows NT kernel */
			TargetWinNT,
			/** @brief Windows CE kernel */
			TargetWinCE,
			/** @brief Windows Portability Library */
			TargetMacintosh, // TODO: not supported
			/** @brief Win32s compatibility library for Windows 3.1 */
			TargetWin32s,
			/** @brief Phar Lap TNT DOS Extender */
			TargetTNT, // TODO: not supported
			/** @brief EFI and UEFI firmware */
			TargetEFI, // TODO: not supported
			/** @brief .NET environment */
			TargetDotNET, // TODO: not supported
			/** @brief Xbox */
			TargetXbox, // TODO: not supported
			//TargetBeOS, // TODO: possible target
			//TargetSkyOS, // TODO: possible target
			//TargetHXDOS, // TODO: possible target
		};
		/** @brief The expected target system, only used for setting up default values */
		target_type target = target_type(0);

		/** @brief Represents the target image type */
		enum output_type
		{
			/** @brief An executable image, conventionally taking the suffix `.exe` */
			OUTPUT_EXE,
			/** @brief A dynamically linked library, conventionally taking the suffix `.dll` */
			OUTPUT_DLL,
			/** @brief A system file or driver, conventionally taking the suffix `.sys` */
			OUTPUT_SYS,
		};
		/*** @brief Whether to generate an executable, library or system file */
		output_type output = OUTPUT_EXE;

		PEFormat(target_type target = TargetWinNT, PEOptionalHeader::SubsystemType subsystem = PEOptionalHeader::WindowsGUI, output_type output = OUTPUT_EXE)
			: COFFFormat(WINDOWS, PECOFF, ::LittleEndian), target(target), output(output)
		{
			optional_header = std::make_unique<PEOptionalHeader>();
			GetOptionalHeader().subsystem = subsystem;
		}

	public:
		/* * * Writer members * * */

		/** @brief Make generated image relocatable (TODO: expose as command line option) */
		bool option_relocatable = true;

		/** @brief Certain header flags are deprecated and should be zero, this option sets them if applicable (TODO: expose as command line option) */
		bool option_include_deprecated_flags = false;

		/** @brief Include COFF line numbers (TODO: not implemented) */
		bool option_coff_line_numbers = false;

		/** @brief Include COFF local symbols (TODO: not implemented) */
		bool option_coff_local_symbols = false;

		/** @brief Include debug information (TODO: not implemented) */
		bool option_debug_info = false;

		/** @brief By default, imported labels address the import address table directly */
		bool option_import_thunks = false;

		Linker::Module * current_module = nullptr;
		std::map<std::pair<std::string, std::string>, uint32_t> import_thunks_by_name;
		std::map<std::pair<std::string, uint16_t>, uint32_t> import_thunks_by_ordinal;

		class PEOptionCollector : public Linker::OptionCollector
		{
		public:
			class TargetEnumeration : public Linker::Enumeration<target_type>
			{
			public:
				TargetEnumeration()
					: Enumeration(
						"WIN9X", TargetWin9x,
						"WINNT", TargetWinNT,
						"WINCE", TargetWinCE,
						"MACINTOSH", TargetMacintosh,
						"WIN32S", TargetWin32s,
						"TNT", TargetTNT,
						"EFI", TargetEFI,
						"DOTNET", TargetDotNET,
						"XBOX", TargetXbox)
				{
					descriptions = {
						{ TargetWin9x, "targets Windows 95/98/ME" },
						{ TargetWinNT, "targets Windows NT" },
						{ TargetWinCE, "targets Windows CE/Windows Phone/Windows Mobile (pre-8)" },
						{ TargetMacintosh, "targets the Windows Portability Library on Macintosh (not implemented)" },
						{ TargetWin32s, "targets the Win32s layer for Windows 3.11" },
						{ TargetTNT, "targets the Phar Lap TNT DOS Extender (not implemented)" },
						{ TargetEFI, "targets EFI/UEFI (not implemented)" },
						{ TargetDotNET, "targets the .NET environment (not implemented)" },
						{ TargetXbox, "targets Xbox (not implemented)" },
						//{ TargetBeOS, "targets BeOS x86 up to Release 3.2" },
						//{ TargetSkyOS, "targets SkyOS" },
						//{ TargetHXDOS, "targets the HX-DOS DOS extender" },
					};
				}
			};

			enum subsystem_determination
			{
				Subsystem_Windows = 1,
				Subsystem_Console,
				Subsystem_Native,
				Subsystem_OS2,
				Subsystem_POSIX,
				Subsystem_EFIApplication,
				Subsystem_EFIBootServiceDriver,
				Subsystem_EFIRuntimeDriver,
				Subsystem_EFI_ROM,
				Subsystem_BootApplication,
			};

			class SubsystemEnumeration : public Linker::Enumeration<subsystem_determination>
			{
			public:
				SubsystemEnumeration()
					: Enumeration(
						"WINDOWS", Subsystem_Windows,
						"CONSOLE", Subsystem_Console,
						"NATIVE", Subsystem_Native,
						"OS2", Subsystem_OS2,
						"POSIX", Subsystem_POSIX,
						"EFI_APPLICATION", Subsystem_EFIApplication,
						"EFI_BOOT_SERVICE_DRIVER", Subsystem_EFIBootServiceDriver,
						"EFI_RUNTIME_DRIVER", Subsystem_EFIRuntimeDriver,
						"EFI_ROM", Subsystem_EFI_ROM,
						"BOOT_APPLICATION", Subsystem_BootApplication)
				{
					descriptions = {
						{ Subsystem_Windows, "creates a graphical program" },
						{ Subsystem_Console, "creates a console (text mode) program" },
						{ Subsystem_Native, "creates a native Windows NT kernel program" },
						{ Subsystem_OS2, "creates a program that uses the OS/2 personality" },
						{ Subsystem_POSIX, "creates a program that uses the POSIX personality" },
						{ Subsystem_EFIApplication, "creates an EFI application" },
						{ Subsystem_EFIBootServiceDriver, "creates an EFI boot service driver" },
						{ Subsystem_EFIRuntimeDriver, "creates an EFI run-time driver" },
						{ Subsystem_EFI_ROM, "creates an EFI ROM image" },
						{ Subsystem_BootApplication, "creates a Windows boot application" },
					};
				}
			};

			class OutputTypeEnumeration : public Linker::Enumeration<output_type>
			{
			public:
				OutputTypeEnumeration()
					: Enumeration(
						"EXE", OUTPUT_EXE,
						"DLL", OUTPUT_DLL,
						"SYS", OUTPUT_SYS)
				{
					descriptions = {
						{ OUTPUT_EXE, "creates an executable" },
						{ OUTPUT_DLL, "creates a dynamic linking library (DLL)" },
						{ OUTPUT_SYS, "creates a system program" }
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
						"BORLAND", CompatibleBorland,
						"GNU", CompatibleGNU)
				{
					descriptions = {
						{ CompatibleNone, "default operation" },
						{ CompatibleWatcom, "mimics the Watcom linker (not implemented)" },
						{ CompatibleMicrosoft, "mimics the Microsoft linker (not implemented)" },
						{ CompatibleBorland, "mimics the Borland linker (not implemented)" },
						{ CompatibleGNU, "mimics the GNU (MinGW) linker (not implemented)" },
					};
				}
			};

			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<Linker::ItemOf<TargetEnumeration>> target{"target", "Windows target type"};
			Linker::Option<Linker::ItemOf<SubsystemEnumeration>> subsystem{"subsystem", "Windows subsystem to target"};
			Linker::Option<Linker::ItemOf<OutputTypeEnumeration>> output{"output", "Output types"};
			Linker::Option<Linker::ItemOf<CompatibilityEnumeration>> compat{"compat", "Mimics the behavior of another linker"};
			Linker::Option<offset_t> image_base{"base", "Base address of image, used for calculating relative virtual addresses"};
			Linker::Option<offset_t> section_align{"section_align", "Section alignment"};
			Linker::Option<bool> import_thunks{"import_thunks", "Create thunk procedures for imported names"};

			PEOptionCollector()
			{
				InitializeFields(stub, target, subsystem, output, compat, image_base, section_align, import_thunks);
			}
		};

		static std::shared_ptr<PEFormat> CreateConsoleApplication(target_type target = TargetWinNT, PEOptionalHeader::SubsystemType subsystem = PEOptionalHeader::WindowsCUI);

		static std::shared_ptr<PEFormat> CreateGUIApplication(target_type target = TargetWinNT, PEOptionalHeader::SubsystemType subsystem = PEOptionalHeader::WindowsGUI);

		static std::shared_ptr<PEFormat> CreateLibraryModule(target_type target = TargetWinNT);

		static std::shared_ptr<PEFormat> CreateDeviceDriver(target_type target = TargetWinNT);

		std::shared_ptr<PEFormat> SimulateLinker(compatibility_type compatibility);

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		std::shared_ptr<Resource>& AddResource(std::shared_ptr<Resource>& resource);

		ImportedLibrary& FetchImportLibrary(std::string library_name, bool create_if_not_present = false);
		void AddImportByName(std::string library_name, std::string entry_name, uint16_t hint);
		void AddImportByOrdinal(std::string library_name, uint16_t ordinal);

		static std::vector<Linker::OptionDescription<void> *> ParameterNames;
		std::vector<Linker::OptionDescription<void> *> GetLinkerScriptParameterNames() override;
		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		/** @brief COFF file header flags, most of these are obsolete, we only use them as precombined flag sets */
		enum
		{
			/** @brief IMAGE_FILE_RELOCS_STRIPPED (F_RELFLG) */
			FLAG_NO_RELOCATIONS = 0x0001, // already defined in COFF
			/** @brief IMAGE_FILE_EXECUTABLE_IMAGE (F_EXEC) */
			FLAG_EXECUTABLE = 0x0002, // already defined in COFF
			/** @brief (deprecated) IMAGE_FILE_LINE_NUMS_STRIPPED (F_LNNO) */
			FLAG_NO_LINE_NUMBERS = 0x0004, // already defined in COFF
			/** @brief (deprecated) IMAGE_FILE_LOCAL_SYMS_STRIPPED (F_LSYMS) */
			FLAG_NO_SYMBOLS = 0x0008, // already defined in COFF
			/** @brief (deprecated) IMAGE_FILE_AGGRESSIVE_WS_TRIM (no COFF meaning) */
			FLAG_TRIM_WORKING_SET = 0x0010,
			/** @brief IMAGE_FILE_LARGE_ADDRESS_AWARE (no COFF meaning) */
			FLAG_LARGE_ADDRESS = 0x0020,
			/** @brief (deprecated) IMAGE_FILE_BYTES_REVERSED_LO (redefined from COFF F_AR16WR) */
			FLAG_LITTLE_ENDIAN = 0x0080,
			/** @brief IMAGE_FILE_32BIT_MACHINE (redefined from COFF F_AR32WR) */
			FLAG_32BIT = 0x0100,
			/** @brief IMAGE_FILE_DEBUG_STRIPPED (redefined from COFF F_AR32W) */
			FLAG_NO_DEBUG = 0x0200,
			/** @brief IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP */
			FLAG_ON_REMOVABLE = 0x0400,
			/** @brief IMAGE_FILE_NET_RUN_FROM_SWAP */
			FLAG_ON_NETWORK = 0x0800,
			/** @brief IMAGE_FILE_SYSTEM */
			FLAG_SYSTEM = 0x1000,
			/** @brief IMAGE_FILE_DLL */
			FLAG_LIBRARY = 0x2000,
			/** @brief IMAGE_FILE_UP_SYSTEM_ONLY */
			FLAG_UNIPROCESSOR_ONLY = 0x4000,
			/** @brief (deprecated) IMAGE_FILE_BYTES_REVERSED_HI */
			FLAG_BIG_ENDIAN = 0x8000,
		};

		void SetOptions(std::map<std::string, std::string>& options) override;
		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);
		void Link(Linker::Module& module);
		void ProcessModule(Linker::Module& module) override;
		void GenerateFile(std::string filename, Linker::Module& module) override;
		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;

		/**
		 * @brief Represents a field that displays a relative virtual address and file offset pair
		 */
		class RVADisplay : public Dumper::Display<offset_t>
		{
		public:
			const PEFormat * pe;
			unsigned width;
			RVADisplay(const PEFormat * pe, unsigned width)
				: pe(pe), width(width)
			{
			}

			void DisplayValue(Dumper::Dumper& dump, std::tuple<offset_t> values) override;
		};

		std::shared_ptr<RVADisplay> MakeRVADisplay(unsigned width = 8) const;
	};
}

#endif /* PEEXE_H */
