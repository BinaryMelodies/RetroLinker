
#include "aout.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace AOut;

::EndianType AOutFormat::GetEndianType() const
{
	switch(cpu)
	{
	case M68010:
	case M68020:
	case SPARC:
		return ::BigEndian;
	case I80386:
	case ARM: /* TODO: check */
	case MIPS1: /* TODO: check */
	case MIPS2: /* TODO: check */
	case PDP11:
		return ::LittleEndian; /* Note: actually, PDP-11 endian */
	default:
		Linker::FatalError("Internal error: invalid CPU type");
	}
}

unsigned AOutFormat::GetWordSize() const
{
	switch(cpu)
	{
	case M68010:
	case M68020:
	case SPARC:
	case I80386:
	case ARM:
	case MIPS1:
	case MIPS2:
		return 4;
	case PDP11:
		return 2;
	default:
		Linker::FatalError("Internal error: invalid CPU type");
	}
}

bool AOutFormat::AttemptFetchMagic(uint8_t signature[4])
{
	/* Check valid magic number */
	uint16_t attempted_magic;
	switch(endiantype)
	{
	case ::LittleEndian:
	case ::PDP11Endian:
		attempted_magic = signature[0] | (signature[1] << 8);
		break;
	case ::BigEndian: /* TODO: or PDP11 */
		attempted_magic = signature[word_size - 1] | (signature[word_size - 2] << 8);
		break;
	default:
		Linker::FatalError("Internal error: invalid endianness");
	}

	if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
		return false;
	magic = magic_type(attempted_magic);
	return true;
}

bool AOutFormat::AttemptReadFile(Linker::Reader& rd, uint8_t signature[4], offset_t image_size)
{
	if(!AttemptFetchMagic(signature))
		return false;

	/* Check valid CPU type (32-bit only) */
	if(word_size == 4)
	{
		switch(endiantype)
		{
		case ::LittleEndian:
			switch(signature[2])
			{
			case 0x64:
				cpu = I80386;
				break;
			case 0x67:
				cpu = ARM;
				break;
			case 0x97:
				cpu = MIPS1;
				break;
			case 0x98:
				cpu = MIPS2;
				break;
			case 0:
				/* probably VAX */
				cpu = UNKNOWN;
				break;
			default:
				return false;
			}
			break;
		case ::BigEndian:
			switch(signature[1])
			{
			case 0x01:
				cpu = M68010;
				break;
			case 0x02:
				cpu = M68020;
				break;
			case 0x03:
				cpu = SPARC;
				break;
			case 0:
				/* probably 68000, "old Sun" */
				cpu = UNKNOWN;
				break;
			default:
				return false;
			}
			break;
		default:
			return false;
		}
	}

	/* Check if all sizes fit within the image */
	rd.Seek(file_offset + word_size);
	uint32_t full_size = 0;
	uint32_t next_size;
	for(int i = 1; i < 8; i++)
	{
		if(i == 5)
		{
			/* entry point */
			rd.Skip(word_size);
			continue;
		}
		next_size = rd.ReadUnsigned(word_size);
		if(full_size + next_size < full_size || full_size + next_size > image_size)
			return false;
		full_size += next_size;
	}
	return true;
}

