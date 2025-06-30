
#include "hunk.h"
#include "../linker/options.h"
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
	Linker::Debug << "Debug: read " << std::hex << type;
	if(is_executable)
		Linker::Debug << " in executable";
	Linker::Debug << std::endl;
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
		block = std::make_shared<DebugBlock>();
		break;
	case HUNK_OVERLAY:
		block = std::make_shared<OverlayBlock>();
		break;
	case HUNK_LIB:
		block = std::make_shared<LibraryBlock>();
		break;
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

void HunkFormat::Block::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Dumper::Region block_region("Block", current_offset, FileSize(), 8);
	AddCommonFields(block_region, index);
	AddExtraFields(block_region, module, hunk, index, current_offset);
	block_region.Display(dump);
}

void HunkFormat::Block::AddCommonFields(Dumper::Region& region, unsigned index) const
{
	region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	static std::map<offset_t, std::string> type_descriptions =
	{
		{ HUNK_UNIT, "HUNK_UNIT" },
		{ HUNK_NAME, "HUNK_NAME" },
		{ HUNK_CODE, "HUNK_CODE" },
		{ HUNK_DATA, "HUNK_DATA" },
		{ HUNK_BSS, "HUNK_BSS" },
		{ HUNK_ABSRELOC32, "HUNK_RELOC32/HUNK_ABSRELOC32" },
		{ HUNK_RELRELOC16, "HUNK_RELOC16/HUNK_RELRELOC16" },
		{ HUNK_RELRELOC8, "HUNK_RELOC8/HUNK_RELRELOC16" },
		{ HUNK_EXT, "HUNK_EXT" },
		{ HUNK_SYMBOL, "HUNK_SYMBOL" },
		{ HUNK_DEBUG, "HUNK_DEBUG" },
		{ HUNK_END, "HUNK_END" },
		{ HUNK_HEADER, "HUNK_HEADER" },
		{ HUNK_OVERLAY, "HUNK_OVERLAY" },
		{ HUNK_BREAK, "HUNK_BREAK" },
		{ HUNK_DRELOC32, "HUNK_DRELOC32" },
		//{ HUNK_DRELOC16, "HUNK_DRELOC16" }, // also HUNK_V37_RELOC32SHORT
		{ HUNK_DRELOC8, "HUNK_DRELOC8" },
		{ HUNK_LIB, "HUNK_LIB" },
		{ HUNK_INDEX, "HUNK_INDEX" },
		{ HUNK_RELOC32SHORT, "HUNK_RELOC32SHORT" },
		{ HUNK_RELRELOC32, "HUNK_RELRELOC32" },
		{ HUNK_ABSRELOC16, "HUNK_ABSRELOC16" },
		{ HUNK_PPC_CODE, "HUNK_PPC_CODE" },
		{ HUNK_RELRELOC26, "HUNK_RELRELOC26" },
	};
	if(is_executable)
	{
		type_descriptions[HUNK_DRELOC32] = "HUNK_RELOC32SHORT (V37)";
	}
	else
	{
		type_descriptions[HUNK_DRELOC32] = "HUNK_DRELOC32";
	}
	region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(type));
}

void HunkFormat::Block::AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::HeaderBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::HeaderBlock::AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	region.AddField("Table size", Dumper::HexDisplay::Make(8), offset_t(table_size));
	region.AddField("First hunk", Dumper::HexDisplay::Make(8), offset_t(first_hunk));
	region.AddField("Last hunk", Dumper::HexDisplay::Make(8), offset_t(first_hunk + hunk_sizes.size() - 1));
}

// RelocatableBlock

uint32_t HunkFormat::RelocatableBlock::GetSizeField() const
{
	uint32_t size = GetSize();
	if(RequiresAdditionalFlags())
		return size | 0xC0000000;
	else
		return size | ((flags & ~LoadPublic) << 29);
}

bool HunkFormat::RelocatableBlock::RequiresAdditionalFlags() const
{
	return loaded_with_additional_flags || (flags & ~(LoadChipMem | LoadFastMem | LoadPublic)) != 0;
}

uint32_t HunkFormat::RelocatableBlock::GetAdditionalFlags() const
{
	return flags & ~LoadPublic;
}

void HunkFormat::RelocatableBlock::Read(Linker::Reader& rd)
{
	uint32_t longword_count = rd.ReadUnsigned(4);
	if((longword_count & BitAdditional) == BitAdditional)
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

void HunkFormat::RelocatableBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	wr.WriteWord(4, GetSizeField());
	if(RequiresAdditionalFlags())
		wr.WriteWord(4, GetAdditionalFlags()); // TODO: this is not tested
	WriteBody(wr);
}

