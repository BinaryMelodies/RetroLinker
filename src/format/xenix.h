#ifndef XENIX_H
#define XENIX_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* b.out and x.out file formats */
namespace Xenix
{
	/**
	 * @brief Xenix b.out executable
	 */
	class BOutFormat : public virtual Linker::SegmentManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */
	};

	/**
	 * @brief Xenix x.out executable
	 */
	class XOutFormat : public virtual Linker::SegmentManager
	{
	public:
		enum cpu_type
		{
			CPU_None = 0,
			CPU_PDP11 = 1,
			CPU_PDP11_23 = 2,
			CPU_Z8K = 3,
			CPU_8086 = 4,
			CPU_68K = 5,
			CPU_Z80 = 6,
			CPU_VAX = 7,
			CPU_NS32K = 8,
			CPU_80286 = 9,
			CPU_80386 = 10,
			CPU_80186 = 11,
		};

		enum relocation_format_type
		{
			REL_X_OUT_LONG = 0,
			REL_X_OUT_SHORT = 1,
			REL_B_OUT = 2,
			REL_A_OUT = 3,
			REL_8086_REL = 4,
			REL_8086_ABS = 5,
		};

		enum symbol_format_type
		{
			SYM_X_OUT = 0,
			SYM_B_OUT = 1,
			SYM_A_OUT = 2,
			SYM_8086_REL = 3,
			SYM_8086_ABS = 4,
			SYM_STRING_TABLE = 5,
		};

		uint16_t header_size = 0;
		uint32_t text_size = 0;
		uint32_t data_size = 0;
		uint32_t bss_size = 0;
		uint32_t symbol_table_size = 0;
		uint32_t relocation_size = 0;
		uint32_t entry_address = 0;
		cpu_type cpu = CPU_None;
		::EndianType endiantype = ::LittleEndian;
		relocation_format_type relocation_format;
		symbol_format_type symbol_format;
		uint16_t runtime_environment = 0;

		// extended header
		uint32_t text_relocation_size = 0;
		uint32_t data_relocation_size = 0;
		uint32_t text_base_address = 0;
		uint32_t data_base_address = 0;
		uint32_t stack_size = 0;
		uint32_t segment_table_offset = 0;
		uint32_t segment_table_size = 0;
		uint32_t machine_dependent_table_offset = 0;
		uint32_t machine_dependent_table_size = 0;
		enum machine_dependent_table_format_type : uint8_t
		{
			MDT_None = 0,
			MDT_286LDT = 1,
		};
		machine_dependent_table_format_type machine_dependent_table_format = MDT_None;
		uint32_t page_size = 0;
		enum operating_system_type : uint8_t
		{
			OS_None = 0,
			OS_Xenix = 1,
			OS_iRMX = 2,
			OS_ConcurrentCPM = 3,
		};
		operating_system_type operating_system = OS_None;
		enum system_version_type : uint8_t
		{
			SystemVersion_Xenix2 = 0,
			SystemVersion_Xenix3 = 1,
			SystemVersion_Xenix5 = 2,
		};
		system_version_type system_version = SystemVersion_Xenix2;
		uint16_t entry_segment = 0;
		uint16_t header_reserved1 = 0;

		uint8_t GetCPUByte() const;
		uint8_t GetRelSymByte() const;

		static constexpr uint16_t Flag_Executable = 0x0001;
		static constexpr uint16_t Flag_SeparateInsData = 0x0002;
		static constexpr uint16_t Flag_PureText = 0x0004;
		static constexpr uint16_t Flag_FixedStack = 0x0008;
		static constexpr uint16_t Flag_TextOverlay = 0x0010;
		static constexpr uint16_t Flag_LargeData = 0x0020;
		static constexpr uint16_t Flag_LargeText = 0x0040;
		static constexpr uint16_t Flag_FloatingPoint = 0x0080;
		static constexpr uint16_t Flag_VirtualModule = 0x0100;
		static constexpr uint16_t Flag_HugeData = 0x0100;
		static constexpr uint16_t Flag_Iterated = 0x0200;
		static constexpr uint16_t Flag_Absolute = 0x0400;
		static constexpr uint16_t Flag_SegmentTable = 0x0800;
		static constexpr uint16_t Flag_AdvisoryLocking = 0x1000;
		static constexpr uint16_t Flag_Xenix53Required = 0x2000;

		static constexpr uint16_t Flag_Xenix2x = 0x4000;
		static constexpr uint16_t Flag_Xenix3x = 0x8000;
		static constexpr uint16_t Flag_Xenix5x = 0xC000;

		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */
	};
}

#endif /* XENIX_H */
