
#include "coff.h"
#include "../linker/location.h"
#include "../linker/section.h"
#include "../linker/position.h"
#include "../linker/resolution.h"
#include "../linker/symbol_name.h"

using namespace COFF;

const std::map<uint32_t, COFF::COFFFormat::MachineType> COFF::COFFFormat::MACHINE_TYPES = {
	/* Standard UNIX COFF values */
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
	{ std::make_pair(0x014C,    MachineType { CPU_I386,  ::LittleEndian }) }, // GNU header, FlexOS header
	{ std::make_pair(0x014D,    MachineType { CPU_NS32K, ::LittleEndian }) }, // TODO: also I860
	// 0x014E -- Intel
	// 0x014F -- Intel
	{ std::make_pair(0x0150,    MachineType { CPU_M68K,  ::BigEndian }) }, // FlexOS header
	{ std::make_pair(0x0151,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x0152,    MachineType { CPU_M68K,  ::BigEndian }) }, // TODO: also 286
	{ std::make_pair(0x0154,    MachineType { CPU_NS32K, ::LittleEndian }) },
	{ std::make_pair(0x0155,    MachineType { CPU_NS32K, ::LittleEndian }) },
	{ std::make_pair(0x0158,    MachineType { CPU_I370,  ::BigEndian }) },
	{ std::make_pair(0x0159,    MachineType { CPU_I370,  ::BigEndian }) }, // Amdahl
	// 0x015A -- I370
	// 0x015B -- I370
	{ std::make_pair(0x015C,    MachineType { CPU_I370,  ::BigEndian }) }, // Amdahl
	{ std::make_pair(0x015D,    MachineType { CPU_I370,  ::BigEndian }) },
	{ std::make_pair(0x0160,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF but also COFF with MIPS optional header?
	// also OpenBSD i960
	{ std::make_pair(0x0162,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0163,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	// 0x0164 -- Zilog
	// 0x0165 -- Zilog
	{ std::make_pair(0x0166,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0168,    MachineType { CPU_WE32K, ::BigEndian }) }, // 3B20 // TODO: also NetBSD SH3
	{ std::make_pair(0x0169,    MachineType { CPU_WE32K, ::BigEndian }) }, // 3B20
	{ std::make_pair(0x016C,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x016D,    MachineType { CPU_M88K,  ::BigEndian }) },
	{ std::make_pair(0x0170,    MachineType { CPU_WE32K, ::BigEndian }) },
	{ std::make_pair(0x0171,    MachineType { CPU_WE32K, ::BigEndian }) },
	// 0x0172 -- WE32K
	{ std::make_pair(0x0175,    MachineType { CPU_I386,  ::LittleEndian }) },
	{ std::make_pair(0x0178,    MachineType { CPU_VAX,   ::LittleEndian }) },
	{ std::make_pair(0x017A,    MachineType { CPU_AM29K, ::BigEndian }) },
	{ std::make_pair(0x017B,    MachineType { CPU_AM29K, ::LittleEndian }) },
	{ std::make_pair(0x017D,    MachineType { CPU_VAX,   ::LittleEndian }) },
	// TODO: 0x017F - CLIPPER (unknown endianness)
	{ std::make_pair(0x0180,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0182,    MachineType { CPU_MIPS,  ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0183,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0185,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0188,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x018F,    MachineType { CPU_ALPHA, ::Undefined }) }, // ECOFF
	{ std::make_pair(0x0194,    MachineType { CPU_M88K,  ::BigEndian }) },
	{ std::make_pair(0x0197,    MachineType { CPU_M68K,  ::BigEndian }) },
	{ std::make_pair(0x01DF,    MachineType { CPU_PPC,   ::BigEndian }) }, // XCOFF
	{ std::make_pair(0x01F7,    MachineType { CPU_PPC64, ::BigEndian }) }, // XCOFF
	{ std::make_pair(0x1572,    MachineType { CPU_AM29K, ::BigEndian }) },
	{ std::make_pair(0x521C,    MachineType { CPU_SHARC, ::LittleEndian }) },
	{ std::make_pair(0x8000,    MachineType { CPU_Z8K,   ::BigEndian }) }, // GNU binutils
	{ std::make_pair(0x805A,    MachineType { CPU_Z80,   ::LittleEndian }) }, // GNU binutils
	{ std::make_pair(0x6500,    MachineType { CPU_W65,   ::LittleEndian }) }, // GNU binutils
	/* Microsoft PE/COFF values, these entries are always stored in little endian order */
	//{ std::make_pair(0x014C,    MachineType { CPU_I386,  ::LittleEndian }) }, // already defined
	{ std::make_pair(0x014D,    MachineType { CPU_I860,  ::LittleEndian }) }, // also old value for Intel 486
	{ std::make_pair(0x014E,    MachineType { CPU_I386,  ::LittleEndian }) }, // old value for Intel Pentium
	//{ std::make_pair(0x0162,    MachineType { CPU_MIPS,  ::LittleEndian }) }, // already defined
	//{ std::make_pair(0x0163,    MachineType { CPU_MIPS,  ::LittleEndian }) }, // already defined
	//{ std::make_pair(0x0166,    MachineType { CPU_MIPS,  ::LittleEndian }) }, // already defined
	{ std::make_pair(0x0168,    MachineType { CPU_MIPS,  ::LittleEndian }) },
	{ std::make_pair(0x0169,    MachineType { CPU_MIPS,  ::LittleEndian }) },
	//{ std::make_pair(0x0183,    MachineType { CPU_ALPHA, ::LittleEndian }) }, // already defined
	{ std::make_pair(0x0184,    MachineType { CPU_ALPHA, ::LittleEndian }) },
	{ std::make_pair(0x01A2,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x01A3,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x01A6,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x01A8,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x01C0,    MachineType { CPU_ARM,   ::LittleEndian }) },
	{ std::make_pair(0x01C2,    MachineType { CPU_ARM,   ::LittleEndian }) },
	{ std::make_pair(0x01C4,    MachineType { CPU_ARM,   ::LittleEndian }) },
	{ std::make_pair(0x01D3,    MachineType { CPU_AM33,  ::LittleEndian }) },
	{ std::make_pair(0x01F0,    MachineType { CPU_PPC,   ::LittleEndian }) },
	{ std::make_pair(0x01F1,    MachineType { CPU_PPC,   ::LittleEndian }) },
	{ std::make_pair(0x0200,    MachineType { CPU_IA64,  ::LittleEndian }) },
	{ std::make_pair(0x0266,    MachineType { CPU_MIPS,  ::LittleEndian }) },
	{ std::make_pair(0x0268,    MachineType { CPU_M68K,  ::LittleEndian }) },
	{ std::make_pair(0x0284,    MachineType { CPU_ALPHA, ::LittleEndian }) },
	{ std::make_pair(0x0290,    MachineType { CPU_HPPA,  ::LittleEndian }) },
	{ std::make_pair(0x0366,    MachineType { CPU_MIPS,  ::LittleEndian }) },
	{ std::make_pair(0x0466,    MachineType { CPU_MIPS,  ::LittleEndian }) },
	{ std::make_pair(0x0500,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x0550,    MachineType { CPU_SH,    ::LittleEndian }) },
	{ std::make_pair(0x0601,    MachineType { CPU_PPC,   ::LittleEndian }) }, // (for Macintosh)
	{ std::make_pair(0x0EBC,    MachineType { CPU_EFI,   ::LittleEndian }) },
	{ std::make_pair(0x5032,    MachineType { CPU_RISCV32, ::LittleEndian }) },
	{ std::make_pair(0x5064,    MachineType { CPU_RISCV64, ::LittleEndian }) },
	{ std::make_pair(0x5128,    MachineType { CPU_RISCV128, ::LittleEndian }) },
	{ std::make_pair(0x6232,    MachineType { CPU_MIPS,  ::LittleEndian }) }, // LoongArch
	{ std::make_pair(0x6264,    MachineType { CPU_MIPS,  ::LittleEndian }) }, // LoongArch
	{ std::make_pair(0x8664,    MachineType { CPU_AMD64, ::LittleEndian }) },
	{ std::make_pair(0x9041,    MachineType { CPU_M32R,  ::LittleEndian }) },
	{ std::make_pair(0xA641,    MachineType { CPU_ARM64, ::LittleEndian }) },
	{ std::make_pair(0xAA64,    MachineType { CPU_ARM64, ::LittleEndian }) },
	// due to overloaded values, these require more than 16 bits and therefore cannot appear in files, included for endianness
	// for now, there are no overloaded values
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

void COFFFormat::UNIXRelocation::Read(Linker::Reader& rd)
{
	switch(coff_variant)
	{
	case COFF:
	case PECOFF:
		address = rd.ReadUnsigned(4);
		symbol_index = rd.ReadUnsigned(4);
		type = rd.ReadUnsigned(2);
		break;
	case ECOFF:
		address = rd.ReadUnsigned(8);
		symbol_index = rd.ReadUnsigned(4);
		type = rd.ReadUnsigned(1);
		information = rd.ReadUnsigned(3);
		break;
	case XCOFF32:
		address = rd.ReadUnsigned(4);
		symbol_index = rd.ReadUnsigned(4);
		information = rd.ReadUnsigned(1);
		type = rd.ReadUnsigned(1);
		break;
	case XCOFF64:
		address = rd.ReadUnsigned(8);
		symbol_index = rd.ReadUnsigned(4);
		information = rd.ReadUnsigned(1);
		type = rd.ReadUnsigned(1);
		break;
	}
}

offset_t COFFFormat::UNIXRelocation::GetAddress() const
{
	return address;
}

size_t COFFFormat::UNIXRelocation::GetSize() const
{
	switch(coff_variant)
	{
	case COFF:
		switch(type)
		{
		case R_ABS:
		case R_TOKEN: // TODO
		default:
			return 0;
		case R_SECREL7:
			return 1;
		case R_DIR16:
		case R_REL16:
		case R_SEG12:
		case R_SECTION:
			return 2;
		case R_DIR32:
		case R_REL32:
		case R_DIR32NB:
		case R_SECREL:
			return 4;
		}
	case PECOFF:
		return 0; // TODO
	case ECOFF:
		return 0; // TODO
	case XCOFF32:
		return 0; // TODO
	case XCOFF64:
		return 0; // TODO
	}
	assert(false);
}

offset_t COFFFormat::UNIXRelocation::GetEntrySize() const
{
	switch(coff_variant)
	{
	case COFF:
	case PECOFF:
		return 10;
	case ECOFF:
		return 16;
	case XCOFF32:
		return 10;
	case XCOFF64:
		return 14;
	}
	assert(false);
}

void COFFFormat::UNIXRelocation::WriteFile(Linker::Writer& wr) const
{
	switch(coff_variant)
	{
	case COFF:
	case PECOFF:
		wr.WriteWord(4, address);
		wr.WriteWord(4, symbol_index);
		wr.WriteWord(2, type);
		break;
	case ECOFF:
		wr.WriteWord(8, address);
		wr.WriteWord(4, symbol_index);
		wr.WriteWord(1, type);
		wr.WriteWord(3, information);
		break;
	case XCOFF32:
		wr.WriteWord(4, address);
		wr.WriteWord(4, symbol_index);
		wr.WriteWord(1, information);
		wr.WriteWord(1, type);
		break;
	case XCOFF64:
		wr.WriteWord(8, address);
		wr.WriteWord(4, symbol_index);
		wr.WriteWord(1, information);
		wr.WriteWord(1, type);
		break;
	}
}

void COFFFormat::UNIXRelocation::FillEntry(Dumper::Entry& entry) const
{
	// TODO
}

void COFFFormat::ZilogRelocation::Read(Linker::Reader& rd)
{
	address = rd.ReadUnsigned(4);
	symbol_index = rd.ReadUnsigned(4);
	offset = rd.ReadUnsigned(4);
	type = rd.ReadUnsigned(2);
	data = rd.ReadUnsigned(2);
}

offset_t COFFFormat::ZilogRelocation::GetAddress() const
{
	return address;
}

size_t COFFFormat::ZilogRelocation::GetSize() const
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
	case CPU_W65:
		switch(type)
		{
		case R_W65_ABS8:
		case R_W65_ABS8S8:
		case R_W65_ABS8S16:
		case R_W65_PCR8:
		case R_W65_DP:
			return 1;
		case R_W65_ABS16:
		case R_W65_ABS16S8:
		case R_W65_ABS16S16:
		case R_W65_PCR16:
			return 2;
		case R_W65_ABS24:
			return 3;
		default:
			Linker::Error << "Error: Unknown relocation type" << std::endl;
			return -1;
		}
	default:
		Linker::Error << "Error: Unknown relocation type" << std::endl;
		return -1;
	}
}

size_t COFFFormat::ZilogRelocation::GetEntrySize() const
{
	return 16;
}

void COFFFormat::ZilogRelocation::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, address);
	wr.WriteWord(4, symbol_index);
	wr.WriteWord(4, offset);
	wr.WriteWord(2, type);
	wr.WriteWord(2, data);
}

void COFFFormat::ZilogRelocation::FillEntry(Dumper::Entry& entry) const
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
	case CPU_W65:
		relocation_type_names[ZilogRelocation::R_W65_ABS8]     = "abs8";
		relocation_type_names[ZilogRelocation::R_W65_ABS16]    = "abs16";
		relocation_type_names[ZilogRelocation::R_W65_ABS24]    = "abs24";
		relocation_type_names[ZilogRelocation::R_W65_ABS8S8]   = "abs8s8";
		relocation_type_names[ZilogRelocation::R_W65_ABS8S16]  = "abs8s16";
		relocation_type_names[ZilogRelocation::R_W65_ABS16S8]  = "abs16s8";
		relocation_type_names[ZilogRelocation::R_W65_ABS16S16] = "abs16s16";
		relocation_type_names[ZilogRelocation::R_W65_PCR8]     = "pcr8";
		relocation_type_names[ZilogRelocation::R_W65_PCR16]    = "pcr16";
		relocation_type_names[ZilogRelocation::R_W65_DP]       = "dp";
		break;
	default:
		/* TODO */
		break;
	}

	entry.AddField("Source", Dumper::HexDisplay::Make(), offset_t(address));
	entry.AddField("Size", Dumper::HexDisplay::Make(1), offset_t(GetSize()));
	entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_names, Dumper::HexDisplay::Make(4)), offset_t(type));
	entry.AddOptionalField("Offset", Dumper::HexDisplay::Make(), offset_t(offset));
	/* TODO */
	entry.AddField("Symbol index", Dumper::HexDisplay::Make(), offset_t(symbol_index));
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

