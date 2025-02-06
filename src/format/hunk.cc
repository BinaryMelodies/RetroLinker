
#include "hunk.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace Amiga;

// Relocation

bool HunkFormat::Relocation::operator <(const Relocation& other) const
{
	return offset < other.offset || (offset == other.offset && (size < other.size || (size == other.size && type < other.type)));
}

// Block

std::shared_ptr<HunkFormat::Block> HunkFormat::Block::ReadBlock(Linker::Reader& rd, bool is_executable)
{
	offset_t current_offset = rd.Tell();
	uint32_t type = rd.ReadUnsigned(4);
	if(rd.Tell() < current_offset + 4)
		return nullptr;
	Linker::Debug << "Debug: read " << std::hex << type << std::endl;
	std::shared_ptr<Block> block = nullptr;
	switch(type)
	{
	case HUNK_UNIT:
	case HUNK_NAME:
		block = std::make_shared<TextBlock>(block_type(type));
		break;
	case HUNK_CODE:
	case HUNK_PPC_CODE:
	case HUNK_DATA:
		block = std::make_shared<LoadBlock>(block_type(type));
		break;
	case HUNK_BSS:
		block = std::make_shared<BssBlock>();
		break;
	case HUNK_ABSRELOC32:
	case HUNK_RELRELOC16:
	case HUNK_RELRELOC8:
	case HUNK_DRELOC32:
	case HUNK_DRELOC16:
	case HUNK_DRELOC8:
	case HUNK_RELOC32SHORT:
	case HUNK_RELRELOC32:
	case HUNK_ABSRELOC16:
	case HUNK_RELRELOC26:
		block = std::make_shared<RelocationBlock>(block_type(type), is_executable);
		break;
	case HUNK_END:
	case HUNK_BREAK:
		block = std::make_shared<Block>(block_type(type));
		break;
	case HUNK_HEADER:
		block = std::make_shared<HeaderBlock>();
		break;
	case HUNK_EXT:
	case HUNK_SYMBOL:
		block = std::make_shared<SymbolBlock>(block_type(type));
		break;
	case HUNK_DEBUG:
	case HUNK_OVERLAY:
	case HUNK_LIB:
	case HUNK_INDEX:
		// TODO
	default:
		Linker::Error << "Error: unrecognized block type " << std::hex << type << ", halting" << std::endl;
		break;
	}
	if(block != nullptr)
	{
		block->Read(rd);
	}
	return block;
}

void HunkFormat::Block::Read(Linker::Reader& rd)
{
}

void HunkFormat::Block::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
}

offset_t HunkFormat::Block::FileSize() const
{
	return 4;
}

void HunkFormat::Block::Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Dumper::Region block_region("Block", current_offset, FileSize(), 8);
	AddCommonFields(block_region, index);
	AddExtraFields(block_region, module, hunk, index, current_offset);
	block_region.Display(dump);
}

void HunkFormat::Block::AddCommonFields(Dumper::Region& region, unsigned index) const
{
	region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	std::map<offset_t, std::string> type_descriptions;
	type_descriptions[HUNK_UNIT] = "HUNK_UNIT";
	type_descriptions[HUNK_NAME] = "HUNK_NAME";
	type_descriptions[HUNK_CODE] = "HUNK_CODE";
	type_descriptions[HUNK_DATA] = "HUNK_DATA";
	type_descriptions[HUNK_BSS] = "HUNK_BSS";
	type_descriptions[HUNK_ABSRELOC32] = "HUNK_RELOC32/HUNK_ABSRELOC32";
	type_descriptions[HUNK_RELRELOC16] = "HUNK_RELOC16/HUNK_RELRELOC16";
	type_descriptions[HUNK_RELRELOC8] = "HUNK_RELOC8/HUNK_RELRELOC16";
	type_descriptions[HUNK_EXT] = "HUNK_EXT";
	type_descriptions[HUNK_SYMBOL] = "HUNK_SYMBOL";
	type_descriptions[HUNK_DEBUG] = "HUNK_DEBUG";
	type_descriptions[HUNK_END] = "HUNK_END";
	type_descriptions[HUNK_HEADER] = "HUNK_HEADER";
	type_descriptions[HUNK_OVERLAY] = "HUNK_OVERLAY";
	type_descriptions[HUNK_BREAK] = "HUNK_BREAK";
	type_descriptions[HUNK_DRELOC32] = is_executable ? "HUNK_RELOC32SHORT (V37)" : "HUNK_DRELOC32";
	type_descriptions[HUNK_DRELOC16] = "HUNK_DRELOC16";
	type_descriptions[HUNK_DRELOC8] = "HUNK_DRELOC8";
	type_descriptions[HUNK_LIB] = "HUNK_LIB";
	type_descriptions[HUNK_INDEX] = "HUNK_INDEX";
	type_descriptions[HUNK_RELOC32SHORT] = "HUNK_RELOC32SHORT";
	type_descriptions[HUNK_RELRELOC32] = "HUNK_RELRELOC32";
	type_descriptions[HUNK_ABSRELOC16] = "HUNK_ABSRELOC16";
	type_descriptions[HUNK_PPC_CODE] = "HUNK_PPC_CODE";
	type_descriptions[HUNK_RELRELOC26] = "HUNK_RELRELOC26";
	region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(type));
}

