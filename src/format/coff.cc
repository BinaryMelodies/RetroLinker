
#include "coff.h"
#include "../linker/location.h"
#include "../linker/section.h"
#include "../linker/symbol.h"

using namespace COFF;

const std::map<uint32_t, COFF::COFFFormat::MachineType> COFF::COFFFormat::MACHINE_TYPES = {
	{ std::make_pair(0x0088,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x0089,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x0093,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x0140,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0142,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF // TODO: also 8086
	{ std::make_pair(0x0143,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x0144,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x0145,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x0146,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x0147,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x0148,    MachineType { CPU_I86,   ::LittleEndian }) }, // TODO: also NS32K
	{ std::make_pair(0x0149,    MachineType { CPU_I86,   ::LittleEndian }) },
	{ std::make_pair(0x014A,    MachineType { CPU_I86,   ::LittleEndian }) }, // actually 286
	{ std::make_pair(CPU_I386,  MachineType { CPU_I386,  ::LittleEndian }) }, // GNU header, FlexOS header
	{ std::make_pair(0x014D,    MachineType { CPU_NS32K, ::LittleEndian }) }, // TODO: also I860
	// 0x014E -- Intel
	// 0x014F -- Intel
	{ std::make_pair(CPU_M68K,  MachineType { CPU_M68K,  ::BigEndian }) }, // FlexOS header
	{ std::make_pair(0x0151,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x0152,    MachineType { CPU_M68K,  ::BigEndian }) }, // TODO: also 286
	{ std::make_pair(CPU_NS32K, MachineType { CPU_NS32K, ::LittleEndian }) },
	{ std::make_pair(0x0155,    MachineType { CPU_NS32K, ::LittleEndian }) },
	{ std::make_pair(CPU_I370,  MachineType { CPU_I370,  ::BigEndian }) },
	{ std::make_pair(0x0159,    MachineType { CPU_I370,  ::BigEndian }) }, // Amdahl
	// 0x015A -- I370
	// 0x015B -- I370
	{ std::make_pair(0x015C,    MachineType { CPU_I370,  ::BigEndian }) }, // Amdahl
	{ std::make_pair(0x015D,    MachineType { CPU_I370,  ::BigEndian }) },
	{ std::make_pair(CPU_MIPS,  MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF but also COFF with MIPS optional header?
	// also OpenBSD i960
	{ std::make_pair(0x0162,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0163,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	// 0x0164 -- Zilog
	// 0x0165 -- Zilog
	{ std::make_pair(0x0166,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0168,    MachineType { CPU_WE32K, ::BigEndian }) }, // 3B20 // TODO: also NetBSD SH3
	{ std::make_pair(0x0169,    MachineType { CPU_WE32K, ::BigEndian }) }, // 3B20
	{ std::make_pair(0x016C,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(CPU_M88K,  MachineType { CPU_M88K,  ::BigEndian }) },
	{ std::make_pair(CPU_WE32K, MachineType { CPU_WE32K, ::BigEndian }) },
	{ std::make_pair(0x0171,    MachineType { CPU_WE32K, ::BigEndian }) },
	// 0x0172 -- WE32K
	{ std::make_pair(0x0175,    MachineType { CPU_I386,  ::LittleEndian }) },
	{ std::make_pair(CPU_VAX,   MachineType { CPU_VAX,   ::LittleEndian }) },
	{ std::make_pair(CPU_AM29K, MachineType { CPU_AM29K, ::BigEndian }) },
	{ std::make_pair(0x017B,    MachineType { CPU_AM29K, ::LittleEndian }) },
	{ std::make_pair(0x017D,    MachineType { CPU_VAX,   ::LittleEndian }) },
	// TODO: 0x017F - CLIPPER
	{ std::make_pair(0x0180,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0182,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(CPU_ALPHA, MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0185,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0188,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x018F,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0194,    MachineType { CPU_M88K,  ::BigEndian }) },
	{ std::make_pair(0x0197,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(CPU_PPC,   MachineType { CPU_PPC,   ::BigEndian }) }, // XCOFF
	{ std::make_pair(CPU_PPC64, MachineType { CPU_PPC64, ::BigEndian }) }, // XCOFF
	{ std::make_pair(0x1572,    MachineType { CPU_AM29K, ::BigEndian }) },
	{ std::make_pair(CPU_SHARC, MachineType { CPU_SHARC, ::LittleEndian }) },
	{ std::make_pair(CPU_Z8K,   MachineType { CPU_Z8K,   ::BigEndian }) }, // GNU binutils
	{ std::make_pair(CPU_Z80,   MachineType { CPU_Z80,   ::LittleEndian }) }, // GNU binutils
	/* due to overloaded values, these require more than 16 bits and therefore cannot appear in files, included for endianness */
	// for now, there are none
};

::EndianType COFFFormat::GetEndianType() const
{
	auto it = MACHINE_TYPES.find(cpu_type);
	assert(it != MACHINE_TYPES.end());
	return it->second.endian;
}

COFFFormat::Relocation::~Relocation()
{
}

void COFFFormat::ZilogRelocation::Read(Linker::Reader& in)
{
	address = in.ReadUnsigned(4);
	symbol_index = in.ReadUnsigned(4);
	offset = in.ReadUnsigned(4);
	type = in.ReadUnsigned(2);
	data = in.ReadUnsigned(2);
}

offset_t COFFFormat::ZilogRelocation::GetAddress()
{
	return address;
}

size_t COFFFormat::ZilogRelocation::GetSize()
{
	switch(cpu_type)
	{
	case CPU_Z80:
		switch(type)
		{
		case R_Z80_IMM8:
		case R_Z80_OFF8:
		case R_Z80_JR:
			return 1;
		case R_Z80_IMM16:
			return 2;
		case R_Z80_IMM24:
			return 3;
		case R_Z80_IMM32:
			return 4;
		default:
			Linker::Error << "Error: Unknown relocation type" << std::endl;
			return -1;
		}
	case CPU_Z8K:
		switch(type)
		{
		case R_Z8K_IMM4L:
		case R_Z8K_IMM4H:
		case R_Z8K_DISP7:
		case R_Z8K_JR:
		case R_Z8K_IMM8:
			return 1;
		case R_Z8K_IMM16:
		case R_Z8K_REL16:
		case R_Z8K_CALLR:
			return 2;
		case R_Z8K_IMM32:
			return 4;
		default:
			Linker::Error << "Error: Unknown relocation type" << std::endl;
			return -1;
		}
	default:
		Linker::Error << "Error: Unknown relocation type" << std::endl;
		return -1;
	}
}

void COFFFormat::ZilogRelocation::FillEntry(Dumper::Entry& entry)
{
	std::map<offset_t, std::string> relocation_type_names;
	switch(cpu_type)
	{
	case CPU_Z80:
		relocation_type_names[ZilogRelocation::R_Z80_IMM8]  = "imm8";
		relocation_type_names[ZilogRelocation::R_Z80_IMM16] = "imm16";
		relocation_type_names[ZilogRelocation::R_Z80_IMM24] = "imm24";
		relocation_type_names[ZilogRelocation::R_Z80_IMM32] = "imm32";
		relocation_type_names[ZilogRelocation::R_Z80_OFF8]  = "off8";
		relocation_type_names[ZilogRelocation::R_Z80_JR]    = "jr";
		break;
	case CPU_Z8K:
		relocation_type_names[ZilogRelocation::R_Z8K_IMM4L] = "imm4l";
		relocation_type_names[ZilogRelocation::R_Z8K_IMM4H] = "imm4h";
		relocation_type_names[ZilogRelocation::R_Z8K_DISP7] = "disp7";
		relocation_type_names[ZilogRelocation::R_Z8K_IMM8]  = "imm8";
		relocation_type_names[ZilogRelocation::R_Z8K_IMM16] = "imm16";
		relocation_type_names[ZilogRelocation::R_Z8K_REL16] = "rel16";
		relocation_type_names[ZilogRelocation::R_Z8K_IMM32] = "imm32";
		relocation_type_names[ZilogRelocation::R_Z8K_JR]    = "jr";
		relocation_type_names[ZilogRelocation::R_Z8K_CALLR] = "callr";
		break;
	default:
		/* TODO */
		break;
	}

	entry.AddField("Source", new Dumper::HexDisplay, (offset_t)address);
	entry.AddField("Size", new Dumper::HexDisplay(1), (offset_t)GetSize());
	entry.AddField("Type", new Dumper::ChoiceDisplay(relocation_type_names), (offset_t)type);
	entry.AddOptionalField("Offset", new Dumper::HexDisplay, (offset_t)offset);
	/* TODO */
	entry.AddField("Symbol index", new Dumper::HexDisplay, (offset_t)symbol_index);
//				entry.AddField("Target", ???);
}

void COFFFormat::Symbol::Read(Linker::Reader& rd)
{
	union
	{
		char buffer[8];
		uint32_t word[2];
	} u;
	rd.ReadData(8, u.buffer);
	if(u.word[0] == 0)
	{
		name = "";
		name_index = ReadUnsigned(4, 4, reinterpret_cast<uint8_t *>(&u.buffer[4]), rd.endiantype);
	}
	else
	{
		name = std::string(u.buffer, strnlen(u.buffer, 8));
		name_index = 0;
	}
	value = rd.ReadUnsigned(4);
	section_number = rd.ReadUnsigned(2);
	type = rd.ReadUnsigned(2);
	storage_class = rd.ReadUnsigned(1);
	auxiliary_count = rd.ReadUnsigned(1);
	rd.Skip(18 * auxiliary_count);
}

bool COFFFormat::Symbol::IsExternal() const
{
	return storage_class == C_EXT && section_number == N_UNDEF && value == 0;
}

void COFFFormat::Section::Initialize()
{
	name = "";
	physical_address = 0;
	address = 0;
	size = 0;
	section_pointer = 0;
	relocation_pointer = 0;
	line_number_pointer = 0;
	relocation_count = 0;
	line_number_count = 0;
	flags = 0;
	image = nullptr;
}

void COFFFormat::Section::Clear()
{
	if(image)
		delete image;
	image = nullptr;
	for(Relocation * rel : relocations)
	{
		delete rel;
	}
	relocations.clear();
}

void COFFFormat::Section::ReadSectionHeader(Linker::Reader& rd)
{
	name = rd.ReadData(8, true);
	physical_address = rd.ReadUnsigned(4);
	address = rd.ReadUnsigned(4);
	size = rd.ReadUnsigned(4);
	section_pointer = rd.ReadUnsigned(4);
	relocation_pointer = rd.ReadUnsigned(4);
	line_number_pointer = rd.ReadUnsigned(4);
	relocation_count = rd.ReadUnsigned(2);
	line_number_count = rd.ReadUnsigned(2);
	flags = rd.ReadUnsigned(4);

	/* TODO: Buffer instead of Section? */
	if(flags & TEXT)
	{
		image = new Linker::Section(".text");
	}
	else if(flags & DATA)
	{
		image = new Linker::Section(".data");
	}
	else if(flags & BSS)
	{
		image = new Linker::Section(".bss");
	}
}

void COFFFormat::Section::WriteSectionHeader(Linker::Writer& wr)
{
	wr.WriteData(8, name);
	wr.WriteWord(4, physical_address);
	wr.WriteWord(4, address);
	wr.WriteWord(4, size);
	wr.WriteWord(4, section_pointer);
	wr.WriteWord(4, relocation_pointer);
	wr.WriteWord(4, line_number_pointer);
	wr.WriteWord(2, relocation_count);
	wr.WriteWord(2, line_number_count);
	wr.WriteWord(4, flags);
}

uint32_t COFFFormat::Section::ActualDataSize()
{
	return size; //image->ActualDataSize();
}

COFFFormat::OptionalHeader::~OptionalHeader()
{
}

void COFFFormat::OptionalHeader::Initialize()
{
}

void COFFFormat::OptionalHeader::PostReadFile(COFFFormat& coff, Linker::Reader& rd)
{
}

void COFFFormat::OptionalHeader::PostWriteFile(COFFFormat& coff, Linker::Writer& wr)
{
}

void COFFFormat::OptionalHeader::Dump(COFFFormat& coff, Dumper::Dumper& dump)
{
}

void COFFFormat::UnknownOptionalHeader::Initialize()
{
	buffer = nullptr;
}

uint32_t COFFFormat::UnknownOptionalHeader::GetSize()
{
	return buffer->ActualDataSize();
}

void COFFFormat::UnknownOptionalHeader::ReadFile(Linker::Reader& rd)
{
	buffer->ReadFile(rd);
}

void COFFFormat::UnknownOptionalHeader::WriteFile(Linker::Writer& wr)
{
	buffer->WriteFile(wr);
}

void COFFFormat::UnknownOptionalHeader::Dump(COFFFormat& coff, Dumper::Dumper& dump)
{
	/* TODO */
}

void COFFFormat::AOutHeader::Initialize()
{
	magic = 0;
	version_stamp = 0;
	code_size = 0;
	data_size = 0;
	bss_size = 0;
	entry_address = 0;
	code_address = 0;
	data_address = 0;
}

uint32_t COFFFormat::AOutHeader::GetSize()
{
	return 28;
}

void COFFFormat::AOutHeader::ReadFile(Linker::Reader& rd)
{
	magic = rd.ReadUnsigned(2);
	version_stamp = rd.ReadUnsigned(2);
	code_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	entry_address = rd.ReadUnsigned(4);
	code_address = rd.ReadUnsigned(4);
	data_address = rd.ReadUnsigned(4);
}

void COFFFormat::AOutHeader::WriteFile(Linker::Writer& wr)
{
	wr.WriteWord(2, magic);
	wr.WriteWord(2, version_stamp); /* unused */
	wr.WriteWord(4, code_size); /* not needed for DJGPP */
	wr.WriteWord(4, data_size); /* not needed for DJGPP */
	wr.WriteWord(4, bss_size); /* not needed for DJGPP */
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, code_address);
	wr.WriteWord(4, data_address);
}

void COFFFormat::AOutHeader::DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region)
{
	std::map<offset_t, std::string> magic_choice;
	magic_choice[ZMAGIC] = "ZMAGIC";
	header_region.AddField("File type", new Dumper::ChoiceDisplay(magic_choice), (offset_t)magic);
	header_region.AddOptionalField("Version stamp", new Dumper::HexDisplay, (offset_t)version_stamp);
	header_region.AddField("Text size", new Dumper::HexDisplay, (offset_t)code_size);
	header_region.AddField("Data size", new Dumper::HexDisplay, (offset_t)data_size);
	header_region.AddField("Bss size",  new Dumper::HexDisplay, (offset_t)bss_size);
	std::string entry;
	switch(coff.cpu_type)
	{
	case CPU_I386:
		header_region.AddField("Entry address (EIP)", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	case CPU_M68K:
	case CPU_Z8K:
		header_region.AddField("Entry address (PC)", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	case CPU_Z80:
		header_region.AddField("Entry address (PC)", new Dumper::HexDisplay(4), (offset_t)entry_address);
		break;
	default:
		header_region.AddField("Entry address", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	}
	header_region.AddField("Text address", new Dumper::HexDisplay, (offset_t)code_address);
	header_region.AddField("Data address", new Dumper::HexDisplay, (offset_t)data_address);
}

void COFFFormat::AOutHeader::Dump(COFFFormat& coff, Dumper::Dumper& dump)
{
	Dumper::Region header_region("Optional header", coff.file_offset + 20, GetSize(), 8);
	DumpFields(coff, dump, header_region);
	header_region.Display(dump);
}

void COFFFormat::FlexOSAOutHeader::Initialize()
{
	relocations_offset = 0;
	stack_size = 0;
}

uint32_t COFFFormat::FlexOSAOutHeader::GetSize()
{
	return 36;
}

void COFFFormat::FlexOSAOutHeader::ReadFile(Linker::Reader& rd)
{
	AOutHeader::ReadFile(rd);
	relocations_offset = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);
}

void COFFFormat::FlexOSAOutHeader::WriteFile(Linker::Writer& wr)
{
	AOutHeader::WriteFile(wr);
	wr.WriteWord(4, relocations_offset);
	wr.WriteWord(4, stack_size);
}

void COFFFormat::FlexOSAOutHeader::PostReadFile(COFFFormat& coff, Linker::Reader& rd)
{
	/* TODO */
}

void COFFFormat::FlexOSAOutHeader::PostWriteFile(COFFFormat& coff, Linker::Writer& wr)
{
	DigitalResearch::CPM68KFormat::CDOS68K_WriteRelocations(wr, coff.relocations);
}

void COFFFormat::FlexOSAOutHeader::DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region)
{
	AOutHeader::DumpFields(coff, dump, header_region);
	header_region.AddField("Data address", new Dumper::HexDisplay, (offset_t)data_address);
	/* TODO: move display to relocation region */
	header_region.AddField("Relocation offset", new Dumper::HexDisplay, (offset_t)relocations_offset);
	header_region.AddField("Stack size", new Dumper::HexDisplay, (offset_t)stack_size);
}

void COFFFormat::GNUAOutHeader::Initialize()
{
	info = 0;
	code_size = 0;
	data_size = 0;
	bss_size = 0;
	symbol_table_size = 0;
	entry_address = 0;
	code_relocation_size = 0;
	data_relocation_size = 0;
}

uint32_t COFFFormat::GNUAOutHeader::GetSize()
{
	return 32;
}

void COFFFormat::GNUAOutHeader::ReadFile(Linker::Reader& wr)
{
	/* TODO */
}

void COFFFormat::GNUAOutHeader::WriteFile(Linker::Writer& wr)
{
	wr.WriteWord(4, info); /* ??? */
	wr.WriteWord(4, code_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, symbol_table_size);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, code_relocation_size);
	wr.WriteWord(4, data_relocation_size);
}

void COFFFormat::GNUAOutHeader::Dump(COFFFormat& coff, Dumper::Dumper& dump)
{
	/* TODO: untested */
	Dumper::Region header_region("Optional header", coff.file_offset + 20, GetSize(), 8);
	header_region.AddField("Info", new Dumper::HexDisplay, (offset_t)info); // TODO: improve display?
	header_region.AddField("Text size", new Dumper::HexDisplay, (offset_t)code_size);
	header_region.AddField("Data size", new Dumper::HexDisplay, (offset_t)data_size);
	header_region.AddField("Bss size",  new Dumper::HexDisplay, (offset_t)bss_size);
	header_region.AddField("Symbol table size",  new Dumper::HexDisplay, (offset_t)symbol_table_size);
	std::string entry;
	switch(coff.cpu_type)
	{
	case CPU_I386:
		header_region.AddField("Entry address (EIP)", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	case CPU_M68K:
	case CPU_Z8K:
		header_region.AddField("Entry address (PC)", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	case CPU_Z80:
		header_region.AddField("Entry address (PC)", new Dumper::HexDisplay(4), (offset_t)entry_address);
		break;
	default:
		header_region.AddField("Entry address", new Dumper::HexDisplay, (offset_t)entry_address);
		break;
	}
	header_region.AddField("Text relocation size", new Dumper::HexDisplay, (offset_t)code_relocation_size);
	header_region.AddField("Data relocation size", new Dumper::HexDisplay, (offset_t)data_relocation_size);
	header_region.Display(dump);
}

void COFFFormat::MIPSAOutHeader::DumpFields(COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region)
{
	/* TODO: untested */
	AOutHeader::DumpFields(coff, dump, header_region);
	header_region.AddField("Bss address", new Dumper::HexDisplay, (offset_t)bss_address);
	header_region.AddField("GPR mask", new Dumper::HexDisplay, (offset_t)gpr_mask);
	header_region.AddField("CPR #1 mask", new Dumper::HexDisplay, (offset_t)cpr_mask[0]);
	header_region.AddField("CPR #2 mask", new Dumper::HexDisplay, (offset_t)cpr_mask[1]);
	header_region.AddField("CPR #3 mask", new Dumper::HexDisplay, (offset_t)cpr_mask[2]);
	header_region.AddField("CPR #4 mask", new Dumper::HexDisplay, (offset_t)cpr_mask[3]);
	header_region.AddField("GP regiser value", new Dumper::HexDisplay, (offset_t)gp_value);
}

void COFFFormat::Initialize()
{
	/* format fields */
	memset(signature, 0, sizeof signature);
	timestamp = 0;
	symbol_table_offset = 0;
	symbol_count = 0;
	optional_header_size = 0;
	optional_header = nullptr;
	flags = 0;
	cpu_type = CPU_UNKNOWN;
	endiantype = (EndianType)0;
	/* writer fields */
	type = GENERIC;
	option_no_relocation = false;
	stub_size = 0;
	stack = nullptr;
	entry_address = 0; /* TODO */
	relocations_offset = 0;
	/* generator fields */
	special_prefix_char = '$';
	option_segmentation = false;
}

void COFFFormat::Clear()
{
	/* member fields */
	for(auto& section : sections)
	{
		delete section;
	}
	sections.clear();
	if(optional_header)
	{
		delete optional_header;
	}
	optional_header = nullptr;
	relocations.clear();
	for(auto& symbol : symbols)
	{
		delete symbol;
	}
	symbols.clear();
	/* writer fields */
	if(stack)
	{
		delete stack;
	}
	stack = nullptr;
}

void COFFFormat::AssignMagicValue()
{
	::WriteWord(2, 2, reinterpret_cast<uint8_t *>(signature), cpu_type, endiantype);
	Linker::Debug << "Debug: COFF MAGIC VALUE " << (int)signature[0] << ", " << (int)signature[1] << std::endl;
}

bool COFFFormat::DetectCpuType(::EndianType expected)
{
	uint16_t value = ::ReadUnsigned(2, 2, reinterpret_cast<uint8_t *>(signature), expected);
	auto it = MACHINE_TYPES.find(value);
	if(it == MACHINE_TYPES.end())
		return false;
	::EndianType actual = it->second.endian;
	if(actual == ::Undefined || actual == expected)
	{
		cpu_type = it->second.actual_cpu;
		endiantype = expected;
		return true;
	}
	else
	{
		return false;
	}
}

void COFFFormat::DetectCpuType()
{
	cpu_type = CPU_UNKNOWN;
	if(DetectCpuType(::LittleEndian) || DetectCpuType(::BigEndian))
		return;

	/* heuristics to determine endianness */
	if(signature[0] == 0x00 || signature[0] == 0x01)
	{
		endiantype = BigEndian;
	}
	else if(signature[1] == 0x00 || signature[1] == 0x01)
	{
		endiantype = LittleEndian;
	}
#if 0
	else if(signature[0] == 0x15 && signature[1] == 0x72)
	{
		/* Am29K */
		endiantype = BigEndian;
	}
	else if(signature[0] == 0x1C && signature[1] == 0x52)
	{
		/* DSP21k or SHARC */
		endiantype = LittleEndian;
	}
#endif
	else
	{
		Linker::Error << "Fatal Error: Unrecognized magic number, unable to determine endianness" << std::endl;
		throw "Unrecognized magic number";
	}
	Linker::Error << "Error: Unrecognized magic number, assuming " << (endiantype == BigEndian ? "big endian" : "little endian") << std::endl;
}

void COFFFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	rd.ReadData(2, signature);
	DetectCpuType();
	rd.endiantype = endiantype;

	section_count = rd.ReadUnsigned(2);
	timestamp = rd.ReadUnsigned(4);
	symbol_table_offset = rd.ReadUnsigned(4);
	symbol_count = rd.ReadUnsigned(4);
	optional_header_size = rd.ReadUnsigned(2);
	flags = rd.ReadUnsigned(2);

	switch(optional_header_size)
	{
	case 28:
		/* a.out header */
		optional_header = new AOutHeader;
		break;
	case 32:
		/* GNU a.out header */
		if(cpu_type == CPU_I386)
		{
			optional_header = new GNUAOutHeader;
		}
		break;
	case 36:
		/* CDOS68K */
		if(cpu_type == CPU_M68K || cpu_type == CPU_I386)
		{
			optional_header = new FlexOSAOutHeader;
			break;
		}
		break;
	}
	if(optional_header == nullptr && optional_header_size != 0)
	{
		optional_header = new UnknownOptionalHeader(optional_header_size);
	}

//	offset_t optional_header_offset = rd.Tell();
	if(optional_header)
	{
		optional_header->ReadFile(rd);
	}
//	/* if not recognized, skip optional header */
//	rd.Seek(optional_header_offset + optional_header_size);

	for(size_t i = 0; i < section_count; i++)
	{
		Section * section = new Section;
		section->ReadSectionHeader(rd);
		sections.push_back(section);
	}

	for(auto section : sections)
	{
		if(section->flags & (Section::TEXT | Section::DATA))
		{
			rd.Seek(file_offset + section->section_pointer);
			dynamic_cast<Linker::Buffer *>(section->image)->ReadFile(rd, section->ActualDataSize());
		}
	}

	switch(cpu_type)
	{
	case CPU_Z80:
	case CPU_Z8K:
		for(auto section : sections)
		{
			if(section->relocation_count > 0)
			{
				rd.Seek(file_offset + section->relocation_pointer);
				for(size_t i = 0; i < section->relocation_count; i++)
				{
					ZilogRelocation * rel = new ZilogRelocation(cpu_type);
					rel->Read(rd);
					section->relocations.push_back(rel);
				}
			}
		}
		break;
	default:
		/* TODO: relocations for other formats */
		break;
	}

	if(symbol_count > 0)
	{
		rd.Seek(file_offset + symbol_table_offset);
		while(symbols.size() < symbol_count)
		{
			Symbol * symbol = new Symbol;
			symbol->Read(rd);
			symbols.push_back(symbol);
			symbols.resize(symbols.size() + symbol->auxiliary_count);
		}
		size_t string_table_offset = rd.Tell();
		for(Symbol * symbol : symbols)
		{
			if(symbol && symbol->name_index != 0)
			{
				rd.Seek(string_table_offset + symbol->name_index);
				Linker::Debug << "Debug: Reading symbol index: " << symbol->name_index << std::endl;
				symbol->name = rd.ReadASCIIZ();
			}
		}

#if 0
		for(Symbol * symbol : symbols)
		{
			if(symbol)
			{
				Linker::Debug << "Debug: " << symbol->name << " = " << symbol->value << std::endl;
			}
			else
			{
				Linker::Debug << "Debug: null" << std::endl;
			}
		}
#endif
	}

	/* TODO: COFF line numbers (not used or generated by this linker) */

	if(optional_header)
	{
		optional_header->PostReadFile(*this, rd);
	}
}

void COFFFormat::WriteFile(Linker::Writer& wr)
{
	if(type == DJGPP && stub_file != "")
	{
		WriteStubImage(wr);
	}

	wr.endiantype = endiantype;

	/* File Header */
	wr.WriteData(2, signature);
	wr.WriteWord(2, section_count);
	wr.WriteWord(4, timestamp); /* unused */
	wr.WriteWord(4, symbol_table_offset); /* unused */
	wr.WriteWord(4, symbol_count); /* unused */
	wr.WriteWord(2, optional_header_size);
	wr.WriteWord(2, flags);

	/* Optional Header */
	if(optional_header)
	{
		optional_header->WriteFile(wr);
	}

	/* Section Header */
	for(auto section : sections)
	{
		section->WriteSectionHeader(wr);
	}

	/* Section Data */
	for(auto section : sections)
	{
		section->image->WriteFile(wr);
	}

	/* TODO: store COFF relocations, symbols */

	if(optional_header)
	{
		optional_header->PostWriteFile(*this, wr);
	}
}

void COFFFormat::Dump(Dumper::Dumper& dump)
{
	if(cpu_type == CPU_I386)
	{
		dump.SetEncoding(Dumper::Block::encoding_cp437);
	}
	else
	{
		dump.SetEncoding(Dumper::Block::encoding_default);
	}

	dump.SetTitle("COFF format");

	std::map<offset_t, std::string> cpu_descriptions;
	cpu_descriptions[CPU_I386]  = "Intel 80386"; // (DJGPP or FlexOS 386)
	cpu_descriptions[CPU_M68K]  = "Motorola 68000";
	cpu_descriptions[CPU_Z80]   = "Zilog Z80";
	cpu_descriptions[CPU_Z8K]   = "Zilog Z8000";

	cpu_descriptions[CPU_I86]   = "Intel 8086";
	cpu_descriptions[CPU_NS32K] = "National Semiconductor NS32000",
	cpu_descriptions[CPU_I370]  = "IBM System/370";
	cpu_descriptions[CPU_MIPS]  = "MIPS";
	cpu_descriptions[CPU_M88K]  = "Motorola 88000";
	cpu_descriptions[CPU_WE32K] = "Western Electric 32000";
	cpu_descriptions[CPU_VAX]   = "DEC VAX";
	cpu_descriptions[CPU_AM29K] = "AMD 29000";
	cpu_descriptions[CPU_ALPHA] = "DEC Alpha";
	cpu_descriptions[CPU_PPC]   = "IBM POWER, 32-bit";
	cpu_descriptions[CPU_PPC64] = "IBM POWER, 64-bit";
	cpu_descriptions[CPU_SHARC] = "SHARC";

	Dumper::Region file_region("File", file_offset, 0, 8); /* TODO: file size */
	file_region.AddField("CPU type", new Dumper::ChoiceDisplay(cpu_descriptions), (offset_t)cpu_type);

	std::map<offset_t, std::string> endian_descriptions;
	endian_descriptions[::LittleEndian] = "little endian";
	endian_descriptions[::BigEndian] = "big endian";
	file_region.AddField("Byte order", new Dumper::ChoiceDisplay(endian_descriptions), (offset_t)endiantype);

	file_region.Display(dump);

	Dumper::Region header_region("File header", file_offset, 20, 8);
	header_region.AddField("Signature", new Dumper::HexDisplay(4), (offset_t)ReadUnsigned(2, 2, reinterpret_cast<uint8_t *>(signature), endiantype));
	header_region.AddOptionalField("Time stamp", new Dumper::HexDisplay, (offset_t)timestamp);
	header_region.AddOptionalField("Flags",
		&Dumper::BitFieldDisplay::Make()
			.AddBitField(0, 1, new Dumper::ChoiceDisplay("no relocations"), true)
			.AddBitField(1, 1, new Dumper::ChoiceDisplay("executable"), true)
			.AddBitField(2, 1, new Dumper::ChoiceDisplay("no line numbers"), true)
			.AddBitField(3, 1, new Dumper::ChoiceDisplay("no symbols"), true)
			.AddBitField(8, 1, new Dumper::ChoiceDisplay("32-bit little endian"), true)
			.AddBitField(9, 1, new Dumper::ChoiceDisplay("32-bit big endian"), true),
		(offset_t)flags);
	header_region.Display(dump);

	if(optional_header)
	{
		optional_header->Dump(*this, dump);
	}

	for(auto& section : sections)
	{
		Dumper::Block block("Section", file_offset + section->section_pointer, section->image, section->address, 8);
		block.InsertField(0, "Name", new Dumper::StringDisplay, section->name);
		if(section->image->ActualDataSize() != section->size)
			block.AddField("Size in memory", new Dumper::HexDisplay, (offset_t)section->size);
		block.AddField("Physical address", new Dumper::HexDisplay, (offset_t)section->physical_address);
		block.AddOptionalField("Line numbers", new Dumper::HexDisplay, (offset_t)section->line_number_pointer); /* TODO */
		block.AddOptionalField("Line numbers count", new Dumper::DecDisplay, (offset_t)section->line_number_count); /* TODO */
		block.AddOptionalField("Flags",
			&Dumper::BitFieldDisplay::Make()
				.AddBitField(5, 1, new Dumper::ChoiceDisplay("text"), true)
				.AddBitField(6, 1, new Dumper::ChoiceDisplay("data"), true)
				.AddBitField(7, 1, new Dumper::ChoiceDisplay("bss"), true),
			(offset_t)section->flags);

		Dumper::Region relocations("Section relocation", file_offset + section->relocation_pointer, 0, 8); /* TODO: size */
		block.AddOptionalField("Count", new Dumper::DecDisplay, (offset_t)section->relocation_count);
		relocations.Display(dump);

		unsigned i = 0;
		for(auto relocation : section->relocations)
		{
			Dumper::Entry relocation_entry("Relocation", i + 1, offset_t(-1) /* TODO: offset */, 8);
			relocation->FillEntry(relocation_entry);
			relocation_entry.Display(dump);

			block.AddSignal(relocation->GetAddress() - section->address, relocation->GetSize());
			i++;
		}

		block.Display(dump);
	}

	Dumper::Region symbol_table("Symbol table", file_offset + symbol_table_offset, symbol_count * 18, 8);
	symbol_table.AddField("Count", new Dumper::DecDisplay, (offset_t)symbol_count);
	symbol_table.Display(dump);
	unsigned i = 0;
	for(auto symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i + 1, file_offset + symbol_table_offset + 18 * i, 8);
		if(symbol)
		{
			/* otherwise, ignored symbol */
			symbol_entry.AddField("Name", new Dumper::StringDisplay, symbol->name);
			symbol_entry.AddOptionalField("Name index", new Dumper::HexDisplay, (offset_t)symbol->name_index);
			symbol_entry.AddField("Value", new Dumper::HexDisplay, (offset_t)symbol->value);
			symbol_entry.AddField("Section", new Dumper::HexDisplay, (offset_t)symbol->section_number);
			symbol_entry.AddField("Type", new Dumper::HexDisplay(4), (offset_t)symbol->type);
			symbol_entry.AddField("Storage class", new Dumper::HexDisplay(2), (offset_t)symbol->storage_class);
			symbol_entry.AddOptionalField("Auxiliary count", new Dumper::DecDisplay, (offset_t)symbol->auxiliary_count);
		}
		symbol_entry.Display(dump);
		i += 1;
	}
}

/* * * Reader members * * */

void COFFFormat::SetupOptions(char special_char, Linker::OutputFormat * format)
{
	special_prefix_char = special_char;
	option_segmentation = format->FormatSupportsSegmentation();
//	option_16bit = format->FormatIs16bit();
//	option_linear = format->FormatIsLinear();
////	option_stack_section = format->FormatSupportsStackSection();
//	option_resources = format->FormatSupportsResources();
//	option_libraries = format->FormatSupportsLibraries();
}

std::string COFFFormat::segment_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEG" << special_prefix_char;
	return oss.str();
}

std::string COFFFormat::segment_of_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGOF" << special_prefix_char;
	return oss.str();
}

std::string COFFFormat::segmented_address_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGADR" << special_prefix_char;
	return oss.str();
}

#if 0
// TODO: can this be used?
std::string COFFFormat::segment_difference_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGDIF" << special_prefix_char;
	return oss.str();
}
#endif

void COFFFormat::GenerateModule(Linker::Module& module)
{
	switch(cpu_type)
	{
	case CPU_I386:
		module.cpu = /*option_16bit ? Linker::Module::I86 :*/ Linker::Module::I386; /* TODO */
		break;
	case CPU_M68K:
		module.cpu = Linker::Module::M68K;
		break;
	case CPU_Z80:
		module.cpu = Linker::Module::I80;
		break;
	case CPU_Z8K:
		module.cpu = Linker::Module::Z8K;
		break;
	default:
		/* unknown CPU */
		break;
	}

	std::vector<Linker::Section *> linker_sections;
	for(Section * section : sections)
	{
		Linker::Section * linker_section = new Linker::Section(section->name);
		linker_section->SetReadable(true);
		if(section->flags & Section::BSS)
		{
			linker_section->SetZeroFilled(true);
			linker_section->Expand(section->size);
		}
		else if(section->flags & (Section::TEXT | Section::DATA))
		{
			Linker::Section * buffer = dynamic_cast<Linker::Section *>(section->image);
			linker_section->Append(*buffer);
		}
		if(section->flags & Section::TEXT)
		{
			linker_section->SetExecable(true);
		}
		else
		{
			linker_section->SetWritable(true);
		}
		module.AddSection(linker_section);
		linker_sections.push_back(linker_section);
	}

	for(Symbol * symbol : symbols)
	{
		if(symbol == nullptr)
			continue;
		switch(symbol->storage_class)
		{
		case C_EXT:
			if(symbol->section_number != N_UNDEF)
				module.AddGlobalSymbol(symbol->name, Linker::Location(linker_sections[symbol->section_number - 1], symbol->value));
			else if(symbol->value != 0)
				module.AddCommonSymbol(symbol->name, Linker::CommonSymbol(symbol->name, symbol->value, 1 /* TODO: alignment */));
			break;
		case C_STAT:
		case C_LABEL:
			if(symbol->section_number == N_ABS)
				module.AddLocalSymbol(symbol->name, Linker::Location(symbol->value));
			else if(symbol->section_number != N_UNDEF && symbol->section_number != N_DEBUG)
				module.AddLocalSymbol(symbol->name, Linker::Location(linker_sections[symbol->section_number - 1], symbol->value));
			break;
		}
	}

	for(size_t i = 0; i < sections.size(); i++)
	{
		Section * section = sections[i];
		for(auto _rel : section->relocations)
		{
			switch(cpu_type)
			{
			case CPU_Z80:
				{
					ZilogRelocation& rel = *dynamic_cast<ZilogRelocation *>(_rel);

					Linker::Location rel_source = Linker::Location(linker_sections[i], rel.address);
					Symbol& sym_target = *symbols[rel.symbol_index];
					Linker::Target rel_target =
						sym_target.storage_class == C_STAT && sym_target.value == 0
						? Linker::Target(Linker::Location(linker_sections[sym_target.section_number - 1]))
						: Linker::Target(Linker::SymbolName(sym_target.name));
					Linker::Relocation obj_rel = Linker::Relocation::Empty();

					size_t rel_size;
					switch(rel.type)
					{
					case ZilogRelocation::R_Z80_IMM8:
					case ZilogRelocation::R_Z80_OFF8:
					case ZilogRelocation::R_Z80_JR:
						rel_size = 1;
						break;
					case ZilogRelocation::R_Z80_IMM16:
						rel_size = 2;
						break;
					case ZilogRelocation::R_Z80_IMM24:
						rel_size = 3;
						break;
					case ZilogRelocation::R_Z80_IMM32:
						rel_size = 4;
						break;
					default:
						Linker::Error << "Error: Unknown relocation type" << std::endl;
						continue;
					}

					switch(rel.type)
					{
					case ZilogRelocation::R_Z80_IMM8:
					case ZilogRelocation::R_Z80_IMM16:
					case ZilogRelocation::R_Z80_IMM24:
					case ZilogRelocation::R_Z80_IMM32:
					case ZilogRelocation::R_Z80_OFF8:
						obj_rel = Linker::Relocation::Absolute(rel_size, rel_source, rel_target, 0, ::LittleEndian);
						break;
					case ZilogRelocation::R_Z80_JR:
						obj_rel = Linker::Relocation::Relative(rel_size, rel_source, rel_target, 0, ::LittleEndian);
						break;
					default:
						break;
					}
					obj_rel.AddCurrentValue();
					module.relocations.push_back(obj_rel);
					Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
				}
				break;
			case CPU_Z8K:
				{
					ZilogRelocation& rel = *dynamic_cast<ZilogRelocation *>(_rel);

					Linker::Location rel_source = Linker::Location(linker_sections[i], rel.address);
					Symbol& sym_target = *symbols[rel.symbol_index];
					Linker::Target rel_target =
						sym_target.storage_class == C_STAT && sym_target.value == 0
						? Linker::Target(Linker::Location(linker_sections[sym_target.section_number - 1]))
						: Linker::Target(Linker::SymbolName(sym_target.name));
					Linker::Relocation obj_rel = Linker::Relocation::Empty();

					switch(rel.type)
					{
					case ZilogRelocation::R_Z8K_IMM4L:
						obj_rel = Linker::Relocation::Offset(1, rel_source, rel_target, 0, ::BigEndian);
						obj_rel.SetMask(0x0F);
						break;
					case ZilogRelocation::R_Z8K_IMM4H:
						/* TODO: untested */
						obj_rel = Linker::Relocation::Offset(1, rel_source, rel_target, 0, ::BigEndian);
						obj_rel.SetMask(0xF0);
						break;
					case ZilogRelocation::R_Z8K_IMM8:
						obj_rel = Linker::Relocation::Offset(1, rel_source, rel_target, 0, ::BigEndian);
						break;
					case ZilogRelocation::R_Z8K_IMM16:
						if(option_segmentation && sym_target.section_number != N_UNDEF && sym_target.section_number != N_ABS && sym_target.section_number != N_DEBUG)
						{
							/* segment part */
							obj_rel = Linker::Relocation::Segment(1, rel_source, rel_target.GetSegment(), rel.offset);
							obj_rel.SetMask(0x7F);
							module.relocations.push_back(obj_rel);
							Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
							/* offset part */
							obj_rel = Linker::Relocation::Offset(1, rel_source + 1, rel_target, rel.offset >> 8, ::BigEndian);
							rel_source.section->WriteWord(2, rel_source.offset, 0x0000, ::BigEndian);
						}
						else
						{
							obj_rel = Linker::Relocation::Offset(2, rel_source, rel_target, 0, ::BigEndian);
						}
						break;
					case ZilogRelocation::R_Z8K_IMM32:
						if(option_segmentation && /*sym_target.section_number != N_UNDEF &&*/ sym_target.section_number != N_ABS && sym_target.section_number != N_DEBUG)
						{
							/* segment part */
							obj_rel = Linker::Relocation::Segment(1, rel_source, rel_target.GetSegment(), rel.offset);
							obj_rel.SetMask(0x7F);
							module.relocations.push_back(obj_rel);
							Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
							/* offset part */
							obj_rel = Linker::Relocation::Offset(2, rel_source + 2, rel_target, rel.offset >> 16, ::BigEndian);
							rel_source.section->WriteWord(2, rel_source.offset, 0x8000, ::BigEndian);
						}
						else
						{
							obj_rel = Linker::Relocation::Offset(4, rel_source, rel_target, 0, ::BigEndian);
						}
						break;
					case ZilogRelocation::R_Z8K_DISP7:
						obj_rel = Linker::Relocation::Relative(1, rel_source, rel_target, 1, ::BigEndian);
						obj_rel.SetSubtract();
						obj_rel.SetMask(0x7F);
						obj_rel.SetShift(1);
						break;
					case ZilogRelocation::R_Z8K_REL16:
						obj_rel = Linker::Relocation::Relative(2, rel_source, rel_target, -2, ::BigEndian);
						break;
					case ZilogRelocation::R_Z8K_JR:
						obj_rel = Linker::Relocation::Relative(1, rel_source, rel_target, -1, ::BigEndian);
						break;
					case ZilogRelocation::R_Z8K_CALLR:
						obj_rel = Linker::Relocation::Relative(2, rel_source, rel_target, 2, ::BigEndian);
						obj_rel.SetSubtract();
						obj_rel.SetMask(0x0FFF);
						obj_rel.SetShift(1);
						break;
					}

#if 0
					if(sym_target.IsExternal())
					{
						if(option_segmentation && sym_target.name.rfind(segment_prefix(), 0) == 0 && (rel.type == ZilogRelocation::R_Z8K_IMM16 || rel.type == ZilogRelocation::R_Z8K_IMM32))
						{
							/* $$SEG$<section name> */
							/* Note: can only refer to a currently present section */
							std::string section_name = sym_target.name.substr(segment_prefix().size());
							Linker::Section * section = module.FindSection(section_name);
							if(section == nullptr)
							{
								Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
							}
							else
							{
								obj_rel = Linker::Relocation::Segment(1, rel_source, Linker::Target(Linker::Location(section)).GetSegment(), rel.offset);
								obj_rel.SetMask(0x7F);
							}
							rel_source.section->WriteWord(2, rel_source.offset, 0, ::BigEndian);
						}
						else if(option_segmentation && sym_target.name.rfind(segment_of_prefix(), 0) == 0 && rel.type == ZilogRelocation::R_Z8K_IMM16)
						{
							/* $$SEGOF$<symbol name> */
							std::string symbol_name = sym_target.name.substr(segment_of_prefix().size());
							obj_rel = Linker::Relocation::Segment(1, rel_source, Linker::Target(Linker::SymbolName(symbol_name)).GetSegment(), rel.offset);
							obj_rel.SetMask(0x7F);
							rel_source.section->WriteWord(2, rel_source.offset, 0, ::BigEndian);
						}
						else if(option_segmentation && sym_target.name.rfind(segmented_address_prefix(), 0) == 0 && (rel.type == ZilogRelocation::R_Z8K_IMM16 || rel.type == ZilogRelocation::R_Z8K_IMM32))
						{
							/* $$SEGADR$<symbol name> */
							std::string symbol_name = sym_target.name.substr(segmented_address_prefix().size());
							rel_target = Linker::Target(Linker::SymbolName(symbol_name));
							/* segment part */
							obj_rel = Linker::Relocation::Segment(1, rel_source, rel_target.GetSegment(), rel.offset);
							obj_rel.SetMask(0x7F);
							module.relocations.push_back(obj_rel);
							Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
							/* offset part */
							if(rel.type == ZilogRelocation::R_Z8K_IMM32)
							{
								obj_rel = Linker::Relocation::Offset(2, rel_source + 2, rel_target, rel.offset >> 16, ::BigEndian);
							}
							else
							{
								obj_rel = Linker::Relocation::Offset(1, rel_source + 1, rel_target, rel.offset >> 8, ::BigEndian);
							}
							rel_source.section->WriteWord(2, rel_source.offset,
								obj_rel.size == 2 ? 0x8000 : 0x0000,
								::BigEndian);
						}
					}
#endif

					// The value in the COFF image is usually unusable, so we ignore it
					//obj_rel.AddCurrentValue();
					module.relocations.push_back(obj_rel);
					Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
				}
				break;
			default:
				/* unknown CPU */
				break;
			}
		}
	}

	/* release section data */
	for(Section * section : sections)
	{
		section->image = nullptr;
	}
}

void COFFFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	ReadFile(rd);
	GenerateModule(module);
}

/* * * Writer members * * */

unsigned COFFFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if((type == CDOS68K || type == CDOS386) && (section_name == ".stack" || section_name.rfind(".stack.", 0) == 0))
	{
		return Linker::Section::Stack;
	}
	else
	{
		return 0;
	}
}

COFFFormat * COFFFormat::CreateWriter(format_type type)
{
	return new COFFFormat(type);
}

void COFFFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub_file = FetchOption(options, "stub", "");

	SetLinkerParameter(options, "code_base", "code_base_address");
	SetLinkerParameter(options, "data_base", "data_base_address");
	SetLinkerParameter(options, "bss_base", "bss_base_address");

	/* TODO */
	option_no_relocation = false;
}