void COFFFormat::Section::Clear()
{
	image = nullptr;
	relocations.clear();
}

void COFFFormat::Section::ReadSectionHeader(Linker::Reader& rd, COFFVariantType coff_variant)
{
	switch(coff_variant)
	{
	case COFF:
	case XCOFF32:
	case PECOFF:
		name = rd.ReadData(8, true);
		physical_address = rd.ReadUnsigned(4);
		address = rd.ReadUnsigned(4);
		size = rd.ReadUnsigned(4);
		section_pointer = rd.ReadUnsigned(4);
		relocation_pointer = rd.ReadUnsigned(4);
		line_number_pointer = rd.ReadUnsigned(4);
		relocation_count = rd.ReadUnsigned(2);
		line_number_count = rd.ReadUnsigned(2);
		flags = rd.ReadUnsigned(coff_variant == XCOFF32 ? 2 : 4);
		break;
	case ECOFF:
		name = rd.ReadData(8, true);
		physical_address = rd.ReadUnsigned(8);
		address = rd.ReadUnsigned(8);
		size = rd.ReadUnsigned(8);
		section_pointer = rd.ReadUnsigned(8);
		relocation_pointer = rd.ReadUnsigned(8);
		line_number_pointer = rd.ReadUnsigned(8);
		relocation_count = rd.ReadUnsigned(2);
		line_number_count = rd.ReadUnsigned(2);
		flags = rd.ReadUnsigned(4);
		break;
	case XCOFF64:
		name = rd.ReadData(8, true);
		physical_address = rd.ReadUnsigned(8);
		address = rd.ReadUnsigned(8);
		size = rd.ReadUnsigned(8);
		section_pointer = rd.ReadUnsigned(8);
		relocation_pointer = rd.ReadUnsigned(8);
		line_number_pointer = rd.ReadUnsigned(8);
		relocation_count = rd.ReadUnsigned(4);
		line_number_count = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
		break;
	}

	/* TODO: Buffer instead of Section? */
	if(flags & TEXT)
	{
		image = std::make_shared<Linker::Section>(".text");
	}
	else if(flags & DATA)
	{
		image = std::make_shared<Linker::Section>(".data");
	}
	else if(flags & BSS)
	{
		image = std::make_shared<Linker::Section>(".bss");
	}
}