void HunkFormat::Block::AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
}

// TextBlock

void HunkFormat::TextBlock::Read(Linker::Reader& rd)
{
	name = HunkFormat::ReadString(rd);
}

void HunkFormat::TextBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	HunkFormat::WriteString(wr, name);
}

offset_t HunkFormat::TextBlock::FileSize() const
{
	return 4 + HunkFormat::MeasureString(name);
}

// HeaderBlock

void HunkFormat::HeaderBlock::Read(Linker::Reader& rd)
{
	while(true)
	{
		uint32_t longword_count;
		std::string name = HunkFormat::ReadString(rd, longword_count);
		if(longword_count == 0)
			break;
		library_names.push_back(name);
	}
	table_size = rd.ReadUnsigned(4);
	first_hunk = rd.ReadUnsigned(4);
	uint32_t last_hunk = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < last_hunk - first_hunk + 1; i++)
	{
		hunk_sizes.emplace_back(rd.ReadUnsigned(4));
	}
}

void HunkFormat::HeaderBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	for(auto& name : library_names)
	{
		HunkFormat::WriteString(wr, name);
	}
	wr.WriteWord(4, 0);
	wr.WriteWord(4, table_size);
	wr.WriteWord(4, first_hunk);
	wr.WriteWord(4, hunk_sizes.size() + first_hunk - 1);
	for(uint32_t size : hunk_sizes)
	{
		wr.WriteWord(4, size);
	}
}

offset_t HunkFormat::HeaderBlock::FileSize() const
{
	offset_t size = 20 + 4 * library_names.size() + 4 * hunk_sizes.size();
	for(auto& name : library_names)
	{
		size += HunkFormat::MeasureString(name);
	}
	return size;
}

void HunkFormat::HeaderBlock::Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	current_offset += 4;
	uint32_t i = 1;
	for(auto& name : library_names)
	{
		// TODO: untested
		Dumper::Entry library_entry("Library", i, current_offset);
		library_entry.AddField("Name", Dumper::StringDisplay::Make(), name);
		library_entry.Display(dump);
		current_offset += HunkFormat::MeasureString(name);
		i++;
	}
	current_offset += 4;
	for(i = 0; i < hunk_sizes.size(); i++)
	{
		Dumper::Entry hunk_size_entry("Hunk size", first_hunk + i, current_offset + i * 4);
		hunk_size_entry.AddField("Size", Dumper::HexDisplay::Make(8), offset_t(hunk_sizes[i]));
		hunk_size_entry.Display(dump);
	}
}

void HunkFormat::HeaderBlock::AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	region.AddField("Table size", Dumper::HexDisplay::Make(8), offset_t(table_size));
	region.AddField("First hunk", Dumper::HexDisplay::Make(8), offset_t(first_hunk));
	region.AddField("Last hunk", Dumper::HexDisplay::Make(8), offset_t(first_hunk + hunk_sizes.size() - 1));
}

// InitialHunkBlock

uint32_t HunkFormat::InitialHunkBlock::GetSizeField() const
{
	uint32_t size = GetSize();
	if(RequiresAdditionalFlags())
		return size | 0xC0000000;
	else
		return size | ((flags & ~LoadPublic) << 29);
}

bool HunkFormat::InitialHunkBlock::RequiresAdditionalFlags() const
{
	return loaded_with_additional_flags || (flags & ~(LoadChipMem | LoadFastMem | LoadPublic)) != 0;
}

uint32_t HunkFormat::InitialHunkBlock::GetAdditionalFlags() const
{
	return flags & ~LoadPublic;
}

void HunkFormat::InitialHunkBlock::Read(Linker::Reader& rd)
{
	uint32_t longword_count = rd.ReadUnsigned(4);
	if((longword_count & FlagMask) == BitAdditional)
	{
		loaded_with_additional_flags = true;
		flags = flag_type(rd.ReadUnsigned(4) | LoadPublic);
	}
	else
	{
		flags = flag_type(((longword_count & FlagMask) >> 29) | LoadPublic);
	}
	ReadBody(rd, longword_count & ~FlagMask);
}

void HunkFormat::InitialHunkBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	wr.WriteWord(4, GetSizeField());
	if(RequiresAdditionalFlags())
		wr.WriteWord(4, GetAdditionalFlags()); // TODO: this is not tested
	WriteBody(wr);
}