void COFFFormat::OnNewSegment(Linker::Segment * segment)
{
	if(segment->name == ".code")
	{
		if(GetCodeSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 0)
		{
			Linker::Warning << "Warning: Out of order `.code` segment" << std::endl;
		}
		sections.push_back(new Section(Section::TEXT, segment));
	}
	else if(segment->name == ".data")
	{
		if(GetDataSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.data` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 1)
		{
			Linker::Warning << "Warning: Out of order `.data` segment" << std::endl;
		}
		sections.push_back(new Section(Section::DATA, segment));
	}
	else if(segment->name == ".bss")
	{
		if(GetBssSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.bss` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 2)
		{
			Linker::Warning << "Warning: Out of order `.bss` segment" << std::endl;
		}
		sections.push_back(new Section(Section::BSS, segment));
	}
	else if((type == CDOS68K || type == CDOS386) && segment->name == ".stack")
	{
		if(stack != nullptr)
		{
			Linker::Error << "Error: Duplicate `.stack` segment, ignoring" << std::endl;
			return;
		}
		stack = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

void COFFFormat::CreateDefaultSegments()
{
	if(GetCodeSegment() == nullptr)
	{
		sections.push_back(new Section(Section::TEXT, new Linker::Segment(".code")));
	}
	if(GetDataSegment() == nullptr)
	{
		sections.push_back(new Section(Section::DATA, new Linker::Segment(".data")));
	}
	if(GetBssSegment() == nullptr)
	{
		sections.push_back(new Section(Section::BSS, new Linker::Segment(".bss")));
	}
	if((type == CDOS68K || type == CDOS386) && stack == nullptr)
	{
		stack = new Linker::Segment(".stack");
	}
}

Script::List * COFFFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align 4;
};

".data"
{
	at max(here, ?data_base_address?);
	all not zero align 4;
	align 4;
};

".bss"
{
	at max(here, ?bss_base_address?);
	all align 4;
	align 4
};
)";

	static const char * SimpleScript_cdos68k = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align 4;
};

