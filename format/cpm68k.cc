
#include "cpm68k.h"

using namespace DigitalResearch;

CPM68KFormat::Relocation::operator size_t() const
{
	return size;
}

CPM68KFormat::magic_type CPM68KFormat::GetSignature() const
{
	if(signature[0] == 0x60)
	{
		if(signature[1] == 0x1A)
			return MAGIC_CONTIGUOUS;
		else if(signature[1] == 0x1B)
			return MAGIC_NONCONTIGUOUS;
		else if(signature[1] == 0x1C)
			return MAGIC_CRUNCHED;
	}
	return magic_type(0); // invalid
}

void CPM68KFormat::SetSignature(magic_type magic)
{
	switch(magic)
	{
	case MAGIC_CONTIGUOUS:
		signature[0] = 0x60;
		signature[1] = 0x1A;
		break;
	case MAGIC_NONCONTIGUOUS:
		signature[0] = 0x60;
		signature[1] = 0x1B;
		break;
	case MAGIC_CRUNCHED:
		signature[0] = 0x60;
		signature[1] = 0x1C;
		break;
	}
}

void CPM68KFormat::Initialize()
{
	/* format fields */
	SetSignature(MAGIC_CONTIGUOUS);
	code_size = 0;
	data_size = 0;
	bss_size = 0;
	symbol_table_size = 0;
	stack_size = 0;
	code_address = 0;
	program_flags = 0;
	relocations_suppressed = 1;
	data_address = 0;
	bss_address = 0;
	code = nullptr;
	data = nullptr;
	/* writer fields */
	system = SYSTEM_UNKNOWN;
	bss_segment = nullptr;
	stack_segment = nullptr;
}

void CPM68KFormat::Clear()
{
	/* format fields */
	if(code)
		delete code;
	code = nullptr;
	if(data)
		delete data;
	data = nullptr;
	relocations.clear();
	/* writer fields */
	if(bss_segment)
		delete bss_segment;
	bss_segment = nullptr;
	if(stack_segment)
		delete stack_segment;
	stack_segment = nullptr;
}

