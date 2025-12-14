
#include "xenix.h"
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
	header_size = rd.ReadUnsigned(2);
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

	/* TODO */
}

offset_t XOutFormat::WriteFile(Linker::Writer& wr) const
{
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

	Dumper::Region header_region("Header", file_offset, header_size, 8);
	header_region.AddField("Text size", Dumper::HexDisplay::Make(), offset_t(text_size));
	header_region.AddField("Data size", Dumper::HexDisplay::Make(), offset_t(data_size));
	header_region.AddField("BSS size", Dumper::HexDisplay::Make(), offset_t(bss_size));
	header_region.AddField("Symbol table size", Dumper::HexDisplay::Make(), offset_t(symbol_table_size));
	header_region.AddField("Relocation table size", Dumper::HexDisplay::Make(), offset_t(relocation_size));
	header_region.AddField("Entry address", Dumper::HexDisplay::Make(), offset_t(entry_address));
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
	header_region.Display(dump);

	// TODO
}

