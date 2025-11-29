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

		class PEOptionalHeader : public AOutHeader
		{
		public:
			static constexpr uint16_t ROM32 = 0x0107;
			static constexpr uint16_t EXE32 = 0x010B;
			static constexpr uint16_t EXE64 = 0x020B;

			static constexpr offset_t Win32Base = 0x00400000;
			static constexpr offset_t Dll32Base = 0x10000000;
			static constexpr offset_t WinCEBase = 0x00010000;
			static constexpr offset_t Win64Base = 0x140000000;
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
			 * @brief Cumulative size of all the headers, including the stub
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

			bool Is64Bit() const;

			PEOptionalHeader()
				: AOutHeader()
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			uint32_t AddressToRVA(offset_t address) const;
			offset_t RVAToAddress(uint32_t rva, bool suppress_on_zero = false) const;

		protected:
			void DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const override;
		};

		PEOptionalHeader& GetOptionalHeader();
		const PEOptionalHeader& GetOptionalHeader() const;

		bool Is64Bit() const;

		uint32_t AddressToRVA(offset_t address) const;
		offset_t RVAToAddress(uint32_t rva, bool suppress_on_zero = false) const;

		class Section : public COFF::COFFFormat::Section
		{
		public:
			// TODO: other flags
			static constexpr uint32_t DISCARDABLE = 0x02000000;
			static constexpr uint32_t EXECUTE = 0x20000000;
			static constexpr uint32_t READ = 0x40000000;
			static constexpr uint32_t WRITE = 0x80000000;

			Section(uint32_t flags = 0, std::shared_ptr<Linker::Image> image = nullptr)
				: COFFFormat::Section(flags, image)
			{
			}

			constexpr offset_t& virtual_size() { return physical_address; }
			constexpr const offset_t& virtual_size() const { return physical_address; }

			constexpr offset_t& relative_virtual_address() { return address; }
			constexpr const offset_t& relative_virtual_address() const { return address; }

			void ReadSectionData(Linker::Reader& rd, const COFFFormat& coff_format) override;
			void WriteSectionData(Linker::Writer& wr, const COFFFormat& coff_format) const override;
			uint32_t ImageSize(const COFFFormat& coff_format) const override;

			virtual void ReadSectionData(Linker::Reader& rd, const PEFormat& fmt);
			virtual void WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const;
			virtual uint32_t ImageSize(const PEFormat& fmt) const;
			virtual uint32_t MemorySize(const PEFormat& fmt) const;
		};

		class Resource
		{
		public:
			// TODO: this is just a sketch

			typedef std::variant<std::string, uint32_t> Reference;

			static constexpr size_t Level_Type = 0;
			static constexpr size_t Level_Name = 1;
			static constexpr size_t Level_Language = 2;
			static constexpr size_t Level_Count = 3;

			std::vector<Reference> full_identifier;
			uint32_t data_rva;
			uint32_t codepage;
			uint32_t reserved = 0;
		};

		class ResourceDirectory
		{
		public:
			// TODO: this is just a sketch

			template <typename Key>
				class Entry
			{
			public:
				Key identifier;
				std::variant<std::shared_ptr<Resource>, std::shared_ptr<ResourceDirectory>> content;
				uint32_t content_rva = 0;
			};

			uint32_t flags = 0;
			uint32_t timestamp = 0;
			version_type version = { };
			std::vector<Entry<std::string>> name_entries;
			std::vector<Entry<uint32_t>> id_entries;

			void AddResource(std::shared_ptr<Resource>& resource, size_t level = 0);
		};

		class ResourcesSection : public Section, public ResourceDirectory
		{
		public:
			ResourcesSection()
				: Section(DATA | READ)
			{
				name = ".rsrc";
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
		};

		std::shared_ptr<ResourcesSection> resources = std::make_shared<ResourcesSection>();

		class ImportDirectory
		{
		public:
			typedef uint16_t Ordinal;
			class Name
			{
			public:
				uint16_t hint;
				std::string name = "";
				uint32_t rva = 0;

				Name(std::string name, uint16_t hint = 0)
					: hint(hint), name(name)
				{
				}
			};

			typedef std::variant<Ordinal, Name> ImportTableEntry;

			uint32_t lookup_table_rva = 0;
			uint32_t address_table_rva = 0;
			uint32_t timestamp = 0;
			uint32_t forwarder_chain = 0;
			std::string name;
			uint32_t name_rva = 0;

			std::vector<ImportTableEntry> import_table;
			std::map<std::string, size_t> imports_by_name;
			std::map<Ordinal, size_t> imports_by_ordinal;

			void AddImportByName(std::string entry_name, uint16_t hint);
			void AddImportByOrdinal(uint16_t ordinal);

			offset_t GetImportByNameAddress(const PEFormat& fmt, std::string name);
			offset_t GetImportByOrdinalAddress(const PEFormat& fmt, uint16_t ordinal);

			ImportDirectory(std::string name)
				: name(name)
			{
			}
		};

		class ImportsSection : public Section
		{
		public:
			std::vector<ImportDirectory> directories;
			std::map<std::string, size_t> library_indexes;

			uint32_t address_table_rva;
			uint32_t address_table_size;

			ImportsSection()
				: Section(DATA | READ)
			{
				name = ".idata";
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
		};

		std::shared_ptr<ImportsSection> imports = std::make_shared<ImportsSection>();

		class Export
		{
		public:
			class Forwarder
			{
			public:
				std::string dll_name;
				std::variant<std::string, uint32_t> reference;
				std::string reference_name;
				uint32_t rva;
			};

			class Name
			{
			public:
				std::string name;
				uint32_t rva = 0;

				Name(std::string name)
					: name(name)
				{
				}
			};

			std::variant<uint32_t, Forwarder> value;
			std::optional<Name> name;

			Export(uint32_t rva)
				: value(rva), name()
			{
			}

			Export(uint32_t rva, std::string name)
				: value(rva), name(name)
			{
			}
		};

		class ExportsSection : public Section
		{
		public:
			uint32_t flags = 0;
			uint32_t timestamp = 0;
			version_type version = { };
			std::string dll_name;
			uint32_t dll_name_rva = 0;
			uint32_t ordinal_base = 0;
			uint32_t address_table_rva = 0;
			uint32_t name_table_rva = 0;
			uint32_t ordinal_table_rva = 0;
			std::vector<std::optional<std::shared_ptr<Export>>> entries;
			std::map<std::string, uint32_t> named_exports;

			void SetEntry(uint32_t ordinal, std::shared_ptr<Export> entry);
			void AddEntry(std::shared_ptr<Export> entry);

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
		};

		std::shared_ptr<ExportsSection> exports = std::make_shared<ExportsSection>();

		class BaseRelocation
		{
		public:
			enum relocation_type
			{
				RelAbsolute = 0,
				RelHigh = 1,
				RelLow = 2,
				RelHighLow = 3,
				RelHighAdj = 4,
				RelMIPSJmpAddr = 5,
				RelARMMov32 = 5,
				RelRISCVHigh20 = 5,
				RelThumbMov32 = 7,
				RelRISCVLow12I = 7,
				RelRISCVLow12S = 8,
				RelLoongArch32MarkLA = 8,
				RelLoongArch64MarkLA = 8,
				RelMIPSJmpAddr16 = 9,
				RelDir64 = 10,
			};
			relocation_type type;
			uint16_t offset;
			uint16_t parameter = 0;

			size_t GetEntryCount(const PEFormat * format) const;
			size_t GetRelocationSize(const PEFormat * format) const;
		};

		class BaseRelocationBlock
		{
		public:
			static constexpr uint32_t PAGE_SIZE = 0x1000;
			uint32_t page_rva;
			uint32_t block_size;
			std::vector<BaseRelocation> relocations_list;
			std::map<uint16_t, BaseRelocation> relocations_map;

			BaseRelocationBlock(uint32_t page_rva = 0)
				: page_rva(page_rva & ~(PAGE_SIZE - 1))
			{
			}
		};

		class BaseRelocationsSection : public Section
		{
		public:
			std::vector<std::shared_ptr<BaseRelocationBlock>> blocks_list;
			std::map<uint32_t, std::shared_ptr<BaseRelocationBlock>> blocks_map;

			BaseRelocationsSection()
				: Section(DATA | DISCARDABLE | READ)
			{
				name = ".reloc";
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
		};

		std::shared_ptr<BaseRelocationsSection> base_relocations = std::make_shared<BaseRelocationsSection>();

		void AddBaseRelocation(uint32_t rva, BaseRelocation::relocation_type type, uint16_t low_ref = 0);

		mutable MZStubWriter stub;

		void ReadFile(Linker::Reader& rd) override;

		::EndianType GetMachineEndianType() const;
		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;

		PEFormat()
			: COFFFormat(WINDOWS, PECOFF, ::LittleEndian)
		{
			optional_header = std::make_unique<PEOptionalHeader>();
		}

	protected:
		void ReadOptionalHeader(Linker::Reader& rd);

	public:
		/* * * Writer members * * */

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
			//TargetBeOS,
			//TargetSkyOS,
			//TargetHXDOS,
		};
		target_type target = target_type(0);

		enum output_type
		{
			OUTPUT_EXE,
			OUTPUT_DLL,
			OUTPUT_SYS,
		};
		output_type output = OUTPUT_EXE;

		/** @brief Make generated image relocatable */
		bool option_relocatable = true;

		/** @brief Certain header flags are deprecated and should be zero, this option sets them if applicable */
		bool option_include_deprecated_flags = false;

		/** @brief Include COFF line numbers (TODO: not implemented) */
		bool option_coff_line_numbers = false;

		/** @brief Include COFF local symbols (TODO: not implemented) */
		bool option_coff_local_symbols = false;

		/** @brief Include debug information (TODO: not implemented) */
		bool option_debug_info = false;

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
				}
			};

			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			Linker::Option<Linker::ItemOf<TargetEnumeration>> target{"target", "Windows target type"};
			Linker::Option<Linker::ItemOf<SubsystemEnumeration>> subsystem{"subsystem", "Windows subsystem to target"};
			Linker::Option<Linker::ItemOf<OutputTypeEnumeration>> output{"output", "Output types"};
			Linker::Option<offset_t> image_base{"base", "Base address of image, used for calculating relative virtual addresses"};
			Linker::Option<offset_t> section_align{"section_align", "Section alignment"};

			PEOptionCollector()
			{
				InitializeFields(stub, target, subsystem, output, image_base, section_align);
			}
		};

		bool FormatSupportsLibraries() const override;

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		std::shared_ptr<Resource>& AddResource(std::shared_ptr<Resource>& resource);

		ImportDirectory& FetchImportLibrary(std::string library_name, bool create_if_not_present = false);
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
	};
}

#endif /* PEEXE_H */
