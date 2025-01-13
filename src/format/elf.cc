
#include "elf.h"

using namespace ELF;

ELFFormat::Segment::Block::~Block()
{
}

offset_t ELFFormat::Segment::Section::GetSize() const
{
	return size;
}

offset_t ELFFormat::Segment::Data::GetSize() const
{
	return image->ActualDataSize();
}

void ELFFormat::ReadFile(Linker::Reader& in)
{
	int c;
	in.Seek(4); // skip signature

	switch(c = in.ReadUnsigned(1))
	{
	case ELFCLASS32:
		wordbytes = 4;
		break;
	case ELFCLASS64:
		wordbytes = 8;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: Illegal ELF class " << int(c);
			Linker::FatalError(message.str());
		}
	}

	switch(in.ReadUnsigned(1))
	{
	case ELFDATA2LSB:
		endiantype = in.endiantype = ::LittleEndian;
		break;
	case ELFDATA2MSB:
		endiantype = in.endiantype = ::BigEndian;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: Illegal ELF byte order " << int(c);
			Linker::FatalError(message.str());
		}
	}

	header_version = in.ReadUnsigned(1);
	if(header_version != EV_CURRENT)
	{
		std::ostringstream message;
		message << "Fatal error: Illegal ELF header version " << int(header_version);
		Linker::FatalError(message.str());
	}

	osabi = in.ReadUnsigned(1);
	abi_version = in.ReadUnsigned(1);

	in.Seek(16);

	object_file_type = file_type(in.ReadUnsigned(2));
	cpu = cpu_type(in.ReadUnsigned(2));
	Linker::Debug << "Debug: " << in.Tell() << std::endl;
	file_version = in.ReadUnsigned(4);
	if(file_version != EV_CURRENT)
	{
		std::ostringstream message;
		message << "Fatal error: Illegal ELF file version " << int(file_version);
		Linker::FatalError(message.str());
	}

	entry = in.ReadUnsigned(wordbytes);
	program_header_offset = in.ReadUnsigned(wordbytes); // phoff
	section_header_offset = in.ReadUnsigned(wordbytes); // shoff
	flags = in.ReadUnsigned(4);
	elf_header_size = in.ReadUnsigned(2); // ehsize
	program_header_entry_size = in.ReadUnsigned(2); // phentsize
	uint16_t phnum = in.ReadUnsigned(2);
	section_header_entry_size = in.ReadUnsigned(2); // shentsize
	uint16_t shnum = in.ReadUnsigned(2);
	section_name_string_table = in.ReadUnsigned(2); // shstrndx

	for(size_t i = 0; i < shnum; i++)
	{
		in.Seek(section_header_offset + i * section_header_entry_size);
		Section section;
		section.name_offset = in.ReadUnsigned(4);
		section.type = Section::section_type(in.ReadUnsigned(4));
		section.flags = in.ReadUnsigned(wordbytes);
		section.address = in.ReadUnsigned(wordbytes);
		section.file_offset = in.ReadUnsigned(wordbytes);
		section.size = in.ReadUnsigned(wordbytes);
		section.link = in.ReadUnsigned(4);
		section.info = in.ReadUnsigned(4);
		section.align = in.ReadUnsigned(wordbytes);
		section.entsize = in.ReadUnsigned(wordbytes);
		sections.push_back(section);
	}

	for(size_t i = 0; i < phnum; i++)
	{
		in.Seek(program_header_offset + i * program_header_entry_size);
		Segment segment;
		segment.type = in.ReadUnsigned(4);
		if(wordbytes == 4)
			segment.flags = in.ReadUnsigned(4);
		segment.offset = in.ReadUnsigned(wordbytes);
		segment.vaddr = in.ReadUnsigned(wordbytes);
		segment.paddr = in.ReadUnsigned(wordbytes);
		segment.filesz = in.ReadUnsigned(wordbytes);
		segment.memsz = in.ReadUnsigned(wordbytes);
		if(wordbytes == 8)
			segment.flags = in.ReadUnsigned(4);
		segment.align = in.ReadUnsigned(wordbytes);
		segments.push_back(segment);
	}

	for(size_t i = 0; i < shnum; i++)
	{
		in.Seek(sections[section_name_string_table].file_offset + sections[i].name_offset);
		sections[i].name = in.ReadASCIIZ();
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Section #" << i << ": `" << sections[i].name << "'" << ", type: " << sections[i].type << ", flags: " << sections[i].flags << std::endl;
#endif
		sections[i].section = nullptr;
		switch(sections[i].type)
		{
		case Section::SHT_PROGBITS:
			sections[i].section = std::make_shared<Linker::Section>(sections[i].name);
			sections[i].section->Expand(sections[i].size);
			in.Seek(sections[i].file_offset);
			sections[i].section->ReadFile(in);
			break;
		case Section::SHT_SYMTAB:
		case Section::SHT_DYNSYM:
			for(size_t j = 0; j < sections[i].size; j += sections[i].entsize)
			{
				in.Seek(sections[i].file_offset + j);
				Symbol symbol;
				symbol.name_offset = in.ReadUnsigned(4);
				if(wordbytes == 4)
				{
					symbol.value = in.ReadUnsigned(wordbytes);
					symbol.size = in.ReadUnsigned(wordbytes);
					symbol.type = in.ReadUnsigned(1);
					symbol.bind = symbol.type >> 4;
					symbol.type &= 0xF;
					symbol.other = in.ReadUnsigned(1);
					symbol.shndx = in.ReadUnsigned(2);
				}
				else
				{
					symbol.type = in.ReadUnsigned(1);
					symbol.bind = symbol.type >> 4;
					symbol.type &= 0xF;
					symbol.other = in.ReadUnsigned(1);
					symbol.shndx = in.ReadUnsigned(2);
					symbol.value = in.ReadUnsigned(wordbytes);
					symbol.size = in.ReadUnsigned(wordbytes);
				}
				symbol.sh_link = sections[i].link;
				sections[i].symbols.push_back(symbol);
			}
			break;
		//case Section::SHT_STRTAB: // TODO
		//	break;
		case Section::SHT_RELA:
		case Section::SHT_REL:
			for(size_t j = 0; j < sections[i].size; j += sections[i].entsize)
			{
				in.Seek(sections[i].file_offset + j);
				Relocation rel;
				rel.offset = in.ReadUnsigned(wordbytes);
				offset_t info = in.ReadUnsigned(wordbytes);
				if(wordbytes == 4)
				{
					rel.symbol = info >> 8;
					rel.type = info & 0xFF;
				}
				else
				{
					rel.symbol = info >> 32;
					rel.type = info;
				}
				rel.addend_from_section_data = sections[i].type == Section::SHT_REL;
				if(rel.addend_from_section_data)
					rel.addend = 0;
				else
					rel.addend = in.ReadUnsigned(wordbytes);
//						Debug::Debug << "Debug: Type " << sections[i].type << " addend " << rel.addend << std::endl;
				rel.sh_link = sections[i].link;
				rel.sh_info = sections[i].info;
				relocations.push_back(rel);
			}
			break;
		case Section::SHT_NOBITS:
			sections[i].section = std::make_shared<Linker::Section>(sections[i].name);
			sections[i].section->SetZeroFilled(true);
			sections[i].section->Expand(sections[i].size);
			break;
		case Section::SHT_GROUP:
			/* TODO */
			break;
		//case Section::SHT_HASH: // TODO
		//	break;
		//case Section::SHT_DYNAMIC: // TODO
		//	break;
		//case Section::SHT_NOTE: // TODO
		//	break;
		//case Section::SHT_INIT_ARRAY: // TODO
		//	break;
		//case Section::SHT_FINI_ARRAY: // TODO
		//	break;
		//case Section::SHT_PREINIT_ARRAY: // TODO
		//	break;
		//case Section::SHT_SYMTAB_SHNDX: // TODO
		//	break;
		default:
			// TODO: read the section anyway
			break;
		}
		if(sections[i].section != nullptr)
		{
			sections[i].section->SetReadable(true);
			if((sections[i].flags & SHF_WRITE))
			{
				sections[i].section->SetWritable(true);
			}
			if((sections[i].flags & SHF_EXECINSTR))
			{
				sections[i].section->SetExecable(true);
			}
			if((sections[i].flags & SHF_MERGE))
			{
				sections[i].section->SetMergeable(true);
				Linker::Debug << "Debug: Mergaeble sections currently not supported" << std::endl;
				/* TODO - with OMF */
			}
			if((sections[i].flags & SHF_GROUP))
			{
				Linker::Debug << "Debug: Groups currently not supported" << std::endl;
				/* TODO - when?? */
			}
#if 0
			if(option_stack_section && sections[i].name == ".stack")
			{
				sections[i].section->SetFlag(Linker::Section::Stack);
			}
			if(option_heap_section && sections[i].name == ".heap")
			{
				sections[i].section->SetFlag(Linker::Section::Heap);
			}
#endif
		}
	}