offset_t HunkFormat::RelocatableBlock::FileSize() const
{
	return 8 + 4 * GetSize() + (RequiresAdditionalFlags() ? 4 : 0);
}

void HunkFormat::RelocatableBlock::AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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
			offset_t(flags));
	}
	else
	{
		region.AddOptionalField("Flags",
			Dumper::BitFieldDisplay::Make(8)
				->AddBitField(29, 1, Dumper::ChoiceDisplay::Make("advisory, hunk can be ignored"), true)
				->AddBitField(30, 1, Dumper::ChoiceDisplay::Make("chip memory"), true)
				->AddBitField(31, 1, Dumper::ChoiceDisplay::Make("fast memory"), true),
			offset_t(flags) & FlagMask);
	}
}

void HunkFormat::RelocatableBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
}

void HunkFormat::RelocatableBlock::WriteBody(Linker::Writer& wr) const
{
}

// LoadBlock

void HunkFormat::LoadBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::BssBlock::AddExtraFields(Dumper::Region& region, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	RelocatableBlock::AddExtraFields(region, module, hunk, index, current_offset);
	region.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(size * 4));
}

// RelocationBlock

bool HunkFormat::RelocationBlock::IsShortRelocationBlock() const
{
	return type == HUNK_RELOC32SHORT || (is_executable && type == HUNK_V37_RELOC32SHORT);
//		|| (is_executable && type == HUNK_RELRELOC32); // TODO: is this correct?
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

	Linker::Debug << "Debug: word read is " << wordread << std::endl;

	while(true)
	{
		uint32_t relocation_count = rd.ReadUnsigned(wordread);
		if(relocation_count == 0)
			break;

		relocations.emplace_back(RelocationData());
		relocations.back().hunk = rd.ReadUnsigned(wordread);
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
	size_t wordwrite = IsShortRelocationBlock() ? 2 : 4;
	for(auto& data : relocations)
	{
		// to make sure to avoid accidentally closing the list prematurely
		if(data.offsets.size() == 0)
			continue;

		size += wordwrite * (2 + data.offsets.size());
	}
	return size;
}

void HunkFormat::RelocationBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	size_t wordread = IsShortRelocationBlock() ? 2 : 4;
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
			static const std::map<offset_t, std::string> relocation_type_descriptions =
			{
				{ Relocation::Absolute, "absolute" },
				{ Relocation::SelfRelative, "PC-relative" },
				{ Relocation::DataRelative, "data relative" },
				{ Relocation::SelfRelative26, "26-bit PC-relative" },
			};
			relocation_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_descriptions), offset_t(GetRelocationType()));
			if(hunk->image != nullptr)
			{
				relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(2 * GetRelocationSize()), offset_t(
					hunk->image->AsImage()->ReadUnsigned(GetRelocationSize(), offset, ::BigEndian)));
			}
			relocation_entry.Display(dump);
			i += 1;
			current_offset += wordread;
		}
		current_offset += 2 * wordread;
	}
}

// SymbolBlock::Unit