".data"
{
	at max(here, ?data_base_address?);
	all not zero align 4;
	align 4;
};

".bss"
{
	at max(here, ?bss_base_address?);
	all not ".stack" align 4;
	align 4
};

".stack"
{
	# TODO: where
	all align 4;
	align 4
};
)";

	if(linker_script != "")
	{
		return LinkerManager::GetScript(module);
	}
	else
	{
		if(type == CDOS68K || type == CDOS386)
		{
			return Script::parse_string(SimpleScript_cdos68k);
		}
		else
		{
			return Script::parse_string(SimpleScript);
		}
	}
}

void COFFFormat::Link(Linker::Module& module)
{
	Script::List * script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

/** @brief Return the segment stored inside the section, note that this only works for binary generation */
Linker::Segment * COFFFormat::GetSegment(Section * section)
{
	return dynamic_cast<Linker::Segment *>(section->image);
}

Linker::Segment * COFFFormat::GetCodeSegment()
{
	for(auto section : sections)
	{
		if((section->flags & Section::TEXT))
			return GetSegment(section);
	}
	return nullptr;
}

Linker::Segment * COFFFormat::GetDataSegment()
{
	for(auto section : sections)
	{
		if((section->flags & Section::DATA))
			return GetSegment(section);
	}
	return nullptr;
}

Linker::Segment * COFFFormat::GetBssSegment()
{
	for(auto section : sections)
	{
		if((section->flags & Section::BSS))
			return GetSegment(section);
	}
	return nullptr;
}

void COFFFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignroing" << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr && resolution.reference == nullptr)
		{
			if(type == DJGPP)
			{
				if(rel.segment_of)
				{
					Linker::Error << "Error: segment relocations not supported, ignoring" << std::endl;
				}
			}
			else if(option_no_relocation || type == CDOS386)
			{
				Linker::Error << "Error: relocations suppressed, generating image anyway" << std::endl;
			}
			else if(rel.size != 2 && rel.size != 4)
			{
				Linker::Error << "Error: Format only supports word and longword relocations: " << rel << ", ignoring" << std::endl;
				continue;
			}
			else
			{
				/* CDOS68K crunched relocations */
				relocations[rel.source.GetPosition().address] = rel.size;
			}
		}
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		entry_address = entry.GetPosition().address;
	}
	else
	{
		entry_address = GetCodeSegment()->base_address;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void COFFFormat::CalculateValues()
{
	switch(type)
	{
	case GENERIC:
		/* TODO */
		throw Linker::Exception("Internal error: no generation type specified, exiting");
	case DJGPP:
		cpu_type = CPU_I386;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_LITTLE_ENDIAN;
		optional_header = new AOutHeader(ZMAGIC);
		break;
	case CDOS386:
		/* TODO */
		cpu_type = CPU_I386;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_LITTLE_ENDIAN;
		optional_header = new FlexOSAOutHeader;
		break;
	case CDOS68K:
		cpu_type = CPU_M68K;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_BIG_ENDIAN;
		optional_header = new FlexOSAOutHeader;
		break;
	}
	endiantype = GetEndianType();

	section_count = sections.size();

	optional_header_size = optional_header->GetSize();

	offset_t offset = 20 + optional_header->GetSize() + sections.size() * 40;
	if(type == DJGPP)
	{
		GetStubImageSize();
	}
	for(auto section : sections)
	{
		Linker::Segment * image = GetSegment(section);
		section->name = image->name;
		section->physical_address = section->address = image->base_address;
		section->size = image->TotalSize();
		section->section_pointer = offset;
		offset += image->data_size;
	}
	if(type == CDOS68K || type == CDOS386)
	{
		relocations_offset = offset;
	}

	if(AOutHeader * header = dynamic_cast<AOutHeader *>(optional_header))
	{
		/* Note: for PE, these would be added up for all code/data/bss segments, but here we assume that one of each is available */
		header->code_size = GetCodeSegment() ? GetCodeSegment()->data_size : 0; /* not needed for DJGPP */
		header->data_size = GetDataSegment() ? GetDataSegment()->data_size : 0; /* not needed for DJGPP */
		header->bss_size  = GetBssSegment() ? GetBssSegment()->zero_fill : 0; /* not needed for DJGPP */
		header->entry_address = entry_address;
		header->code_address = GetCodeSegment() ? GetCodeSegment()->base_address : 0;
		header->data_address = GetDataSegment() ? GetDataSegment()->base_address : 0;
	}

	if(FlexOSAOutHeader * header = dynamic_cast<FlexOSAOutHeader *>(optional_header))
	{
		header->relocations_offset = relocations_offset;
		header->stack_size = stack->zero_fill;
	}

	AssignMagicValue();
}

void COFFFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	switch(module.cpu)
	{
	case Linker::Module::I386:
		if(type != DJGPP && type != CDOS386)
		{
			Linker::Error << "Error: Only DJGPP (and FlexOS 386) supports Intel 80386 binaries, switching to it" << std::endl;
			type = DJGPP;
		}
		break;
	case Linker::Module::M68K:
		if(type != CDOS68K)
		{
			Linker::Error << "Error: Only Concurrent DOS 68K supports Motorola 68000 binaries, switching to it" << std::endl;
			type = CDOS68K;
		}
		break;
	default:
		Linker::Error << "Error: Format only supports Intel 80386 and Motorola 68000 binaries" << std::endl;
	}

	if(linker_parameters.find("code_base_address") == linker_parameters.end())
	{
		if(type == DJGPP)
		{
			linker_parameters["code_base_address"] = Linker::Location(0x1000);
		}
		else
		{
			Linker::Warning << "Warning: no base address specified for .code, setting to 0" << std::endl;
			linker_parameters["code_base_address"] = Linker::Location();
		}
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string COFFFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	switch(type)
	{
	case DJGPP:
		return filename + ".exe";
	case CDOS68K:
		return filename + ".68k";
	case CDOS386:
		return filename + ".386";
	default:
		assert(false);
	}
}
