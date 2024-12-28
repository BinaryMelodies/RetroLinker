
#include "minix.h"

using namespace MINIX;

void MINIXFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

bool MINIXFormat::FormatIs16bit() const
{
	/* TODO: only for I86, test it */
	return true;
}

::EndianType MINIXFormat::GetEndianType(cpu_type cpu)
{
	switch(cpu & 3)
	{
	case 0:
		return ::LittleEndian;
	case 3:
		return ::BigEndian;
	case 1:
	case 2:
		/* TODO: other bits? */
		return ::PDP11Endian;
	default:
		assert(false);
	}
}

::EndianType MINIXFormat::GetEndianType() const
{
	return GetEndianType(cpu);
}

void MINIXFormat::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */
}

void MINIXFormat::OnNewSegment(Linker::Segment * segment)
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
	else if(segment->name == ".heap")
	{
		/* TODO */
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

void MINIXFormat::CreateDefaultSegments()
{
	if(code == nullptr)
	{
		code = new Linker::Segment(".code");
	}
	if(data == nullptr)
	{
		data = new Linker::Segment(".data");
	}
	if(bss == nullptr)
	{
		bss = new Linker::Segment(".bss");
	}
}

Script::List * MINIXFormat::GetScript(Linker::Module& module)
{
	static const char * SplitScript = R"(
".code"
{
	at ?code_base_address?;
	all exec align 4;
	align 16; # TODO: needed?
};

".data"
{
	at max(here, ?code_base_address?);
	base here;
	all not zero align 4;
};

".bss"
{
	all not ".heap"
		align 4;
};

".heap"
{
	all align 4;
};
)";

	static const char * CombinedScript = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align 4;
};

".data"
{
	all not zero align 4;
};

".bss"
{
	all not ".heap" align 4;
};

".heap"
{
	all align 4;
};
)";

	/* TODO: code_base_address */

	if(linker_script != "")
	{
		return LinkerManager::GetScript(module);
	}
	else if((format & FormatSeparate))
	{
		return Script::parse_string(SplitScript);
	}
	else
	{
		return Script::parse_string(CombinedScript);
	}
}

void MINIXFormat::Link(Linker::Module& module)
{
	Script::List * script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();

	/* TODO: heap */
}

void MINIXFormat::ProcessModule(Linker::Module& module)
{
#if 0
	/* TODO: utilize */
	if(code_base_address == 0 && (format & UnmappedZeroPage))
	{
		code.base_address = PAGE_SIZE;
		data.base_address = PAGE_SIZE; /* gets overwritten for tiny model */
	}
	else
	{
		code.base_address = code_base_address;
	}
#endif

	/* TODO: linker_parameters["code_base_address"] and linker_parameters["data_base_address"] */

	Link(module);

	uint32_t end;
	end = bss->GetEndAddress();
	/* TODO: This is how ld86 does it, maybe make it customizable? */
	if(heap_top_address < end + 0x0100)
		heap_top_address = end + 0x8000;
	if(heap_top_address > 0x10000)
		heap_top_address = 0x10000;

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
//			Linker::Debug << "Debug: " << rel << std::endl;
//			Linker::Debug << "Debug: " << resolution << std::endl;
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr && resolution.reference != nullptr)
		{
			Linker::Error << "Error: inter-segment differences are not supported, generating image anyway (generate with relocations disable)" << std::endl;
		}
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	entry_address = format & UnmappedZeroPage ? PAGE_SIZE : 0;

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		if(position.address != entry_address || position.segment != code)
		{
			Linker::Error << "Error: entry point must be beginning of .code segment, ignoring" << std::endl;
		}
	}
}

void MINIXFormat::CalculateValues()
{
}

/* ld86
code.total_size
	etextpadoff - code.base_address
data.total_size
	edataoffset - bdataoffset
bss.total_size
	endoffset - edataoffset
entry
	0 | 4096
total_memory
	heap_top_value
*/

void MINIXFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = GetEndianType();
	wr.WriteData(2, "\x01\x03\?\?");
	wr.WriteWord(1, format);
	wr.WriteWord(1, cpu);
	wr.WriteWord(1, 0x20);
	wr.Skip(3);
	wr.WriteWord(4, code->data_size);
	wr.WriteWord(4, data->data_size);
	wr.WriteWord(4, bss->zero_fill);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, heap_top_address); /* total memory */
	wr.WriteWord(4, 0); /* symbol table */
	code->WriteFile(wr);
	data->WriteFile(wr);
}

void MINIXFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	switch(module.cpu)
	{
	case Linker::Module::I86:
		cpu = I86;
		break;
	case Linker::Module::M68K:
		cpu = M68K;
		break;
	default:
		Linker::Error << "Error: Unsupported architecture" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string MINIXFormat::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

