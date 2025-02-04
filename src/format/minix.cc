
#include <sstream>
#include "minix.h"
#include "../linker/buffer.h"
#include "../linker/position.h"
#include "../linker/resolution.h"
#include "../linker/reader.h"

using namespace MINIX;

MINIXFormat::Relocation MINIXFormat::Relocation::Read(Linker::Reader& rd)
{
	Relocation rel;
	rel.address = rd.ReadUnsigned(4);
	rel.symbol = rd.ReadUnsigned(2);
	rel.type = rd.ReadUnsigned(2);
	return rel;
}

void MINIXFormat::Relocation::FetchSymbolName(std::vector<Symbol>& symbols)
{
	if(symbol < symbols.size())
	{
		symbol_name = symbols[symbol].name;
	}
}

void MINIXFormat::Relocation::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, address);
	wr.WriteWord(2, symbol);
	wr.WriteWord(2, type);
}

void MINIXFormat::Relocation::Dump(Dumper::Dumper& dump, unsigned index, offset_t relocations_offset) const
{
	Dumper::Entry relocation_entry("Relocation", index + 1, relocations_offset + index * 8, 8);
	std::map<offset_t, std::string> relocation_descriptions;
	relocation_descriptions[R_ABBS] = "R_ABBS";
	relocation_descriptions[R_RELLBYTE] = "R_RELLBYTE";
	relocation_descriptions[R_PCRBYTE] = "R_PCRBYTE";
	relocation_descriptions[R_RELWORD] = "R_RELWORD";
	relocation_descriptions[R_PCRWORD] = "R_PCRWORD";
	relocation_descriptions[R_RELLONG] = "R_RELLONG";
	relocation_descriptions[R_PCRLONG] = "R_PCRLONG";
	relocation_descriptions[R_REL3BYTE] = "R_REL3BYTE";
	relocation_descriptions[R_KBRANCHE] = "R_KBRANCHE";
	relocation_descriptions[R_SEGWORD] = "R_SEGWORD";
	relocation_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_descriptions, Dumper::HexDisplay::Make(4)), offset_t(type));
	relocation_entry.AddField("Symbol number", Dumper::DecDisplay::Make(), offset_t(symbol));
	std::string name;
	switch(symbol)
	{
	case S_ABS:
		name = "absolute reference";
		break;
	case S_TEXT:
		name = "code segment";
		break;
	case S_DATA:
		name = "data segment";
		break;
	case S_BSS:
		name = "bss segment";
		break;
	case S_FTEXT:
		name = "far code segment";
		break;
	default:
		name = symbol_name;
	}
	relocation_entry.AddOptionalField("Symbol", Dumper::StringDisplay::Make(), name);
	relocation_entry.AddField("Address", Dumper::HexDisplay::Make(8), offset_t(address));
	relocation_entry.Display(dump);
}

size_t MINIXFormat::Relocation::GetSize() const
{
	switch(type)
	{
	case R_ABBS:
		return 0; // TODO
	case R_RELLBYTE:
	case R_PCRBYTE:
		return 1;
	case R_RELWORD:
	case R_PCRWORD:
	case R_SEGWORD:
		return 2;
	case R_RELLONG:
	case R_PCRLONG:
		return 4;
	case R_REL3BYTE:
		return 3;
	case R_KBRANCHE:
		return 0; // TODO
	default:
		return 0;
	}
}

MINIXFormat::Symbol MINIXFormat::Symbol::Read(Linker::Reader& rd)
{
	Symbol sym;
	sym.name = rd.ReadData(8);
	sym.name.erase(sym.name.find_last_not_of(' ') + 1);
	sym.value = rd.ReadSigned(4);
	sym.sclass = rd.ReadUnsigned(1);
	sym.numaux = rd.ReadUnsigned(1);
	sym.type = rd.ReadUnsigned(2);
	return sym;
}

