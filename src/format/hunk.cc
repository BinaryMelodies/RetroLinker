
#include "hunk.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace Amiga;

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

void HunkFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

std::shared_ptr<HunkFormat::Block> HunkFormat::Block::ReadBlock(Linker::Reader& rd)
{
	uint32_t type = rd.ReadUnsigned(4);
	std::shared_ptr<Block> block = nullptr;
	switch(type)
	{
	case HUNK_HEADER:
		block = std::make_shared<HeaderBlock>();
		break;
	// TODO
	}
	block->Read(rd);
	return block;
}

void HunkFormat::Block::Read(Linker::Reader& rd)
{
}

void HunkFormat::Block::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
}

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
	ReadBody(rd, longword_count);
}

void HunkFormat::InitialHunkBlock::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, type);
	wr.WriteWord(4, GetSizeField());
	if(RequiresAdditionalFlags())
		wr.WriteWord(4, GetAdditionalFlags()); // TODO: this is not tested
	WriteBody(wr);
}

void HunkFormat::InitialHunkBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
}

void HunkFormat::InitialHunkBlock::WriteBody(Linker::Writer& wr) const
{
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

uint32_t HunkFormat::BssBlock::GetSize() const
{
	return size;
}

void HunkFormat::BssBlock::ReadBody(Linker::Reader& rd, uint32_t longword_count)
{
	size = longword_count;
}

void HunkFormat::RelocationBlock::Read(Linker::Reader& rd)
{
	while(true)
	{
		uint32_t relocation_count = rd.ReadUnsigned(4);
		if(relocation_count == 0)
			break;

		RelocationData data;
		data.hunk = rd.ReadUnsigned(4);
		for(uint32_t i = 0; i < relocation_count; i++)
		{
			data.offsets.push_back(rd.ReadUnsigned(4));
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
		assert(image->data_size == 0);
		std::shared_ptr<BssBlock> bss = std::make_shared<BssBlock>((image->TotalSize() + 3) / 4, flags);
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

void HunkFormat::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */

	linker_parameters["fast_memory_flag"] = FastMemory;
	linker_parameters["chip_memory_flag"] = ChipMemory;
}

void HunkFormat::AddHunk(const Hunk& hunk)
{
	hunks.push_back(hunk);
	segment_index[hunks.back().image] = hunks.size() - 1;
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
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
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

