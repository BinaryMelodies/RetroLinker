
#include "hunk.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace Amiga;

// Block

std::shared_ptr<HunkFormat::Block> HunkFormat::Block::ReadBlock(Linker::Reader& rd)
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
		block = std::make_shared<UnitBlock>();
		break;
	case HUNK_CODE:
	case HUNK_PPC_CODE:
	case HUNK_DATA:
		block = std::make_shared<LoadBlock>(block_type(type));
		break;
	case HUNK_BSS:
		block = std::make_shared<BssBlock>();
		break;
	case HUNK_RELOC32:
		block = std::make_shared<RelocationBlock>(block_type(type));
		break;
	case HUNK_END:
		block = std::make_shared<Block>(block_type(type));
		break;
	case HUNK_HEADER:
		block = std::make_shared<HeaderBlock>();
		break;
	// TODO
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

// UnitBlock

void HunkFormat::UnitBlock::Read(Linker::Reader& rd)
{
	name = HunkFormat::ReadString(rd);
}

void HunkFormat::UnitBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	HunkFormat::WriteString(wr, name);
}

offset_t HunkFormat::UnitBlock::FileSize() const
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
	return (flags & ~(LoadChipMem | LoadFastMem | LoadPublic)) != 0;
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

void HunkFormat::InitialHunkBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
}

void HunkFormat::InitialHunkBlock::WriteBody(Linker::Writer& wr) const
{
}

// LoadBlock

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

// RelocationBlock

void HunkFormat::RelocationBlock::Read(Linker::Reader& rd)
{
	relocations.clear();

	while(true)
	{
		uint32_t relocation_count = rd.ReadUnsigned(4);
		if(relocation_count == 0)
			break;

		relocations.emplace_back(RelocationData());
		relocations.back().hunk = rd.ReadUnsigned(4);
		for(uint32_t i = 0; i < relocation_count; i++)
		{
			relocations.back().offsets.push_back(rd.ReadUnsigned(4));
		}
	}
}

void HunkFormat::RelocationBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	for(auto& data : relocations)
	{
		// to make sure to avoid accidentally closing the list prematurely
		if(data.offsets.size() == 0)
			continue;

		wr.WriteWord(4, data.offsets.size());
		wr.WriteWord(4, data.hunk);
		for(auto offset : data.offsets)
		{
			wr.WriteWord(4, offset);
		}
	}
	wr.WriteWord(4, 0); /* terminator */
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

	if(relocations.size() != 0)
	{
		/* Relocation block */
		std::shared_ptr<RelocationBlock> relocation = std::make_shared<RelocationBlock>(Block::HUNK_RELOC32);
		for(auto it : relocations)
		{
			relocation->relocations.emplace_back(RelocationBlock::RelocationData(it.first));
			for(uint32_t rel : it.second)
			{
				relocation->relocations.back().offsets.push_back(rel);
			}
		}
		blocks.push_back(relocation);
	}

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
		// TODO
		break;
	case Block::HUNK_RELOC32:
		// TODO
		break;
	case Block::HUNK_END:
		// TODO
		break;
	default:
		Linker::Error << "Error: invalid block in hunk" << std::endl;
	}

	blocks.emplace_back(block);
}

// HunkFormat

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
		for(auto& block : hunk.blocks)
		{
			Linker::Debug << "Debug: size of " << std::hex << block->type << " is " << block->FileSize() << std::endl;
			size += block->FileSize();
		}
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
		block = Block::ReadBlock(rd);
	}

	for(; rd.Tell() < end && block != nullptr; block = Block::ReadBlock(rd))
	{
		Hunk hunk;
		hunk.AppendBlock(block);
		for(block = Block::ReadBlock(rd); block != nullptr; block = Block::ReadBlock(rd))
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

	// TODO
}

std::string HunkFormat::ReadString(Linker::Reader& rd, uint32_t& longword_count)
{
	longword_count = rd.ReadUnsigned(4);
	return rd.ReadData(longword_count * 4, '\0');
}

std::string HunkFormat::ReadString(Linker::Reader& rd)
{
	uint32_t tmp;
	return ReadString(rd, tmp);
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
			hunks[source].relocations[target].push_back(position.address);
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