void COFFFormat::Section::WriteSectionHeader(Linker::Writer& wr, COFFVariantType coff_variant)
{
	switch(coff_variant)
	{
	default:
		Linker::FatalError("Internal error: undefined COFF variant");
	case COFF:
	case XCOFF32:
	case PECOFF:
		wr.WriteData(8, name);
		wr.WriteWord(4, physical_address);
		wr.WriteWord(4, address);
		wr.WriteWord(4, size);
		wr.WriteWord(4, section_pointer);
		wr.WriteWord(4, relocation_pointer);
		wr.WriteWord(4, line_number_pointer);
		wr.WriteWord(2, relocation_count);
		wr.WriteWord(2, line_number_count);
		wr.WriteWord(coff_variant == XCOFF32 ? 2 : 4, flags);
		break;
	case ECOFF:
		wr.WriteData(8, name);
		wr.WriteWord(8, physical_address);
		wr.WriteWord(8, address);
		wr.WriteWord(8, size);
		wr.WriteWord(8, section_pointer);
		wr.WriteWord(8, relocation_pointer);
		wr.WriteWord(8, line_number_pointer);
		wr.WriteWord(2, relocation_count);
		wr.WriteWord(2, line_number_count);
		wr.WriteWord(4, flags);
		break;
	case XCOFF64:
		wr.WriteData(8, name);
		wr.WriteWord(8, physical_address);
		wr.WriteWord(8, address);
		wr.WriteWord(8, size);
		wr.WriteWord(8, section_pointer);
		wr.WriteWord(8, relocation_pointer);
		wr.WriteWord(8, line_number_pointer);
		wr.WriteWord(4, relocation_count);
		wr.WriteWord(4, line_number_count);
		wr.WriteWord(4, flags);
		break;
	}
}

uint32_t COFFFormat::Section::ImageSize() const
{
	return size; //image->ImageSize();
}

COFFFormat::OptionalHeader::~OptionalHeader()
{
}

void COFFFormat::OptionalHeader::PostReadFile(COFFFormat& coff, Linker::Reader& rd)
{
}

void COFFFormat::OptionalHeader::PostWriteFile(const COFFFormat& coff, Linker::Writer& wr) const
{
}

void COFFFormat::OptionalHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
}

uint32_t COFFFormat::UnknownOptionalHeader::GetSize() const
{
	return buffer->ImageSize();
}

void COFFFormat::UnknownOptionalHeader::ReadFile(Linker::Reader& rd)
{
	buffer->ReadFile(rd);
}

void COFFFormat::UnknownOptionalHeader::WriteFile(Linker::Writer& wr) const
{
	buffer->WriteFile(wr);
}

offset_t COFFFormat::UnknownOptionalHeader::CalculateValues(COFFFormat& coff)
{
	return 0;
}

void COFFFormat::UnknownOptionalHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
	/* TODO */
}

uint32_t COFFFormat::AOutHeader::GetSize() const
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

void COFFFormat::AOutHeader::WriteFile(Linker::Writer& wr) const
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

offset_t COFFFormat::AOutHeader::CalculateValues(COFFFormat& coff)
{
	/* Note: for PE, these would be added up for all code/data/bss segments, but here we assume that one of each is available */
	code_size = coff.GetCodeSegment() ? coff.GetCodeSegment()->data_size : 0; /* not needed for DJGPP */
	data_size = coff.GetDataSegment() ? coff.GetDataSegment()->data_size : 0; /* not needed for DJGPP */
	bss_size  = coff.GetBssSegment() ? coff.GetBssSegment()->zero_fill : 0; /* not needed for DJGPP */
	entry_address = coff.entry_address;
	code_address = coff.GetCodeSegment() ? coff.GetCodeSegment()->base_address : 0;
	data_address = coff.GetDataSegment() ? coff.GetDataSegment()->base_address : 0;
	return 0;
}

void COFFFormat::AOutHeader::DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const
{
	std::map<offset_t, std::string> magic_choice;
	magic_choice[ZMAGIC] = "ZMAGIC";
	header_region.AddField("File type", Dumper::ChoiceDisplay::Make(magic_choice, Dumper::HexDisplay::Make(4)), offset_t(magic));
	header_region.AddOptionalField("Version stamp", Dumper::HexDisplay::Make(), offset_t(version_stamp));
	header_region.AddField("Text size", Dumper::HexDisplay::Make(), offset_t(code_size));
	header_region.AddField("Data size", Dumper::HexDisplay::Make(), offset_t(data_size));
	header_region.AddField("Bss size",  Dumper::HexDisplay::Make(), offset_t(bss_size));
	std::string entry;
	switch(coff.cpu_type)
	{
	case CPU_I386:
		header_region.AddField("Entry address (EIP)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_M68K:
	case CPU_Z8K:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_Z80:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(4), offset_t(entry_address));
		break;
	case CPU_W65:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(6), offset_t(entry_address));
		break;
	default:
		header_region.AddField("Entry address", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	}
	header_region.AddField("Text address", Dumper::HexDisplay::Make(), offset_t(code_address));
	header_region.AddField("Data address", Dumper::HexDisplay::Make(), offset_t(data_address));
}

