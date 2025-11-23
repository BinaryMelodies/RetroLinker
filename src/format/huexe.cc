
#include "huexe.h"
#include "../linker/options.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace X68000;

void HUFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

static Linker::OptionDescription<offset_t> p_base_address("base_address", "Starting address of binary image");

std::vector<Linker::OptionDescription<void> *> HUFormat::ParameterNames =
{
	&p_base_address,
};

std::vector<Linker::OptionDescription<void> *> HUFormat::GetLinkerScriptParameterNames()
{
	return ParameterNames;
}

void HUFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		code = segment;
	}
	else if(segment->name == ".data")
	{
		data = segment;
	}
	else if(segment->name == ".bss")
	{
		bss = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

void HUFormat::CreateDefaultSegments()
{
	if(code == nullptr)
	{
		code = std::make_shared<Linker::Segment>(".code");
	}
	if(data == nullptr)
	{
		data = std::make_shared<Linker::Segment>(".data");
	}
	if(bss == nullptr)
	{
		bss = std::make_shared<Linker::Segment>(".bss");
	}
}

std::unique_ptr<Script::List> HUFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?base_address?;
	all not write align 2;
	align 2;
};

".data"
{
	all not zero align 2;
	align 2;
};

".bss"
{
	all align 2;
	align 2;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void HUFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
	Linker::Debug << "Debug: " << linker_parameters["base_address"] << ", " << code->base_address << std::endl;
}

void HUFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr && resolution.reference == nullptr)
		{
			if(option_no_relocation)
			{
				Linker::Error << "Error: relocations suppressed, generating image anyway" << std::endl;
			}
			else
			{
				/* we don't need to record inter-segment distances */
				if(rel.size != 2 && rel.size != 4)
				{
					Linker::Error << "Error: Format only support word and longword relocations: " << rel << std::endl;
				}
				uint32_t offset = rel.source.GetPosition().address - code->base_address;
				if((offset & 1))
					Linker::Error << "Error: Illegal relocation offset" << std::endl;
				relocations[offset] = rel.size;
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
		entry_address = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void HUFormat::CalculateValues()
{
	relocation_size = 2 * relocations.size();
	uint32_t last_relocation = 0;
	for(auto it : relocations)
	{
		uint32_t offset = it.first - last_relocation;
		if(offset > 0xFFFF)
		{
			relocation_size += 4;
		}
		last_relocation = offset;
	}
}

offset_t HUFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(3, "HU\0");
	wr.WriteWord(1, load_mode);
	wr.WriteWord(4, code->base_address);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, code->data_size);
	wr.WriteWord(4, data->data_size);
	wr.WriteWord(4, bss->zero_fill);
	wr.WriteWord(4, relocation_size);
	wr.WriteWord(4, 0); /* symbol table */
	wr.Seek(0x40);
	code->WriteFile(wr);
	data->WriteFile(wr);

	uint32_t last_relocation = 0;
	for(auto it : relocations)
	{
		uint32_t offset = it.first - last_relocation;
		if((offset & 1))
			Linker::Error << "Error: Illegal relocation offset" << std::endl;
		if(it.second == 2)
			offset |= 1;
		else
			offset &= ~1;
		if(offset > 0xFFFF)
		{
			wr.WriteWord(2, 1);
			wr.WriteWord(4, offset);
		}
		else
		{
			wr.WriteWord(2, offset);
		}
		last_relocation = it.first;
	}

	return offset_t(-1);
}

void HUFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("HU format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void HUFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string HUFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".x";
}