std::unique_ptr<HunkFormat::SymbolBlock::Unit> HunkFormat::SymbolBlock::Unit::ReadData(Linker::Reader& rd)
{
	uint32_t length = rd.ReadUnsigned(4);
	if(length == 0)
		return nullptr;

	uint8_t type = length >> 24;
	length &= 0x00FFFFFF;

	std::string name = HunkFormat::ReadString(length, rd);

	std::unique_ptr<HunkFormat::SymbolBlock::Unit> unit;
	switch(type)
	{
	case EXT_SYMB:
	case EXT_DEF:
	case EXT_ABS:
	case EXT_RES:
	case 0x21: // TODO: appears in library, not sure about meaning
		unit = std::make_unique<Definition>(symbol_type(type), name);
		break;

	case EXT_ABSREF32:
	case EXT_RELREF16:
	case EXT_RELREF8:
	case EXT_DREF32: // TODO: guessing format
	case EXT_DREF16: // TODO: guessing format
	case EXT_DREF8: // TODO: guessing format
	case EXT_RELREF32: // TODO: guessing format
	case EXT_ABSREF16: // TODO: guessing format
	case EXT_ABSREF8: // TODO: guessing format
	case EXT_RELREF26:
		unit = std::make_unique<References>(symbol_type(type), name);
		break;

	case EXT_COMMON:
	case EXT_RELCOMMON: // TODO: guessing format
		unit = std::make_unique<CommonReferences>(symbol_type(type), name);
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

void HunkFormat::SymbolBlock::Unit::DumpContents(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
}

void HunkFormat::SymbolBlock::Unit::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::SymbolBlock::Definition::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::SymbolBlock::References::DumpContents(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	current_offset += Unit::FileSize() + 4;
	unsigned i = 0;
	for(uint32_t reference : references)
	{
		Dumper::Entry reference_entry("Reference", i + 1, current_offset);
		reference_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(reference));
		reference_entry.Display(dump);
		current_offset += 4;
		i++;
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

void HunkFormat::SymbolBlock::CommonReferences::AddExtraFields(Dumper::Dumper& dump, Dumper::Entry& entry, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
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

void HunkFormat::SymbolBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	unsigned i = 0;
	for(auto& unit : symbols)
	{
		Dumper::Entry unit_entry("External", i + 1, current_offset);
		static const std::map<offset_t, std::string> type_descriptions =
		{
			{ Unit::EXT_SYMB, "EXT_SYMB - internal symbol definition" },
			{ Unit::EXT_DEF, "EXT_DEF - relocatable symbol definition" },
			{ Unit::EXT_ABS, "EXT_ABS - absolute symbol definition" },
			{ Unit::EXT_RES, "EXT_RES - resident library symbol definition" },
			{ Unit::EXT_ABSREF32, "EXT_REF32/EXT_ABSREF32 - 32-bit absolute reference to symbol" },
			{ Unit::EXT_COMMON, "EXT_COMMON - 32-bit absolute reference to common symbol" },
			{ Unit::EXT_RELREF16, "EXT_REF16/EXT_RELREF16 - 16-bit relative reference to symbol" },
			{ Unit::EXT_RELREF8, "EXT_REF8/EXT_RELREF8 - 8-bit relative reference to symbol" },
			{ Unit::EXT_DREF32, "EXT_DREF32 - 32-bit data relative reference to symbol" },
			{ Unit::EXT_DREF16, "EXT_DREF16 - 16-bit data relative reference to symbol" },
			{ Unit::EXT_DREF8, "EXT_DREF8 - 8-bit data relative reference to symbol" },
			{ Unit::EXT_RELREF32, "EXT_RELREF32 - 32-bit relative reference to symbol" },
			{ Unit::EXT_RELCOMMON, "EXT_RELCOMMON - 32-bit relative reference to common symbol" },
			{ Unit::EXT_ABSREF16, "EXT_ABSREF16 - 16-bit absolute reference to symbol" },
			{ Unit::EXT_ABSREF8, "EXT_ABSREF8 - 8-bit absolute reference to symbol" },
			{ Unit::EXT_RELREF26, "EXT_RELREF26 - 26-bit relative reference to symbol in 4-byte word" },
		};

		unit_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(unit->type));
		unit_entry.AddField("Name", Dumper::StringDisplay::Make(), unit->name);
		unit->AddExtraFields(dump, unit_entry, module, hunk, i, current_offset);
		unit_entry.Display(dump);

		unit->DumpContents(dump, module, hunk, i, current_offset);

		current_offset += unit->FileSize();
		i++;
	}
}

// DebugBlock

void HunkFormat::DebugBlock::Read(Linker::Reader& rd)
{
	uint32_t longword_count = rd.ReadUnsigned(4);
	image = Linker::Buffer::ReadFromFile(rd, longword_count * 4);
}

void HunkFormat::DebugBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	wr.WriteWord(4, ::AlignTo(image->ImageSize(), 4) / 4);
	image->WriteFile(wr);
	if((image->ImageSize() & 3) != 0)
		wr.Skip(-image->ImageSize() & 3);
}

offset_t HunkFormat::DebugBlock::FileSize() const
{
	return 8 + ::AlignTo(image->ImageSize(), 4);
}

void HunkFormat::DebugBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);

	Dumper::Block hunk_block("Data", current_offset + 8, image->AsImage(), 0, 8);
	hunk_block.Display(dump);
}

// OverlayBlock