void COFFFormat::AOutHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
	Dumper::Region header_region("Optional header", coff.file_offset + 20, GetSize(), 8);
	DumpFields(coff, dump, header_region);
	header_region.Display(dump);
}

uint32_t COFFFormat::FlexOSAOutHeader::GetSize() const
{
	return 36;
}

void COFFFormat::FlexOSAOutHeader::ReadFile(Linker::Reader& rd)
{
	AOutHeader::ReadFile(rd);
	relocations_offset = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);
}

void COFFFormat::FlexOSAOutHeader::WriteFile(Linker::Writer& wr) const
{
	AOutHeader::WriteFile(wr);
	wr.WriteWord(4, relocations_offset);
	wr.WriteWord(4, stack_size);
}

offset_t COFFFormat::FlexOSAOutHeader::CalculateValues(COFFFormat& coff)
{
	AOutHeader::CalculateValues(coff);
	relocations_offset = coff.relocations_offset;
	stack_size = coff.stack->zero_fill;
	return DigitalResearch::CPM68KFormat::CDOS68K_MeasureRelocations(coff.relocations);;
}

void COFFFormat::FlexOSAOutHeader::PostReadFile(COFFFormat& coff, Linker::Reader& rd)
{
	/* TODO */
}

void COFFFormat::FlexOSAOutHeader::PostWriteFile(const COFFFormat& coff, Linker::Writer& wr) const
{
	DigitalResearch::CPM68KFormat::CDOS68K_WriteRelocations(wr, coff.relocations);
}

void COFFFormat::FlexOSAOutHeader::DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const
{
	AOutHeader::DumpFields(coff, dump, header_region);
	header_region.AddField("Data address", Dumper::HexDisplay::Make(), offset_t(data_address));
	/* TODO: move display to relocation region */
	header_region.AddField("Relocation offset", Dumper::HexDisplay::Make(), offset_t(relocations_offset));
	header_region.AddField("Stack size", Dumper::HexDisplay::Make(), offset_t(stack_size));
}

uint32_t COFFFormat::GNUAOutHeader::GetSize() const
{
	return 32;
}

void COFFFormat::GNUAOutHeader::ReadFile(Linker::Reader& wr)
{
	/* TODO */
}

void COFFFormat::GNUAOutHeader::WriteFile(Linker::Writer& wr) const
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

offset_t COFFFormat::GNUAOutHeader::CalculateValues(COFFFormat& coff)
{
	return 0;
}

void COFFFormat::GNUAOutHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
	/* TODO: untested */
	Dumper::Region header_region("Optional header", coff.file_offset + 20, GetSize(), 8);
	header_region.AddField("Info", Dumper::HexDisplay::Make(), offset_t(info)); // TODO: improve display?
	header_region.AddField("Text size", Dumper::HexDisplay::Make(), offset_t(code_size));
	header_region.AddField("Data size", Dumper::HexDisplay::Make(), offset_t(data_size));
	header_region.AddField("Bss size",  Dumper::HexDisplay::Make(), offset_t(bss_size));
	header_region.AddField("Symbol table size",  Dumper::HexDisplay::Make(), offset_t(symbol_table_size));
	std::string entry;
	switch(coff.cpu_type)
	{
	case CPU_I386:
		header_region.AddField("Entry address (EIP)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_M68K:
	case CPU_Z8K:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_Z80:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(4), offset_t(entry_address));
		break;
	case CPU_W65:
		header_region.AddField("Entry address (PC)", Dumper::HexDisplay::Make(6), offset_t(entry_address));
		break;
	default:
		header_region.AddField("Entry address", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	}
	header_region.AddField("Text relocation size", Dumper::HexDisplay::Make(), offset_t(code_relocation_size));
	header_region.AddField("Data relocation size", Dumper::HexDisplay::Make(), offset_t(data_relocation_size));
	header_region.Display(dump);
}

uint32_t COFFFormat::MIPSAOutHeader::GetSize() const
{
	return 56;
}

void COFFFormat::MIPSAOutHeader::ReadFile(Linker::Reader& rd)
{
	AOutHeader::ReadFile(rd);
	bss_address = rd.ReadUnsigned(4);
	gpr_mask = rd.ReadUnsigned(4);
	cpr_mask[0] = rd.ReadUnsigned(4);
	cpr_mask[1] = rd.ReadUnsigned(4);
	cpr_mask[2] = rd.ReadUnsigned(4);
	cpr_mask[3] = rd.ReadUnsigned(4);
	gp_value = rd.ReadUnsigned(4);
}

void COFFFormat::MIPSAOutHeader::WriteFile(Linker::Writer& wr) const
{
	AOutHeader::WriteFile(wr);
	wr.WriteWord(4, bss_address);
	wr.WriteWord(4, gpr_mask);
	wr.WriteWord(4, cpr_mask[0]);
	wr.WriteWord(4, cpr_mask[1]);
	wr.WriteWord(4, cpr_mask[2]);
	wr.WriteWord(4, cpr_mask[3]);
}

offset_t COFFFormat::MIPSAOutHeader::CalculateValues(COFFFormat& coff)
{
	return AOutHeader::CalculateValues(coff);
}

void COFFFormat::MIPSAOutHeader::DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const
{
	/* TODO: untested */
	AOutHeader::DumpFields(coff, dump, header_region);
	header_region.AddField("Bss address", Dumper::HexDisplay::Make(), offset_t(bss_address));
	header_region.AddField("GPR mask", Dumper::HexDisplay::Make(), offset_t(gpr_mask));
	header_region.AddField("CPR #1 mask", Dumper::HexDisplay::Make(), offset_t(cpr_mask[0]));
	header_region.AddField("CPR #2 mask", Dumper::HexDisplay::Make(), offset_t(cpr_mask[1]));
	header_region.AddField("CPR #3 mask", Dumper::HexDisplay::Make(), offset_t(cpr_mask[2]));
	header_region.AddField("CPR #4 mask", Dumper::HexDisplay::Make(), offset_t(cpr_mask[3]));
	header_region.AddField("GP regiser value", Dumper::HexDisplay::Make(), offset_t(gp_value));
}

uint32_t COFFFormat::ECOFFAOutHeader::GetSize() const
{
	return 80;
}

void COFFFormat::ECOFFAOutHeader::ReadFile(Linker::Reader& rd)
{
	magic = rd.ReadUnsigned(2);
	version_stamp = rd.ReadUnsigned(2);
	build_revision = rd.ReadUnsigned(2);
	rd.Skip(2);
	code_size = rd.ReadUnsigned(8);
	data_size = rd.ReadUnsigned(8);
	bss_size = rd.ReadUnsigned(8);
	entry_address = rd.ReadUnsigned(8);
	code_address = rd.ReadUnsigned(8);
	data_address = rd.ReadUnsigned(8);
	bss_address = rd.ReadUnsigned(8);
	gpr_mask = rd.ReadUnsigned(4);
	fpr_mask = rd.ReadUnsigned(4);
	global_pointer = rd.ReadUnsigned(8);
}

