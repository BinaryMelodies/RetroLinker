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
		class Segment
		{
		public:
			enum segment_type : uint16_t
			{
				Null = 0x00,
				Text = 0x01,
				Data = 0x02,
				SymbolTable = 0x03,
				Relocation = 0x04,
				SegmentStringTable = 0x05,
				GroupDefinition = 0x06,
				IteratedData = 0x40,
				TSS = 0x41,
				LODFIX = 0x42,
				DescriptorNames = 0x43,
				DebugText = 0x44,
				DebugRelocation = 0x45,
				OverlayTable = 0x46,
				SymbolStringTable = 0x48,
			};
			segment_type type = Null;
			uint16_t attributes = 0;

			uint16_t number = 0;
			uint8_t log2_align = 0;
			uint8_t reserved1 = 0;
			offset_t offset = 0;
			uint32_t file_size = 0;
			uint32_t memory_size = 0;
			uint32_t base_address = 0;
			uint16_t name_offset = 0;
			//std::string name; // TODO
			uint16_t reserved2 = 0;
			uint32_t reserved3 = 0;

			std::shared_ptr<Linker::Contents> contents;

			static constexpr uint16_t Attribute_Iterated = 0x0001;
			static constexpr uint16_t Attribute_Huge = 0x0002;
			static constexpr uint16_t Attribute_ImplicitBss = 0x0004;
			static constexpr uint16_t Attribute_Pure = 0x0008;
			static constexpr uint16_t Attribute_ExpandDown = 0x0010;
			static constexpr uint16_t Attribute_Private = 0x0020;
			static constexpr uint16_t Attribute_32Bit = 0x0040;
			static constexpr uint16_t Attribute_MemoryImage = 0x8000;

			static constexpr uint16_t Attribute_SymbolTable_Bell = 0x0000;
			static constexpr uint16_t Attribute_SymbolTable_XOut = 0x0001;
			static constexpr uint16_t Attribute_SymbolTable_IslandDebugger = 0x0002;

			static constexpr uint16_t Attribute_Relocation_XOutSegmented = 0x0001;
			static constexpr uint16_t Attribute_Relocation_8086Segmented = 0x0002;

			void Calculate(XOutFormat& xout);
			static Segment ReadHeader(Linker::Reader& rd, XOutFormat& xout);
			void ReadContents(Linker::Reader& rd, XOutFormat& xout);
			void WriteHeader(Linker::Writer& wr, const XOutFormat& xout) const;
			void WriteContents(Linker::Writer& wr, const XOutFormat& xout) const;
			void Dump(Dumper::Dumper& dump, const XOutFormat& xout, uint32_t index) const;
		};

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

		std::vector<Segment> segments;

		uint8_t GetCPUByte() const;
		uint8_t GetRelSymByte() const;
#if 0
		/** @brief Returns the unit in which segment offsets are measured in */
		offset_t GetPageSize() const;
#endif

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

		void Clear() override;
		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */
	};
}

#endif /* XENIX_H */