void CPM68KFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::BigEndian;
	rd.ReadData(2, signature);
	code_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	symbol_table_size = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);
	if(system == SYSTEM_UNKNOWN && stack_size != 0)
	{
		system = SYSTEM_CDOS68K;
	}
	program_flags = code_address = rd.ReadUnsigned(4);
	relocations_suppressed = rd.ReadUnsigned(2);
	if(GetSignature() == MAGIC_NONCONTIGUOUS)
	{
		data_address = rd.ReadUnsigned(4);
		bss_address = rd.ReadUnsigned(4);
	}
	if(system == SYSTEM_UNKNOWN)
	{
		if(relocations_suppressed == 0xFFFF && symbol_table_size == 0)
		{
			Linker::Debug << "Debug: Relocation suppression word is 0xFFFF with no symbol table, assuming Human68k .z file" << std::endl;
			system = SYSTEM_HUMAN68K;
		}
		else
		{
			if(relocations_suppressed == 0)
			{
				long current = rd.Tell();
				if(rd.ReadUnsigned(2) == 0)
				{
					/* Early GEMDOS has a 0 longword instead of a 0 word */
					Linker::Debug << "Debug: Relocation suppression word is 0x0000 followed by another 0x0000 word, assuming early GEMDOS .prg file" << std::endl;
					system = SYSTEM_GEMDOS_EARLY;
				}
				else
				{
					rd.SeekEnd();
					long size = rd.Tell();
					size -= current;
					if(size < 2 * (code_size + data_size) + symbol_table_size)
					{
						/* relocation section too short for CP/M-68K */
						Linker::Debug << "Debug: Relocation section too short for CP/M-68K, assuming GEMDOS .prg file" << std::endl;
						system = SYSTEM_GEMDOS;
					}
					else
					{
						/* some fast heuristics */
						rd.Seek(file_offset + 0x1C + code_size + data_size + symbol_table_size);
						uint32_t first_relocation = rd.ReadUnsigned(4);
						if(first_relocation > code_size + data_size)
						{
							Linker::Debug << "Debug: First relocation too large for GEMDOS, assuming CP/M-68K .68k file" << std::endl;
							system = SYSTEM_CPM68K;
						}
						else if(rd.ReadUnsigned(1) != 0)
						{
							Linker::Debug << "Debug: Byte 5 in relocation section non-zero, invalid for CP/M-68K executable, assuming GEMDOS .prg file" << std::endl;
							system = SYSTEM_GEMDOS;
						}
						else
						{
							Linker::Debug << "Debug: Fallback case, assuming CP/M-68K .68k file" << std::endl;
							system = SYSTEM_CPM68K;
						}
					}
					rd.Seek(current);
				}
			}

			if(system == SYSTEM_UNKNOWN)
			{
				if((program_flags & 0xFF000001) != 0 || program_flags < 0x400)
				{
					/* invalid starting address for CP/M-68K */
					Linker::Debug << "Debug: Invalid starting address for CP/M-68K, assuming GEMDOS .prg file" << std::endl;
					system = SYSTEM_GEMDOS;
					code_address = 0;
				}
				else if((program_flags & 0x0FFFEFC0) != 0)
				{
					/* undefined program flags for GEMDOS/Atari TOS */
					Linker::Debug << "Debug: Undefined program flags for GEMDOS, assuming CP/M-68K .68k file" << std::endl;
					system = SYSTEM_CPM68K;
					program_flags = 0;
				}
			}
		}
	}

	if(system == SYSTEM_GEMDOS)
	{
		code_address = 0;
	}
	else if(system != SYSTEM_UNKNOWN)
	{
		program_flags = 0;
	}

	if(GetSignature() != MAGIC_NONCONTIGUOUS)
	{
		data_address = code_address + code_size;
		bss_address = data_address + data_size;
	}

	rd.Seek(file_offset + (GetSignature() == MAGIC_NONCONTIGUOUS ? 0x24 : 0x1C));

	if(code)
	{
		delete code;
	}
	Linker::Buffer * code_section = new Linker::Section(".text");
	code_section->ReadFile(rd, code_size);
	code = code_section;

	if(data)
	{
		delete data;
	}
	Linker::Buffer * data_section = new Linker::Section(".data");
	data_section->ReadFile(rd, data_size);
	data = data_section;

	/* TODO: symbol table */

	rd.Seek(file_offset + (GetSignature() == MAGIC_NONCONTIGUOUS ? 0x24 : 0x1C) + code_size + data_size + symbol_table_size);

	switch(system)
	{
	case SYSTEM_CDOS68K:
		if(GetSignature() == MAGIC_CRUNCHED)
		{
#if 0 /* TODO */
			/* relocations must be present */
			CDOS68K_WriteRelocations(wr, relocations);
			break;
#endif
		}
	case SYSTEM_CPM68K:
		if(relocations_suppressed == 0)
		{
			size_t size = 2;
			for(size_t i = 0; i < code_size + data_size; i += 2)
			{
				uint32_t base_address = i < code_size ? code_address : data_address;
				uint16_t word = rd.ReadUnsigned(2);
				switch(word & 7)
				{
				case 1:
					relocations[base_address + i] = Relocation{size, 1};
					break;
				case 2:
					relocations[base_address + i] = Relocation{size, 2};
					break;
				case 3:
					relocations[base_address + i] = Relocation{size, 3};
					break;
				case 0:
				case 7:
					break;
				case 5:
					size = 4;
					continue;
				case 4:
					/* TODO: undefined symbol */
					break;
				case 6:
					/* 16-bit PC-relative: undefined symbol */
					break;
				}
			}
		}
		break;
	case SYSTEM_GEMDOS:
	case SYSTEM_GEMDOS_EARLY:
		if(relocations_suppressed == 0)
		{
			uint32_t address = rd.ReadUnsigned(4);
			if(address != 0)
			{
				relocations[address] = Relocation{4, 2};
				for(;;)
				{
					uint8_t disp = rd.ReadUnsigned(1);
					if(disp == 0)
					{
						break;
					}
					else if(disp == 1)
					{
						address += 0xFE;
					}
					else
					{
						address += disp;
						relocations[address] = Relocation{4, 2};
					}
				}
			}
		}
		break;
	case SYSTEM_UNKNOWN:
	case SYSTEM_HUMAN68K:
		break;
	}

	file_size = rd.Tell() - file_offset;
}