void COFFFormat::ECOFFAOutHeader::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, magic);
	wr.WriteWord(2, version_stamp);
	wr.WriteWord(2, build_revision);
	wr.Skip(2);
	wr.WriteWord(8, code_size);
	wr.WriteWord(8, data_size);
	wr.WriteWord(8, bss_size);
	wr.WriteWord(8, entry_address);
	wr.WriteWord(8, code_address);
	wr.WriteWord(8, data_address);
	wr.WriteWord(8, bss_address);
	wr.WriteWord(4, gpr_mask);
	wr.WriteWord(4, fpr_mask);
	wr.WriteWord(8, global_pointer);
}

offset_t COFFFormat::ECOFFAOutHeader::CalculateValues(COFFFormat& coff)
{
	return 0;
}

void COFFFormat::ECOFFAOutHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
	// TODO
}

uint32_t COFFFormat::XCOFFAOutHeader::GetSize() const
{
	return is64 ? 110 : 72;
}

void COFFFormat::XCOFFAOutHeader::ReadFile(Linker::Reader& rd)
{
	magic = rd.ReadUnsigned(2);
	version_stamp = rd.ReadUnsigned(2);
	if(is64)
	{
		debugger_data = rd.ReadUnsigned(4);
		code_address = rd.ReadUnsigned(8);
		data_address = rd.ReadUnsigned(8);
		toc_address = rd.ReadUnsigned(8);
	}
	else
	{
		code_size = rd.ReadUnsigned(4);
		data_size = rd.ReadUnsigned(4);
		bss_size = rd.ReadUnsigned(4);
		entry_address = rd.ReadUnsigned(4);
		code_address = rd.ReadUnsigned(4);
		data_address = rd.ReadUnsigned(4);
		toc_address = rd.ReadUnsigned(4);
	}
	entry_section = rd.ReadUnsigned(2);
	code_section = rd.ReadUnsigned(2);
	data_section = rd.ReadUnsigned(2);
	toc_section = rd.ReadUnsigned(2);
	loader_section = rd.ReadUnsigned(2);
	bss_section = rd.ReadUnsigned(2);
	code_align = rd.ReadUnsigned(2);
	data_align = rd.ReadUnsigned(2);
	module_type = rd.ReadUnsigned(2);
	cpu_flags = rd.ReadUnsigned(1);
	cpu_type = rd.ReadUnsigned(1);
	if(!is64)
	{
		maximum_stack_size = rd.ReadUnsigned(4);
		maximum_data_size = rd.ReadUnsigned(4);
		debugger_data = rd.ReadUnsigned(4);
	}
	code_page_size = rd.ReadUnsigned(1);
	text_page_size = rd.ReadUnsigned(1);
	stack_page_size = rd.ReadUnsigned(1);
	flags = rd.ReadUnsigned(1);
	if(is64)
	{
		code_size = rd.ReadUnsigned(8);
		data_size = rd.ReadUnsigned(8);
		bss_size = rd.ReadUnsigned(8);
		entry_address = rd.ReadUnsigned(8);
		maximum_stack_size = rd.ReadUnsigned(8);
		maximum_data_size = rd.ReadUnsigned(8);
	}
	tdata_section = rd.ReadUnsigned(2);
	tbss_section = rd.ReadUnsigned(2);
	if(is64)
	{
		xcoff64_flags = rd.ReadUnsigned(2);
#if 0
		// TODO: this seems to make the header too long, is it 111 bytes long?
		shared_memory_page = rd.ReadUnsigned(1);
#endif
	}
}

void COFFFormat::XCOFFAOutHeader::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, magic);
	wr.WriteWord(2, version_stamp);
	if(is64)
	{
		wr.WriteWord(4, debugger_data);
		wr.WriteWord(8, code_address);
		wr.WriteWord(8, data_address);
		wr.WriteWord(8, toc_address);
	}
	else
	{
		wr.WriteWord(4, code_size);
		wr.WriteWord(4, data_size);
		wr.WriteWord(4, bss_size);
		wr.WriteWord(4, entry_address);
		wr.WriteWord(4, code_address);
		wr.WriteWord(4, data_address);
		wr.WriteWord(4, toc_address);
	}
	wr.WriteWord(2, entry_section);
	wr.WriteWord(2, code_section);
	wr.WriteWord(2, data_section);
	wr.WriteWord(2, toc_section);
	wr.WriteWord(2, loader_section);
	wr.WriteWord(2, bss_section);
	wr.WriteWord(2, code_align);
	wr.WriteWord(2, data_align);
	wr.WriteWord(2, module_type);
	wr.WriteWord(1, cpu_flags);
	wr.WriteWord(1, cpu_type);
	if(!is64)
	{
		wr.WriteWord(4, maximum_stack_size);
		wr.WriteWord(4, maximum_data_size);
		wr.WriteWord(4, debugger_data);
	}
	wr.WriteWord(1, code_page_size);
	wr.WriteWord(1, text_page_size);
	wr.WriteWord(1, stack_page_size);
	wr.WriteWord(1, flags);
	if(is64)
	{
		wr.WriteWord(8, code_size);
		wr.WriteWord(8, data_size);
		wr.WriteWord(8, bss_size);
		wr.WriteWord(8, entry_address);
		wr.WriteWord(8, maximum_stack_size);
		wr.WriteWord(8, maximum_data_size);
	}
	wr.WriteWord(2, tdata_section);
	wr.WriteWord(2, tbss_section);
	if(is64)
	{
		wr.WriteWord(2, xcoff64_flags);
#if 0
		// TODO: this seems to make the header too long, is it 111 bytes long?
		wr.WriteWord(1, shared_memory_page);
#endif
	}
}

offset_t COFFFormat::XCOFFAOutHeader::CalculateValues(COFFFormat& coff)
{
	return 0;
}

void COFFFormat::XCOFFAOutHeader::Dump(const COFFFormat& coff, Dumper::Dumper& dump) const
{
	// TODO
}

void COFFFormat::Clear()
{
	/* member fields */
	sections.clear();
	optional_header = nullptr;
	relocations.clear();
	symbols.clear();
	/* writer fields */
	stack = nullptr;
}

void COFFFormat::AssignMagicValue()
{
	::WriteWord(2, 2, reinterpret_cast<uint8_t *>(signature), cpu_type, endiantype);
	Linker::Debug << "Debug: COFF MAGIC VALUE " << int(signature[0]) << ", " << int(signature[1]) << std::endl;
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
		Linker::FatalError("Fatal error: Unrecognized magic number, unable to determine endianness");
	}
	Linker::Error << "Error: Unrecognized magic number, assuming " << (endiantype == BigEndian ? "big endian" : "little endian") << std::endl;
}

void COFFFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	ReadCOFFHeader(rd);
	ReadOptionalHeader(rd);
	ReadRestOfFile(rd);
	file_size = rd.Tell() - file_offset; // TODO: read might have finished inside the COFF image
}