void MINIXFormat::Symbol::Write(Linker::Writer& wr) const
{
	wr.WriteData(8, name, '\0');
	wr.WriteWord(4, value);
	wr.WriteWord(1, sclass);
	wr.WriteWord(1, numaux);
	wr.WriteWord(2, type);
}

void MINIXFormat::Symbol::Dump(Dumper::Dumper& dump, unsigned index, offset_t relocations_offset) const
{
	Dumper::Entry symbol_entry("Symbol", index, relocations_offset + index * 8, 8);
	symbol_entry.AddField("Name", Dumper::StringDisplay::Make(), name);
	symbol_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(value));
	std::map<offset_t, std::string> section_descriptions;
	section_descriptions[N_UNDF] = "undefined";
	section_descriptions[N_ABS] = "absolute";
	section_descriptions[N_TEXT] = "code";
	section_descriptions[N_DATA] = "data";
	section_descriptions[N_BSS] = "bss";
	section_descriptions[N_COMM] = "common";
	symbol_entry.AddField("Section", Dumper::ChoiceDisplay::Make(section_descriptions, Dumper::HexDisplay::Make(2)), offset_t(sclass & N_SECT));
	std::map<offset_t, std::string> sclass_descriptions;
	sclass_descriptions[S_NULL] = "none";
	sclass_descriptions[S_EXT] = "external";
	sclass_descriptions[S_STAT] = "static";
	symbol_entry.AddField("Storage class", Dumper::ChoiceDisplay::Make(section_descriptions, Dumper::HexDisplay::Make(2)), offset_t(sclass & N_CLASS));
	symbol_entry.Display(dump);
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
	uint32_t symbol_count = 0;
	switch(format_version)
	{
	case 0:
		code_size = rd.ReadUnsigned(4);
		data_size = rd.ReadUnsigned(4);
		bss_size = rd.ReadUnsigned(4);
		entry_address = rd.ReadUnsigned(4);
		total_memory = rd.ReadUnsigned(4);
		symbol_count = rd.ReadUnsigned(4);
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
		symbol_count = rd.ReadUnsigned(4);
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
			if(format_version == 0)
			{
				far_code_size = rd.ReadUnsigned(4);
			}
			else
			{
				far_code_size = rd.ReadUnsigned(2);
				rd.Skip(2);
			}
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
		code_relocations.emplace_back(Relocation::Read(rd));
	}
	for(i = 0; i < far_code_relocation_size; i += 8)
	{
		far_code_relocations.emplace_back(Relocation::Read(rd));
	}
	for(i = 0; i < data_relocation_size; i += 8)
	{
		data_relocations.emplace_back(Relocation::Read(rd));
	}
	for(i = 0; i < symbol_count; i += 16)
	{
		symbols.emplace_back(Symbol::Read(rd));
	}

	for(auto& rel : code_relocations)
	{
		rel.FetchSymbolName(symbols);
	}
	for(auto& rel : far_code_relocations)
	{
		rel.FetchSymbolName(symbols);
	}
	for(auto& rel : data_relocations)
	{
		rel.FetchSymbolName(symbols);
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
	if(auto total_memory_option = FetchIntegerOption(options, "total_memory"))
	{
		if(format_version == uint16_t(-1))
		{
			format_version = 0;
		}

		if(format_version != 0)
		{
			Linker::Error << "Error: total memory only supported in file format version 0, ignoring" << std::endl;
		}
		else
		{
			total_memory = total_memory_option.value();
		}
	}

	if(auto stack_size_option = FetchIntegerOption(options, "stack_size"))
	{
		if(format_version == uint16_t(-1))
		{
			format_version = 1;
		}

		if(format_version != 1)
		{
			Linker::Error << "Error: stack size option only supported in file format version 1, ignoring" << std::endl;
		}
		else
		{
			stack_size = stack_size_option.value();
		}
	}

	if(auto heap_size_option = FetchIntegerOption(options, "heap_size"))
	{
		if(format_version == uint16_t(-1))
		{
			format_version = 1;
		}

		if(format_version != 1)
		{
			Linker::Error << "Error: heap size option only supported in file format version 1, ignoring" << std::endl;
		}
		else
		{
			heap_size = heap_size_option.value();
		}
	}
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
		heap = segment;
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

void MINIXFormat::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	SegmentManager::SetLinkScript(script_file, options);

	if((format & UnmappedZeroPage) != 0)
	{
		// TODO: untested
		if(linker_parameters.find("code_base_address") == linker_parameters.end())
		{
			linker_parameters["code_base_address"] = PAGE_SIZE;
		}
		if(linker_parameters.find("data_base_address") == linker_parameters.end())
		{
			/* gets overwritten for tiny model */
			linker_parameters["data_base_address"] = PAGE_SIZE;
		}
	}
}