#if 0
template <typename SizeType>
	static void CDOS68K_WriteRelocations(Linker::Writer& wr, std::map<uint32_t, SizeType> relocations)
{
	/* TODO: test */
	offset_t last_relocation = 0;
	for(auto it : relocations)
	{
		offset_t difference = it.first - last_relocation;
		uint8_t highbit = it.second/*.size*/ == 2 ? 0x80 : 0x00;
		if(difference != 0 && difference <= 0x7C)
		{
			wr.WriteWord(1, highbit | difference);
		}
		else if(difference < 0x100)
		{
			wr.WriteWord(1, highbit | 0x7D);
			wr.WriteWord(1, difference);
		}
		else if(difference < 0x10000)
		{
			wr.WriteWord(1, highbit | 0x7E);
			wr.WriteWord(2, difference);
		}
		else
		{
			wr.WriteWord(1, highbit | 0x7F);
			wr.WriteWord(4, difference);
		}
	}
}
#endif

void CPM68KFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(2, signature);
	wr.WriteWord(4, code_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, symbol_table_size);
	wr.WriteWord(4, stack_size);
	wr.WriteWord(4, (system == SYSTEM_GEMDOS || system == SYSTEM_GEMDOS_EARLY) ? program_flags : code_address);
	wr.WriteWord(2, relocations_suppressed);
	if(GetSignature() == MAGIC_NONCONTIGUOUS)
	{
		wr.WriteWord(4, data_address);
		wr.WriteWord(4, bss_address);
	}
	code->WriteFile(wr);
	data->WriteFile(wr);
	switch(system)
	{
	case SYSTEM_CDOS68K:
		if(GetSignature() == MAGIC_CRUNCHED)
		{
			/* relocations must be present */
			CDOS68K_WriteRelocations(wr, relocations);
			break;
		}
	case SYSTEM_CPM68K:
		if(relocations_suppressed == 0)
		{
			for(size_t i = 0; i < code->ActualDataSize(); i++)
			{
				auto it = relocations.find(code_address + i);
				if(it == relocations.end())
				{
					wr.WriteWord(2, 7);
				}
				else
				{
					if(it->second.size == 4)
					{
						wr.WriteWord(2, 5);
						i++;
					}
					wr.WriteWord(2, it->second.segment);
				}
			}
			for(size_t i = 0; i < data->ActualDataSize(); i++)
			{
				auto it = relocations.find(data_address + i);
				if(it == relocations.end())
				{
					wr.WriteWord(2, 7);
				}
				else
				{
					if(it->second.size == 4)
					{
						wr.WriteWord(2, 5);
						i++;
					}
					wr.WriteWord(2, it->second.segment);
				}
			}
		}
		break;
	case SYSTEM_GEMDOS:
	case SYSTEM_GEMDOS_EARLY:
		if(relocations_suppressed == 0)
		{
			offset_t last_relocation = -1;
			for(auto it : relocations)
			{
				if(last_relocation == (offset_t)-1)
				{
					wr.WriteWord(4, it.first);
					last_relocation = it.first;
				}
				else
				{
					offset_t difference = it.first - last_relocation;
					while(difference > 254)
					{
						wr.WriteWord(1, 1);
						difference -= 254;
					}
					wr.WriteWord(1, difference);
					last_relocation = it.first;
				}
			}
			if(last_relocation == (offset_t)-1)
				wr.WriteWord(4, 0);
			else
				wr.WriteWord(1, 0);
		}
		break;
	case SYSTEM_UNKNOWN:
	case SYSTEM_HUMAN68K:
		break;
	}
}