void AOutFormat::ReadFile(Linker::Reader& rd)
{
	// TODO: this can probably be simplified
	// suggested algorithm:
	// if first two bytes are not a valid magic number, it is big endian (and thus 32-bit), otherwise little endian
	// if little endian, assume 32-bit and check file size, if too big, must be 16-bit
	// finally, mask out 0x03FF0000 (BSD) to get machine type, or 0x00FF0000 (Linux) if that fails, fallback to VAX
	uint8_t signature[4];
	rd.SeekEnd();
	offset_t file_end = rd.Tell();
	rd.Seek(file_offset);
	rd.ReadData(sizeof(signature), signature);
	word_size = 4;

	// Big endian:
	// .. 01 .. ..
	// .. 02 .. ..
	// .. 03 .. ..
	// byte 2 must be 00 (QMAGIC) or 01

	// Little endian:
	// .. .. 64 ..
	// .. .. 67 ..
	// .. .. 97 ..
	// .. .. 98 ..
	// byte 1 must be 00 (QMAGIC) or 01

	if(signature[1] == 0x00 || signature[1] == 0x01)
	{
		/* could be multiple formats, make multiple attempts */
		endiantype = ::LittleEndian;
		word_size = 2;
		/* first attempt 16-bit little endian (PDP-11, most likely input format) */
		if(!AttemptReadFile(rd, signature, file_end - file_offset))
		{
			endiantype = ::LittleEndian;
			word_size = 4;
			/* then attempt 32-bit little endian (Intel 80386, most likely format found on system) */
			if(!AttemptReadFile(rd, signature, file_end - file_offset))
			{
				endiantype = ::BigEndian;
				word_size = 4;
				/* then attempt 32-bit big endian (Motorola 68000, most likely format if not little endian) */
				if(!AttemptReadFile(rd, signature, file_end - file_offset))
				{
					endiantype = ::BigEndian;
					word_size = 2;
					/* finally, attempt 16-bit big endian (unsure if any ever supported UNIX with a.out) */
					if(!AttemptReadFile(rd, signature, file_end - file_offset))
					{
						Linker::FatalError("Fatal error: Unable to determine file format");
					}
				}
			}
		}
	}
	else
	{
		switch(signature[1])
		{
		case 0x02:
			endiantype = ::BigEndian;
			cpu = M68020;
			break;
		case 0x03:
			endiantype = ::BigEndian;
			cpu = SPARC;
			break;
		default:
			switch(signature[2])
			{
			case 0x00:
				endiantype = ::LittleEndian;
				cpu = UNKNOWN;
				break;
			case 0x64:
				endiantype = ::LittleEndian;
				cpu = I80386;
				break;
			case 0x67:
				endiantype = ::LittleEndian;
				cpu = ARM;
				break;
			case 0x97:
				endiantype = ::LittleEndian;
				cpu = MIPS1;
				break;
			case 0x98:
				endiantype = ::LittleEndian;
				cpu = MIPS2;
				break;
			}
			break;
		}
		if(!AttemptFetchMagic(signature))
		{
			Linker::FatalError("Fatal error: Unable to determine file format");
		}
	}

	Linker::Debug << "Debug: a.out endian type: " << endiantype << ", CPU type: " << cpu << ", magic value: " << magic << ", word size: " << word_size << std::endl;

	rd.endiantype = endiantype;

	rd.Seek(file_offset + word_size);

	code_size = rd.ReadUnsigned(word_size);
	data_size = rd.ReadUnsigned(word_size);
	bss_size = rd.ReadUnsigned(word_size);
	symbol_table_size = rd.ReadUnsigned(word_size);
	entry_address = rd.ReadUnsigned(word_size);
	code_relocation_size = rd.ReadUnsigned(word_size);
	data_relocation_size = rd.ReadUnsigned(word_size);

	code = Linker::Section::ReadFromFile(rd, code_size, ".text");
	data = Linker::Section::ReadFromFile(rd, data_size, ".data");

	if(word_size == 2)
	{
		/* TODO: this is only for PDP-11 */
		if(data_relocation_size == 0)
		{
			/* indicates that relocations are present */
			int source_segment = 0;
			for(size_t i = 0; i < code_size + data_size; i += 2)
			{
				if(i == code_size)
				{
					source_segment = 1;
				}

				uint16_t value = rd.ReadUnsigned(2);

				if((value & 0xE) == 0)
				{
					continue;
				}

				Linker::Debug << "Debug: Offset " << std::hex << i << " relocation " << std::hex << value << std::endl;

				switch(source_segment)
				{
				case 0:
					/* text */
					code_relocations[i] = value;
					break;
				case 1:
					/* data */
					data_relocations[i - code_size] = value;
					break;
				}
			}
		}

		if(symbol_table_size != 0)
		{
			for(size_t i = 0; i < symbol_table_size; i += 8)
			{
				Symbol symbol;
				symbol.unknown = rd.ReadUnsigned(2);
				symbol.name_offset = rd.ReadUnsigned(2);
				symbol.type = rd.ReadUnsigned(2);
				symbol.value = rd.ReadUnsigned(2);
				symbols.push_back(symbol);
			}

			offset_t string_table_offset = code_size + data_size;
			if(data_relocation_size == 0)
			{
				string_table_offset *= 2;
			}
			string_table_offset += file_offset + 0x10 + symbol_table_size;
			Linker::Debug << "Debug: String table offset " << std::hex << string_table_offset << std::endl;
			for(auto& symbol : symbols)
			{
				rd.Seek(string_table_offset + symbol.name_offset);
				symbol.name = rd.ReadASCIIZ();
				Linker::Debug << "Debug: Symbol " << symbol.name << " (offset " << symbol.name_offset << ") of type " << symbol.type << " value " << symbol.value << std::endl;
			}
		}
	}

#if 0
	for(auto it : code_relocations)
	{
		wr.WriteWord(4, it.first);
		wr.WriteWord(4, it.second);
	}

	for(auto it : data_relocations)
	{
		wr.WriteWord(4, it.first);
		wr.WriteWord(4, it.second);
	}
#endif

	/* this is required for module generation */
	bss = std::make_shared<Linker::Section>(".bss");
}

