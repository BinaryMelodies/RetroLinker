
#include "pcos.h"

using namespace PCOS;

CMDFormat::MemoryBlock::~MemoryBlock()
{
}

uint16_t CMDFormat::MemoryBlock::GetLength() const
{
	return 0;
}

void CMDFormat::MemoryBlock::ReadFile(Linker::Reader& rd, uint16_t length)
{
}

void CMDFormat::MemoryBlock::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(1, type);
	wr.WriteWord(2, GetLength());
}

std::unique_ptr<CMDFormat::MemoryBlock> CMDFormat::MemoryBlock::ReadFile(Linker::Reader& rd)
{
	int type = rd.ReadUnsigned(1);
	std::unique_ptr<CMDFormat::MemoryBlock> block;
	switch(type)
	{
	case TYPE_LOAD:
		block = std::make_unique<CMDFormat::LoadBlock>();
		break;
	case TYPE_OFFSET_RELOCATION:
	case TYPE_SEGMENT_RELOCATION:
		block = std::make_unique<CMDFormat::RelocationBlock>(type);
		break;
	case TYPE_END:
		// end blocks do not need a special subclass
		block = std::make_unique<CMDFormat::MemoryBlock>(type);
		break;
	default:
		block = std::make_unique<CMDFormat::UnknownBlock>(type);
		break;
	}
	uint16_t length = rd.ReadUnsigned(2);
	block->ReadFile(rd, length);
	return block;
}

std::unique_ptr<Dumper::Region> CMDFormat::MemoryBlock::MakeRegion(std::string name, offset_t offset, unsigned display_width) const
{
	return std::make_unique<Dumper::Region>(name, offset, 3 + GetLength(), display_width);
}

void CMDFormat::MemoryBlock::AddFields(Dumper::Region& region, CMDFormat& module) const
{
}

void CMDFormat::MemoryBlock::DumpContents(Dumper::Dumper& dump, offset_t file_offset) const
{
}

void CMDFormat::MemoryBlock::Dump(Dumper::Dumper& dump, offset_t file_offset, CMDFormat& module) const
{
	std::unique_ptr<Dumper::Region> region = MakeRegion("Block", file_offset, 6);
	std::map<offset_t, std::string> type_descriptions;
	type_descriptions[TYPE_LOAD] = "load block";
	type_descriptions[TYPE_OFFSET_RELOCATION] = "offset relocations";
	type_descriptions[TYPE_SEGMENT_RELOCATION] = "segment relocations";
	type_descriptions[TYPE_END] = "end block";
	region->AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(2)), offset_t(type));
	AddFields(*region, module);
	region->Display(dump);

	DumpContents(dump, file_offset);
	// TODO: display relocations, they are contained in different blocks
}

uint16_t CMDFormat::LoadBlock::GetLength() const
{
	return 4 + image->ImageSize();
}

void CMDFormat::LoadBlock::ReadFile(Linker::Reader& rd, uint16_t length)
{
	block_id = rd.ReadUnsigned(4);
	image = Linker::Buffer::ReadFromFile(rd, length - 4);
}

void CMDFormat::LoadBlock::WriteFile(Linker::Writer& wr) const
{
	MemoryBlock::WriteFile(wr);
	wr.WriteWord(4, block_id);
	image->WriteFile(wr);
}

std::unique_ptr<Dumper::Region> CMDFormat::LoadBlock::MakeRegion(std::string name, offset_t offset, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset + 3 + 4, image->AsImage(), 0, display_width, 4, 4);
}

void CMDFormat::LoadBlock::AddFields(Dumper::Region& region, CMDFormat& module) const
{
	region.AddField("Block ID", Dumper::HexDisplay::Make(4), offset_t(block_id));

	Dumper::Block& load_block = dynamic_cast<Dumper::Block&>(region);
	for(auto& block : module.blocks)
	{
		if(RelocationBlock * blockp = dynamic_cast<RelocationBlock *>(block.get()))
		{
			if(blockp->source == (block_id >> 24))
			{
				for(uint16_t offset : blockp->offsets)
				{
					load_block.AddSignal(offset, 2);
				}
			}
		}
	}
}

uint16_t CMDFormat::RelocationBlock::GetLength() const
{
	return 2 + 2 * offsets.size();
}

void CMDFormat::RelocationBlock::ReadFile(Linker::Reader& rd, uint16_t length)
{
	source = rd.ReadUnsigned(1);
	target = rd.ReadUnsigned(1);
	for(uint16_t i = 2; i < length; i += 2)
	{
		offsets.push_back(rd.ReadUnsigned(2));
	}
}

