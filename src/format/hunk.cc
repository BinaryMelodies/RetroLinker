
#include "hunk.h"

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

void HunkFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

uint32_t HunkFormat::Hunk::GetSizeField()
{
	uint32_t size = (image->TotalSize() + 3) >> 2;
	if(RequiresAdditionalFlags())
		return size | 0xC0000000;
	else
		return size | ((flags & ~LoadPublic) << 29);
}

bool HunkFormat::Hunk::RequiresAdditionalFlags()
{
	return (flags & ~(LoadChipMem | LoadFastMem | LoadPublic)) != 0;
}

uint32_t HunkFormat::Hunk::GetAdditionalFlags()
{
	return flags & ~LoadPublic;
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
	uint32_t hunk_type;
	unsigned hunk_flags = Hunk::LoadAny;
	if(!(flags & Linker::Section::Writable))
	{
		hunk_type = cpu;
	}
	else if(!(flags & Linker::Section::ZeroFilled))
	{
		hunk_type = HUNK_DATA;
	}
	else
	{
		hunk_type = HUNK_BSS;
	}

	if((flags & ChipMemory))
	{
		Linker::Debug << "Debug: Setting flags to chip memory" << std::endl;
		hunk_flags = Hunk::LoadChipMem;
	}
	else if((flags & FastMemory))
	{
		Linker::Debug << "Debug: Setting flags to fast memory" << std::endl;
		hunk_flags = Hunk::LoadFastMem;
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
		return LinkerManager::GetScript(module);
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
		cpu = HUNK_CODE;
		break;
	case Linker::Module::PPC:
		cpu = HUNK_PPC_CODE;
		break;
	default:
		assert(false);
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
}

void HunkFormat::CalculateValues()
{
}

void HunkFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteWord(4, HUNK_HEADER);
	wr.WriteWord(4, 0); /* no libraries */
	wr.WriteWord(4, hunks.size());
	wr.WriteWord(4, 0); /* first hunk: hunk #0 */
	wr.WriteWord(4, hunks.size() - 1); /* last hunk */
	for(Hunk& hunk : hunks)
	{
		wr.WriteWord(4, hunk.GetSizeField());
	}
	for(Hunk& hunk : hunks)
	{
		/* Initial hunk block */
		wr.WriteWord(4, hunk.hunk_type);
		wr.WriteWord(4, hunk.GetSizeField());
		if(hunk.RequiresAdditionalFlags())
			wr.WriteWord(4, hunk.GetAdditionalFlags()); /* TODO: must be tested out */
		if(hunk.hunk_type != HUNK_BSS)
		{
			/* bss hunks do not contain data */
//					Linker::Debug << "Debug: No! " << hunk.image->total_size << ", " << hunk.image->data_size << std::endl;
			hunk.image->WriteFile(wr);
			int skip = (-hunk.image->data_size) & 3;
			if(skip != 0)
			{
				wr.Skip(skip);
			}
		}
		else
		{
			assert(hunk.image->data_size == 0);
		}
		if(hunk.relocations.size() != 0)
		{
			/* Relocations block */
			wr.WriteWord(4, HUNK_RELOC32);
			for(auto it : hunk.relocations)
			{
				wr.WriteWord(4, it.second.size());
				wr.WriteWord(4, it.first); /* hunk number */
				for(uint32_t relocation : it.second)
				{
					wr.WriteWord(4, relocation);
				}
			}
			wr.WriteWord(4, 0); /* terminator */
		}
		/* End block */
		wr.WriteWord(4, HUNK_END);
	}
}

void HunkFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string HunkFormat::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