offset_t AOutFormat::ImageSize() const
{
	return 8 * word_size + code->ImageSize() + data->ImageSize() + code_relocations.size() * 8 + data_relocations.size() * 8;
}

offset_t AOutFormat::WriteFile(Linker::Writer& wr) const
{
	if(system == DJGPP1 && stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}

	wr.endiantype = endiantype;

	wr.WriteWord(word_size, magic | (cpu << 16));
	wr.WriteWord(word_size, code_size);
	wr.WriteWord(word_size, data_size);
	wr.WriteWord(word_size, bss_size);
	wr.WriteWord(word_size, symbol_table_size);
	wr.WriteWord(word_size, entry_address);
	wr.WriteWord(word_size, code_relocation_size);
	wr.WriteWord(word_size, data_relocation_size);

	code->WriteFile(wr);
	data->WriteFile(wr);

	for(auto it : code_relocations)
	{
		wr.WriteWord(4, it.first);
		wr.WriteWord(4, it.second);
	}

	for(auto it : data_relocations)
	{
		wr.WriteWord(4, it.first);
		wr.WriteWord(4, it.second);
	}

	return ImageSize();
}


void AOutFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("UNIX a.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 2 * word_size);
	file_region.Display(dump);

	// TODO
}

/* * * Reader * * */