offset_t HunkFormat::InitialHunkBlock::FileSize() const
{
	return 8 + 4 * GetSize() + (RequiresAdditionalFlags() ? 4 : 0);
}

void HunkFormat::InitialHunkBlock::AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	if(RequiresAdditionalFlags())
	{
		region.AddOptionalField("Flags",
			Dumper::BitFieldDisplay::Make(8)
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("public"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("chip memory"), true)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("fast memory"), true)
				->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("local memory"), true) // TODO: meaning
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("24-bit DMA"), true) // TODO: meaning
				->AddBitField(16, 1, Dumper::ChoiceDisplay::Make("clear"), true), // TODO: meaning
			offset_t(flags) & FlagMask);
	}
	else
	{
		region.AddOptionalField("Flags",
			Dumper::BitFieldDisplay::Make(8)
				->AddBitField(30, 1, Dumper::ChoiceDisplay::Make("chip memory"), true)
				->AddBitField(31, 1, Dumper::ChoiceDisplay::Make("fast memory"), true),
			offset_t(flags) & FlagMask);
	}
}

void HunkFormat::InitialHunkBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
}

void HunkFormat::InitialHunkBlock::WriteBody(Linker::Writer& wr) const
{
}

// LoadBlock

void HunkFormat::LoadBlock::Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);

	Dumper::Block hunk_block("Data", current_offset + (RequiresAdditionalFlags() ? 12 : 8), image->AsImage(), 0, 8);
	offset_t header_hunk_size = module.GetHunkSizeInHeader(index);
	if(header_hunk_size > image->ImageSize())
	{
		hunk_block.AddOptionalField("Additional memory", Dumper::HexDisplay::Make(8), offset_t(header_hunk_size - image->ImageSize()));
	}
	for(auto pair : hunk->relocations)
	{
		for(auto rel : pair.second)
		{
			hunk_block.AddSignal(rel.offset, rel.size);
		}
	}
	hunk_block.Display(dump);
}

uint32_t HunkFormat::LoadBlock::GetSize() const
{
	return ::AlignTo(image->ImageSize(), 4) / 4;
}

void HunkFormat::LoadBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
	image = Linker::Buffer::ReadFromFile(rd, longword_count * 4);
}

void HunkFormat::LoadBlock::WriteBody(Linker::Writer& wr) const
{
	uint32_t full_size = GetSize() * 4;
	offset_t image_start = wr.Tell();
	image->WriteFile(wr);
	if(wr.Tell() != image_start + full_size)
		wr.Skip(image_start + full_size - wr.Tell());
}

// BssBlock

offset_t HunkFormat::BssBlock::FileSize() const
{
	return 8;
}

uint32_t HunkFormat::BssBlock::GetSize() const
{
	return size;
}

void HunkFormat::BssBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
	size = longword_count;
}

void HunkFormat::BssBlock::AddExtraFields(Dumper::Region& region, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	InitialHunkBlock::AddExtraFields(region, module, hunk, index, current_offset);
	region.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(size * 4));
}

// RelocationBlock

bool HunkFormat::RelocationBlock::IsShortRelocationBlock() const
{
	return type == HUNK_RELOC32SHORT || (is_executable && type == HUNK_V37_RELOC32SHORT);
}

size_t HunkFormat::RelocationBlock::GetRelocationSize() const
{
	switch(type)
	{
	default:
		assert(false);
	case HUNK_ABSRELOC32:
	case HUNK_DRELOC32:
	case HUNK_RELOC32SHORT:
	case HUNK_RELRELOC32:
	case HUNK_RELRELOC26:
		return 4;
	case HUNK_RELRELOC16:
	case HUNK_DRELOC16:
	case HUNK_ABSRELOC16:
		return 2;
	case HUNK_RELRELOC8:
	case HUNK_DRELOC8:
		return 1;
	}
}

HunkFormat::Relocation::relocation_type HunkFormat::RelocationBlock::GetRelocationType() const
{
	switch(type)
	{
	default:
		assert(false);
	case HUNK_ABSRELOC32:
	case HUNK_RELOC32SHORT:
	case HUNK_ABSRELOC16:
		return Relocation::Absolute;
	case HUNK_DRELOC32:
		return is_executable
			? Relocation::Absolute // V37 uses this type for RELOC32SHORT
			: Relocation::DataRelative;
	case HUNK_DRELOC16:
	case HUNK_DRELOC8:
		return Relocation::DataRelative;
	case HUNK_RELRELOC16:
	case HUNK_RELRELOC8:
	case HUNK_RELRELOC32:
		return Relocation::SelfRelative;
	case HUNK_RELRELOC26:
		return Relocation::SelfRelative26;
	}
}