void HunkFormat::OverlayBlock::Read(Linker::Reader& rd)
{
	uint32_t longword_count = rd.ReadUnsigned(4);
	maximum_level = rd.ReadUnsigned(4);
	rd.Skip(4 * (maximum_level - 2));
	for(uint32_t i = maximum_level; i < longword_count; i += 8)
	{
		OverlaySymbol symbol;
		symbol.offset = rd.ReadUnsigned(4);
		symbol.res1 = rd.ReadUnsigned(4);
		symbol.res2 = rd.ReadUnsigned(4);
		symbol.level = rd.ReadUnsigned(4);
		symbol.ordinate = rd.ReadUnsigned(4);
		symbol.first_hunk = rd.ReadUnsigned(4);
		symbol.symbol_hunk = rd.ReadUnsigned(4);
		symbol.symbol_offset = rd.ReadUnsigned(4);
		overlay_data_table.emplace_back(symbol);
	}
}

void HunkFormat::OverlayBlock::Write(Linker::Writer& wr) const
{
	Block::Write(wr);
	wr.WriteWord(4, (FileSize() - 8) / 4);
	wr.WriteWord(4, maximum_level);
	wr.Skip(4 * (maximum_level - 2));
	for(auto& symbol : overlay_data_table)
	{
		wr.WriteWord(4, symbol.offset);
		wr.WriteWord(4, symbol.res1);
		wr.WriteWord(4, symbol.res2);
		wr.WriteWord(4, symbol.level);
		wr.WriteWord(4, symbol.ordinate);
		wr.WriteWord(4, symbol.first_hunk);
		wr.WriteWord(4, symbol.symbol_hunk);
		wr.WriteWord(4, symbol.symbol_offset);
	}
}

offset_t HunkFormat::OverlayBlock::FileSize() const
{
	return 8 + 4 * maximum_level + 32 * overlay_data_table.size();
}

void HunkFormat::OverlayBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	Block::Dump(dump, module, hunk, index, current_offset);
	// TODO
}

// LibraryBlock

void HunkFormat::LibraryBlock::Read(Linker::Reader& rd)
{
	uint32_t longword_count = rd.ReadUnsigned(4);
	offset_t end = rd.Tell() + 4 * longword_count;
	std::shared_ptr<Block> next_block = rd.Tell() < end ? Block::ReadBlock(rd, false) : nullptr;
	hunks = std::make_unique<Module>();
	hunks->ReadFile(rd, next_block, end);
}

void HunkFormat::LibraryBlock::Write(Linker::Writer& wr) const
{
	Block::Write(wr);
	wr.WriteWord(4, (FileSize() - 8) / 4);
	hunks->WriteFile(wr);
}

offset_t HunkFormat::LibraryBlock::FileSize() const
{
	return 8 + hunks->ImageSize();
}

void HunkFormat::LibraryBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	// TODO
}

// IndexBlock

HunkFormat::IndexBlock::Definition HunkFormat::IndexBlock::Definition::Read(Linker::Reader& rd)
{
	Definition def;
	def.string_offset = rd.ReadUnsigned(2);
	def.symbol_offset = rd.ReadUnsigned(2);
	def.type = rd.ReadUnsigned(2);
	return def;
}

void HunkFormat::IndexBlock::Definition::Write(Linker::Writer& wr) const
{
	wr.WriteWord(2, string_offset);
	wr.WriteWord(2, symbol_offset);
	wr.WriteWord(2, type);
}

HunkFormat::IndexBlock::HunkEntry HunkFormat::IndexBlock::HunkEntry::Read(Linker::Reader& rd)
{
	HunkEntry hunk;
	hunk.string_offset = rd.ReadUnsigned(2);
	hunk.hunk_size = rd.ReadUnsigned(2);
	hunk.hunk_type = rd.ReadUnsigned(2); // highest 2 bits are Fast and Chip flags
	uint16_t ref_count = rd.ReadUnsigned(2);
	for(uint16_t i = 0; i < ref_count; i++)
	{
		hunk.references.push_back(rd.ReadUnsigned(2));
	}
	uint16_t def_count = rd.ReadUnsigned(2);
	for(uint16_t i = 0; i < def_count; i++)
	{
		hunk.definitions.push_back(Definition::Read(rd));
	}
	return hunk;
}

void HunkFormat::IndexBlock::HunkEntry::Write(Linker::Writer& wr) const
{
	HunkEntry hunk;
	wr.WriteWord(2, string_offset);
	wr.WriteWord(2, hunk_size);
	wr.WriteWord(2, hunk_type);
	wr.WriteWord(2, references.size());
	for(uint16_t ref : references)
	{
		wr.WriteWord(2, ref);
	}
	wr.WriteWord(2, definitions.size());
	for(auto& def : definitions)
	{
		def.Write(wr);
	}
}

