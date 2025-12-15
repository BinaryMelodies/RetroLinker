#ifndef EPOC_H
#define EPOC_H

#include <array>
#include "../linker/format.h"

namespace EPOC
{
	class SymbianFormat : public Linker::Format
	{
	public:
		/* * * General members * * */
		enum executable_type
		{
			KDirectFileStoreLayoutUid = 0x10000037,
			KPermanentFileStoreLayoutUid = 0x10000050,
			KDynamicLibraryUid = 0x10000079,
			KExecutableImageUid = 0x1000007A,
		};
		executable_type uid1 = executable_type(0);

		enum application_type
		{
			/* for KDirectFileStoreLayoutUid or KPermanentFileStoreLayoutUid */
			_OPLObjectModule = 0x10000073,
			_OPLApplication = 0x10000074,
			KUidAppDllDoc = 0x10003A12,
			KUidAppInfoFile = 0x10003A38,
			/* for KDynamicLibraryUid */
			_OPLExtensions = 0x1000005D,
			_Application = 0x1000006C,
			KSharedLibraryUid = 0x1000008D,
			KLogicalDeviceDriverUid8 = 0x100000AE,
			KLogicalDeviceDriverUid16 = 0x100000AF,
			KUidApp = 0x100039CE,
			KPhysicalDeviceDriverUid16 = 0x100039D0,
			KPhysicalDeviceDriverUid8 = 0x100000AD,
			/* ? */
			KMachineConfigurationUid = 0x100000F4,
			KLocaleDllUid16 = 0x100039E6,
			KLocaleDllUid8 = 0x100000C3,
			KKeyboardUid = 0x100000DB,
			KEka1EntryStubUid = 0x101FDF0F,
			KKeyboardDataUid16 = 0x100039E0,
			KKeyboardDataUid8 = 0x100000DC,
			KKeyboardTranUid16 = 0x100039E1,
			KKeyboardTranUid8 = 0x100000DD,
			KConsoleDllUid16 = 0x100039E7,
			KConsoleDllUid8 = 0x100000C5,
			KSystemStartupModeKey = 0x10204BB5,
			KSystemEmulatorOrientationKey = 0x10204BB6,
			KServerProcessUid = 0x1000008C,
			KFileSystemUid = 0x1000008F,
			KFileServerUid = 0x100000BB,
			KLocalFileSystemUid = 0x100000D6,
			KFileServerDllUid = 0x100000BD,
		};
		application_type uid2 = application_type(0);

		uint32_t uid3 = 0;
		uint32_t uid_checksum = 0;

		bool new_format = true;

		enum cpu_type
		{
			ECpuUnknown = 0,
			ECpuX86 = 0x1000,
			ECpuArmV4 = 0x2000,
			ECpuArmV5 = 0x2001,
			ECpuArmV6 = 0x2002,
			ECpuMCore = 0x4000,
		};
		cpu_type cpu = ECpuUnknown;

		uint32_t header_crc = 0;
		union
		{
			uint32_t code_checksum;
			uint32_t module_version;
		};
		union
		{
			uint32_t data_checksum;
			uint32_t compression_type;
		};

		uint32_t tool_version;
		uint64_t timestamp;
		enum flags_type
		{
		};
		flags_type flags = flags_type(0);
		uint32_t heap_size_min = 0;
		uint32_t heap_size_max = 0;
		uint32_t stack_size = 0;
		uint32_t bss_size = 0;
		uint32_t entry_point = 0;
		uint32_t code_address = 0;
		uint32_t data_address = 0;
		uint32_t export_table_offset = 0;
		uint32_t import_table_offset = 0;
		uint32_t code_offset = 0;
		uint32_t data_offset = 0;
		uint32_t import_offset = 0;
		uint32_t code_relocation_offset = 0;
		uint32_t data_relocation_offset = 0;
		uint32_t process_priority = 0;

		// new header
		uint32_t uncompressed_size = 0;
		uint32_t secure_id = 0;
		uint32_t vendor_id = 0;
		enum capability_type
		{
			// TODO
		};
		std::array<capability_type, 2> capabilities;
		uint32_t exception_descriptor = 0;
		uint8_t export_description_type = 0;
		std::vector<uint8_t> export_description;

		std::shared_ptr<Linker::Contents> code, data;

		struct ImportBlock
		{
			uint32_t name_offset = 0;
			std::vector<uint32_t> imports;
		};

		std::vector<ImportBlock> dll_reference_table;
		uint32_t dll_reference_table_size = 0;

		struct Relocation
		{
			uint16_t offset = 0;
			enum relocation_type
			{
				FromCode = 1,
				FromData = 2,
				FromEither = 3,
			};
			relocation_type type = relocation_type(0);
		};

		struct RelocationBlock
		{
			uint32_t page_offset = 0;
			std::vector<Relocation> relocations;
		};

		struct RelocationSection
		{
			std::vector<RelocationBlock> blocks;
			uint32_t GetSize() const;
			uint32_t GetCount() const;
			void ReadFile(Linker::Reader& rd);
		};
		RelocationSection code_relocations, data_relocations;

		SymbianFormat()
			: code_checksum(0), data_checksum(0)
		{
		}

		void ReadFile(Linker::Reader& rd) override;

		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* EPOC_H */
