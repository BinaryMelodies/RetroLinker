
#include "xenix.h"
#include "../linker/buffer.h"
#include "../linker/location.h"

/* TODO: unimplemented */

using namespace Xenix;

void BOutFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t BOutFormat::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void BOutFormat::Dump(Dumper::Dumper& dump) const
{
	// TODO: set encoding to other for non-x86?
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("b.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void XOutFormat::Segment::Calculate(XOutFormat& xout)
{
	// TODO: untested

#if 0
	offset_t page_size = xout.GetPageSize();
	if(page_size > 1)
	{
		if((offset % page_size) != 0)
			offset += page_size - (offset % page_size);
	}
#endif
	file_size = contents ? contents->ImageSize() : 0;
	// TODO
}

XOutFormat::Segment XOutFormat::Segment::ReadHeader(Linker::Reader& rd, XOutFormat& xout)
{
	Segment segment;
	segment.type = segment_type(rd.ReadUnsigned(2));
	segment.attributes = rd.ReadUnsigned(2);
	segment.number = rd.ReadUnsigned(2);
	segment.log2_align = rd.ReadUnsigned(1);
	segment.reserved1 = rd.ReadUnsigned(1);
#if 0
	segment.offset = offset_t(rd.ReadUnsigned(4)) * xout.GetPageSize();
#else
	segment.offset = rd.ReadUnsigned(4);
#endif
	Linker::Debug << "Debug: file size stored at offset " << std::hex << rd.Tell() << std::endl;
	segment.file_size = rd.ReadUnsigned(4);
	segment.memory_size = rd.ReadUnsigned(4);
	segment.base_address = rd.ReadUnsigned(4); // TODO: transform according to page size?
	segment.name_offset = rd.ReadUnsigned(2);
	segment.reserved2 = rd.ReadUnsigned(2);
	segment.reserved3 = rd.ReadUnsigned(4);
	return segment;
}

void XOutFormat::Segment::ReadContents(Linker::Reader& rd, XOutFormat& xout)
{
	// TODO: also read name

	if(file_size != 0)
	{
		Linker::Debug << "Debug: reading " << std::hex << file_size << " from offset " << std::hex << (xout.file_offset + offset) << std::endl;
		rd.Seek(xout.file_offset + offset);
		contents = Linker::Buffer::ReadFromFile(rd, file_size);
	}
}

void XOutFormat::Segment::WriteHeader(Linker::Writer& wr, const XOutFormat& xout) const
{
	// TODO: untested

	wr.WriteWord(2, type);
	wr.WriteWord(2, attributes);
	wr.WriteWord(2, number);
	wr.WriteWord(1, log2_align);
	wr.WriteWord(1, reserved1);
#if 0
	wr.WriteWord(4, offset / xout.GetPageSize());
#else
	wr.WriteWord(4, offset);
#endif
	wr.WriteWord(4, file_size);
	wr.WriteWord(4, memory_size);
	wr.WriteWord(4, base_address); // TODO: transform according to page size?
	wr.WriteWord(2, name_offset);
	wr.WriteWord(2, reserved2);
	wr.WriteWord(4, reserved3);
}

void XOutFormat::Segment::WriteContents(Linker::Writer& wr, const XOutFormat& xout) const
{
	// TODO: untested

	if(contents)
	{
		wr.Seek(xout.file_offset + offset);
		contents->WriteFile(wr);
	}
}

void XOutFormat::Segment::Dump(Dumper::Dumper& dump, const XOutFormat& xout, uint32_t index) const
{
	Dumper::Region header_region("Segment header", xout.header_size + index * 0x20, 0x20, 8);
	header_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(index + 1));
	header_region.Display(dump);

	static const std::map<offset_t, std::string> type_descriptions =
	{
		{ Null, "unused" },
		{ Text, "text" },
		{ Data, "data" },
		{ SymbolTable, "symbol table segment (SYMS)" },
		{ Relocation, "relocation segment (REL)" },
		{ SegmentStringTable, "string table for segment table (SESTR)" },
		{ GroupDefinition, "group definition segment (GRPS)" },
		{ IteratedData, "iterated data (IDATA)" },
		{ TSS, "x86 task state segment (TSS)" },
		{ LODFIX, "\"lodfix\" segment (LFIX)" },
		{ DescriptorNames, "descriptor names (DNAME)" },
		{ DebugText, "debug text segment (DTEXT/IDBG)" },
		{ DebugRelocation, "debug relocations (DFIX)" },
		{ OverlayTable, "overlay table (OVTAB)" },
		{ SymbolStringTable, "string table for symbols (SYSTR)" },
	};

	Dumper::Block segment_block("Segment", xout.file_offset + offset, contents->AsImage(), base_address, 8);
	segment_block.AddField("Number", Dumper::DecDisplay::Make(), offset_t(number));
	segment_block.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(2)), offset_t(type));
	switch(type)
	{
	default:
		segment_block.AddField("Attributes", Dumper::BitFieldDisplay::Make(4)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("iterated records (ITER)"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("huge elements (HUGE)"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("implicit bss (BSS)"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("read-only shareable (PURE)"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("x86 expand down (EDOWN)"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("cannot be combined (PRIV)"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("32-bit"), true)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("represents memory image (MEM)"), true),
				offset_t(attributes));
		break;
	case SymbolTable:
		{
			static const std::map<offset_t, std::string> format_descriptions =
			{
				{ Attribute_SymbolTable_Bell, "Bell 5.2" },
				{ Attribute_SymbolTable_XOut, "x.out segmented" },
				{ Attribute_SymbolTable_IslandDebugger, "island debugger support" },
			};
			segment_block.AddField("Attributes", Dumper::BitFieldDisplay::Make(4)
				->AddBitField(0, 14, "Symbol table format", Dumper::ChoiceDisplay::Make(format_descriptions), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("represents memory image (MEM)"), true),
					offset_t(attributes));
		}
		break;
	case Relocation:
		{
			static const std::map<offset_t, std::string> format_descriptions =
			{
				{ Attribute_Relocation_XOutSegmented, "x.out segmented" },
				{ Attribute_Relocation_8086Segmented, "8086 x.out segmented" },
			};
			segment_block.AddField("Attributes", Dumper::BitFieldDisplay::Make(4)
				->AddBitField(0, 14, "Symbol table format", Dumper::ChoiceDisplay::Make(format_descriptions), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("represents memory image (MEM)"), true),
					offset_t(attributes));
		}
		break;
	}
	segment_block.AddField("Alignment", Dumper::HexDisplay::Make(8), offset_t(1) << log2_align);
	segment_block.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(memory_size));
	segment_block.AddField("Name offset", Dumper::HexDisplay::Make(4), offset_t(name_offset));
	segment_block.AddOptionalField("Reserved @0x07", Dumper::HexDisplay::Make(2), offset_t(reserved1));
	segment_block.AddOptionalField("Reserved @0x1A", Dumper::HexDisplay::Make(4), offset_t(reserved2));
	segment_block.AddOptionalField("Reserved @0x1C", Dumper::HexDisplay::Make(9), offset_t(reserved3));
	segment_block.Display(dump);
}

void XOutFormat::Clear()
{
	// extended header members might not be overwritten by ReadFile
	text_relocation_size = 0;
	data_relocation_size = 0;
	text_base_address = 0;
	data_base_address = 0;
	stack_size = 0;
	segment_table_offset = 0;
	segment_table_size = 0;
	machine_dependent_table_offset = 0;
	machine_dependent_table_size = 0;
	machine_dependent_table_format = MDT_None;
	page_size = 0;
	operating_system = OS_None;
	system_version = SystemVersion_Xenix2;
	entry_segment = 0;
	header_reserved1 = 0;

	segments.clear();
}

void XOutFormat::CalculateValues()
{
	// TODO: untested

	segment_table_size = segments.size() * 0x20;

	if((runtime_environment & Flag_SegmentTable) != 0
	|| segment_table_offset
	|| segment_table_size
	|| machine_dependent_table_offset
	|| machine_dependent_table_size
	|| machine_dependent_table_format
	|| page_size
	|| operating_system
	|| system_version
	|| entry_segment
	|| header_reserved1)
	{
		header_size = std::max(uint16_t(0x4C), header_size);
	}
	else if(text_relocation_size
	|| data_relocation_size
	|| text_base_address
	|| data_base_address
	|| stack_size)
	{
		header_size = std::max(uint16_t(0x34), header_size);
	}
	else
	{
		header_size = std::max(uint16_t(0x20), header_size);
	}

	page_size = std::min(offset_t(0x255 << 9), AlignTo(page_size, 0x200));
}

void XOutFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();

	rd.Seek(file_offset + 0x1C);
	uint8_t cpu_byte = rd.ReadUnsigned(1);
	cpu = cpu_type(cpu_byte & 0x3F);
	switch(cpu_byte >> 6)
	{
	case 0:
		endiantype = ::PDP11Endian;
		break;
	case 1:
		endiantype = ::LittleEndian;
		break;
	case 2:
		endiantype = ::BigEndian;
		break;
	case 3:
		endiantype = ::AntiPDP11Endian;
		break;
	}
	rd.endiantype = endiantype;
	rd.Seek(file_offset + 2);
	header_size = 0x20 + rd.ReadUnsigned(2);
	text_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	symbol_table_size = rd.ReadUnsigned(4);
	relocation_size = rd.ReadUnsigned(4);
	entry_address = rd.ReadUnsigned(4);
	rd.ReadUnsigned(1);
	uint8_t relsym_byte = rd.ReadUnsigned(1);
	symbol_format = symbol_format_type(relsym_byte & 0xF);
	relocation_format = relocation_format_type(relsym_byte >> 4);
	runtime_environment = rd.ReadUnsigned(2);

	if(header_size >= 0x24)
	{
		text_relocation_size = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x28)
	{
		data_relocation_size = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x2C)
	{
		text_base_address = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x30)
	{
		data_base_address = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x34)
	{
		stack_size = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x38)
	{
		segment_table_offset = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x3C)
	{
		segment_table_size = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x40)
	{
		machine_dependent_table_offset = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x44)
	{
		machine_dependent_table_size = rd.ReadUnsigned(4);
	}

	if(header_size >= 0x45)
	{
		machine_dependent_table_format = machine_dependent_table_format_type(rd.ReadUnsigned(1));
	}

	if(header_size >= 0x46)
	{
		page_size = uint32_t(rd.ReadUnsigned(1)) << 9;
	}

	if(header_size >= 0x47)
	{
		operating_system = operating_system_type(rd.ReadUnsigned(1));
	}

	if(header_size >= 0x48)
	{
		system_version = system_version_type(rd.ReadUnsigned(1));
	}

	if(header_size >= 0x4A)
	{
		entry_segment = rd.ReadUnsigned(2);
	}

	if(header_size >= 0x4C)
	{
		header_reserved1 = rd.ReadUnsigned(2);
	}

	segments.clear();
	for(uint32_t segment_offset = 0; segment_offset < segment_table_size; segment_offset += 0x20)
	{
		rd.Seek(file_offset + segment_table_offset + segment_offset);
		segments.emplace_back(Segment::ReadHeader(rd, *this));
	}

	for(auto& segment : segments)
	{
		segment.ReadContents(rd, *this);
	}

	/* TODO */
}

offset_t XOutFormat::WriteFile(Linker::Writer& wr) const
{
	// TODO: untested

	wr.endiantype = endiantype;
	wr.Seek(file_offset);
	wr.WriteData(2, "\x06\x02");
	wr.WriteWord(2, std::max(uint16_t(0x20), header_size) - 0x20);
	wr.WriteWord(4, text_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, symbol_table_size);
	wr.WriteWord(4, relocation_size);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(1, GetCPUByte());
	wr.WriteWord(1, GetRelSymByte());
	wr.WriteWord(2, runtime_environment);

	if(header_size >= 0x24)
	{
		wr.WriteWord(4, text_relocation_size);
	}

	if(header_size >= 0x28)
	{
		wr.WriteWord(4, data_relocation_size);
	}

	if(header_size >= 0x2C)
	{
		wr.WriteWord(4, text_base_address);
	}

	if(header_size >= 0x30)
	{
		wr.WriteWord(4, data_base_address);
	}

	if(header_size >= 0x34)
	{
		wr.WriteWord(4, stack_size);
	}

	if(header_size >= 0x38)
	{
		wr.WriteWord(4, segment_table_offset);
	}

	if(header_size >= 0x3C)
	{
		wr.WriteWord(4, segment_table_size);
	}

	if(header_size >= 0x40)
	{
		wr.WriteWord(4, machine_dependent_table_offset);
	}

	if(header_size >= 0x44)
	{
		wr.WriteWord(4, machine_dependent_table_size);
	}

	if(header_size >= 0x45)
	{
		wr.WriteWord(1, machine_dependent_table_format);
	}

	if(header_size >= 0x46)
	{
		wr.WriteWord(1, page_size >> 9);
	}

	if(header_size >= 0x47)
	{
		wr.WriteWord(1, operating_system);
	}

	if(header_size >= 0x48)
	{
		wr.WriteWord(1, system_version);
	}

	if(header_size >= 0x4A)
	{
		wr.WriteWord(2, entry_segment);
	}

	if(header_size >= 0x4C)
	{
		wr.WriteWord(2, header_reserved1);
	}

	/* TODO */

	return offset_t(-1);
}

uint8_t XOutFormat::GetCPUByte() const
{
	uint8_t byte_value = cpu;
	switch(endiantype)
	{
	case ::PDP11Endian:
	default:
		break;
	case ::LittleEndian:
		byte_value |= 0x40;
		break;
	case ::BigEndian:
		byte_value |= 0x80;
		break;
	case ::AntiPDP11Endian:
		byte_value |= 0xC0;
		break;
	}
	return byte_value;
}

uint8_t XOutFormat::GetRelSymByte() const
{
	return (relocation_format << 4) | symbol_format;
}

#if 0
offset_t XOutFormat::GetPageSize() const
{
	if(cpu == CPU_80386)
	{
		return page_size;
	}
	else
	{
		return 1;
	}
}
#endif

void XOutFormat::Dump(Dumper::Dumper& dump) const
{
	// TODO: set encoding to other for non-x86?
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("x.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	static const std::map<offset_t, std::string> cpu_description =
	{
		{ CPU_None,     "none" },
		{ CPU_PDP11,    "DEC PDP-11" },
		{ CPU_PDP11_23, "DEC PDP-11/23" }, // TODO: is this it?
		{ CPU_Z8K,      "Zilog Z8000" },
		{ CPU_8086,     "Intel 8086" },
		{ CPU_68K,      "Motorola 68000" },
		{ CPU_Z80,      "Zilog Z80" },
		{ CPU_VAX,      "DEC VAX 780/750" },
		{ CPU_NS32K,    "NS 32016 (16032)" },
		{ CPU_80286,    "Intel 80286" },
		{ CPU_80386,    "Intel 80386" },
		{ CPU_80186,    "Intel 80186" },
	};

	static const std::map<offset_t, std::string> byte_order_description =
	{
		{ 0, "PDP-11 byte order" },
		{ 1, "little endian" },
		{ 2, "big endian" },
		{ 3, "reversed PDP-11 byte order" },
	};

	static const std::map<offset_t, std::string> symbol_format_description =
	{
		{ REL_X_OUT_LONG,  "x.out long form" },
		{ REL_X_OUT_SHORT, "x.out short form" },
		{ REL_B_OUT,       "b.out" },
		{ REL_A_OUT,       "a.out" },
		{ REL_8086_REL,    "8086 relocatable" },
		{ REL_8086_ABS,    "8086 absolute" },
	};

	static const std::map<offset_t, std::string> relocation_format_description =
	{
		{ SYM_X_OUT,        "x.out" },
		{ SYM_B_OUT,        "b.out" },
		{ SYM_A_OUT,        "a.out" },
		{ SYM_8086_REL,     "8086 relocatable" },
		{ SYM_8086_ABS,     "8086 absolute" },
		{ SYM_STRING_TABLE, "string table" },
	};

	static const std::map<offset_t, std::string> version_description =
	{
		{ 1, "2.x" },
		{ 2, "2.x" },
		{ 3, "5.x" },
	};

	static const std::map<offset_t, std::string> system_description =
	{
		{ OS_Xenix,         "Xenix" },
		{ OS_iRMX,          "Intel iRMX" },
		{ OS_ConcurrentCPM, "Concurrent CP/M" },
	};

	Dumper::Region header_region("Header", file_offset, header_size, 8);
	header_region.AddField("Text size", Dumper::HexDisplay::Make(8), offset_t(text_size));
	header_region.AddField("Data size", Dumper::HexDisplay::Make(8), offset_t(data_size));
	header_region.AddField("BSS size", Dumper::HexDisplay::Make(8), offset_t(bss_size));
	header_region.AddField("Symbol table size", Dumper::HexDisplay::Make(8), offset_t(symbol_table_size));
	header_region.AddField("Relocation table size", Dumper::HexDisplay::Make(8), offset_t(relocation_size));
	if(entry_segment == 0)
	{
		header_region.AddField("Entry address", Dumper::HexDisplay::Make(8), offset_t(entry_address));
	}
	else
	{
		header_region.AddField("Entry address", Dumper::SegmentedDisplay::Make(8), offset_t(entry_segment), offset_t(entry_address));
	}
	header_region.AddField("CPU type byte",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 6, "CPU", Dumper::ChoiceDisplay::Make(cpu_description), false)
			->AddBitField(6, 2, "Byte order", Dumper::ChoiceDisplay::Make(byte_order_description), false),
		offset_t(GetCPUByte()));
	header_region.AddField("Relocation/symbol format byte",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 4, "Symbol format", Dumper::ChoiceDisplay::Make(symbol_format_description), false)
			->AddBitField(4, 4, "Relocation format", Dumper::ChoiceDisplay::Make(relocation_format_description), false),
		offset_t(GetRelSymByte()));
	header_region.AddField("Runtime environment",
		Dumper::BitFieldDisplay::Make(4)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("executable (EXEC)"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("separate text & data space (SEP)"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("pure text (PURE)"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("fixed stack (FS)"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("text overlay (OVER)"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("large model data (LDATA)"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("large model text (LTEXT)"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("floating point hardware required (FPH)"), true)
			->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("virtual module (VMOD) or huge model data (HDATA)"), true)
			->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("iterated text/data present (ITER)"), true)
			->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("absolute memory image with physical addresses (ABS)"), true)
			->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("segment table present (SEG)"), true)
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("use advisory locking (LOCK)"), true)
			->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("Xenix Verison 3.5 functionality required"), true)
			->AddBitField(14, 2, "Xenix Version", Dumper::ChoiceDisplay::Make(version_description), true),
		offset_t(runtime_environment));
	if(cpu == CPU_80386)
	{
		static const std::map<offset_t, std::string> module_type_description =
		{
			{ 0x0001, "executable, no shared libraries" },
			{ 0x0100, "shared library" },
			{ 0x0101, "executable, uses shared libraries" },
			{ 0x0401, "standalone program, kernel" },
			{ 0x0500, "virtual kernel module" },
		};
		header_region.AddOptionalField("Module type", Dumper::ChoiceDisplay::Make(module_type_description), offset_t(runtime_environment & 0x0501));
	}
	header_region.AddOptionalField("Text relocation size", Dumper::HexDisplay::Make(8), offset_t(text_relocation_size));
	header_region.AddOptionalField("Data relocation size", Dumper::HexDisplay::Make(8), offset_t(data_relocation_size));
	header_region.AddOptionalField("Text base address", Dumper::HexDisplay::Make(8), offset_t(text_base_address));
	header_region.AddOptionalField("Data base address", Dumper::HexDisplay::Make(8), offset_t(data_base_address));
	header_region.AddOptionalField("Stack size", Dumper::HexDisplay::Make(8), offset_t(stack_size));
	header_region.AddOptionalField("Page size", Dumper::HexDisplay::Make(5), offset_t(page_size));
	header_region.AddOptionalField("Operating system", Dumper::ChoiceDisplay::Make(system_description), offset_t(operating_system));
	switch(operating_system)
	{
	case OS_Xenix:
		{
			static const std::map<offset_t, std::string> system_version_description =
			{
				{ 0, "2.x" },
				{ 1, "2.x" },
				{ 2, "5.x" },
			};
			header_region.AddOptionalField("System version", Dumper::ChoiceDisplay::Make(system_version_description, Dumper::HexDisplay::Make(2)), offset_t(system_version));
		}
		break;
	default:
		header_region.AddOptionalField("System version", Dumper::HexDisplay::Make(2), offset_t(system_version));
		break;
	}
	header_region.AddOptionalField("Reserved (@0x4A)", Dumper::HexDisplay::Make(8), offset_t(header_reserved1));
	header_region.Display(dump);

	if(segment_table_size != 0 || segment_table_offset != 0)
	{
		Dumper::Region segment_table_region("Segment table", segment_table_offset, segment_table_size, 8);
		segment_table_region.Display(dump);
	}

	if(machine_dependent_table_size != 0 || machine_dependent_table_offset != 0)
	{
		static const std::map<offset_t, std::string> mdt_format_description =
		{
			{ MDT_None,   "None" },
			{ MDT_286LDT, "Intel 80286 LDT" },
		};

		Dumper::Region machine_dependent_table_region("Machine dependent table", machine_dependent_table_offset, machine_dependent_table_size, 8);
		machine_dependent_table_region.AddField("Format", Dumper::ChoiceDisplay::Make(mdt_format_description), offset_t(machine_dependent_table_format));
		machine_dependent_table_region.Display(dump);
	}

	uint32_t segment_index = 0;
	for(auto& segment : segments)
	{
		segment.Dump(dump, *this, segment_index);
		segment_index ++;
	}

	// TODO
}