void CPM68KFormat::Dump(Dumper::Dumper& dump)
{
	switch(system)
	{
	case SYSTEM_GEMDOS_EARLY:
	case SYSTEM_GEMDOS:
	case SYSTEM_UNKNOWN:
		dump.SetEncoding(Dumper::Block::encoding_st);
		break;
	default:
		dump.SetEncoding(Dumper::Block::encoding_default);
		break;
	}

	dump.SetTitle("68K format");

	std::map<offset_t, std::string> system_descriptions;
	system_descriptions[SYSTEM_CPM68K] = "CP/M-68K";
	system_descriptions[SYSTEM_GEMDOS_EARLY] = "early GEMDOS";
	system_descriptions[SYSTEM_GEMDOS] = "GEMDOS";
	system_descriptions[SYSTEM_HUMAN68K] = "Human68k";
	system_descriptions[SYSTEM_CDOS68K] = "Concurrent DOS 68K";

	std::map<offset_t, std::string> format_descriptions;
	format_descriptions[MAGIC_CONTIGUOUS] = "contiguous (0x60 0x1A)";
	format_descriptions[MAGIC_NONCONTIGUOUS] = "non-contiguous (0x60 0x1B)";
	format_descriptions[MAGIC_CRUNCHED] = "crunched relocations (0x60 0x1C)";

	offset_t header_size = GetSignature() == MAGIC_NONCONTIGUOUS ? 0x24 : 0x1C;

	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.AddField("Magic", new Dumper::ChoiceDisplay(format_descriptions), (offset_t)GetSignature());
	file_region.AddField("System", new Dumper::ChoiceDisplay(system_descriptions), (offset_t)system);
	file_region.Display(dump);

	Dumper::Region header_region("Header", file_offset, header_size, 8);
	header_region.AddField("Suppression word", new Dumper::HexDisplay(4), (offset_t)relocations_suppressed);
	header_region.AddField("Bss size", new Dumper::HexDisplay, (offset_t)bss_size);

	Dumper::Block code_block("Code segment", file_offset + header_size, code,
		system != SYSTEM_GEMDOS && system != SYSTEM_GEMDOS_EARLY ? code_address : 0, 8);
	Dumper::Block data_block("Data segment", file_offset + header_size + code_size, data,
		system != SYSTEM_GEMDOS && system != SYSTEM_GEMDOS_EARLY ? data_address : code_size, 8);

	header_region.AddField("Bss address", new Dumper::HexDisplay, (offset_t)bss_address); /* TODO: place as part of a container */

	header_region.AddOptionalField("Symbol table size", new Dumper::HexDisplay, (offset_t)symbol_table_size);
	header_region.AddOptionalField("Stack size", new Dumper::HexDisplay, (offset_t)stack_size);
	if(system == SYSTEM_GEMDOS)
	{
		std::map<offset_t, std::string> memory_protection;
		memory_protection[0] = "private memory";
		memory_protection[1] = "global memory";
		memory_protection[2] = "supervisor global memory";
		memory_protection[3] = "read only global memory";
		header_region.AddField("Program flags",
			&Dumper::BitFieldDisplay::Make()
				.AddBitField(0, 1, new Dumper::ChoiceDisplay("fastload (do not clear heap)"), true)
				.AddBitField(1, 1, new Dumper::ChoiceDisplay("may load program into alternate RAM"), true)
				.AddBitField(2, 1, new Dumper::ChoiceDisplay("may Malloc from alternate RAM"), true)
				.AddBitField(3, 1, new Dumper::ChoiceDisplay("no heap required (MagiC 6.0 shared library)"), true)
				.AddBitField(4, 2, new Dumper::ChoiceDisplay(memory_protection), false)
				.AddBitField(12, 1, new Dumper::ChoiceDisplay("shared text"), true)
				.AddBitField(28, 4, new Dumper::DecDisplay(" 128 KiB alternate RAM"), true),
		(offset_t)program_flags);
	}

	header_region.Display(dump);

//			Dumper::Region relocations_region("Relocations", file_offset + code_size + data_size + symbol_table_size, 0); /* TODO: unknown size */

	std::map<offset_t, std::string> segment_names;
	segment_names[1] = "Data";
	segment_names[2] = "Text";
	segment_names[3] = "Bss";

	size_t i = 0;
	for(auto relocation : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, offset_t(-1) /* TODO: offset */, 8);
		relocation_entry.AddField("Source", new Dumper::HexDisplay, (offset_t)relocation.first);
		relocation_entry.AddField("Size", new Dumper::HexDisplay(1), (offset_t)relocation.second.size);
		relocation_entry.AddField("Target", new Dumper::ChoiceDisplay(segment_names), (offset_t)relocation.second.segment);
		relocation_entry.Display(dump);

		if(relocation.first < code_address + code_size)
			code_block.AddSignal(relocation.first - code_address, relocation.second.size);
		else
			data_block.AddSignal(relocation.first - code_address - code_size, relocation.second.size);
		i++;
	}

	code_block.Display(dump);
	data_block.Display(dump);

	/* TODO: symbol table */
}