void HunkFormat::RelocationBlock::Read(Linker::Reader& rd)
{
	relocations.clear();

	size_t wordread = IsShortRelocationBlock() ? 2 : 4;

	while(true)
	{
		uint32_t relocation_count = rd.ReadUnsigned(wordread);
		if(relocation_count == 0)
			break;

		relocations.emplace_back(RelocationData());
		relocations.back().hunk = rd.ReadUnsigned(4);
		for(uint32_t i = 0; i < relocation_count; i++)
		{
			relocations.back().offsets.push_back(rd.ReadUnsigned(wordread));
		}
	}

	if((rd.Tell() & 3) != 0)
	{
		rd.Skip(-rd.Tell() & 3);
	}
}

void HunkFormat::RelocationBlock::Write(Linker::Writer& wr) const
{
	size_t wordwrite = IsShortRelocationBlock() ? 2 : 4;

	wr.WriteWord(4, type);
	for(auto& data : relocations)
	{
		// to make sure to avoid accidentally closing the list prematurely
		if(data.offsets.size() == 0)
			continue;

		wr.WriteWord(wordwrite, data.offsets.size());
		wr.WriteWord(wordwrite, data.hunk);
		for(auto offset : data.offsets)
		{
			wr.WriteWord(wordwrite, offset);
		}
	}
	wr.WriteWord(wordwrite, 0); /* terminator */

	if((wr.Tell() & 3) != 0)
	{
		wr.Skip(-wr.Tell() & 3);
	}
}

offset_t HunkFormat::RelocationBlock::FileSize() const
{
	offset_t size = 8;
	for(auto& data : relocations)
	{
		// to make sure to avoid accidentally closing the list prematurely
		if(data.offsets.size() == 0)
			continue;

		size += 8 + 4 * data.offsets.size();
	}
	return size;
}

void HunkFormat::RelocationBlock::Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	unsigned i = 0;
	current_offset += 12;
	for(auto& data : relocations)
	{
		for(auto& offset : data.offsets)
		{
			Dumper::Entry relocation_entry("Relocation", i + 1, current_offset);
			relocation_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(offset));
			relocation_entry.AddField("Target hunk", Dumper::DecDisplay::Make(), offset_t(data.hunk)); // TODO: add first_hunk?
			relocation_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(GetRelocationSize()));
			std::map<offset_t, std::string> relocation_type_descriptions;
			relocation_type_descriptions[Relocation::Absolute] = "absolute";
			relocation_type_descriptions[Relocation::SelfRelative] = "PC-relative";
			relocation_type_descriptions[Relocation::DataRelative] = "data relative";
			relocation_type_descriptions[Relocation::SelfRelative26] = "26-bit PC-relative";
			relocation_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_descriptions), offset_t(GetRelocationType()));
			if(hunk->image != nullptr)
			{
				relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(2 * GetRelocationSize()), offset_t(
					hunk->image->AsImage()->ReadUnsigned(GetRelocationSize(), offset, ::BigEndian)));
			}
			relocation_entry.Display(dump);
			i += 1;
			current_offset += 4;
		}
		current_offset += 8;
	}
}

// SymbolBlock::Unit

std::unique_ptr<HunkFormat::SymbolBlock::Unit> HunkFormat::SymbolBlock::Unit::ReadData(Linker::Reader& rd)
{
	uint32_t length = rd.ReadUnsigned(4);
	if(length == 0)
		return nullptr;

	uint8_t type = length >> 24;
	length = 0x00FFFFFF;

	std::string name = HunkFormat::ReadString(length, rd);

	std::unique_ptr<HunkFormat::SymbolBlock::Unit> unit;
	switch(type)
	{
	case EXT_SYMB:
	case EXT_DEF:
	case EXT_ABS:
	case EXT_RES:
		unit = std::make_unique<Definition>(symbol_type(type), name);
		break;

	case EXT_ABSREF32:
	case EXT_RELREF16:
	case EXT_RELREF8:
	case EXT_RELREF26:
		unit = std::make_unique<References>(symbol_type(type), name);
		break;

	case EXT_COMMON:
		unit = std::make_unique<CommonReferences>(symbol_type(type), name);
		break;

	case EXT_DREF32:
	case EXT_DREF16:
	case EXT_DREF8:
		unit = std::make_unique<Unit>(symbol_type(type), name);
		break;

	default:
		Linker::FatalError("Fatal error: invalid symbol data unit type, unable to parse file");
	}
	unit->Read(rd);
	return unit;
}

void HunkFormat::SymbolBlock::Unit::Read(Linker::Reader& rd)
{
}

void HunkFormat::SymbolBlock::Unit::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, (type << 24) | ((::AlignTo(name.length(), 4) / 4) & 0x00FFFFFF));
	HunkFormat::WriteStringContents(wr, name);
}

offset_t HunkFormat::SymbolBlock::Unit::FileSize() const
{
	return 4 + ::AlignTo(name.length(), 4);
}

