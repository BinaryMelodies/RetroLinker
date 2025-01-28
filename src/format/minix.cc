
#include "minix.h"
#include <sstream>

using namespace MINIX;

MINIXFormat::relocation MINIXFormat::relocation::Read(Linker::Reader& rd)
{
	relocation rel;
	rel.address = rd.ReadUnsigned(4);
	rel.symbol = rd.ReadUnsigned(2);
	rel.type = rd.ReadUnsigned(2);
	return rel;
}

void MINIXFormat::relocation::Write(Linker::Writer& wr)
{
	wr.WriteWord(4, address);
	wr.WriteWord(2, symbol);
	wr.WriteWord(2, type);
}

void MINIXFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::EndianType(0); // should not matter
	rd.Skip(2); // signature: \x01\x03
	format = format_type(rd.ReadUnsigned(1));
	cpu = cpu_type(rd.ReadUnsigned(1));
	// the cpu field can be used to determine the endianness
	rd.endiantype = GetEndianType();
	header_size = rd.ReadUnsigned(1);
	rd.Skip(1);
	format_version = rd.ReadUnsigned(2);

	uint32_t code_size;
	uint32_t data_size;
	uint32_t far_code_size = 0;
	switch(format_version)
	{
	case 0:
		code_size = rd.ReadUnsigned(4);
		data_size = rd.ReadUnsigned(4);
		bss_size = rd.ReadUnsigned(4);
		entry_address = rd.ReadUnsigned(4);
		heap_top_address = rd.ReadUnsigned(4);
		symbol_table_offset = rd.ReadUnsigned(4);
		break;
	case 1:
		code_size = rd.ReadUnsigned(2);
		rd.Skip(2);
		data_size = rd.ReadUnsigned(2);
		rd.Skip(2);
		entry_address = rd.ReadUnsigned(2);
		rd.Skip(2);
		entry_address = rd.ReadUnsigned(4);
		heap_size = rd.ReadUnsigned(2);
		stack_size = rd.ReadUnsigned(2);
		symbol_table_offset = rd.ReadUnsigned(4);
		break;
	default:
		{
			std::ostringstream oss;
			oss << "Fatal error: unknown format version " << format_version;
			Linker::FatalError(oss.str());
		}
	}
	uint32_t code_relocation_size = 0;
	uint32_t data_relocation_size = 0;
	uint32_t far_code_relocation_size = 0;
	if(header_size >= 0x30)
	{
		code_relocation_size = rd.ReadUnsigned(4);
		data_relocation_size = rd.ReadUnsigned(4);
		code_relocation_base = rd.ReadUnsigned(4);
		data_relocation_base = rd.ReadUnsigned(4);
		if(header_size >= 0x40)
		{
			far_code_size = rd.ReadUnsigned(2);
			rd.Skip(2);
			far_code_relocation_size = rd.ReadUnsigned(4);
		}
	}
	rd.Seek(header_size);

	if(code_size != 0)
	{
		code = Linker::Buffer::ReadFromFile(rd, code_size);
	}
	if(far_code_size != 0)
	{
		far_code = Linker::Buffer::ReadFromFile(rd, far_code_size);
	}
	if(data_size != 0)
	{
		data = Linker::Buffer::ReadFromFile(rd, data_size);
	}

	uint32_t i;
	for(i = 0; i < code_relocation_size; i += 8)
	{
		code_relocations.emplace_back(relocation::Read(rd));
	}
	for(i = 0; i < far_code_relocation_size; i += 8)
	{
		far_code_relocations.emplace_back(relocation::Read(rd));
	}
	for(i = 0; i < data_relocation_size; i += 8)
	{
		data_relocations.emplace_back(relocation::Read(rd));
	}
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

void MINIXFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
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

std::unique_ptr<Script::List> MINIXFormat::GetScript(Linker::Module& module)
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
	at max(here, ?data_base_address?);
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
	std::unique_ptr<Script::List> script = GetScript(module);

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

	bss_size = bss->zero_fill;

#if 0
	for(auto section : code->sections)
	{
		Linker::Debug << "Code segment: " << section->name << std::endl;
	}
	for(auto section : data->sections)
	{
		Linker::Debug << "Data segment: " << section->name << std::endl;
	}
	for(auto section : bss->sections)
	{
		Linker::Debug << "Bss  segment: " << section->name << std::endl;
	}
#endif
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

offset_t MINIXFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = GetEndianType();
	wr.WriteData(2, "\x01\x03");
	wr.WriteWord(1, format);
	wr.WriteWord(1, cpu);
	wr.WriteWord(1, header_size);
	wr.Skip(1);
	wr.WriteWord(2, format_version);
	switch(format_version)
	{
	case 0:
		wr.WriteWord(4, code->ImageSize());
		wr.WriteWord(4, data->ImageSize());
		wr.WriteWord(4, bss_size);
		wr.WriteWord(4, entry_address);
		wr.WriteWord(4, heap_top_address); /* total memory */
		wr.WriteWord(4, 0); /* symbol table */
		break;
	case 1:
		wr.WriteWord(2, code->ImageSize());
		wr.Skip(2);
		wr.WriteWord(2, data->ImageSize());
		wr.Skip(2);
		wr.WriteWord(4, bss_size);
		wr.Skip(2);
		wr.WriteWord(4, entry_address);
		wr.WriteWord(2, heap_size);
		wr.WriteWord(2, stack_size);
		wr.WriteWord(4, 0); /* symbol table */
		break;
	}
	if(header_size >= 0x30)
	{
		wr.WriteWord(4, code_relocations.size() * 8);
		wr.WriteWord(4, data_relocations.size() * 8);
		wr.WriteWord(4, code_relocation_base);
		wr.WriteWord(4, data_relocation_base);
		if(header_size >= 0x40)
		{
			wr.WriteWord(2, far_code->ImageSize());
			wr.Skip(2);
			wr.WriteWord(4, far_code_relocations.size() * 8);
		}
	}
	wr.Seek(header_size);
	if(code != nullptr)
	{
		code->WriteFile(wr);
	}
	if(far_code != nullptr)
	{
		far_code->WriteFile(wr);
	}
	if(data != nullptr)
	{
		data->WriteFile(wr);
	}

	for(auto& rel : code_relocations)
	{
		rel.Write(wr);
	}
	for(auto& rel : far_code_relocations)
	{
		rel.Write(wr);
	}
	for(auto& rel : data_relocations)
	{
		rel.Write(wr);
	}

	return offset_t(-1);
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