void COFFFormat::ReadCOFFHeader(Linker::Reader& rd)
{
	rd.ReadData(2, signature);
	DetectCpuType();
	rd.endiantype = endiantype;

	if(coff_variant == COFFVariantType(0))
	{
		coff_variant = COFF;
		// TODO: determine coff_variant
#if 0
		switch(uint8_t(signature[0]) | (uint8_t(signature[1]) << 8))
		{
		case 0x0183:
		case 0x0188:
		case 0x018F:
			coff_variant = ECOFF;
			break;
		case 0xDF01:
			coff_variant = XCOFF32;
			break;
		case 0xF701:
			coff_variant = XCOFF64;
			break;
		}
#endif
	}

	switch(coff_variant)
	{
	case PECOFF:
		rd.endiantype = ::LittleEndian;

	case COFF:
	case XCOFF32:
		section_count = rd.ReadUnsigned(2);
		timestamp = rd.ReadUnsigned(4);
		symbol_table_offset = rd.ReadUnsigned(4);
		symbol_count = rd.ReadUnsigned(4);
		optional_header_size = rd.ReadUnsigned(2);
		flags = rd.ReadUnsigned(2);
		break;

	case ECOFF:
		section_count = rd.ReadUnsigned(2);
		timestamp = rd.ReadUnsigned(4);
		symbol_table_offset = rd.ReadUnsigned(8); // extended
		symbol_count = rd.ReadUnsigned(4);
		optional_header_size = rd.ReadUnsigned(2);
		flags = rd.ReadUnsigned(2);
		break;

	case XCOFF64:
		section_count = rd.ReadUnsigned(2);
		timestamp = rd.ReadUnsigned(4);
		symbol_table_offset = rd.ReadUnsigned(8); // extended
		optional_header_size = rd.ReadUnsigned(2);
		flags = rd.ReadUnsigned(2);
		symbol_count = rd.ReadUnsigned(4); // moved
		break;
	}
}

void COFFFormat::ReadOptionalHeader(Linker::Reader& rd)
{
	switch(optional_header_size)
	{
	case 28:
		/* a.out header */
		optional_header = std::make_unique<AOutHeader>();
		break;
	case 32:
		/* GNU a.out header */
		if(cpu_type == CPU_I386)
		{
			optional_header = std::make_unique<GNUAOutHeader>();
		}
		break;
	case 36:
		/* CDOS68K */
		if(cpu_type == CPU_M68K || cpu_type == CPU_I386)
		{
			optional_header = std::make_unique<FlexOSAOutHeader>();
			break;
		}
		break;
	case 56:
		/* MIPS header */
		if(cpu_type == CPU_MIPS)
		{
			optional_header = std::make_unique<MIPSAOutHeader>();
			break;
		}
		break;
	case 80:
		/* ECOFF header */
		if(cpu_type == CPU_ALPHA && coff_variant == ECOFF)
		{
			optional_header = std::make_unique<ECOFFAOutHeader>();
			break;
		}
		break;
	case 72:
		/* XCOFF32 header */
		if(cpu_type == CPU_PPC && (coff_variant == COFF || coff_variant == XCOFF32))
		{
			coff_variant = XCOFF32;
			optional_header = std::make_unique<XCOFFAOutHeader>(false);
			break;
		}
		break;
	case 110:
		/* XCOFF64 header */
		if(cpu_type == CPU_PPC64 && coff_variant == XCOFF64)
		{
			optional_header = std::make_unique<XCOFFAOutHeader>(true);
			break;
		}
		break;
	}
	if(optional_header == nullptr && optional_header_size != 0)
	{
		optional_header = std::make_unique<UnknownOptionalHeader>(optional_header_size);
	}

//	offset_t optional_header_offset = rd.Tell();
	if(optional_header)
	{
		optional_header->ReadFile(rd);
	}
//	/* if not recognized, skip optional header */
//	rd.Seek(optional_header_offset + optional_header_size);
}