void HunkFormat::SymbolBlock::Unit::DumpContents(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
}

void HunkFormat::SymbolBlock::Unit::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
}

// SymbolBlock::Definition

void HunkFormat::SymbolBlock::Definition::Read(Linker::Reader& rd)
{
	value = rd.ReadUnsigned(4);
}

void HunkFormat::SymbolBlock::Definition::Write(Linker::Writer& wr) const
{
	Unit::Write(wr);
	wr.WriteWord(4, value);
}

offset_t HunkFormat::SymbolBlock::Definition::FileSize() const
{
	return Unit::FileSize() + 4;
}

void HunkFormat::SymbolBlock::Definition::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(value));
}

// SymbolBlock::References

void HunkFormat::SymbolBlock::References::Read(Linker::Reader& rd)
{
	uint32_t reference_count = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < reference_count; i++)
	{
		references.push_back(rd.ReadUnsigned(4));
	}
}

void HunkFormat::SymbolBlock::References::Write(Linker::Writer& wr) const
{
	Unit::Write(wr);
	wr.WriteWord(4, references.size());
	for(uint32_t reference : references)
	{
		wr.WriteWord(4, reference);
	}
}

offset_t HunkFormat::SymbolBlock::References::FileSize() const
{
	return Unit::FileSize() + 4 + 4 * references.size();
}

void HunkFormat::SymbolBlock::References::DumpContents(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	current_offset += Unit::FileSize() + 4;
	for(uint32_t reference : references)
	{
		Dumper::Entry reference_entry("Reference", current_offset);
		reference_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(reference));
		reference_entry.Display(dump);
		current_offset += 4;
	}
}

// SymbolBlock::CommonReferences

void HunkFormat::SymbolBlock::CommonReferences::Read(Linker::Reader& rd)
{
	size = rd.ReadUnsigned(4);
	uint32_t reference_count = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < reference_count; i++)
	{
		references.push_back(rd.ReadUnsigned(4));
	}
}

void HunkFormat::SymbolBlock::CommonReferences::Write(Linker::Writer& wr) const
{
	Unit::Write(wr);
	wr.WriteWord(4, size);
	wr.WriteWord(4, references.size());
	for(uint32_t reference : references)
	{
		wr.WriteWord(4, reference);
	}
}

offset_t HunkFormat::SymbolBlock::CommonReferences::FileSize() const
{
	return Unit::FileSize() + 8 + 4 * references.size();
}

void HunkFormat::SymbolBlock::CommonReferences::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(size));
}

// SymbolBlock

void HunkFormat::SymbolBlock::Read(Linker::Reader& rd)
{
	while(true)
	{
		std::unique_ptr<Unit> unit = Unit::ReadData(rd);
		if(unit == nullptr)
			break;
		symbols.emplace_back(std::move(unit));
	}
}

void HunkFormat::SymbolBlock::Write(Linker::Writer& wr) const
{
	for(auto& unit : symbols)
	{
		unit->Write(wr);
	}
	wr.WriteWord(4, 0);
}

offset_t HunkFormat::SymbolBlock::FileSize() const
{
	// type longword and terminating longword
	offset_t size = 8;
	for(auto& unit : symbols)
	{
		size += unit->FileSize();
	}
	return size;
}

void HunkFormat::SymbolBlock::Dump(Dumper::Dumper& dump, const HunkFormat& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	unsigned i = 0;
	for(auto& unit : symbols)
	{
		Dumper::Entry unit_entry("External", i, current_offset);
		std::map<offset_t, std::string> type_descriptions;
		type_descriptions[Unit::EXT_SYMB] = "EXT_SYMB - internal symbol definition";
		type_descriptions[Unit::EXT_DEF] = "EXT_DEF - relocatable symbol definition";
		type_descriptions[Unit::EXT_ABS] = "EXT_ABS - absolute symbol definition";
		type_descriptions[Unit::EXT_RES] = "EXT_RES - resident library symbol definition";
		type_descriptions[Unit::EXT_ABSREF32] = "EXT_ABSREF32/EXT_REF32 - 32-bit reference to symbol";
		type_descriptions[Unit::EXT_COMMON] = "EXT_COMMON - 32-bit reference to common symbol";
		type_descriptions[Unit::EXT_RELREF16] = "EXT_RELREF16/EXT_REF16 - 16-bit relative reference to symbol";
		type_descriptions[Unit::EXT_RELREF8] = "EXT_RELREF8/EXT_REF8 - 8-bit relative reference to symbol";
		type_descriptions[Unit::EXT_DREF32] = "EXT_DREF32 - 32-bit data relative reference to symbol";
		type_descriptions[Unit::EXT_DREF16] = "EXT_DREF16 - 16-bit data relative reference to symbol";
		type_descriptions[Unit::EXT_DREF8] = "EXT_DREF8 - 8-bit data relative reference to symbol";
		type_descriptions[Unit::EXT_RELREF26] = "EXT_RELREF26 - 26-bit relative reference to symbol";

		unit_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(unit->type));
		unit_entry.AddField("Name", Dumper::StringDisplay::Make(), unit->name);
		unit->AddExtraFields(dump, unit_entry, module, hunk, i, current_offset);
		unit_entry.Display(dump);

		unit->DumpContents(dump, module, hunk, i, current_offset);

		current_offset += unit->FileSize();
		i++;
	}
}