void CPM68KFormat::CalculateValues()
{
	if(system == SYSTEM_HUMAN68K)
	{
		symbol_table_size = 0;
	}
	if(system != SYSTEM_CDOS68K)
	{
		stack_size = 0;
	}
	if(system == SYSTEM_GEMDOS_EARLY)
	{
		code_address = 0;
	}
	if(system == SYSTEM_HUMAN68K || system == SYSTEM_UNKNOWN)
	{
		relocations_suppressed = 0xFFFF;
	}
	else if(system == SYSTEM_GEMDOS_EARLY)
	{
		/* early GEMDOS did not allow relocation suppression */
		relocations_suppressed = 0;
	}
	/* TODO: calculate file_size, not very important */
}

/* * * Writer members * * */

Linker::Segment * CPM68KFormat::CodeSegment()
{
	return dynamic_cast<Linker::Segment *>(code);
}

Linker::Segment * CPM68KFormat::DataSegment()
{
	return dynamic_cast<Linker::Segment *>(data);
}

unsigned CPM68KFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(system == SYSTEM_CDOS68K && (section_name == ".stack" || section_name.rfind(".stack.", 0) == 0))
	{
		return Linker::Section::Stack;
	}
	else
	{
		return 0;
	}
}

void CPM68KFormat::SetOptions(std::map<std::string, std::string>& options)
{
	option_no_relocation = options.find("noreloc") != options.end();
	relocations_suppressed = options.find("reloc") != options.end() ? 0 : 1;
	if(option_no_relocation && relocations_suppressed == 0)
	{
		throw new Linker::Exception("Fatal error: inconsistent expectations for relocations, aborting");
	}

	SetLinkerParameter(options, "code_base", "code_base_address");
	SetLinkerParameter(options, "data_base", "data_base_address");
	SetLinkerParameter(options, "bss_base", "bss_base_address");

#if 0
	auto code_base_it = options.find("code_base");
	if(code_base_it != options.end())
	{
		try
		{
			offset_t code_base_address = std::stoll(code_base_it->second, nullptr, 0);
			linker_parameters["code_base_address"] = Linker::Location(code_base_address);
		}
		catch(std::invalid_argument a)
		{
			Linker::Error << "Error: Unable to parse " << code_base_it->second << ", ignoring" << std::endl;
		}
	}
	else if(type == CPM68K || type == CPM68K_NONCONT)
	{
		linker_parameters["code_base_address"] = Linker::Location(0x0500);
	}
	else if(type == HUMAN68K)
	{
		linker_parameters["code_base_address"] = Linker::Location(0x0006F800);
	}
	auto data_base_it = options.find("data_base");
	if(data_base_it != options.end())
	{
		if(type != CPM68K_NONCONT)
		{
			Linker::Error << "Error: data_base option only supported for non-contiguous files, ignoring" << std::endl;
		}
		try
		{
			offset_t data_base_address = std::stoll(data_base_it->second, nullptr, 0);
			linker_parameters["data_base_address"] = Linker::Location(data_base_address);
		}
		catch(std::invalid_argument a)
		{
			Linker::Error << "Error: Unable to parse " << data_base_it->second << ", ignoring" << std::endl;
		}
	}
	auto bss_base_it = options.find("bss_base");
	if(bss_base_it != options.end())
	{
		if(type != CPM68K_NONCONT)
		{
			Linker::Error << "Error: bss_base option only supported for non-contiguous files, ignoring" << std::endl;
		}
		try
		{
			offset_t bss_base_address = std::stoll(bss_base_it->second, nullptr, 0);
			linker_parameters["bss_base_address"] = Linker::Location(bss_base_address);
		}
		catch(std::invalid_argument a)
		{
			Linker::Error << "Error: Unable to parse " << bss_base_it->second << ", ignoring" << std::endl;
		}
	}
#endif
}