offset_t HunkFormat::IndexBlock::HunkEntry::FileSize() const
{
	return 10 + 2 * references.size() + 6 * definitions.size();
}

HunkFormat::IndexBlock::ProgramUnit HunkFormat::IndexBlock::ProgramUnit::Read(Linker::Reader& rd)
{
	ProgramUnit unit;
	unit.string_offset = rd.ReadSigned(2);
	unit.first_hunk_offset = rd.ReadUnsigned(2);
	uint16_t hunk_count = rd.ReadUnsigned(2);
	for(uint16_t i = 0; i < hunk_count; i++)
	{
		unit.hunks.emplace_back(HunkEntry::Read(rd));
	}
	return unit;
}

void HunkFormat::IndexBlock::ProgramUnit::Write(Linker::Writer& wr) const
{
	wr.WriteWord(2, string_offset);
	wr.WriteWord(2, first_hunk_offset);
	wr.WriteWord(2, hunks.size());
	for(auto& hunk : hunks)
	{
		hunk.Write(wr);
	}
}

offset_t HunkFormat::IndexBlock::ProgramUnit::FileSize() const
{
	offset_t size = 6;
	for(auto& hunk : hunks)
	{
		size += hunk.FileSize();
	}
	return size;
}

offset_t HunkFormat::IndexBlock::StringTableSize() const
{
	offset_t size = 0;
	for(auto& string : strings)
	{
		size += string.size() + 1;
	}
	return size;
}

void HunkFormat::IndexBlock::Read(Linker::Reader& rd)
{
	// TODO: untested
	uint32_t longword_count = rd.ReadUnsigned(4);
	offset_t block_content_offset = rd.Tell();
	uint16_t string_block_size = rd.ReadUnsigned(2);
	offset_t string_block_start = rd.Tell();
	strings.clear();
	while(rd.Tell() < string_block_start + string_block_size)
	{
		strings.push_back(rd.ReadASCIIZ());
	}
	if((rd.Tell() & 1) != 0)
		rd.Skip(1);
	while(rd.Tell() < block_content_offset + longword_count * 4)
	{
		units.emplace_back(ProgramUnit::Read(rd));
	}
	if((rd.Tell() & 3) != 0)
		rd.Skip(-rd.Tell() & 3);
}

void HunkFormat::IndexBlock::Write(Linker::Writer& wr) const
{
	Block::Write(wr);
	wr.WriteWord(4, (FileSize() - 8) / 4);
	wr.WriteWord(2, StringTableSize());
	for(auto& string : strings)
	{
		wr.WriteData(string);
		wr.WriteWord(1, 0);
	}
	if((wr.Tell() & 1) != 0)
		wr.Skip(1);
	for(auto& unit : units)
	{
		unit.Write(wr);
	}
	if((wr.Tell() & 3) != 0)
		wr.Skip(-wr.Tell() & 3);
}

offset_t HunkFormat::IndexBlock::FileSize() const
{
	offset_t total_size = ::AlignTo(10 + StringTableSize(), 2);
	for(auto& unit : units)
	{
		total_size += unit.FileSize();
	}
	return ::AlignTo(total_size, 4);
}

void HunkFormat::IndexBlock::Dump(Dumper::Dumper& dump, const Module& module, const Hunk * hunk, unsigned index, offset_t current_offset) const
{
	// TODO
}

// Hunk