// Hunk

void HunkFormat::Hunk::ProduceBlocks()
{
	/* Initial hunk block */
	if(type != Hunk::Bss)
	{
		std::shared_ptr<LoadBlock> load = std::make_shared<LoadBlock>(Block::block_type(type), flags);
		load->image = image;
		blocks.push_back(load);
	}
	else
	{
		std::shared_ptr<BssBlock> bss = std::make_shared<BssBlock>((GetMemorySize() + 3) / 4, flags);
		blocks.push_back(bss);
	}

	if(name != "")
	{
		blocks.push_back(std::make_shared<TextBlock>(Block::HUNK_NAME, name));
	}

	if(relocations.size() != 0)
	{
		/* Relocation block */
		std::shared_ptr<RelocationBlock> relocation = std::make_shared<RelocationBlock>(Block::HUNK_ABSRELOC32);
		for(auto it : relocations)
		{
			relocation->relocations.emplace_back(RelocationBlock::RelocationData(it.first));
			for(Relocation rel : it.second)
			{
				if(rel.size == 4)
				{
					relocation->relocations.back().offsets.push_back(rel.offset);
				}
			}
		}
		blocks.push_back(relocation);
	}

	// TODO: external symbols
	// TODO: symbol table
	// TODO: debug

	/* End block */
	std::shared_ptr<Block> end = std::make_shared<Block>(Block::HUNK_END);
	blocks.push_back(end);
}

// Hunk

uint32_t HunkFormat::Hunk::GetMemorySize() const
{
	if(type != Hunk::Bss)
	{
		return image->ImageSize();
	}
	else
	{
		assert(image == nullptr || image->ImageSize() == 0);
		if(Linker::Segment * segment = dynamic_cast<Linker::Segment *>(image.get()))
		{
			return segment->TotalSize();
		}
		else
		{
			return image_size;
		}
	}
}

uint32_t HunkFormat::Hunk::GetFileSize() const
{
	offset_t size = 0;
	for(auto& block : blocks)
	{
//		Linker::Debug << "Debug: size of " << std::hex << block->type << " is " << block->FileSize() << std::endl;
		size += block->FileSize();
	}
	return size;
}

uint32_t HunkFormat::Hunk::GetSizeField()
{
	if(blocks.size() == 0)
	{
		ProduceBlocks();
		if(blocks.size() == 0)
		{
			Linker::Error << "Internal error: Hunk produced no blocks" << std::endl;
			return 0;
		}
	}
	if(InitialHunkBlock * block = dynamic_cast<InitialHunkBlock *>(blocks[0].get()))
	{
		return block->GetSizeField();
	}
	else
	{
		Linker::Error << "Internal error: Hunk produced unexpected first block" << std::endl;
		return 0;
	}
}

void HunkFormat::Hunk::AppendBlock(std::shared_ptr<Block> block)
{
	if(blocks.size() == 0)
	{
		if(dynamic_cast<InitialHunkBlock *>(block.get()))
		{
			type = hunk_type(block->type);
		}
		else
		{
			type = Invalid;
		}
	}

	switch(block->type)
	{
	case Block::HUNK_NAME:
		name = dynamic_cast<TextBlock *>(block.get())->name;
		break;
	case Block::HUNK_CODE:
	case Block::HUNK_DATA:
	case Block::HUNK_PPC_CODE:
		if(image == nullptr)
		{
			image = dynamic_cast<LoadBlock *>(block.get())->image;
		}
		else
		{
			Linker::Error << "Error: duplicate code/data block in hunk" << std::endl;
		}
		break;
	case Block::HUNK_BSS:
		image = nullptr;
		image_size = dynamic_cast<BssBlock *>(block.get())->size;
		break;
	case Block::HUNK_ABSRELOC32:
	case Block::HUNK_RELRELOC16:
	case Block::HUNK_RELRELOC8:
	case Block::HUNK_DRELOC32:
	case Block::HUNK_DRELOC16:
	case Block::HUNK_DRELOC8:
	case Block::HUNK_RELOC32SHORT:
	case Block::HUNK_RELRELOC32:
	case Block::HUNK_ABSRELOC16:
	case Block::HUNK_RELRELOC26:
		{
			RelocationBlock * relocation_block = dynamic_cast<RelocationBlock *>(block.get());
			for(auto& data : relocation_block->relocations)
			{
				if(relocations.find(data.hunk) == relocations.end())
					relocations[data.hunk] = std::set<Relocation>();
				for(auto offset : data.offsets)
				{
					// TODO: more information needs to be stored
					relocations[data.hunk].insert(Relocation(relocation_block->GetRelocationSize(), relocation_block->GetRelocationType(), offset));
				}
			}
		}
		break;
	case Block::HUNK_END:
		// TODO
		break;
	case Block::HUNK_EXT:
	case Block::HUNK_SYMBOL:
		// TODO
		break;
	case Block::HUNK_DEBUG:
	case Block::HUNK_OVERLAY:
	case Block::HUNK_BREAK:
	case Block::HUNK_LIB:
	case Block::HUNK_INDEX:
		// TODO
	default:
		Linker::Error << "Error: invalid block in hunk" << std::endl;
	}

	blocks.emplace_back(block);
}

