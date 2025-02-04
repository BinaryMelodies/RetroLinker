#ifndef PEEXE_H
#define PEEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/segment_manager.h"
#include "coff.h"
#include "mzexe.h"

/* TODO: unimplemented */

namespace Microsoft
{
	/**
	 * @brief Microsoft PE .EXE portable executable file format
	 */
	class PEFormat : public COFF::COFFFormat, protected Microsoft::MZStubWriter
	{
	public:
		char pe_signature[4];

		class PEOptionalHeader : public AOutHeader
		{
		public:
			static constexpr uint16_t ROM32 = 0x0107;
			static constexpr uint16_t EXE32 = 0x010B;
			static constexpr uint16_t EXE64 = 0x020B;

			static constexpr offset_t Win32Base = 0x00400000;
			static constexpr offset_t Dll32Base = 0x10000000;
			static constexpr offset_t WinCEBase = 0x00010000;
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
			/** @brief Represents a version entry with major and minor versions */
			struct version_type
			{
				uint16_t major, minor;
			};
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

			uint32_t GetSize() override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) override;

			offset_t CalculateValues(COFFFormat& coff) override;

		protected:
			void DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) override;
		};

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

	protected:
		void ReadOptionalHeader(Linker::Reader& rd);
		using Microsoft::MZStubWriter::WriteStubImage;
	};
}

#endif /* PEEXE_H */