void CPM68KFormat::OnNewSegment(Linker::Segment * segment)
{
	if(segment->name == ".code")
	{
		if(code != nullptr)
		{
			Linker::Error << "Error: duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		if(data != nullptr || bss_segment != nullptr)
		{
			Linker::Warning << "Warning: wrong order for `.code` segment" << std::endl;
		}
		code = segment;
	}
	else if(segment->name == ".data")
	{
		if(data != nullptr)
		{
			Linker::Error << "Error: duplicate `.data` segment, ignoring" << std::endl;
			return;
		}
		if(bss_segment != nullptr)
		{
			Linker::Warning << "Warning: wrong order for `.code` segment" << std::endl;
		}
		data = segment;
	}
	else if(segment->name == ".bss")
	{
		if(bss_segment != nullptr)
		{
			Linker::Error << "Error: duplicate `.bss` segment, ignoring" << std::endl;
			return;
		}
		bss_segment = segment;
	}
	else if(system == SYSTEM_CDOS68K && segment->name == ".stack")
	{
		if(stack_segment != nullptr)
		{
			Linker::Error << "Error: duplicate `.stack` segment, ignoring" << std::endl;
			return;
		}
		stack_segment = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected one of `.code`, `.data`, `.bss`";
		if(system == SYSTEM_CDOS68K)
		{
			Linker::Error << ", `.stack`";
		}
		Linker::Error << ", ignoring" << std::endl;
	}
}

void CPM68KFormat::CreateDefaultSegments()
{
	if(code == nullptr)
	{
		code = new Linker::Segment(".code");
	}
	if(data == nullptr)
	{
		data = new Linker::Segment(".data");
	}
	if(bss_segment == nullptr)
	{
		bss_segment = new Linker::Segment(".bss");
	}
	if(system == SYSTEM_CDOS68K && stack_segment == nullptr)
	{
		stack_segment = new Linker::Segment(".stack");
	}
}

Script::List * CPM68KFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align 4;
};

".data"
{
	at max(here, ?data_base_address?);
	all not zero align 4;
	align 4;
};

".bss"
{
	at max(here, ?bss_base_address?);
	all align 4;
	align 4;
};
)";

	static const char * SimpleScript_cdos68k = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align 4;
};

".data"
{
	at max(here, ?data_base_address?);
	all not zero align 4;
	align 4;
};

".bss"
{
	at max(here, ?bss_base_address?);
	all not ".stack" align 4;
	align 4;
};

".stack"
{
	all align 4;
	align 4;
};
)";

	if(linker_script != "")
	{
		return LinkerManager::GetScript(module);
	}
	else
	{
		switch(system)
		{
		case SYSTEM_CDOS68K:
			return Script::parse_string(SimpleScript_cdos68k);
		default:
			return Script::parse_string(SimpleScript);
		}
	}
}

