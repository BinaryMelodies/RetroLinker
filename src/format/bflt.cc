
#include "bflt.h"
#include "../linker/location.h"
#include "../linker/module.h"
#include "../linker/position.h"
#include "../linker/relocation.h"
#include "../linker/resolution.h"

using namespace BFLT;

void BFLTFormat::Clear()
{
	/* format fields */
	relocations.clear();
	code = nullptr;
	data = nullptr;
	/* writer fields */
	bss = nullptr;
	stack = nullptr;
	ClearSegmentManager();
}

void BFLTFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t BFLTFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = endian_type;
	wr.WriteData(4, "bFLT");
	wr.WriteWord(4, format_version);
	wr.WriteWord(4, entry);
	wr.WriteWord(4, data_offset);
	wr.WriteWord(4, bss_offset);
	wr.WriteWord(4, bss_end_offset);
	wr.WriteWord(4, stack_size);
	wr.WriteWord(4, relocation_offset);
	wr.WriteWord(4, relocations.size());
	wr.WriteWord(4, flags);

	wr.Seek(64);

	if(code != nullptr)
	{
		code->WriteFile(wr);
	}
	if(data != nullptr)
	{
		data->WriteFile(wr);
	}

	wr.Seek(relocation_offset);
	for(auto relocation : relocations)
	{
		if(format_version <= 2)
		{
			wr.WriteWord(4, (relocation.offset & 0x3FFFFFFF) | (relocation.type << 30));
		}
		else
		{
			wr.WriteWord(4, relocation.offset);
		}
	}

	return 0;
}

void BFLTFormat::Dump(Dumper::Dumper& dump) const
{
	/* TODO */
}

void BFLTFormat::CalculateValues()
{
	if(relocations.size() != 0)
		relocation_offset = bss_offset;
	else
		relocation_offset = 0;
	/* TODO */
}

/* * * Writer members * * */

void BFLTFormat::SetOptions(std::map<std::string, std::string>& options)
{
	if(options.find("ram") != options.end())
	{
		flags |= FLAG_RAM;
	}

	if(options.find("got") != options.end())
	{
		flags |= FLAG_GOTPIC;
		// TODO: not yet supported
	}
}

void BFLTFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
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
	else if(segment->name == ".stack")
	{
		stack = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

void BFLTFormat::CreateDefaultSegments()
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

void BFLTFormat::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	SegmentManager::SetLinkScript(script_file, options);

#if 0
	if(linker_parameters.find("code_base_address") == linker_parameters.end())
	{
		linker_parameters["code_base_address"] = 64; // after the header
	}
#endif
}

std::unique_ptr<Script::List> BFLTFormat::GetScript(Linker::Module& module)
{
	static const char * Script = R"(
".code"
{
	at 64;
	all exec align 4;
};

".data"
{
	at max(here, ?data_base_address?);
	base here;
	all not zero align 4;
};

".bss"
{
	all not ".stack"
		align 4;
};

".stack"
{
	all align 4;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(Script);
	}
}

void BFLTFormat::ProcessModule(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
		if(resolution.target != nullptr)
		{
			if(resolution.reference != nullptr)
			{
				Linker::Error << "Error: inter-segment differences are not supported, generating image anyway (generate with relocations disable)" << std::endl;
			}
			Relocation::relocation_type segment_num = Relocation::relocation_type(-1);
			if(resolution.target == code)
				segment_num = Relocation::Text;
			else if(resolution.target == data)
				segment_num = Relocation::Data;
			else if(resolution.target == bss)
				segment_num = Relocation::Bss;
			assert(segment_num != Relocation::relocation_type(-1));
			if(rel.size != 4)
			{
				Linker::Error << "Error: Format only supports 32-bit relocations: " << rel << ", ignoring" << std::endl;
			}
			else
			{
				relocations.push_back(Relocation { segment_num, rel.source.GetPosition().address });
				if(format_version <= 2)
					resolution.value -= resolution.target->base_address;
			}
		}
		rel.WriteWord(resolution.value);
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	Linker::Location _entry;
	if(module.FindGlobalSymbol(".entry", _entry))
	{
		entry = _entry.GetPosition().address;
	}

	data_offset = std::dynamic_pointer_cast<Linker::Segment>(data)->base_address;
	bss_offset = bss->base_address;
	bss_end_offset = bss_offset + bss->zero_fill;
	stack_size = stack ? stack->zero_fill : 0x200; // TODO: default stack size?
}

void BFLTFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	/* TODO: is the architecture important? */

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string BFLTFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