void HunkFormat::Hunk::ProduceBlocks(HunkFormat& fmt, Module& module)
{
	if(name != "")
	{
		blocks.push_back(std::make_shared<TextBlock>(Block::HUNK_NAME, name));
	}

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

	/* Relocation blocks */
	std::map<Block::block_type, std::shared_ptr<RelocationBlock>> relocation_blocks;

	for(auto it : relocations)
	{
		for(Relocation rel : it.second)
		{
			Block::block_type btype = Block::block_type(0);
			switch(rel.type)
			{
			case Relocation::Absolute:
				switch(rel.size)
				{
				case 2:
					if(fmt.system == V39)
					{
						btype = Block::HUNK_ABSRELOC16;
					}
					break;
				case 4:
					if(module.IsExecutable())
					{
						switch(fmt.system)
						{
						case V1:
							btype = Block::HUNK_ABSRELOC32;
							break;
						case V37:
							// TODO: only if possible to fit
							btype = Block::HUNK_V37_RELOC32SHORT;
							break;
						case V38:
						case V39:
							btype = Block::HUNK_RELOC32SHORT;
							break;
						}
					}
					else
					{
						btype = Block::HUNK_ABSRELOC32;
					}
					break;
				}
				break;
			case Relocation::SelfRelative:
				switch(rel.size)
				{
				case 1:
					btype = Block::HUNK_RELRELOC8;
					break;
				case 2:
					btype = Block::HUNK_RELRELOC16;
					break;
				case 4:
					if(fmt.system == V39)
					{
						btype = Block::HUNK_RELRELOC32;
					}
					break;
				}
				break;
			case Relocation::DataRelative:
				switch(rel.size)
				{
				case 1:
					btype = Block::HUNK_DRELOC8;
					break;
				case 2:
					btype = Block::HUNK_DRELOC16;
					break;
				case 4:
					btype = Block::HUNK_DRELOC32;
					break;
				}
				break;
			case Relocation::SelfRelative26:
				btype = Block::HUNK_RELRELOC26;
				break;
			}

			if(module.IsExecutable())
			{
				switch(btype)
				{
				case Block::HUNK_ABSRELOC32:
					break;
				case Block::HUNK_V37_RELOC32SHORT:
					if(fmt.system != V37)
						btype = Block::block_type(0);
					break;
				case Block::HUNK_RELOC32SHORT:
					if(fmt.system == V1)
						btype = Block::block_type(0);
					break;
				case Block::HUNK_RELRELOC32:
					if(fmt.system != V39)
						btype = Block::block_type(0);
					break;
				default:
					// invalid in executable
					btype = Block::block_type(0);
					break;
				}
			}

			if(btype != Block::block_type(0))
			{
				if(relocation_blocks.find(btype) == relocation_blocks.end())
				{
					relocation_blocks[btype] = std::make_shared<RelocationBlock>(btype, module.IsExecutable());
					relocation_blocks[btype]->relocations.emplace_back(RelocationBlock::RelocationData(it.first));
				}
				else if(relocation_blocks[btype]->relocations.back().hunk != it.first)
				{
					relocation_blocks[btype]->relocations.emplace_back(RelocationBlock::RelocationData(it.first));
				}
				relocation_blocks[btype]->relocations.back().offsets.push_back(rel.offset);
			}
			else
			{
				Linker::Warning << "Warning: generating relocation of size " << rel.size << " and type ";
				switch(rel.type)
				{
				case Relocation::Absolute:
					Linker::Warning << "absolute";
					break;
				case Relocation::SelfRelative:
					Linker::Warning << "PC-relative";
					break;
				case Relocation::DataRelative:
					Linker::Warning << "data section relative";
					break;
				case Relocation::SelfRelative26:
					Linker::Warning << "26-bit PC-relative";
					break;
				}
				Linker::Warning << " is not supported, ignoring" << std::endl;
			}
		}
	}

	for(auto pair : relocation_blocks)
	{
		blocks.push_back(pair.second);
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

uint32_t HunkFormat::Hunk::GetSizeField(HunkFormat& fmt, Module& module)
{
	if(blocks.size() == 0)
	{
		ProduceBlocks(fmt, module);
		if(blocks.size() == 0)
		{
			Linker::Error << "Internal error: Hunk produced no blocks" << std::endl;
			return 0;
		}
	}
	if(RelocatableBlock * block = dynamic_cast<RelocatableBlock *>(blocks[0].get()))
	{
		return block->GetSizeField();
	}

	if(blocks[0]->type == Block::HUNK_NAME && blocks.size() >= 2)
	{
		if(RelocatableBlock * block = dynamic_cast<RelocatableBlock *>(blocks[1].get()))
		{
			return block->GetSizeField();
		}
	}

	Linker::Error << "Internal error: Hunk produced unexpected first block" << std::endl;
	return 0;
}

void HunkFormat::Hunk::AppendBlock(std::shared_ptr<Block> block)
{
	if(blocks.size() == 0)
	{
		if(dynamic_cast<RelocatableBlock *>(block.get()))
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
		if(image == nullptr && image_size == 0)
		{
			image = dynamic_cast<LoadBlock *>(block.get())->image;
		}
		else
		{
			Linker::Error << "Error: duplicate code/data/bss block in hunk" << std::endl;
		}
		break;
	case Block::HUNK_BSS:
		if(image == nullptr && image_size == 0)
		{
			image_size = dynamic_cast<BssBlock *>(block.get())->size;
		}
		else
		{
			Linker::Error << "Error: duplicate code/data/bss block in hunk" << std::endl;
		}
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
		// TODO
		break;
	case Block::HUNK_OVERLAY:
	case Block::HUNK_BREAK:
	case Block::HUNK_LIB:
	case Block::HUNK_INDEX:
		// TODO
	default:
		Linker::Error << "Error: invalid block " << std::hex << block->type << " in hunk" << std::endl;
	}

	blocks.emplace_back(block);
}

// Module

bool HunkFormat::Module::IsExecutable() const
{
	return start_block != nullptr && start_block->type == Block::HUNK_HEADER;
}

offset_t HunkFormat::Module::ImageSize() const
{
	offset_t size = 0;
	if(start_block != nullptr)
	{
//		Linker::Debug << "Debug: size of " << std::hex << start_block->type << " is " << start_block->FileSize() << std::endl;
		size += start_block->FileSize();
	}
	for(auto& hunk : hunks)
	{
		size += hunk.GetFileSize();
	}
	return size;
}

void HunkFormat::Module::ReadFile(Linker::Reader& rd, std::shared_ptr<Block>& next_block, offset_t end)
{
	start_block = nullptr;
	hunks.clear();
	end_block = nullptr;

	if(next_block->type == Block::HUNK_UNIT || next_block->type == Block::HUNK_HEADER)
	{
		start_block = next_block;
		next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
	}
	else if(next_block->type == Block::HUNK_LIB)
	{
		start_block = next_block;
		next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
		if(next_block->type != Block::HUNK_INDEX)
		{
			Linker::Error << "Error: expected HUNK_INDEX" << std::endl;
		}
		else
		{
			end_block = next_block;
			next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
		}
		return;
	}

	while(next_block != nullptr)
	{
		Hunk hunk;
		while(next_block != nullptr)
		{
			if(next_block->type == Block::HUNK_OVERLAY || next_block->type == Block::HUNK_BREAK)
			{
				end_block = next_block;
				next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
				return;
			}
			else if(next_block->type == Block::HUNK_UNIT || next_block->type == Block::HUNK_LIB)
			{
				return;
			}
			else
			{
				hunk.AppendBlock(next_block);
				if(next_block->type == Block::HUNK_END)
					break;
				next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
			}
		}
		hunks.emplace_back(hunk);
		next_block = rd.Tell() < end ? Block::ReadBlock(rd, IsExecutable()) : nullptr;
	}
}

void HunkFormat::Module::WriteFile(Linker::Writer& wr) const
{
	if(start_block != nullptr)
	{
		start_block->Write(wr);
	}

	for(const Hunk& hunk : hunks)
	{
		for(auto& block : hunk.blocks)
		{
			block->Write(wr);
		}
	}

	if(end_block != nullptr)
	{
		end_block->Write(wr);
	}
}

void HunkFormat::Module::Dump(Dumper::Dumper& dump, offset_t current_offset, unsigned index) const
{
	Dumper::Region file_region("Module", current_offset, ImageSize(), 8);
	file_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	file_region.Display(dump);

	if(start_block == nullptr)
	{
		Dumper::Region header_region("Missing header", current_offset, 0, 8);
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

	if(end_block != nullptr)
	{
		end_block->Dump(dump, *this, nullptr, current_block, current_offset);
	}
}

offset_t HunkFormat::Module::GetHunkSizeInHeader(uint32_t index) const
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

// HunkFormat

bool HunkFormat::IsExecutable() const
{
	return modules.size() > 0 && modules[0].IsExecutable();
}

offset_t HunkFormat::ImageSize() const
{
	offset_t size = 0;
	for(auto& module : modules)
	{
		size += module.ImageSize();
	}
	return size;
}

void HunkFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);

	std::shared_ptr<Block> next_block = Block::ReadBlock(rd, IsExecutable());

	while(rd.Tell() < end)
	{
		Module module;
		module.ReadFile(rd, next_block, end);
		modules.emplace_back(module);
	}
}

offset_t HunkFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;

	for(const Module& module : modules)
	{
		module.WriteFile(wr);
	}

	return ImageSize();
}

void HunkFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Hunk format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	offset_t current_offset = 0;
	unsigned module_number = 0;

	for(auto& module : modules)
	{
		module.Dump(dump, current_offset, module_number);
		current_offset += module.ImageSize();
		module_number++;
	}
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

class HunkOptionCollector : public Linker::OptionCollector
{
public:
	Linker::Option<std::optional<std::string>> system{"system", "Target system version, determines generated hunk types, permitted options: v1, v37, v38, v39"};

	HunkOptionCollector()
	{
		InitializeFields(system);
	}
};

void HunkFormat::SetOptions(std::map<std::string, std::string>& options)
{
	HunkOptionCollector collector;
	collector.ConsiderOptions(options);
	if(auto system_version = collector.system())
	{
		if(*system_version == "v1")
		{
			Linker::Debug << "Debug: Generating for V1" << std::endl;
			system = V1;
		}
		else if(*system_version == "v37")
		{
			Linker::Debug << "Debug: Generating for V37" << std::endl;
			system = V37;
		}
		else if(*system_version == "v38")
		{
			Linker::Debug << "Debug: Generating for V38" << std::endl;
			system = V38;
		}
		else if(*system_version == "v39")
		{
			Linker::Debug << "Debug: Generating for V39" << std::endl;
			system = V39;
		}
		else
		{
			Linker::Error << "Error: Unexpected system version " << *system_version << ", expected v1, v37, v38, v39, using v1" << std::endl;
			system = V1;
		}
	}

	/* TODO */

	linker_parameters["fast_memory_flag"] = FastMemory;
	linker_parameters["chip_memory_flag"] = ChipMemory;
}