void AOutFormat::GenerateModule(Linker::Module& module) const
{
	switch(cpu)
	{
	case M68010:
	case M68020:
		module.cpu = Linker::Module::M68K;
		break;
	case I80386:
		module.cpu = /*option_16bit ? Linker::Module::I86 :*/ Linker::Module::I386; /* TODO */
		break;
	case SPARC:
		module.cpu = Linker::Module::SPARC;
		break;
	case ARM:
		module.cpu = Linker::Module::ARM;
		break;
	case MIPS1:
	case MIPS2:
		module.cpu = Linker::Module::MIPS;
		break;
	case UNKNOWN:
		if(word_size == 2)
		{
			if(endiantype == ::LittleEndian || endiantype == ::PDP11Endian)
			{
				/* make a guess */
				module.cpu = Linker::Module::PDP11;
			}
		}
		else
		{
			if(endiantype == ::BigEndian)
			{
				module.cpu = Linker::Module::M68K;
			}
		}
		break;
	default:
		/* unknown CPU */
		break;
	}

	std::shared_ptr<Linker::Section> linker_code, linker_data, linker_bss;
	linker_code = std::dynamic_pointer_cast<Linker::Section>(code);
	linker_data = std::dynamic_pointer_cast<Linker::Section>(data);
	linker_bss  = std::dynamic_pointer_cast<Linker::Section>(bss);

	linker_bss->SetZeroFilled(true);
	linker_bss->Expand(bss_size);

	linker_code->SetReadable(true);
	linker_data->SetReadable(true);
	linker_bss->SetReadable(true);

	linker_code->SetExecutable(true);
	linker_data->SetWritable(true);
	linker_bss->SetWritable(true);

	module.AddSection(linker_code);
	module.AddSection(linker_data);
	module.AddSection(linker_bss);

	for(const Symbol& symbol : symbols)
	{
		offset_t offset;
		std::shared_ptr<Linker::Section> section;
		switch(symbol.type)
		{
		case 0x20:
			/* external or common */
			if(symbol.value != 0)
			{
				module.AddCommonSymbol(Linker::SymbolDefinition::CreateCommon(symbol.name, "", symbol.value, 1 /* TODO: alignment */));
			}
			continue;
		case 0x01:
			/* absolute */
			offset = symbol.value;
			section = nullptr;
			break;
		case 0x22:
		case 0x02:
			/* text based */
			offset = symbol.value;
			section = linker_code;
			break;
		case 0x23:
		case 0x03:
			/* data based */
			offset = symbol.value - code_size;
			section = linker_data;
			break;
		case 0x24:
		case 0x04:
			/* bss based */
			offset = symbol.value - code_size - data_size;
			section = linker_bss;
			break;
		default:
			Linker::Error << "Internal error: unhandled symbol type" << std::endl;
			continue;
		}
		Linker::Debug << "Debug: Add symbol " << symbol.name << ": " << Linker::Location(section, offset) << std::endl;
		module.AddGlobalSymbol(symbol.name, Linker::Location(section, offset));
	}

	for(int i = 0; i < 2; i++)
	{
		std::shared_ptr<Linker::Section> section = i == 0 ? linker_code : linker_data;
		for(auto& rel : i == 0 ? code_relocations : data_relocations)
		{
			/* TODO: only PDP-11 specific */
			Linker::Location rel_source = Linker::Location(section, rel.first);
			Linker::Target rel_target;
			uint32_t base = section->ReadUnsigned(2, rel.first); // 6
			bool is_relative = rel.second & 1;
			if(is_relative)
			{
				base += rel.first; // E
				if(i == 1)
					base += code_size;
			}
			uint32_t addend = 0;

			/* the values stored in the object file are already relative to the start of the code segment */
			switch(rel.second & 0x0E)
			{
			case 0x02:
				/* text based variable */
				rel_target = Linker::Target(Linker::Location(linker_code, base));
				break;
			case 0x04:
				/* data based variable */
				rel_target = Linker::Target(Linker::Location(linker_data, base - code_size));
				break;
			case 0x06:
				/* bss based variable */
				rel_target = Linker::Target(Linker::Location(linker_bss, base - code_size - data_size));
				break;
			case 0x08:
				/* external symbol */
				{
					const Symbol& symbol = symbols[rel.second >> 4];
#if 0
					if(symbol.name == ".text")
					{
						rel_target = Linker::Target(Linker::Location(linker_code, base), true);
					}
					else if(symbol.name == ".data")
					{
						rel_target = Linker::Target(Linker::Location(linker_data, base), true);
					}
					else if(symbol.name == ".bss")
					{
						rel_target = Linker::Target(Linker::Location(linker_bss, base), true);
					}
					else
#endif
					{
						addend = -2;
						rel_target = Linker::Target(Linker::SymbolName(symbol.name));
					}
				}
				break;
			}

			Linker::Relocation obj_rel =
				is_relative
				? Linker::Relocation::Relative(2, rel_source, rel_target, addend, ::LittleEndian)
				: Linker::Relocation::Absolute(2, rel_source, rel_target, addend, ::LittleEndian);
			//obj_rel.AddCurrentValue();
			module.relocations.push_back(obj_rel);
			Linker::Debug << "Debug: a.out relocation " << obj_rel << std::endl;
		}
	}
}

/* * * Writer * * */

std::shared_ptr<AOutFormat> AOutFormat::CreateWriter(system_type system, magic_type magic)
{
	std::shared_ptr<AOutFormat> format = std::make_shared<AOutFormat>();
	format->system = system;
	format->magic = magic;
	return format;
}

std::shared_ptr<AOutFormat> AOutFormat::CreateWriter(system_type system)
{
	return CreateWriter(system, GetDefaultMagic(system));
}

AOutFormat::magic_type AOutFormat::GetDefaultMagic(system_type system)
{
	switch(system)
	{
	default:
	case DJGPP1:
		return ZMAGIC;
	case PDOS386:
		return OMAGIC;
	}
}

void AOutFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub.filename = FetchOption(options, "stub", "");

	/* TODO: other parameters */
}

void AOutFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
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

void AOutFormat::CreateDefaultSegments()
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

std::unique_ptr<Script::List> AOutFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?code_base_address?;
	all not write align 4;
	align ?align?;
};