// HunkFormat

bool HunkFormat::IsExecutable() const
{
	return start_block != nullptr && start_block->type == Block::HUNK_HEADER;
}

offset_t HunkFormat::ImageSize() const
{
	offset_t size = 0;
	if(start_block != nullptr)
	{
		Linker::Debug << "Debug: size of " << std::hex << start_block->type << " is " << start_block->FileSize() << std::endl;
		size += start_block->FileSize();
	}
	for(auto& hunk : hunks)
	{
		size += hunk.GetFileSize();
	}
	return size;
}

void HunkFormat::ReadFile(Linker::Reader& rd)
{
	start_block = nullptr;
	hunks.clear();

	std::shared_ptr<Block> block;

	rd.endiantype = ::BigEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);
	block = Block::ReadBlock(rd);

	if(block == nullptr)
		return;

	if(block->type == Block::HUNK_UNIT || block->type == Block::HUNK_HEADER)
	{
		start_block = block;
		block = Block::ReadBlock(rd, IsExecutable());
	}

	for(; rd.Tell() < end && block != nullptr; block = Block::ReadBlock(rd, IsExecutable()))
	{
		Hunk hunk;
		hunk.AppendBlock(block);
		for(block = Block::ReadBlock(rd, IsExecutable()); block != nullptr; block = Block::ReadBlock(rd, IsExecutable()))
		{
			hunk.AppendBlock(block);
			if(block->type == Block::HUNK_END)
				break;
		}
		hunks.emplace_back(hunk);
	}
}

offset_t HunkFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;

	start_block->Write(wr);

	for(const Hunk& hunk : hunks)
	{
		for(auto& block : hunk.blocks)
		{
			block->Write(wr);
		}
	}

	return offset_t(-1);
}

void HunkFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Hunk format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	offset_t current_offset = 0;

	if(start_block == nullptr)
	{
		Dumper::Region header_region("Missing header", file_offset, 0, 8);
		header_region.Display(dump);
	}
	else
	{
		start_block->Dump(dump, *this, nullptr, 0, 0);
		current_offset = start_block->FileSize();
	}

	unsigned current_block = 1;
	unsigned hunk_number = 0; // TODO: starting number
	for(auto& hunk : hunks)
	{
		Dumper::Region hunk_region("Hunk", current_offset, hunk.GetFileSize(), 8);
		hunk_region.AddField("Number", Dumper::DecDisplay::Make(), offset_t(hunk_number));
		hunk_region.AddOptionalField("Name", Dumper::StringDisplay::Make(), hunk.name);
		hunk_region.Display(dump);
		for(auto& block : hunk.blocks)
		{
			block->Dump(dump, *this, &hunk, current_block, current_offset);
			current_offset += block->FileSize();
			current_block++;
		}
		hunk_number += 1;
	}
}

offset_t HunkFormat::GetHunkSizeInHeader(uint32_t index) const
{
	// TODO: untested
	if(const HeaderBlock * header = dynamic_cast<const HeaderBlock *>(start_block.get()))
	{
		if(index < header->hunk_sizes.size())
		{
			return header->hunk_sizes[index];
		}
	}
	return 0;
}

std::string HunkFormat::ReadString(uint32_t longword_count, Linker::Reader& rd)
{
	return rd.ReadData(longword_count * 4, '\0');
}

std::string HunkFormat::ReadString(Linker::Reader& rd, uint32_t& longword_count)
{
	longword_count = rd.ReadUnsigned(4);
	return ReadString(longword_count, rd);
}

std::string HunkFormat::ReadString(Linker::Reader& rd)
{
	uint32_t tmp;
	return ReadString(rd, tmp);
}

void HunkFormat::WriteStringContents(Linker::Writer& wr, std::string name)
{
	offset_t size = ::AlignTo(name.size(), 4);
	wr.WriteData(size, name, '\0');
}