void HunkFormat::AddHunk(const Hunk& hunk)
{
	modules[0].hunks.push_back(hunk);
	segment_index[std::dynamic_pointer_cast<Linker::Segment>(modules[0].hunks.back().image)] = modules[0].hunks.size() - 1;
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
	align 8; # This seems to be required for proper functioning for relative relocations
};

".data"
{
	all not zero and not customflag(?chip_memory_flag?) and not customflag(?fast_memory_flag?) align 4;
	align 8;
};

".bss"
{
	all not customflag(?chip_memory_flag?) and not customflag(?fast_memory_flag?) align 4;
	align 8;
};

".fast.data"
{
	all not zero and not customflag(?fast_memory_flag?) align 4;
	align 8;
};

".fast.bss"
{
	all not customflag(?fast_memory_flag?) align 4;
	align 8;
};

".chip.data"
{
	all not zero and not customflag(?chip_memory_flag?) align 4;
	align 8;
};

".chip.bss"
{
	all not customflag(?chip_memory_flag?) align 4;
	align 8;
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
	modules.emplace_back(Module());

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
		if(resolution.target == nullptr)
		{
			rel.WriteWord(resolution.value);
		}
		else
		{
			if(rel.size != 4)
			{
				Linker::Error << "Error: only longword relocations are supported, ignoring" << std::endl;
				/* TODO: maybe others are supported as well */
				continue;
			}
			Linker::Position position = rel.source.GetPosition();
			uint32_t source = segment_index[position.segment];
			uint32_t target = segment_index[resolution.target];
			Relocation::relocation_type type;
			if(resolution.reference != nullptr)
			{
				if(system != V39 || resolution.reference != position.segment)
				{
					Linker::Error << "Error: intersegment differences not supported, ignoring" << std::endl;
					continue;
				}
				type = Relocation::SelfRelative;
				rel.WriteWord(resolution.value + (target == 1 ? 0x70 : -0x20)); // TODO: what is this displacement
			}
			else
			{
				type = Relocation::Absolute;
				rel.WriteWord(resolution.value - resolution.target->base_address);
			}
			if(type == Relocation::SelfRelative)
			{
				Linker::Debug << "Debug: relative, target = " << target << std::endl;
				Linker::Debug << rel << std::endl;
				Linker::Debug << resolution << std::endl;
			}
			else if(type == Relocation::Absolute)
			{
				Linker::Debug << "Debug: absolute, target = " << target << std::endl;
				Linker::Debug << rel << std::endl;
				Linker::Debug << resolution << std::endl;
			}
			modules[0].hunks[source].relocations[target].insert(Relocation(rel.size, type, position.address));
		}
	}

	std::shared_ptr<HeaderBlock> header = std::make_shared<HeaderBlock>();
	modules[0].start_block = header;
	header->table_size = modules[0].hunks.size();
	header->first_hunk = 0;
	for(Hunk& hunk : modules[0].hunks)
	{
		header->hunk_sizes.emplace_back(hunk.GetSizeField(*this, modules[0]));
	}
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