void COFFFormat::ReadRestOfFile(Linker::Reader& rd)
{
	for(size_t i = 0; i < section_count; i++)
	{
		std::unique_ptr<Section> section = std::make_unique<Section>();
		section->ReadSectionHeader(rd, coff_variant);
		sections.push_back(std::move(section));
	}

	for(auto& section : sections)
	{
		if(section->flags & (Section::TEXT | Section::DATA))
		{
			rd.Seek(file_offset + section->section_pointer);
			std::dynamic_pointer_cast<Linker::Buffer>(section->image)->ReadFile(rd, section->ImageSize());
		}
	}

	switch(cpu_type)
	{
	case CPU_Z80:
	case CPU_Z8K:
	case CPU_W65:
		for(auto& section : sections)
		{
			if(section->relocation_count > 0)
			{
				rd.Seek(file_offset + section->relocation_pointer);
				for(size_t i = 0; i < section->relocation_count; i++)
				{
					std::unique_ptr<ZilogRelocation> rel = std::make_unique<ZilogRelocation>(cpu_type);
					rel->Read(rd);
					section->relocations.push_back(std::move(rel));
				}
			}
		}
		break;
	default:
		for(auto& section : sections)
		{
			if(section->relocation_count > 0)
			{
				rd.Seek(file_offset + section->relocation_pointer);
				for(size_t i = 0; i < section->relocation_count; i++)
				{
					std::unique_ptr<UNIXRelocation> rel = std::make_unique<UNIXRelocation>(coff_variant, cpu_type);
					rel->Read(rd);
					section->relocations.push_back(std::move(rel));
				}
			}
		}
		break;
	}

	if(symbol_count > 0)
	{
		rd.Seek(file_offset + symbol_table_offset);
		while(symbols.size() < symbol_count)
		{
			std::unique_ptr<Symbol> symbol = std::make_unique<Symbol>();
			symbol->Read(rd);
			symbols.push_back(std::move(symbol));
			symbols.resize(symbols.size() + symbols.back()->auxiliary_count);
		}
		size_t string_table_offset = rd.Tell();
		for(auto& symbol : symbols)
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

offset_t COFFFormat::WriteFile(Linker::Writer& wr) const
{
	if(type == DJGPP && stub_file != "")
	{
		const_cast<COFFFormat *>(this)->WriteStubImage(wr);
	}

	wr.endiantype = endiantype;
	WriteFileContents(wr);
	return offset_t(-1);
}

offset_t COFFFormat::ImageSize() const
{
	return file_size;
}

offset_t COFFFormat::WriteFileContents(Linker::Writer& wr) const
{
	/* File Header */
	switch(coff_variant)
	{
	case PECOFF:
	case COFF:
	case XCOFF32:
		wr.WriteData(2, signature);
		wr.WriteWord(2, section_count);
		wr.WriteWord(4, timestamp);
		wr.WriteWord(4, symbol_table_offset);
		wr.WriteWord(4, symbol_count);
		wr.WriteWord(2, optional_header_size);
		wr.WriteWord(2, flags);
		break;

	case ECOFF:
		wr.WriteData(2, signature);
		wr.WriteWord(2, section_count);
		wr.WriteWord(4, timestamp);
		wr.WriteWord(8, symbol_table_offset);
		wr.WriteWord(4, symbol_count);
		wr.WriteWord(2, optional_header_size);
		wr.WriteWord(2, flags);
		break;

	case XCOFF64:
		wr.WriteData(2, signature);
		wr.WriteWord(2, section_count);
		wr.WriteWord(4, timestamp);
		wr.WriteWord(8, symbol_table_offset);
		wr.WriteWord(2, optional_header_size);
		wr.WriteWord(2, flags);
		wr.WriteWord(4, symbol_count);
		break;
	}

	/* Optional Header */
	if(optional_header)
	{
		optional_header->WriteFile(wr);
	}

	/* Section Header */
	for(auto& section : sections)
	{
		section->WriteSectionHeader(wr, coff_variant);
	}

	/* Section Data */
	for(auto& section : sections)
	{
		section->image->WriteFile(wr);
	}

	/* TODO: store COFF relocations, symbols */

	if(optional_header)
	{
		optional_header->PostWriteFile(*this, wr);
	}

	return file_size;
}

void COFFFormat::Dump(Dumper::Dumper& dump) const
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
	cpu_descriptions[CPU_W65]   = "WDC 6502/65C816";

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

	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.AddField("CPU type", Dumper::ChoiceDisplay::Make(cpu_descriptions, Dumper::HexDisplay::Make(4)), offset_t(cpu_type));

	std::map<offset_t, std::string> endian_descriptions;
	endian_descriptions[::LittleEndian] = "little endian";
	endian_descriptions[::BigEndian] = "big endian";
	file_region.AddField("Byte order", Dumper::ChoiceDisplay::Make(endian_descriptions), offset_t(endiantype));

	file_region.Display(dump);

	Dumper::Region header_region("File header", file_offset, 20, 8);
	header_region.AddField("Signature", Dumper::HexDisplay::Make(4), offset_t(ReadUnsigned(2, 2, reinterpret_cast<const uint8_t *>(signature), endiantype)));
	header_region.AddOptionalField("Time stamp", Dumper::HexDisplay::Make(), offset_t(timestamp));
	header_region.AddOptionalField("Flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("no relocations"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("executable"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("no line numbers"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("no symbols"), true)
			->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("32-bit little endian"), true)
			->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("32-bit big endian"), true),
		offset_t(flags));
	header_region.Display(dump);

	if(optional_header)
	{
		optional_header->Dump(*this, dump);
	}

	for(auto& section : sections)
	{
		Dumper::Block block("Section", file_offset + section->section_pointer, section->image->AsImage(), section->address, 8);
		block.InsertField(0, "Name", Dumper::StringDisplay::Make(), section->name);
		if((section->image != nullptr ? section->image->ImageSize() : 0) != section->size)
			block.AddField("Size in memory", Dumper::HexDisplay::Make(), offset_t(section->size));
		block.AddField("Physical address", Dumper::HexDisplay::Make(), offset_t(section->physical_address));
		block.AddOptionalField("Line numbers", Dumper::HexDisplay::Make(), offset_t(section->line_number_pointer)); /* TODO */
		block.AddOptionalField("Line numbers count", Dumper::DecDisplay::Make(), offset_t(section->line_number_count)); /* TODO */
		block.AddOptionalField("Flags",
			Dumper::BitFieldDisplay::Make()
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("text"), true)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("data"), true)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("bss"), true),
			offset_t(section->flags));

		Dumper::Region relocations("Section relocation", file_offset + section->relocation_pointer, 0, 8); /* TODO: size */
		block.AddOptionalField("Count", Dumper::DecDisplay::Make(), offset_t(section->relocation_count));
		relocations.Display(dump);

		unsigned i = 0;
		for(auto& relocation : section->relocations)
		{
			Dumper::Entry relocation_entry("Relocation", i + 1, offset_t(-1) /* TODO: offset */, 8);
			relocation->FillEntry(relocation_entry);
			// TODO: fill addend
			relocation_entry.Display(dump);

			block.AddSignal(relocation->GetAddress() - section->address, relocation->GetSize());
			i++;
		}

		block.Display(dump);
	}

	Dumper::Region symbol_table("Symbol table", file_offset + symbol_table_offset, symbol_count * 18, 8);
	symbol_table.AddField("Count", Dumper::DecDisplay::Make(), offset_t(symbol_count));
	symbol_table.Display(dump);
	unsigned i = 0;
	for(auto& symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i + 1, file_offset + symbol_table_offset + 18 * i, 8);
		if(symbol)
		{
			/* otherwise, ignored symbol */
			symbol_entry.AddField("Name", Dumper::StringDisplay::Make(), symbol->name);
			symbol_entry.AddOptionalField("Name index", Dumper::HexDisplay::Make(), offset_t(symbol->name_index));
			symbol_entry.AddField("Value", Dumper::HexDisplay::Make(), offset_t(symbol->value));
			symbol_entry.AddField("Section", Dumper::HexDisplay::Make(), offset_t(symbol->section_number));
			symbol_entry.AddField("Type", Dumper::HexDisplay::Make(4), offset_t(symbol->type));
			symbol_entry.AddField("Storage class", Dumper::HexDisplay::Make(2), offset_t(symbol->storage_class));
			symbol_entry.AddOptionalField("Auxiliary count", Dumper::DecDisplay::Make(), offset_t(symbol->auxiliary_count));
		}
		symbol_entry.Display(dump);
		i += 1;
	}
}

/* * * Reader members * * */

void COFFFormat::SetupOptions(std::shared_ptr<Linker::OutputFormat> format)
{
	option_segmentation = format->FormatSupportsSegmentation();
}

bool COFFFormat::FormatRequiresDataStreamFix() const
{
	return cpu_type == CPU_W65; // the w65 assembler generates faulty binary
}