void CMDFormat::RelocationBlock::WriteFile(Linker::Writer& wr) const
{
	MemoryBlock::WriteFile(wr);
	wr.WriteWord(1, source);
	wr.WriteWord(1, target);
	for(auto rel : offsets)
	{
		wr.WriteWord(2, rel);
	}
}

void CMDFormat::RelocationBlock::AddFields(Dumper::Region& region, CMDFormat& module) const
{
	region.AddField("Source block", Dumper::HexDisplay::Make(2), offset_t(source));
	region.AddField("Target block", Dumper::HexDisplay::Make(2), offset_t(target));
}

void CMDFormat::RelocationBlock::DumpContents(Dumper::Dumper& dump, offset_t file_offset) const
{
	unsigned i = 0;
	for(auto rel : offsets)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, file_offset + 3 + 2 + 2 * i, 6);
		relocation_entry.AddField("Offset", Dumper::HexDisplay::Make(2), offset_t(rel));
		relocation_entry.Display(dump);
		i ++;
	}
}

uint16_t CMDFormat::UnknownBlock::GetLength() const
{
	return image->ImageSize();
}

void CMDFormat::UnknownBlock::ReadFile(Linker::Reader& rd, uint16_t length)
{
	image = Linker::Buffer::ReadFromFile(rd, length);
}

void CMDFormat::UnknownBlock::WriteFile(Linker::Writer& wr) const
{
	MemoryBlock::WriteFile(wr);
	image->WriteFile(wr);
}

std::unique_ptr<Dumper::Region> CMDFormat::UnknownBlock::MakeRegion(std::string name, offset_t offset, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset, image->AsImage(), 0, display_width, 4, 4);
}

void CMDFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.Seek(1); // first byte should be 0x02
	file_header_size = rd.ReadUnsigned(2);
	rd.Skip(4); // should be "TLOC"
	rd.ReadData(linker_version);
	type = file_type(rd.ReadUnsigned(1));
	rd.Skip(2); // unknown
	entry_point = rd.ReadUnsigned(3);
	stack_size = rd.ReadUnsigned(2);
	rd.Skip(44); // unknown
	allocation_length = rd.ReadUnsigned(2);
	rd.Seek(3 + file_header_size);
	while(true)
	{
		blocks.push_back(MemoryBlock::ReadFile(rd));
		if(blocks.back()->type == MemoryBlock::TYPE_END)
			break;
	}
}

offset_t CMDFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.Seek(0);
	wr.WriteWord(1, 0x02);
	wr.WriteWord(2, file_header_size);
	wr.WriteData("TLOC");
	wr.WriteData(linker_version);
	wr.WriteWord(1, type);
	wr.Skip(2); // unknown
	wr.WriteWord(3, entry_point);
	wr.WriteWord(2, stack_size);
	wr.Skip(44); // unknown
	wr.WriteWord(2, allocation_length);
	wr.Seek(3 + file_header_size);
	for(auto& block: blocks)
	{
		block->WriteFile(wr);
	}

	return offset_t(-1);
}

void CMDFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PCOS cmd/sav format");
	Dumper::Region file_region("File", file_offset, 0, 6); // TODO: get size
	file_region.AddField("Header size", Dumper::HexDisplay::Make(4), offset_t(file_header_size));
	file_region.AddField("Linker version", Dumper::StringDisplay::Make("'"), std::string(std::begin(linker_version), std::end(linker_version)));

	std::map<offset_t, std::string> file_type_descriptions;
	file_type_descriptions[TYPE_CMD] = "CMD file";
	file_type_descriptions[TYPE_SAV] = "SAV file";
	file_region.AddField("Type", Dumper::ChoiceDisplay::Make(file_type_descriptions), offset_t(type));

	file_region.AddField("Entry point", Dumper::HexDisplay::Make(6), offset_t(entry_point));
	file_region.AddField("Stack size", Dumper::HexDisplay::Make(4), offset_t(stack_size));
	file_region.AddField("Allocation size", Dumper::HexDisplay::Make(4), offset_t(allocation_length));

	file_region.Display(dump);

	offset_t file_offset = 3 + file_header_size;

	for(auto& block : blocks)
	{
		block->Dump(dump, file_offset, *this);
		file_offset += 3 + block->GetLength();
	}
}

void CMDFormat::CalculateValues()
{
	// TODO
}

std::string CMDFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + (type == TYPE_SAV ? ".sav" : ".cmd");
}