void HunkFormat::WriteString(Linker::Writer& wr, std::string name)
{
	offset_t size = ::AlignTo(name.size(), 4);
	wr.WriteWord(4, size / 4);
	wr.WriteData(size, name, '\0');
}

offset_t HunkFormat::MeasureString(std::string name)
{
	return 4 + ::AlignTo(name.size(), 4);
}

unsigned HunkFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(section_name == ".chip" || section_name.rfind(".chip.", 0) == 0)
	{
		Linker::Debug << "Debug: Using chip memory" << std::endl;
		return ChipMemory;
	}
	else if(section_name == ".fast" || section_name.rfind(".fast.", 0) == 0)
	{
		Linker::Debug << "Debug: Using fast memory" << std::endl;
		return FastMemory;
	}
	else
	{
		return 0;
	}
}

void HunkFormat::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */

	linker_parameters["fast_memory_flag"] = FastMemory;
	linker_parameters["chip_memory_flag"] = ChipMemory;
}

void HunkFormat::AddHunk(const Hunk& hunk)
{
	hunks.push_back(hunk);
	segment_index[std::dynamic_pointer_cast<Linker::Segment>(hunks.back().image)] = hunks.size() - 1;
}

void HunkFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return; /* ignore segment */

	unsigned flags = segment->sections.front()->GetFlags();
	Linker::Debug << "Debug: section flags " << std::hex << flags << std::endl;
	Hunk::hunk_type hunk_type;
	unsigned hunk_flags = LoadBlock::LoadAny;
	if(!(flags & Linker::Section::Writable))
	{
		hunk_type = Hunk::hunk_type(cpu);
	}
	else if(!(flags & Linker::Section::ZeroFilled))
	{
		hunk_type = Hunk::Data;
	}
	else
	{
		hunk_type = Hunk::Bss;
	}

	if((flags & ChipMemory))
	{
		Linker::Debug << "Debug: Setting flags to chip memory" << std::endl;
		hunk_flags = LoadBlock::LoadChipMem;
	}
	else if((flags & FastMemory))
	{
		Linker::Debug << "Debug: Setting flags to fast memory" << std::endl;
		hunk_flags = LoadBlock::LoadFastMem;
	}

	AddHunk(Hunk(hunk_type, segment, hunk_flags));
}

std::unique_ptr<Script::List> HunkFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	all exec align 4; # TODO: are these needed?
	all not write align 4;
	align 4;
};

".data"
{
	at 0;
	all not zero and not customflag(?chip_memory_flag?) and not customflag(?fast_memory_flag?) align 4;
	align 4;
};

".bss"
{
	at 0;
	all not customflag(?chip_memory_flag?) and not customflag(?fast_memory_flag?) align 4;
	align 4;
};

".fast.data"
{
	at 0;
	all not zero and not customflag(?fast_memory_flag?) align 4;
	align 4;
};

".fast.bss"
{
	at 0;
	all not customflag(?fast_memory_flag?) align 4;
	align 4;
};

".chip.data"
{
	at 0;
	all not zero and not customflag(?chip_memory_flag?) align 4;
	align 4;
};

".chip.bss"
{
	at 0;
	all not customflag(?chip_memory_flag?) align 4;
	align 4;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		/* TODO: Large Model */
		return Script::parse_string(SimpleScript);
	}
}

void HunkFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void HunkFormat::ProcessModule(Linker::Module& module)
{
	/* .code */
	switch(module.cpu)
	{
	case Linker::Module::M68K:
		cpu = CPU_M68K;
		break;
	case Linker::Module::PPC:
		cpu = CPU_PPC;
		break;
	default:
		Linker::FatalError("Fatal error: invalid CPU type");
	}

	Link(module);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr)
		{
			if(resolution.reference != nullptr)
			{
				Linker::Error << "Error: intersegment differences not supported, ignoring" << std::endl;
				continue;
			}
			if(rel.size != 4)
			{
				Linker::Error << "Error: only longword relocations are supported, ignoring" << std::endl;
				/* TODO: maybe others are supported as well */
				continue;
			}
			Linker::Position position = rel.source.GetPosition();
			uint32_t source = segment_index[position.segment];
			uint32_t target = segment_index[resolution.target];
			hunks[source].relocations[target].insert(Relocation(rel.size, Relocation::Absolute, position.address));
		}
	}

	std::shared_ptr<HeaderBlock> header = std::make_shared<HeaderBlock>();
	header->table_size = hunks.size();
	header->first_hunk = 0;
	for(Hunk& hunk : hunks)
	{
		header->hunk_sizes.emplace_back(hunk.GetSizeField());
	}
	start_block = header;
}

void HunkFormat::CalculateValues()
{
}

void HunkFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string HunkFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