void COFFFormat::GenerateModule(Linker::Module& module) const
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
	case CPU_W65:
		module.cpu = Linker::Module::MOS6502; // or W65K
		break;
	default:
		/* unknown CPU */
		break;
	}

	std::vector<std::shared_ptr<Linker::Section>> linker_sections;
	std::map<std::string, offset_t> linker_section_addresses;
	for(auto& section : sections)
	{
		std::shared_ptr<Linker::Section> linker_section = std::make_shared<Linker::Section>(section->name);
		linker_section->SetReadable(true);
		if(section->flags & Section::BSS)
		{
			linker_section->SetZeroFilled(true);
			linker_section->Expand(section->size);
		}
		else if(section->flags & (Section::TEXT | Section::DATA))
		{
			std::shared_ptr<Linker::Section> buffer = std::dynamic_pointer_cast<Linker::Section>(section->image);
			linker_section->Append(*buffer);
		}
		if(section->flags & Section::TEXT)
		{
			linker_section->SetExecutable(true);
		}
		else
		{
			linker_section->SetWritable(true);
		}
		module.AddSection(linker_section);
		linker_sections.push_back(linker_section);
		linker_section_addresses[section->name] = section->address;
	}

	for(auto& symbol : symbols)
	{
		if(symbol == nullptr)
			continue;

		offset_t displacement = 0;
		switch(symbol->storage_class)
		{
		case C_EXT:
			if(symbol->section_number != N_UNDEF)
			{
				if(cpu_type == CPU_W65)
					displacement = -sections[symbol->section_number - 1]->address; // symbols in W65 are shifted by the section base
				module.AddGlobalSymbol(symbol->name, Linker::Location(linker_sections[symbol->section_number - 1], symbol->value + displacement));
			}
			else if(symbol->value != 0)
			{
				module.AddCommonSymbol(Linker::SymbolDefinition::CreateCommon(symbol->name, "", symbol->value, 1 /* TODO: alignment */));
			}
			break;
		case C_STAT:
		case C_LABEL:
			if(symbol->section_number == N_ABS)
			{
				module.AddLocalSymbol(symbol->name, Linker::Location(symbol->value));
			}
			else if(symbol->section_number != N_UNDEF && symbol->section_number != N_DEBUG)
			{
				if(cpu_type == CPU_W65)
					displacement = -sections[symbol->section_number - 1]->address; // symbols in W65 are shifted by the section base
				module.AddLocalSymbol(symbol->name, Linker::Location(linker_sections[symbol->section_number - 1], symbol->value + displacement));
			}
			break;
		}
	}

	for(size_t i = 0; i < sections.size(); i++)
	{
		const std::unique_ptr<Section>& section = sections[i];
		for(auto& _rel : section->relocations)
		{
			switch(cpu_type)
			{
			case CPU_Z80:
				{
					ZilogRelocation& rel = *dynamic_cast<ZilogRelocation *>(_rel.get());

					Linker::Location rel_source = Linker::Location(linker_sections[i], rel.address);
					Symbol& sym_target = *symbols[rel.symbol_index];
					Linker::Target rel_target =
						sym_target.storage_class == C_STAT && sym_target.value == 0
						? Linker::Target(Linker::Location(linker_sections[sym_target.section_number - 1]))
						: Linker::Target(Linker::SymbolName(sym_target.name));
					Linker::Relocation obj_rel = Linker::Relocation::Empty();

					size_t rel_size = rel.GetSize();

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
					ZilogRelocation& rel = *dynamic_cast<ZilogRelocation *>(_rel.get());

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
			case CPU_W65:
				{
					ZilogRelocation& rel = *dynamic_cast<ZilogRelocation *>(_rel.get());

					Linker::Location rel_source = Linker::Location(linker_sections[i], rel.address - sections[i]->address);
					Symbol& sym_target = *symbols[rel.symbol_index];
					bool is_section = sym_target.storage_class == C_STAT && sym_target.value == 0;
					Linker::Target rel_target =
						is_section
						? Linker::Target(Linker::Location(linker_sections[sym_target.section_number - 1]))
						: Linker::Target(Linker::SymbolName(sym_target.name));
					Linker::Relocation obj_rel = Linker::Relocation::Empty();

					offset_t rel_addend = 0;
					if(!is_section)
					{
						auto it = linker_section_addresses.find(sym_target.name);
						if(it != linker_section_addresses.end())
						{
							// Section values are already added
							Linker::Debug << "Debug: " << sym_target.name << ", start: " << it->second << std::endl;
							rel_addend -= it->second;
						}
					}

					size_t rel_size = rel.GetSize();

					switch(rel.type)
					{
					case ZilogRelocation::R_W65_PCR8:
					case ZilogRelocation::R_W65_PCR16:
						obj_rel = Linker::Relocation::Relative(rel_size, rel_source, rel_target, 0, ::LittleEndian);
						break;
					case ZilogRelocation::R_W65_ABS8S8:
					case ZilogRelocation::R_W65_ABS16S8:
						obj_rel = Linker::Relocation::Absolute(rel_size, rel_source, rel_target, rel_addend, ::LittleEndian);
						obj_rel.SetShift(8);
						break;
					case ZilogRelocation::R_W65_ABS8S16:
					case ZilogRelocation::R_W65_ABS16S16:
						obj_rel = Linker::Relocation::Absolute(rel_size, rel_source, rel_target, rel_addend, ::LittleEndian);
						obj_rel.SetShift(16);
					default:
						obj_rel = Linker::Relocation::Absolute(rel_size, rel_source, rel_target, rel_addend, ::LittleEndian);
						break;
					}
					obj_rel.AddCurrentValue();
					module.relocations.push_back(obj_rel);
					Linker::Debug << "Debug: COFF relocation " << obj_rel << std::endl;
					Linker::Debug << "Debug: value at location: " << obj_rel.addend - rel_addend << std::endl;
				}
				break;
			default:
				/* unknown CPU */
				break;
			}
		}
	}

	/* release section data */
	for(auto& section : sections)
	{
		section->image = nullptr;
	}
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

std::shared_ptr<COFFFormat> COFFFormat::CreateWriter(format_type type)
{
	return std::make_shared<COFFFormat>(type);
}

void COFFFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub_file = FetchOption(options, "stub", "");

	/* TODO */
	option_no_relocation = false;
}

void COFFFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
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
		sections.push_back(std::make_unique<Section>(Section::TEXT, segment));
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
		sections.push_back(std::make_unique<Section>(Section::DATA, segment));
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
		sections.push_back(std::make_unique<Section>(Section::BSS, segment));
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
		sections.push_back(std::make_unique<Section>(Section::TEXT, std::make_shared<Linker::Segment>(".code")));
	}
	if(GetDataSegment() == nullptr)
	{
		sections.push_back(std::make_unique<Section>(Section::DATA, std::make_shared<Linker::Segment>(".data")));
	}
	if(GetBssSegment() == nullptr)
	{
		sections.push_back(std::make_unique<Section>(Section::BSS, std::make_shared<Linker::Segment>(".bss")));
	}
	if((type == CDOS68K || type == CDOS386) && stack == nullptr)
	{
		stack = std::make_shared<Linker::Segment>(".stack");
	}
}

std::unique_ptr<Script::List> COFFFormat::GetScript(Linker::Module& module)
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
		return SegmentManager::GetScript(module);
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
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

/** @brief Return the segment stored inside the section, note that this only works for binary generation */
std::shared_ptr<Linker::Segment> COFFFormat::GetSegment(std::unique_ptr<Section>& section)
{
	return std::dynamic_pointer_cast<Linker::Segment>(section->image);
}

std::shared_ptr<Linker::Segment> COFFFormat::GetCodeSegment()
{
	for(auto& section : sections)
	{
		if((section->flags & Section::TEXT))
			return GetSegment(section);
	}
	return nullptr;
}

std::shared_ptr<Linker::Segment> COFFFormat::GetDataSegment()
{
	for(auto& section : sections)
	{
		if((section->flags & Section::DATA))
			return GetSegment(section);
	}
	return nullptr;
}

std::shared_ptr<Linker::Segment> COFFFormat::GetBssSegment()
{
	for(auto& section : sections)
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
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
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
		Linker::FatalError("Internal error: no generation type specified, exiting");
	case DJGPP:
		cpu_type = CPU_I386;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_LITTLE_ENDIAN;
		optional_header = std::make_unique<AOutHeader>(ZMAGIC);
		break;
	case CDOS386:
		/* TODO */
		cpu_type = CPU_I386;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_LITTLE_ENDIAN;
		optional_header = std::make_unique<FlexOSAOutHeader>();
		break;
	case CDOS68K:
		cpu_type = CPU_M68K;
		flags = FLAG_NO_RELOCATIONS | FLAG_EXECUTABLE | FLAG_NO_LINE_NUMBERS | FLAG_NO_SYMBOLS | FLAG_32BIT_BIG_ENDIAN;
		optional_header = std::make_unique<FlexOSAOutHeader>();
		break;
	}
	endiantype = GetEndianType();

	section_count = sections.size();

	optional_header_size = optional_header->GetSize();

	offset_t offset = 0;

	switch(coff_variant)
	{
	case PECOFF:
	case COFF:
	case XCOFF32:
		offset = 20;
		break;

	case ECOFF:
	case XCOFF64:
		offset = 24;
		break;
	}

	offset += optional_header->GetSize() + sections.size() * 40;

	if(type == DJGPP)
	{
		GetStubImageSize();
	}
	for(auto& section : sections)
	{
		std::shared_ptr<Linker::Segment> image = GetSegment(section);
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

	offset += optional_header->CalculateValues(*this);
	if(file_size == offset_t(-1) || file_size < offset)
	{
		file_size = offset;
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

std::string COFFFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
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
		Linker::FatalError("Internal error: invalid target system");
	}
}