void CPM68KFormat::Link(Linker::Module& module)
{
	if(GetSignature() != MAGIC_NONCONTIGUOUS && linker_script != "")
	{
		if(linker_parameters.find("data_base_address") != linker_parameters.end())
		{
			Linker::Warning << "Warning: data_base_address parameter only supported for non-contiguous files, ignoring" << std::endl;
		}
		if(linker_parameters.find("bss_base_address") != linker_parameters.end())
		{
			Linker::Warning << "Warning: bss_base_address parameter only supported for non-contiguous files, ignoring" << std::endl;
		}
	}

	Script::List * script = GetScript(module);

	ProcessScript(script, module);
	CreateDefaultSegments();
}

void CPM68KFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignroing" << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(system == SYSTEM_HUMAN68K)
		{
			Linker::Warning << "Warning: Relocation suppressed" << std::endl;
		}
		else if(resolution.target != nullptr)
		{
			if(resolution.reference != nullptr)
			{
				/* inter-segment reference */
				if(GetSignature() == MAGIC_NONCONTIGUOUS && !option_no_relocation)
				{
					Linker::Error << "Error: non-contiguous files do not support inter-segment differences, generating image anyway (generate with relocations disabled)" << std::endl;
				}
			}
			else if(option_no_relocation)
			{
				Linker::Error << "Error: relocations suppressed, generating image anyway" << std::endl;
			}
			else
			{
				unsigned segment_num = -1;
				if(resolution.target == code)
					segment_num = 2; /* as encoded in the CP/M-68K executable */
				else if(resolution.target == data)
					segment_num = 1; /* as encoded in the CP/M-68K executable */
				else if(resolution.target == bss_segment)
					segment_num = 3; /* as encoded in the CP/M-68K executable */
//						Linker::Debug << "Debug: " << resolution.target->name << std::endl;
				assert(segment_num != (unsigned)-1);
				if((system == SYSTEM_GEMDOS || system == SYSTEM_GEMDOS_EARLY) && rel.size != 4)
				{
					Linker::Error << "Error: Format only supports longword relocations: " << rel << ", ignoring" << std::endl;
					continue;
				}
				else if(rel.size != 2 && rel.size != 4)
				{
					Linker::Error << "Error: Format only supports word and longword relocations: " << rel << ", ignoring" << std::endl;
					continue;
				}
				offset_t address = rel.source.GetPosition().address;
				if((address & 1) != 0)
				{
					Linker::Error << "Error: misaligned relocation, ignoring" << std::endl;
					continue;
				}
				relocations[address] = Relocation{rel.size, segment_num};
				relocations_suppressed = 0;
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
		Linker::Position position = entry.GetPosition();
		if(position.address != 0 || position.segment != code)
		{
			Linker::Error << "Error: entry point must be beginning of .code segment, ignoring" << std::endl;
		}
	}

	code_size = code ? code->ActualDataSize() : 0;
	data_size = data ? data->ActualDataSize() : 0;
	bss_size = bss_segment ? bss_segment->zero_fill : 0;
	stack_size = stack_segment ? stack_segment->zero_fill : 0;

	if(system != SYSTEM_GEMDOS)
	{
		/* uses program_flags instead */
		code_address = CodeSegment()->base_address;
	}

	if(GetSignature() == MAGIC_NONCONTIGUOUS)
	{
		data_address = DataSegment()->base_address;
		bss_address = bss_segment->base_address;
	}
}

void CPM68KFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		throw new Linker::Exception("Fatal error: Format only supports Motorola 68000 binaries");
	}

	if(linker_parameters.find("code_base_address") == linker_parameters.end())
	{
		if(system == SYSTEM_CPM68K)
		{
			linker_parameters["code_base_address"] = Linker::Location(0x0500);
		}
		else if(system == SYSTEM_HUMAN68K)
		{
			linker_parameters["code_base_address"] = Linker::Location(0x0006F800); /* the emulator used this address for loading .R and .X files */
		}
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string CPM68KFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	switch(system)
	{
	case SYSTEM_CPM68K:
	case SYSTEM_CDOS68K:
		return filename + ".68k";
	case SYSTEM_GEMDOS:
	case SYSTEM_GEMDOS_EARLY:
		return filename + ".prg";
	case SYSTEM_HUMAN68K:
		return filename + ".z";
	default:
		assert(false);
	}
}