std::unique_ptr<Script::List> MINIXFormat::GetScript(Linker::Module& module)
{
	// TODO: stack not in link script

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
		return SegmentManager::GetScript(module);
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
}

void MINIXFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	// TODO: set format version according to the given values
	if(format_version == uint16_t(-1))
	{
		format_version = 0;
	}

	switch(format_version)
	{
	case 0:
		{
			// TODO: heap segment untested
			uint32_t end = heap == nullptr ? bss->GetEndAddress() : heap->GetEndAddress();
			if(end > 0x10000)
			{
				Linker::Error << "Error: generated image exceeds 64 KiB" << std::endl;
			}
			/* This is how ld86 does it */
			if((heap == nullptr || heap->zero_fill == 0)
			&& total_memory < end + 0x0100)
				total_memory = end + 0x8000;
			if(total_memory > 0x10000)
				total_memory = 0x10000;
		}
		break;
	case 1:
		// TODO: untested
		if(heap != nullptr)
		{
			heap_size = heap->zero_fill;
		}
		if(stack != nullptr)
		{
			stack_size = stack->zero_fill;
		}
		break;
	}

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
		else if(enable_relocations && rel.segment_of && resolution.target != nullptr)
		{
			Relocation exe_rel;
			exe_rel.type = Relocation::R_SEGWORD;

			if(resolution.target == code)
				exe_rel.symbol = Relocation::S_TEXT;
			else if(resolution.target == far_code)
				exe_rel.symbol = Relocation::S_FTEXT;
			else // data, bss, etc.
				exe_rel.symbol = Relocation::S_DATA;

			Linker::Position source = rel.source.GetPosition();
			auto& segment = source.segment;
			exe_rel.address = source.address;
			if(segment == code)
			{
				exe_rel.address -= dynamic_cast<Linker::Segment *>(code.get())->GetStartAddress();
				code_relocations.emplace_back(exe_rel);
			}
			else if(segment == far_code)
			{
				exe_rel.address -= dynamic_cast<Linker::Segment *>(far_code.get())->GetStartAddress();
				far_code_relocations.emplace_back(exe_rel);
			}
			else // data, bss, etc.
			{
				exe_rel.address -= dynamic_cast<Linker::Segment *>(data.get())->GetStartAddress();
				data_relocations.emplace_back(exe_rel);
			}
		}
	}

	if(enable_symbols)
	{
		for(auto& mention : module.symbol_sequence)
		{
			Symbol symbol = { };
			symbol.name = mention.name;
			Linker::Position position;
			// TODO: rewrite
			switch(mention.binding)
			{
			case Linker::SymbolDefinition::Undefined:
				symbol.sclass = Symbol::S_EXT;
				break;
			case Linker::SymbolDefinition::Local:
				symbol.sclass = Symbol::S_STAT;
				position = mention.location.GetPosition();
				symbol.value = position.address;
				if(position.segment == nullptr)
				{
					symbol.sclass = Symbol::N_ABS;
				}
				else if(position.segment == code || position.segment == far_code)
				{
					symbol.sclass = Symbol::N_TEXT | Symbol::S_STAT;
				}
				else if(position.segment == data)
				{
					symbol.sclass = Symbol::N_DATA | Symbol::S_STAT;
				}
				else
				{
					symbol.sclass = Symbol::N_BSS | Symbol::S_STAT;
				}
				break;
			case Linker::SymbolDefinition::Global:
				position = mention.location.GetPosition();
				symbol.value = position.address;
				if(position.segment == nullptr)
				{
					symbol.sclass = Symbol::N_ABS;
				}
				else if(position.segment == code || position.segment == far_code)
				{
					symbol.sclass = Symbol::N_TEXT | Symbol::S_EXT;
				}
				else if(position.segment == data)
				{
					symbol.sclass = Symbol::N_DATA | Symbol::S_EXT;
				}
				else
				{
					symbol.sclass = Symbol::N_BSS | Symbol::S_EXT;
				}
				break;
			case Linker::SymbolDefinition::Weak:
				position = mention.location.GetPosition();
				symbol.value = position.address;
				if(position.segment == nullptr)
				{
					symbol.sclass = Symbol::N_ABS;
				}
				else if(position.segment == code || position.segment == far_code)
				{
					symbol.sclass = Symbol::N_TEXT | Symbol::S_EXT;
				}
				else if(position.segment == data)
				{
					symbol.sclass = Symbol::N_DATA | Symbol::S_EXT;
				}
				else
				{
					symbol.sclass = Symbol::N_BSS | Symbol::S_EXT;
				}
				break;
			case Linker::SymbolDefinition::Common:
			case Linker::SymbolDefinition::LocalCommon:
				Linker::Error << "Internal error: " << mention.name << " should not be common" << std::endl;
				break;
#if 0
				position = mention.location.GetPosition();
				symbol.value = position.address;
				if(position.segment == nullptr)
				{
					symbol.sclass = Symbol::N_ABS;
				}
				else if(position.segment == code || position.segment == far_code)
				{
					symbol.sclass = Symbol::N_TEXT | Symbol::S_EXT;
				}
				else if(position.segment == data)
				{
					symbol.sclass = Symbol::N_DATA | Symbol::S_EXT;
				}
				else
				{
					symbol.sclass = Symbol::N_BSS | Symbol::S_EXT;
				}
				break;
#endif
			}
			symbols.push_back(symbol);
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

	uint8_t minimum_header_size = 0x20;
	if(far_code != nullptr && far_code->ImageSize() > 0)
	{
		minimum_header_size = 0x40;
	}
	else if(code_relocations.size() > 0 || data_relocations.size() > 0)
	{
		minimum_header_size = 0x30;
	}

	if(header_size < minimum_header_size)
	{
		header_size = minimum_header_size;
	}

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

offset_t MINIXFormat::ImageSize() const
{
	return header_size
		+ (code != nullptr ? code->ImageSize() : 0)
		+ (far_code != nullptr ? far_code->ImageSize() : 0)
		+ (data != nullptr ? data->ImageSize() : 0)
		+ code_relocations.size() * 8
		+ far_code_relocations.size() * 8
		+ data_relocations.size() * 8
		+ symbols.size() * 16;
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

offset_t MINIXFormat::WriteFile(Linker::Writer& wr) const
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
		wr.WriteWord(4, total_memory); /* total memory */
		wr.WriteWord(4, symbols.size() * 16);
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
		wr.WriteWord(4, symbols.size() * 16);
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
	for(auto& sym : symbols)
	{
		sym.Write(wr);
	}

	return ImageSize();
}

void MINIXFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("MINIX format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("unmapped zero"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("page aligned"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("new-style symbol table"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("combined executable"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("split executable"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("pure text"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("text overlay"), true),
		offset_t(format));

	std::map<offset_t, std::string> endian_descriptions;
	endian_descriptions[0] = "little endian";
	endian_descriptions[1] = "reversed PDP-11 endian";
	endian_descriptions[2] = "PDP-11 endian";
	endian_descriptions[3] = "big endian";

	std::map<offset_t, std::string> cpu_descriptions;
	cpu_descriptions[I86 >> 2] = "Intel 8086";
	cpu_descriptions[M68K >> 2] = "Motorola 680xx";
	cpu_descriptions[NS16K >> 2] = "NS320xx";
	cpu_descriptions[I386 >> 2] = "Intel 80386";
	cpu_descriptions[SPARC >> 2] = "Sun SPARC";

	file_region.AddField("CPU",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0, 2, Dumper::ChoiceDisplay::Make(endian_descriptions), false)
			->AddBitField(2, 6, Dumper::ChoiceDisplay::Make(cpu_descriptions), false),
		offset_t(cpu));

	file_region.AddField("Header size", Dumper::HexDisplay::Make(2), offset_t(header_size));
	file_region.AddField("Header format version", Dumper::DecDisplay::Make(), offset_t(format_version));
	file_region.AddField("Entry", Dumper::HexDisplay::Make(8), offset_t(entry_address));

	switch(format_version)
	{
	case 0:
		file_region.AddOptionalField("Total memory", Dumper::HexDisplay::Make(8), offset_t(total_memory));
		break;
	case 1:
		file_region.AddOptionalField("Heap size", Dumper::HexDisplay::Make(8), offset_t(heap_size));
		file_region.AddOptionalField("Stack size", Dumper::HexDisplay::Make(8), offset_t(stack_size));
		break;
	}

	if(code_relocations.size() != 0)
	{
		file_region.AddOptionalField("Code relocation base address", Dumper::HexDisplay::Make(8), offset_t(code_relocation_base));
	}
	/*if(far_code_relocations.size() != 0)
	{
		file_region.AddOptionalField("Far code relocation base address", Dumper::HexDisplay::Make(8), offset_t(far_code_relocation_base));
	}*/
	if(data_relocations.size() != 0)
	{
		file_region.AddOptionalField("Data relocation base address", Dumper::HexDisplay::Make(8), offset_t(data_relocation_base));
	}

	file_region.Display(dump);

	offset_t current_offset = header_size;

	Dumper::Block code_block("Code", current_offset, code->AsImage(), 0 /* TODO: what is the base address? */, 8);
	for(auto& rel : code_relocations)
	{
		size_t size = rel.GetSize();
		if(size != 0)
			code_block.AddSignal(rel.address, 2);
	}
	code_block.Display(dump);
	current_offset += code->ImageSize();

	if(far_code != nullptr && far_code->ImageSize() > 0)
	{
		Dumper::Block far_code_block("Far code", current_offset, far_code->AsImage(), 0, 8);
		for(auto& rel : far_code_relocations)
		{
			size_t size = rel.GetSize();
			if(size != 0)
				far_code_block.AddSignal(rel.address, 2);
		}
		far_code_block.Display(dump);
		current_offset += far_code->ImageSize();
	}

	Dumper::Block data_block("Code", current_offset, data->AsImage(), 0 /* TODO: what is the base address? */ + ((format & FormatCombined) ? code->ImageSize() : 0), 8);
	for(auto& rel : data_relocations)
	{
		size_t size = rel.GetSize();
		if(size != 0)
			data_block.AddSignal(rel.address, 2);
	}
	data_block.Display(dump);
	current_offset += data->ImageSize();

	unsigned i = 0;
	for(auto& rel : code_relocations)
	{
		rel.Dump(dump, i, current_offset);
		i++;
	}
	current_offset += code_relocations.size() * 8;
	i = 0;
	for(auto& rel : far_code_relocations)
	{
		rel.Dump(dump, i, current_offset);
		i++;
	}
	current_offset += far_code_relocations.size() * 8;
	i = 0;
	for(auto& rel : data_relocations)
	{
		rel.Dump(dump, i, current_offset);
		i++;
	}
	current_offset += data_relocations.size() * 8;
	i = 0;
	for(auto& sym : symbols)
	{
		sym.Dump(dump, i, current_offset);
		i++;
	}
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

std::string MINIXFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

