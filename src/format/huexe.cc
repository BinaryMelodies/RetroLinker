
#include "huexe.h"
#include "../linker/buffer.h"
#include "../linker/options.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace X68000;

void HUFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.Skip(3); // "HU\0"
	load_mode = load_mode_type(rd.ReadUnsigned(1));
	base_address = rd.ReadUnsigned(4);
	entry_address = rd.ReadUnsigned(4);
	code_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	relocation_size = rd.ReadUnsigned(4);
	symbol_table_size = rd.ReadUnsigned(4);
	debug_line_number_table_size = rd.ReadUnsigned(4);
	debug_symbol_table_size = rd.ReadUnsigned(4);
	debug_string_table_size = rd.ReadUnsigned(4);
	rd.Skip(0x10);
	bound_module_list_offset = rd.ReadUnsigned(4);

	code = Linker::Buffer::ReadFromFile(rd, code_size);
	data = Linker::Buffer::ReadFromFile(rd, data_size);

	uint32_t relocation_offset = 0;
	relocation_sequence.clear();
	while(relocation_offset < relocation_size)
	{
		Relocation relocation;
		uint32_t displacement = rd.ReadUnsigned(2);
		relocation_offset += 2;
		if(displacement == 0x0001)
		{
			relocation.absolute_displacement = true;
			relocation_offset += 4;
			if(relocation_offset > relocation_size)
			{
				Linker::Error << "Error: relocation section overflow" << std::endl;
				break;
			}
			displacement = rd.ReadUnsigned(4);
		}
		else
		{
			relocation.absolute_displacement = false;
		}
		relocation.is16bit = (displacement & 1) != 0;
		relocation.displacement = displacement & ~1;
		relocation_sequence.push_back(relocation);
	}

	relocations.clear();
	uint32_t last_relocation = 0;
	for(auto relocation : relocation_sequence)
	{
		if(relocation.absolute_displacement)
		{
			last_relocation = relocation.displacement;
		}
		else
		{
			last_relocation += relocation.displacement;
		}
		relocations[last_relocation] = relocation.is16bit ? 2 : 4;
	}
}

std::shared_ptr<Linker::Segment> HUFormat::GetCodeSegment()
{
	return std::static_pointer_cast<Linker::Segment>(code);
}

std::shared_ptr<const Linker::Segment> HUFormat::GetCodeSegment() const
{
	return std::static_pointer_cast<const Linker::Segment>(code);
}

std::shared_ptr<Linker::Segment> HUFormat::GetDataSegment()
{
	return std::static_pointer_cast<Linker::Segment>(data);
}

std::shared_ptr<const Linker::Segment> HUFormat::GetDataSegment() const
{
	return std::static_pointer_cast<const Linker::Segment>(data);
}

std::shared_ptr<Linker::Segment> HUFormat::GetBssSegment()
{
	return std::static_pointer_cast<Linker::Segment>(bss);
}

std::shared_ptr<const Linker::Segment> HUFormat::GetBssSegment() const
{
	return std::static_pointer_cast<const Linker::Segment>(bss);
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
	Linker::Debug << "Debug: " << linker_parameters["base_address"] << ", " << GetCodeSegment()->base_address << std::endl;
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
				uint32_t offset = rel.source.GetPosition().address - GetCodeSegment()->base_address;
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
	uint32_t last_relocation = 0;
	relocation_sequence.clear();
	for(auto it : relocations)
	{
		Relocation relocation;
		uint32_t offset = it.first - last_relocation;
		if((offset & 1))
			Linker::Error << "Error: Illegal relocation offset" << std::endl;
		relocation.is16bit = it.second == 2;
		if(offset > 0xFFFF)
		{
			relocation.absolute_displacement = true;
			relocation.displacement = it.first;
		}
		else
		{
			relocation.absolute_displacement = false;
			relocation.displacement = offset;
		}
		relocation_sequence.push_back(relocation);
		last_relocation = it.first;
	}

	relocation_size = 2 * relocation_sequence.size();
	for(auto relocation : relocation_sequence)
	{
		if(relocation.absolute_displacement)
			relocation_size += 4;
	}

	base_address = GetCodeSegment()->base_address;
	code_size = GetCodeSegment()->data_size;
	data_size = GetDataSegment()->data_size;
	bss_size = GetBssSegment()->zero_fill;
}