".data"
{
	at align(here, ?data_align?);
	all not zero align 4;
	align ?align?; # TODO: not needed?
};

".bss"
{
	all align 4;
	align ?align?;
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

void AOutFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

std::shared_ptr<Linker::Segment> AOutFormat::GetCodeSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(code);
}

std::shared_ptr<Linker::Segment> AOutFormat::GetDataSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(data);
}

std::shared_ptr<Linker::Segment> AOutFormat::GetBssSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(bss);
}

void AOutFormat::ProcessModule(Linker::Module& module)
{
	switch(system)
	{
	case DJGPP1:
		linker_parameters["code_base_address"] = Linker::Location(0x1020);
		linker_parameters["align"] = Linker::Location(0x1000);
		linker_parameters["data_align"] = Linker::Location(0x400000);
		break;
	case PDOS386:
		linker_parameters["code_base_address"] = Linker::Location();
		linker_parameters["align"] = Linker::Location(4); /* TODO: is this needed? */
		linker_parameters["data_align"] = Linker::Location(4); /* TODO: is this needed? */
		break;
	default:
		Linker::FatalError("Internal error: invalid target system");
	}

	Link(module);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(system == PDOS386 && resolution.target != nullptr && resolution.reference == nullptr)
		{
			uint32_t symbol;
			if(resolution.target == code)
				symbol = 4;
			else if(resolution.target == data)
				symbol = 6;
			else if(resolution.target == bss)
				symbol = 8;
			else
				Linker::FatalError("Internal error: invalid target segment for relocation");
			switch(rel.size)
			{
			case 1:
				break;
			case 2:
				symbol |= 0x02000000;
				break;
			case 4:
				symbol |= 0x04000000;
				break;
			case 8:
				symbol |= 0x06000000;
				break;
			default:
				Linker::Error << "Error: format does not support " << (rel.size << 3) << "-bit relocations, ignoring" << std::endl;
				continue;
			}
			if(rel.IsRelative())
			{
				symbol |= 0x01000000;
			}
			if(rel.size != 4)
			{
				Linker::Warning << "Warning: PDOS/386 only supports 32-bit relocations, generating anyway" << std::endl;
			}
			Linker::Position position = rel.source.GetPosition();
			if(position.segment == code)
			{
				code_relocations[position.address - GetCodeSegment()->base_address] = symbol;
			}
			else if(position.segment == data)
			{
				data_relocations[position.address - GetDataSegment()->base_address] = symbol;
			}
			else if(position.segment != nullptr)
			{
				Linker::Error << "Error: Unable to generate relocation for segment `" << position.segment->name << "', ignoring" << std::endl;
			}
			else
			{
				Linker::Error << "Error: Unable to generate global relocation, ignoring" << std::endl;
			}
		}
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	/* TODO: can it be changed? (check on DJGPP and PDOS386 as well!) */
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		entry_address = entry.GetPosition().address;
	}
	else
	{
		entry_address = GetCodeSegment()->base_address;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void AOutFormat::CalculateValues()
{
	if(system == DJGPP1 && stub.filename != "")
	{
		file_offset = stub.GetStubImageSize();
	}

	endiantype = GetEndianType();
	word_size = GetWordSize();
	code_size = GetCodeSegment()->data_size;
	data_size = GetDataSegment()->data_size;
	bss_size  = GetBssSegment()->zero_fill;
	symbol_table_size = 0;
	code_relocation_size = code_relocations.size() * 8;
	data_relocation_size = data_relocations.size() * 8;
}

void AOutFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	switch(module.cpu)
	{
	case Linker::Module::I386:
		cpu = I80386;
		break;
	case Linker::Module::M68K:
		cpu = M68020; /* TODO: maybe M68010 is enough? */
		break;
	default:
		Linker::Error << "Error: Format only supports Intel 80386 and Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string AOutFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	switch(system)
	{
	case DJGPP1:
	case PDOS386:
		return filename + ".exe";
	case UNIX:
		return filename;
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::string AOutFormat::GetDefaultExtension(Linker::Module& module) const
{
	switch(system)
	{
	case DJGPP1:
	case PDOS386:
		return "a.exe";
	case UNIX:
		return "a.out";
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