#if DISPLAY_LOGS
	size_t i = 0;
#endif
	for(Section& section : sections)
	{
		if(section.type == 2)
		{
			/* SYMTAB */
			for(Symbol& symbol : section.symbols)
			{
				in.Seek(sections[symbol.sh_link].file_offset + symbol.name_offset);
				symbol.name = in.ReadASCIIZ();
#if DISPLAY_LOGS
				Linker::Debug << "Debug: Symbol #" << i << ": `" << symbol.name << "' = 0x" << std::hex << symbol.shndx << ":" << std::dec << symbol.value << std::endl;
#endif
				symbol.location = Linker::Location();
				symbol.defined = false;
				switch(symbol.shndx)
				{
				case SHN_UNDEF:
					break;
				case SHN_ABS:
					symbol.location = Linker::Location(symbol.value);
					symbol.defined = true;
					break;
				case SHN_COMMON:
					symbol.unallocated = true;
					symbol.specification = Linker::CommonSymbol(symbol.name, symbol.size, symbol.value);
					break;
				case SHN_XINDEX:
					Linker::Warning << "Warning: Extended section numbers not supported" << std::endl;
					break;
				default:
					symbol.location = Linker::Location(sections[symbol.shndx].section, symbol.value);
					symbol.defined = true;
				}
#if DISPLAY_LOGS
				i++;
#endif
			}
		}
	}

	// TODO: test
	for(size_t i = 0; i < phnum; i++)
	{
		offset_t offset = segments[i].offset;
		offset_t end = segments[i].offset + segments[i].filesz;
		offset_t covered = offset;
		while(offset < end)
		{
			offset_t next_offset = end;
			for(size_t j = 0; j < shnum; j++)
			{
				if(offset < sections[j].file_offset && sections[j].file_offset < next_offset)
				{
					next_offset = sections[j].file_offset;
				}

				if(sections[j].type == Section::SHT_NOBITS)
				{
					if(sections[j].file_offset == offset)
					{
						segments[i].blocks.push_back(std::make_shared<Segment::Section>(j, 0, 0));
					}
				}
				else
				{
					if(sections[j].file_offset == offset)
					{
						offset_t size = std::min(end - offset, sections[j].size);
						segments[i].blocks.push_back(std::make_shared<Segment::Section>(j, 0, size));
						if(covered < offset + size)
							covered = offset + size;
					}
					else if(offset == segments[i].offset && sections[j].file_offset < offset && offset < sections[j].file_offset + sections[j].size)
					{
						offset_t cutoff = offset - sections[j].file_offset;
						offset_t size = std::min(end - offset, sections[j].size - cutoff);
						segments[i].blocks.push_back(std::make_shared<Segment::Section>(j, cutoff, size));
						if(covered < offset + size)
							covered = offset + size;
					}
				}
			}
			if(covered < next_offset)
			{
				std::shared_ptr<Segment::Data> data = std::make_shared<Segment::Data>();
				data->file_offset = covered;
				in.Seek(covered);
				data->image = Linker::Buffer::ReadFromFile(in, next_offset - covered);
				segments[i].blocks.push_back(data);
				covered = next_offset;
			}
			offset = next_offset;
		}
	}
}

void ELFFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

void ELFFormat::Dump(Dumper::Dumper& dump)
{
	/* TODO */
}

void ELFFormat::SetupOptions(std::shared_ptr<Linker::OutputFormat> format)
{
	option_16bit = format->FormatIs16bit();
	option_linear = format->FormatIsLinear();
}

void ELFFormat::GenerateModule(Linker::Module& module) const
{
	switch(cpu)
	{
	case EM_386:
		module.cpu = option_16bit ? Linker::Module::I86 : Linker::Module::I386;
		break;
	case EM_68K:
		module.cpu = Linker::Module::M68K;
		break;
	case EM_ARM:
		module.cpu = Linker::Module::ARM;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: Unknown CPU type in ELF file " << cpu;
			Linker::FatalError(message.str());
		}
	}

	for(const Section& section : sections)
	{
		if((section.flags & SHF_ALLOC) != 0 && section.section != nullptr)
		{
			module.AddSection(section.section);
		}
	}

	for(const Section& section : sections)
	{
		if(section.type == 2)
		{
			/* SYMTAB */
			for(const Symbol& symbol : section.symbols)
			{
				if(symbol.name != "" && symbol.defined)
				{
					switch(symbol.bind)
					{
					case STB_LOCAL:
						module.AddLocalSymbol(symbol.name, symbol.location);
						break;
					default:
						module.AddGlobalSymbol(symbol.name, symbol.location);
						break;
					}
				}
				else if(symbol.unallocated)
				{
					module.AddCommonSymbol(symbol.name, symbol.specification);
				}
				else
				{
					module.AddUndefinedSymbol(symbol.name);
				}
			}
		}
	}

	for(Relocation rel : relocations)
	{
		Linker::Location rel_source = Linker::Location(sections[rel.sh_info].section, rel.offset);
		const Symbol& sym_target = sections[rel.sh_link].symbols[rel.symbol];
		Linker::Target rel_target = sym_target.defined ? Linker::Target(sym_target.location) : Linker::Target(Linker::SymbolName(sym_target.name));
		Linker::Relocation obj_rel = Linker::Relocation::Empty();
		size_t rel_size;
		switch(cpu)
		{
		case EM_386:
			/* TODO: 386 linear model will have to use Absolute instead of Offset */
			switch(rel.type)
			{
			case R_386_8:
			case R_386_PC8:
				rel_size = 1;
				break;
			case R_386_16:
			case R_386_PC16:
				rel_size = 2;
				break;
			case R_386_32:
			case R_386_PC32:
				rel_size = 4;
				break;
			default:
				continue;
			}

			switch(rel.type)
			{
			case R_386_8:
			case R_386_16:
			case R_386_32:
				obj_rel =
					option_linear
					? Linker::Relocation::Absolute(rel_size, rel_source, rel_target, rel.addend, ::LittleEndian)
					: Linker::Relocation::Offset(rel_size, rel_source, rel_target, rel.addend, ::LittleEndian);
				break;
			case R_386_PC8:
			case R_386_PC16:
			case R_386_PC32:
				obj_rel = Linker::Relocation::Relative(rel_size, rel_source, rel_target, rel.addend, ::LittleEndian);
				break;
			}
			break;

		case EM_68K:
			switch(rel.type)
			{
			case R_68K_8:
			case R_68K_PC8:
				rel_size = 1;
				break;
			case R_68K_16:
			case R_68K_PC16:
				rel_size = 2;
				break;
			case R_68K_32:
			case R_68K_PC32:
				rel_size = 4;
				break;
			default:
				continue;
			}

			switch(rel.type)
			{
			case R_68K_8:
			case R_68K_16:
			case R_68K_32:
				obj_rel = Linker::Relocation::Absolute(rel_size, rel_source, rel_target, rel.addend, ::BigEndian);
				break;
			case R_68K_PC8:
			case R_68K_PC16:
			case R_68K_PC32:
				obj_rel = Linker::Relocation::Relative(rel_size, rel_source, rel_target, rel.addend, ::BigEndian);
				break;
			}
			break;

		case EM_ARM:
			/* TODO: use endianness of object file, or make parametrizable */
			switch(rel.type)
			{
			case R_ARM_ABS32:
				obj_rel = Linker::Relocation::Absolute(4, rel_source, rel_target, rel.addend, ::LittleEndian);
				break;
			case R_ARM_CALL:
			case R_ARM_JUMP24:
			case R_ARM_PC24:
				obj_rel = Linker::Relocation::Relative(4, rel_source, rel_target, rel.addend, ::LittleEndian);
				obj_rel.SetMask(0x00FFFFFF);
				obj_rel.SetShift(2);
				break;
			case R_ARM_V4BX:
				continue;
			default:
				Linker::Warning << "Warning: unhandled ARM relocation type " << rel.type << std::endl;
				continue;
			}
			break;
		}
		if(rel.addend_from_section_data)
			obj_rel.AddCurrentValue();
		module.AddRelocation(obj_rel);
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Relocation at #" << rel.sh_info << ":" << std::hex << rel.offset << std::dec << " to symbol " << rel.symbol << ", type " << rel.type << std::endl;
#endif
	}
}

void ELFFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	ReadFile(rd);
	GenerateModule(module);
}