offset_t HUFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(3, "HU\0");
	wr.WriteWord(1, load_mode);
	wr.WriteWord(4, base_address);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, code_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, relocation_size);
	wr.WriteWord(4, symbol_table_size);
	wr.WriteWord(4, debug_line_number_table_size);
	wr.WriteWord(4, debug_symbol_table_size);
	wr.WriteWord(4, debug_string_table_size);
	wr.Skip(0x10);
	wr.WriteWord(4, bound_module_list_offset);

	code->WriteFile(wr);
	data->WriteFile(wr);

	for(auto relocation : relocation_sequence)
	{
		uint32_t displacement = relocation.displacement;
		if(relocation.is16bit)
			displacement |= 1;
		else
			displacement &= ~1;
		if(relocation.absolute_displacement)
		{
			wr.WriteWord(2, 1);
			wr.WriteWord(4, relocation.displacement);
		}
		else
		{
			wr.WriteWord(2, relocation.displacement);
		}
	}

	// TODO: symbol table

	return offset_t(-1);
}

void HUFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	static const std::map<offset_t, std::string> load_mode_description =
	{
		{ MODE_NORMAL,   "normal" },
		{ MODE_SMALLEST, "smallest block" },
		{ MODE_HIGHEST,  "highest address" },
	};

	dump.SetTitle("HU format");
	Dumper::Region file_region("File", 0, 0x40 + code_size + data_size + relocation_size + symbol_table_size, 8); // TODO: other entries
	file_region.AddField("Load mode", Dumper::ChoiceDisplay::Make(load_mode_description, Dumper::HexDisplay::Make(2)), offset_t(load_mode));
	file_region.AddField("Entry (PC)", Dumper::HexDisplay::Make(8), offset_t(entry_address));
	file_region.AddOptionalField("Debug line number table size", Dumper::HexDisplay::Make(8), offset_t(debug_line_number_table_size));
	file_region.AddOptionalField("Debug symbol table size", Dumper::HexDisplay::Make(8), offset_t(debug_symbol_table_size));
	file_region.AddOptionalField("Debug string table size", Dumper::HexDisplay::Make(8), offset_t(debug_string_table_size));
	file_region.AddOptionalField("Offset to bound module list", Dumper::HexDisplay::Make(8), offset_t(bound_module_list_offset));
	file_region.Display(dump);

	Dumper::Block code_block("Code", 0x40, code, base_address, 8);
	Dumper::Block data_block("Data", 0x40 + code_size, data, base_address + code_size, 8);

	for(auto it : relocations)
	{
		if(it.first < code_size)
		{
			code_block.AddSignal(it.first, it.second);
		}
		else
		{
			data_block.AddSignal(it.first, it.second);
		}
	}

	code_block.Display(dump);
	data_block.Display(dump);

	Dumper::Region bss_region("BSS", 0x40 + code_size + data_size, bss_size, 8);
	bss_region.AddField("Address", Dumper::HexDisplay::Make(8), offset_t(base_address + code_size + data_size));
	bss_region.Display(dump);

	if(relocation_size != 0)
	{
		Dumper::Region relocations_region("Relocations", 0x40 + code_size + data_size, relocation_size, 8);
		relocations_region.Display(dump);

		uint32_t relocation_offset = 0x40 + code_size + data_size;
		uint32_t relocation_index = 0;
		uint32_t relocation_source = 0;
		for(auto relocation : relocation_sequence)
		{
			Dumper::Entry relocation_entry("Relocation", relocation_index + 1, relocation_offset, 8);
			relocation_entry.AddField("Word", Dumper::HexDisplay::Make(4), offset_t(relocation.absolute_displacement ? 0x0001 : relocation.displacement | (relocation.is16bit ? 1 : 0)));
			if(relocation.absolute_displacement)
				relocation_source = relocation.displacement;
			else
				relocation_source += relocation.displacement;
			relocation_entry.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(relocation_source));
			relocation_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation.is16bit ? 2 : 4));
			relocation_entry.AddField("Record bytes", Dumper::DecDisplay::Make(), offset_t(relocation.absolute_displacement ? 6 : 2));
			relocation_entry.Display(dump);

			relocation_index++;
			relocation_offset += relocation.absolute_displacement ? 6 : 2;
		}
	}

	if(symbol_table_size != 0)
	{
		Dumper::Region symbol_table_region("Symbol table", 0x40 + code_size + data_size + relocation_size, symbol_table_size, 8);
		symbol_table_region.Display(dump);
		// TODO: write symbols
	}
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

