
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

void ELFFormat::Section::Dump(Dumper::Dumper& dump, size_t wordbytes, unsigned index)
{
	switch(type)
	{
	// TODO: other types
	default:
		{
			Dumper::Block section_block("Section", file_offset, section, address, 2 * wordbytes);
			section_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index));
			section_block.InsertField(1, "Name", Dumper::StringDisplay::Make(), name);
			section_block.InsertField(2, "Name offset", Dumper::HexDisplay::Make(), offset_t(name_offset));

			std::map<offset_t, std::string> type_descriptions;
			type_descriptions[SHT_NULL] = "SHT_NULL - Inactive";
			type_descriptions[SHT_PROGBITS] = "SHT_PROGBITS - Program data";
			type_descriptions[SHT_SYMTAB] = "SHT_SYMTAB - Symbol table";
			type_descriptions[SHT_STRTAB] = "SHT_STRTAB - String table";
			type_descriptions[SHT_RELA] = "SHT_RELA - Relocations with explicit addends";
			type_descriptions[SHT_HASH] = "SHT_HASH - Hash table";
			type_descriptions[SHT_DYNAMIC] = "SHT_DYNAMIC - Dynamic linking information";
			type_descriptions[SHT_NOTE] = "SHT_NOTE - File information";
			type_descriptions[SHT_NOBITS] = "SHT_NOBITS - Zero filled section";
			type_descriptions[SHT_REL] = "SHT_REL - Relocations without explicit addends";
			type_descriptions[SHT_SHLIB] = "SHT_SHLIB - reserved";
			type_descriptions[SHT_DYNSYM] = "SHT_DYNSYM - Dynamic linking symbol table";
			type_descriptions[SHT_INIT_ARRAY] = "SHT_INIT_ARRAY - Array of initialization functions";
			type_descriptions[SHT_FINI_ARRAY] = "SHT_INIT_ARRAY - Array of termination functions";
			type_descriptions[SHT_PREINIT_ARRAY] = "SHT_PREINIT_ARRAY - Array of pre-initialization functions";
			type_descriptions[SHT_GROUP] = "SHT_GROUP - Section group";
			type_descriptions[SHT_SYMTAB_SHNDX] = "SHT_SYMTAB_SHNDX - Symbol table section indexes";
				/* IBM OS/2 specific */
			type_descriptions[SHT_OS] = "SHT_OS - (IBM OS/2) Target operating system identification";
			type_descriptions[SHT_IMPORTS] = "SHT_IMPORTS - (IBM OS/2) External symbol references";
			type_descriptions[SHT_EXPORTS] = "SHT_EXPORTS - (IBM OS/2) Exported symbols";
			type_descriptions[SHT_RES] = "SHT_RES - (IBM OS/2) Resource data";
				/* GNU extensions */
			type_descriptions[SHT_GNU_INCREMENTAL_INPUTS] = "SHT_GNU_INCREMENTAL_INPUTS";
			type_descriptions[SHT_GNU_ATTRIBUTES] = "SHT_GNU_ATTRIBUTES";
			type_descriptions[SHT_GNU_HASH] = "SHT_GNU_HASH";
			type_descriptions[SHT_GNU_LIBLIST] = "SHT_GNU_LIBLIST";
			type_descriptions[SHT_SUNW_verdef] = "SHT_SUNW_verdef";
			type_descriptions[SHT_SUNW_verneed] = "SHT_SUNW_verneed";
			type_descriptions[SHT_SUNW_versym] = "SHT_SUNW_versym";
			section_block.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(type));

			section_block.AddField("Flags",
				Dumper::BitFieldDisplay::Make()
					->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("SHF_WRITE - writable"), true)
					->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("SHF_ALLOC - occupies memory during execution"), true)
					->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("SHF_EXECINSTR - contains executable instructions"), true)
					->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("SHF_MERGE - section data may be merged to eliminate duplication"), true)
					->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("SHF_STRINGS - null-terminated strings"), true)
					->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("SHF_INFO_LINK - sh_info contains section index"), true)
					->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("SHF_LINK_ORDER - order with section in sh_link must be preserved"), true)
					->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("SHF_OS_NONCONFORMING - requires OS-specific processing"), true)
					->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("SHF_GROUP - member of a section group"), true)
					->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("SHF_TLS - thread-local storage"), true)
					->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("SHF_COMPRESSED - compressed"), true),
				offset_t(flags));

			// TODO: only display if NOBITS or section data is nullptr?
			section_block.AddField("Size", Dumper::HexDisplay::Make(), offset_t(size));

			std::string name_link;
			switch(type)
			{
			case SHT_DYNAMIC:
			case SHT_SYMTAB:
			case SHT_DYNSYM:
			case SHT_GROUP:
			case SHT_SYMTAB_SHNDX:
				name_link = "Link to string table";
				break;
			case SHT_HASH:
			case SHT_REL:
			case SHT_RELA:
				name_link = "Link to symbol table";
				break;
			default:
				name_link = "Link";
				break;
			}
			section_block.AddOptionalField(name_link, Dumper::DecDisplay::Make(), offset_t(link));

			std::string name_info;
			switch(type)
			{
			case SHT_GROUP:
				name_link = "Info, symbol index";
				break;
			case SHT_SYMTAB:
			case SHT_DYNSYM:
				name_link = "Info, symbol index after last local symbol";
				break;
			case SHT_REL:
			case SHT_RELA:
				name_link = "Info, section index";
				break;
			default:
				if((flags & SHF_INFO_LINK))
					name_link = "Info, section index";
				else
					name_link = "Info";
				break;
			}
			section_block.AddOptionalField(name_info, Dumper::DecDisplay::Make(), offset_t(info));

			section_block.AddField("Alignment", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(align));

			std::string name_entsize;
			if((flags & SHF_STRINGS))
				name_entsize = "Character size";
			else
				name_entsize = "Entry size";
			section_block.AddOptionalField(name_entsize, Dumper::HexDisplay::Make(2 * wordbytes), offset_t(entsize));
			section_block.Display(dump);
		}
		break;
	}
}

void ELFFormat::ReadFile(Linker::Reader& rd)
{
	rd.Seek(4); // skip signature

	file_class = rd.ReadUnsigned(1);
	switch(file_class)
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
			message << "Fatal error: Illegal ELF class " << int(file_class);
			Linker::FatalError(message.str());
		}
	}

	data_encoding = rd.ReadUnsigned(1);
	switch(data_encoding)
	{
	case ELFDATA2LSB:
		endiantype = rd.endiantype = ::LittleEndian;
		break;
	case ELFDATA2MSB:
		endiantype = rd.endiantype = ::BigEndian;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: Illegal ELF byte order " << int(data_encoding);
			Linker::FatalError(message.str());
		}
	}

	header_version = rd.ReadUnsigned(1);
	if(header_version != EV_CURRENT)
	{
#if 0
		std::ostringstream message;
		message << "Fatal error: Unrecognized ELF header version " << int(header_version);
		Linker::FatalError(message.str());
#endif
		Linker::Warning << "Warning: Unrecognized ELF header version " << int(header_version);
	}

	osabi = rd.ReadUnsigned(1);
	abi_version = rd.ReadUnsigned(1);

	rd.Seek(16);

	object_file_type = file_type(rd.ReadUnsigned(2));
	cpu = cpu_type(rd.ReadUnsigned(2));
	file_version = rd.ReadUnsigned(4);
	if(file_version != EV_CURRENT)
	{
#if 0
		std::ostringstream message;
		message << "Fatal error: Unrecognized ELF file version " << int(file_version);
		Linker::FatalError(message.str());
#endif
		Linker::Warning << "Warning: Unrecognized ELF file version " << int(file_version);
	}

	entry = rd.ReadUnsigned(wordbytes);
	program_header_offset = rd.ReadUnsigned(wordbytes); // phoff
	section_header_offset = rd.ReadUnsigned(wordbytes); // shoff
	flags = rd.ReadUnsigned(4);
	elf_header_size = rd.ReadUnsigned(2); // ehsize
	program_header_entry_size = rd.ReadUnsigned(2); // phentsize
	uint16_t phnum = rd.ReadUnsigned(2);
	section_header_entry_size = rd.ReadUnsigned(2); // shentsize
	uint16_t shnum = rd.ReadUnsigned(2);
	section_name_string_table = rd.ReadUnsigned(2); // shstrndx

	for(size_t i = 0; i < shnum; i++)
	{
		rd.Seek(section_header_offset + i * section_header_entry_size);
		Section section;
		section.name_offset = rd.ReadUnsigned(4);
		section.type = Section::section_type(rd.ReadUnsigned(4));
		section.flags = rd.ReadUnsigned(wordbytes);
		section.address = rd.ReadUnsigned(wordbytes);
		section.file_offset = rd.ReadUnsigned(wordbytes);
		section.size = rd.ReadUnsigned(wordbytes);
		section.link = rd.ReadUnsigned(4);
		section.info = rd.ReadUnsigned(4);
		section.align = rd.ReadUnsigned(wordbytes);
		section.entsize = rd.ReadUnsigned(wordbytes);
		sections.push_back(section);
	}

	for(size_t i = 0; i < phnum; i++)
	{
		rd.Seek(program_header_offset + i * program_header_entry_size);
		Segment segment;
		segment.type = rd.ReadUnsigned(4);
		if(wordbytes == 8)
			segment.flags = rd.ReadUnsigned(4);
		segment.offset = rd.ReadUnsigned(wordbytes);
		segment.vaddr = rd.ReadUnsigned(wordbytes);
		segment.paddr = rd.ReadUnsigned(wordbytes);
		segment.filesz = rd.ReadUnsigned(wordbytes);
		segment.memsz = rd.ReadUnsigned(wordbytes);
		if(wordbytes == 4)
			segment.flags = rd.ReadUnsigned(4);
		segment.align = rd.ReadUnsigned(wordbytes);
		segments.push_back(segment);
	}

	for(size_t i = 0; i < shnum; i++)
	{
		rd.Seek(sections[section_name_string_table].file_offset + sections[i].name_offset);
		sections[i].name = rd.ReadASCIIZ();
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Section #" << i << ": `" << sections[i].name << "'" << ", type: " << sections[i].type << ", flags: " << sections[i].flags << std::endl;
#endif
		sections[i].section = nullptr;
		switch(sections[i].type)
		{
		case Section::SHT_PROGBITS:
			sections[i].section = std::make_shared<Linker::Section>(sections[i].name);
			sections[i].section->Expand(sections[i].size);
			rd.Seek(sections[i].file_offset);
			sections[i].section->ReadFile(rd);
			break;
		case Section::SHT_SYMTAB:
		case Section::SHT_DYNSYM:
			for(size_t j = 0; j < sections[i].size; j += sections[i].entsize)
			{
				rd.Seek(sections[i].file_offset + j);
				Symbol symbol;
				symbol.name_offset = rd.ReadUnsigned(4);
				if(wordbytes == 4)
				{
					symbol.value = rd.ReadUnsigned(wordbytes);
					symbol.size = rd.ReadUnsigned(wordbytes);
					symbol.type = rd.ReadUnsigned(1);
					symbol.bind = symbol.type >> 4;
					symbol.type &= 0xF;
					symbol.other = rd.ReadUnsigned(1);
					symbol.shndx = rd.ReadUnsigned(2);
				}
				else
				{
					symbol.type = rd.ReadUnsigned(1);
					symbol.bind = symbol.type >> 4;
					symbol.type &= 0xF;
					symbol.other = rd.ReadUnsigned(1);
					symbol.shndx = rd.ReadUnsigned(2);
					symbol.value = rd.ReadUnsigned(wordbytes);
					symbol.size = rd.ReadUnsigned(wordbytes);
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
				rd.Seek(sections[i].file_offset + j);
				Relocation rel;
				rel.offset = rd.ReadUnsigned(wordbytes);
				offset_t info = rd.ReadUnsigned(wordbytes);
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
					rel.addend = rd.ReadUnsigned(wordbytes);
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
			}
//			if((sections[i].flags & SHF_GROUP))
//			{
//				Linker::Debug << "Debug: Groups currently not supported" << std::endl;
//				/* TODO - when?? */
//			}
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
				rd.Seek(sections[symbol.sh_link].file_offset + symbol.name_offset);
				symbol.name = rd.ReadASCIIZ();
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
				rd.Seek(covered);
				data->image = Linker::Buffer::ReadFromFile(rd, next_offset - covered);
				segments[i].blocks.push_back(data);
				covered = next_offset;
			}
			offset = next_offset;
		}
	}

	if(cpu == EM_HOBBIT)
	{
		/* BeOS Hobbit section */
		rd.SeekEnd();
		offset_t end = rd.Tell();
		rd.Seek(end - 8);
		if(rd.ReadData(4) == "RSRC")
		{
			Linker::Debug << "Debug: There is an AT&T Hobbit BeOS resource block" << std::endl;
			hobbit_beos_resource_offset = rd.ReadUnsigned(4);
			rd.Seek(hobbit_beos_resource_offset);
			if(rd.Tell() != hobbit_beos_resource_offset)
			{
				Linker::Warning << "Warning: Invalid resource block" << std::endl;
				hobbit_beos_resource_offset = 0;
			}
			else
			{
				uint32_t resource_count = rd.ReadUnsigned(4);
				for(uint32_t i = 0; i < resource_count; i++)
				{
					HobbitBeOSResource resource;
					rd.ReadData(4, resource.type);
					resource.unknown1 = rd.ReadUnsigned(4);
					resource.offset = rd.ReadUnsigned(4);
					resource.size = rd.ReadUnsigned(4);
					resource.unknown2 = rd.ReadUnsigned(4);
					hobbit_beos_resources.push_back(resource);
				}
				for(auto& resource : hobbit_beos_resources)
				{
					rd.Seek(hobbit_beos_resource_offset + 4 + 20 * hobbit_beos_resources.size() + resource.offset);
					resource.image = Linker::Buffer::ReadFromFile(rd, resource.size);
				}
			}
		}
	}
}

void ELFFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO: test */

	wr.endiantype = endiantype;

	wr.WriteData(4, "\7F" "ELF");
	wr.WriteWord(1, file_class);
	wr.WriteWord(1, data_encoding);
	wr.WriteWord(1, header_version);
	wr.WriteWord(1, osabi);
	wr.WriteWord(1, abi_version);

	wr.Seek(16);

	wr.WriteWord(2, object_file_type);
	wr.WriteWord(2, cpu);
	wr.WriteWord(4, file_version);
	wr.WriteWord(wordbytes, entry);
	wr.WriteWord(wordbytes, program_header_offset); // phoff
	wr.WriteWord(wordbytes, section_header_offset); // shoff
	wr.WriteWord(4, flags);
	wr.WriteWord(2, elf_header_size); // ehsize
	wr.WriteWord(2, program_header_entry_size); // phentsize
	wr.WriteWord(2, segments.size()); // phnum
	wr.WriteWord(2, section_header_entry_size); // shentsize
	wr.WriteWord(2, sections.size()); // shnum
	wr.WriteWord(2, section_name_string_table); // shstrndx

	unsigned i;

	i = 0;
	for(auto& segment : segments)
	{
		wr.Seek(program_header_offset + i++ * program_header_entry_size);
		wr.WriteWord(4, segment.type);
		if(wordbytes == 8)
			wr.WriteWord(4, segment.flags);
		wr.WriteWord(wordbytes, segment.offset);
		wr.WriteWord(wordbytes, segment.vaddr);
		wr.WriteWord(wordbytes, segment.paddr);
		wr.WriteWord(wordbytes, segment.filesz);
		wr.WriteWord(wordbytes, segment.memsz);
		if(wordbytes == 4)
			wr.WriteWord(4, segment.flags);
		wr.WriteWord(wordbytes, segment.align);
	}

	i = 0;
	for(auto& section : sections)
	{
		wr.Seek(section_header_offset + i++ * section_header_entry_size);
		wr.WriteWord(4, section.name_offset);
		wr.WriteWord(4, section.type);
		wr.WriteWord(wordbytes, section.flags);
		wr.WriteWord(wordbytes, section.address);
		wr.WriteWord(wordbytes, section.file_offset);
		wr.WriteWord(wordbytes, section.size);
		wr.WriteWord(4, section.link);
		wr.WriteWord(4, section.info);
		wr.WriteWord(wordbytes, section.align);
		wr.WriteWord(wordbytes, section.entsize);
	}

	/* TODO */
}

void ELFFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("ELF format");

//	Dumper::Region file_region("File", 0, 0, 2 * wordbytes); /* TODO: file size */
//	/* TODO */
//	file_region.Display(dump);

	Dumper::Region identification_region("ELF Identification", 0, 16, 2);
	std::map<offset_t, std::string> class_descriptions;
	class_descriptions[ELFCLASS32] = "32-bit";
	class_descriptions[ELFCLASS64] = "64-bit";
	identification_region.AddField("File class", Dumper::ChoiceDisplay::Make(class_descriptions), offset_t(file_class));
	std::map<offset_t, std::string> encoding_descriptions;
	encoding_descriptions[ELFDATA2LSB] = "2's complement little endian";
	encoding_descriptions[ELFDATA2MSB] = "2's complement big endian";
	identification_region.AddField("Data encoding", Dumper::ChoiceDisplay::Make(encoding_descriptions), offset_t(data_encoding));
	identification_region.AddField("ELF header version", Dumper::DecDisplay::Make(), offset_t(header_version));
	std::map<offset_t, std::string> osabi_descriptions;
	osabi_descriptions[0] = "None";
	identification_region.AddField("OS/ABI extensions", Dumper::ChoiceDisplay::Make(osabi_descriptions), offset_t(osabi));
	identification_region.AddField("ABI version", Dumper::DecDisplay::Make(), offset_t(abi_version));
	identification_region.Display(dump);

	Dumper::Region header_region("ELF Header", 0, elf_header_size, 2 * wordbytes);
	std::map<offset_t, std::string> file_type_descriptions;
	file_type_descriptions[ET_NONE] = "No file type";
	file_type_descriptions[ET_REL] = "Relocatable file";
	file_type_descriptions[ET_EXEC] = "Executable file";
	file_type_descriptions[ET_DYN] = "Shared object file";
	file_type_descriptions[ET_CORE] = "Core file";
	header_region.AddField("Object file type", Dumper::ChoiceDisplay::Make(file_type_descriptions), offset_t(object_file_type));

	std::map<offset_t, std::string> cpu_descriptions;
	// Most of these descriptions are from https://www.sco.com/developers/gabi/latest/ch4.eheader.html
	// Some adjustments using https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
	cpu_descriptions[EM_NONE] = "No machine";
	cpu_descriptions[EM_M32] = "AT&T WE 32100";
	cpu_descriptions[EM_SPARC] = "SUN SPARC";
	cpu_descriptions[EM_386] = "Intel 80386 (also Intel 8086/80286)";
	cpu_descriptions[EM_68K] = "Motorola 68000 (m68k family)";
	cpu_descriptions[EM_88K] = "Motorola 88000 (m88k family)";
	cpu_descriptions[EM_IAMCU] = "Intel MCU";
	cpu_descriptions[EM_860] = "Intel 80860";
	cpu_descriptions[EM_MIPS] = "MIPS I Architecture (MIPS R3000, big-endian only)";
	cpu_descriptions[EM_S370] = "IBM System/370 Processor";
	cpu_descriptions[EM_MIPS_RS3_LE] = "MIPS RS3000 Little-endian (deprecated)";
	cpu_descriptions[EM_OLD_SPARCV9] = "Old version of Sparc v9 (deprecated)"; // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
	cpu_descriptions[EM_PARISC] = "Hewlett-Packard PA-RISC";
	cpu_descriptions[EM_VPP500] = "Fujitsu VPP500";
	cpu_descriptions[EM_SPARC32PLUS] = "Enhanced instruction set SPARC (\"v8plus\")";
	cpu_descriptions[EM_960] = "Intel 80960";
	cpu_descriptions[EM_PPC] = "PowerPC";
	cpu_descriptions[EM_PPC64] = "64-bit PowerPC";
	cpu_descriptions[EM_S390] = "IBM System/390 Processor";
	cpu_descriptions[EM_SPU] = "Sony/Toshiba/IBM SPU/SPC";
	cpu_descriptions[EM_V800] = "NEC V800 series";
	cpu_descriptions[EM_FR20] = "Fujitsu FR20";
	cpu_descriptions[EM_RH32] = "TRW RH-32";
	cpu_descriptions[EM_MCORE] = "Motorola RCE (M*Core) or Fujitsu MMA";
	cpu_descriptions[EM_ARM] = "ARM 32-bit architecture (AARCH32)";
	cpu_descriptions[EM_ALPHA] = "Digital Alpha";
	cpu_descriptions[EM_SH] = "Renesas (Hitachi) SuperH/SH";
	cpu_descriptions[EM_SPARCV9] = "SPARC Version 9 (64-bit)";
	cpu_descriptions[EM_TRICORE] = "Siemens TriCore embedded processor";
	cpu_descriptions[EM_ARC] = "Argonaut RISC Core, Argonaut Technologies Inc.";
	cpu_descriptions[EM_H8_300] = "Renesas (Hitachi) H8/300";
	cpu_descriptions[EM_H8_300H] = "Renesas (Hitachi) H8/300H";
	cpu_descriptions[EM_H8S] = "Renesas (Hitachi) H8S";
	cpu_descriptions[EM_H8_500] = "Renesas (Hitachi) H8/500";
	cpu_descriptions[EM_IA_64] = "Intel IA-64 processor architecture (Itanium)";
	cpu_descriptions[EM_MIPS_X] = "Stanford MIPS-X";
	cpu_descriptions[EM_COLDFIRE] = "Motorola ColdFire";
	cpu_descriptions[EM_68HC12] = "Motorola M68HC12";
	cpu_descriptions[EM_MMA] = "Fujitsu MMA Multimedia Accelerator";
	cpu_descriptions[EM_PCP] = "Siemens PCP";
	cpu_descriptions[EM_NCPU] = "Sony nCPU embedded RISC processor";
	cpu_descriptions[EM_NDR1] = "Denso NDR1 microprocessor";
	cpu_descriptions[EM_STARCORE] = "Motorola Star*Core processor";
	cpu_descriptions[EM_ME16] = "Toyota ME16 processor";
	cpu_descriptions[EM_ST100] = "STMicroelectronics ST100 processor";
	cpu_descriptions[EM_TINYJ] = "Advanced Logic Corp. TinyJ embedded processor family";
	cpu_descriptions[EM_X86_64] = "AMD x86-64 architecture";
	cpu_descriptions[EM_PDSP] = "Sony DSP Processor";
	cpu_descriptions[EM_PDP10] = "Digital Equipment Corp. PDP-10";
	cpu_descriptions[EM_PDP11] = "Digital Equipment Corp. PDP-11";
	cpu_descriptions[EM_FX66] = "Siemens FX66 microcontroller";
	cpu_descriptions[EM_ST9PLUS] = "STMicroelectronics ST9+ 8/16 bit microcontroller";
	cpu_descriptions[EM_ST7] = "STMicroelectronics ST7 8-bit microcontroller";
	cpu_descriptions[EM_68HC16] = "Motorola MC68HC16 Microcontroller";
	cpu_descriptions[EM_68HC11] = "Motorola MC68HC11 Microcontroller";
	cpu_descriptions[EM_68HC08] = "Motorola MC68HC08 Microcontroller";
	cpu_descriptions[EM_68HC05] = "Motorola MC68HC05 Microcontroller";
	cpu_descriptions[EM_SVX] = "Silicon Graphics SVx";
	cpu_descriptions[EM_ST19] = "STMicroelectronics ST19 8-bit microcontroller";
	cpu_descriptions[EM_VAX] = "Digital VAX";
	cpu_descriptions[EM_CRIS] = "Axis Communication 32-bit embedded processor";
	cpu_descriptions[EM_JAVELIN] = "Infineon Technologies 32-bit embedded processor";
	cpu_descriptions[EM_FIREPATH] = "Element 14 64-bit DSP Processor";
	cpu_descriptions[EM_ZSP] = "LSI Logic 16-bit DSP Processor";
	cpu_descriptions[EM_MMIX] = "Donald Knuth's educational 64-bit processor";
	cpu_descriptions[EM_HUANY] = "Harvard University machine-independent object file";
	cpu_descriptions[EM_PRISM] = "SiTera Prism";
	cpu_descriptions[EM_AVR] = "Atmel AVR 8-bit microcontroller";
	cpu_descriptions[EM_FR30] = "Fujitsu FR30";
	cpu_descriptions[EM_D10V] = "Mitsubishi D10V";
	cpu_descriptions[EM_D30V] = "Mitsubishi D30V";
	cpu_descriptions[EM_V850] = "Renesas (NEC) v850";
	cpu_descriptions[EM_M32R] = "Renesas (Mitsubishi) M32R";
	cpu_descriptions[EM_MN10300] = "Matsushita MN10300";
	cpu_descriptions[EM_MN10200] = "Matsushita MN10200";
	cpu_descriptions[EM_PJ] = "picoJava";
	cpu_descriptions[EM_OPENRISC] = "OpenRISC 1000 32-bit embedded processor";
	cpu_descriptions[EM_ARC_COMPACT] = "ARC International ARCompact processor";
	cpu_descriptions[EM_XTENSA] = "Tensilica Xtensa Architecture";
	cpu_descriptions[EM_VIDEOCORE] = "Alphamosaic VideoCore processor/old Sunplus S+core7 backend magic number"; // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
	cpu_descriptions[EM_TMM_GPP] = "Thompson Multimedia General Purpose Processor";
	cpu_descriptions[EM_NS32K] = "National Semiconductor 32000 series";
	cpu_descriptions[EM_TPC] = "Tenor Network TPC processor";
	cpu_descriptions[EM_SNP1K] = "Trebia SNP 1000 processor/old picoJava";
	cpu_descriptions[EM_ST200] = "STMicroelectronics (www.st.com) ST200 microcontroller";
	cpu_descriptions[EM_IP2K] = "Ubicom IP2022/IP2xxx microcontroller family";
	cpu_descriptions[EM_MAX] = "MAX Processor";
	cpu_descriptions[EM_CR] = "National Semiconductor CompactRISC microprocessor";
	cpu_descriptions[EM_F2MC16] = "Fujitsu F2MC16";
	cpu_descriptions[EM_MSP430] = "Texas Instruments embedded microcontroller msp430";
	cpu_descriptions[EM_BLACKFIN] = "Analog Devices Blackfin (DSP) processor";
	cpu_descriptions[EM_SE_C33] = "S1C33 Family of Seiko Epson processor";
	cpu_descriptions[EM_SEP] = "Sharp embedded microprocessor";
	cpu_descriptions[EM_ARCA] = "Arca RISC Microprocessor";
	cpu_descriptions[EM_UNICORE] = "Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University";
	cpu_descriptions[EM_EXCESS] = "eXcess: 16/32/64-bit configurable embedded CPU";
	cpu_descriptions[EM_DXP] = "Icera Semiconductor Inc. Deep Execution Processor";
	cpu_descriptions[EM_ALTERA_NIOS32] = "Altera Nios II soft-core processor";
	cpu_descriptions[EM_CRX] = "National Semiconductor CompactRISC CRX microprocessor";
	cpu_descriptions[EM_XGATE] = "Motorola XGATE embedded processor/old National Semiconductor CompactRISC";
	cpu_descriptions[EM_C166] = "Infineon C16x/XC16x processor";
	cpu_descriptions[EM_M16C] = "Renesas M16C series microprocessors";
	cpu_descriptions[EM_DSPIC30F] = "Microchip Technology dsPIC30F Digital Signal Controller";
	cpu_descriptions[EM_CE] = "Freescale Communication Engine RISC core";
	cpu_descriptions[EM_M32C] = "Renesas M32C series microprocessors";
	cpu_descriptions[EM_TSK3000] = "Altium TSK3000 core";
	cpu_descriptions[EM_RS08] = "Freescale RS08 embedded processor";
	cpu_descriptions[EM_SHARC] = "Analog Devices SHARC family of 32-bit DSP processors";
	cpu_descriptions[EM_ECOG2] = "Cyan Technology eCOG2 microprocessor";
	cpu_descriptions[EM_SCORE7] = "Sunplus Score/Sunplus S+core7 RISC processor";
	cpu_descriptions[EM_DSP24] = "New Japan Radio (NJR) 24-bit DSP Processor";
	cpu_descriptions[EM_VIDEOCORE3] = "Broadcom Videocore III processor";
	cpu_descriptions[EM_LATTICEMICO32] = "RISC processor for Lattice FPGA architecture";
	cpu_descriptions[EM_SE_CE17] = "Seiko Epson C17 family";
	cpu_descriptions[EM_TI_C6000] = "The Texas Instruments TMS320C6000 DSP family";
	cpu_descriptions[EM_TI_C2000] = "The Texas Instruments TMS320C2000 DSP family";
	cpu_descriptions[EM_TI_C5500] = "The Texas Instruments TMS320C55x DSP family";
	cpu_descriptions[EM_TI_ARP32] = "Texas Instruments Application Specific RISC Processor, 32-bit fetch";
	cpu_descriptions[EM_TI_PRU] = "Texas Instruments Programmable Realtime Unit";
	cpu_descriptions[EM_MMDSP_PLUS] = "STMicroelectronics 64bit VLIW Data Signal Processor";
	cpu_descriptions[EM_CYPRESS_M8C] = "Cypress M8C microprocessor";
	cpu_descriptions[EM_R32C] = "Renesas R32C series microprocessors";
	cpu_descriptions[EM_TRIMEDIA] = "NXP Semiconductors TriMedia architecture family";
	cpu_descriptions[EM_QDSP6] = "QUALCOMM DSP6 Processor";
	cpu_descriptions[EM_8051] = "Intel 8051 and variants";
	cpu_descriptions[EM_STXP7X] = "STMicroelectronics STxP7x family of configurable and extensible RISC processors";
	cpu_descriptions[EM_NDS32] = "Andes Technology compact code size embedded RISC processor family";
	cpu_descriptions[EM_ECOG1] = "Cyan Technology eCOG1X family";
	cpu_descriptions[EM_MAXQ30] = "Dallas Semiconductor MAXQ30 Core Micro-controllers";
	cpu_descriptions[EM_XIMO16] = "New Japan Radio (NJR) 16-bit DSP Processor";
	cpu_descriptions[EM_MANIK] = "M2000 Reconfigurable RISC Microprocessor";
	cpu_descriptions[EM_CRAYNV2] = "Cray Inc. NV2 vector architecture";
	cpu_descriptions[EM_RX] = "Renesas RX family";
	cpu_descriptions[EM_METAG] = "Imagination Technologies META processor architecture";
	cpu_descriptions[EM_MCST_ELBRUS] = "MCST Elbrus general purpose hardware architecture";
	cpu_descriptions[EM_ECOG16] = "Cyan Technology eCOG16 family";
	cpu_descriptions[EM_CR16] = "National Semiconductor CompactRISC CR16 16-bit microprocessor";
	cpu_descriptions[EM_ETPU] = "Freescale Extended Time Processing Unit";
	cpu_descriptions[EM_SLE9X] = "Infineon Technologies SLE9X core";
	cpu_descriptions[EM_L10M] = "Intel L10M";
	cpu_descriptions[EM_K10M] = "Intel K10M";
	cpu_descriptions[EM_AARCH64] = "ARM 64-bit architecture (AARCH64)";
	cpu_descriptions[EM_AVR32] = "Atmel Corporation 32-bit microprocessor family";
	cpu_descriptions[EM_STM8] = "STMicroelectronics STM8 8-bit microcontroller";
	cpu_descriptions[EM_TILE64] = "Tilera TILE64 multicore architecture family";
	cpu_descriptions[EM_TILEPRO] = "Tilera TILEPRO multicore architecture family";
	cpu_descriptions[EM_MICROBLAZE] = "Xilinx MicroBlaze 32-bit RISC soft processor core";
	cpu_descriptions[EM_CUDA] = "NVIDIA CUDA architecture";
	cpu_descriptions[EM_TILEGX] = "Tilera TILE-Gx multicore architecture family";
	cpu_descriptions[EM_CLOUDSHIELD] = "CloudShield architecture family";
	cpu_descriptions[EM_COREA_1ST] = "KIPO-KAIST Core-A 1st generation processor family";
	cpu_descriptions[EM_COREA_2ND] = "KIPO-KAIST Core-A 2nd generation processor family";
	cpu_descriptions[EM_ARC_COMPACT2] = "Synopsys ARCompact V2";
	cpu_descriptions[EM_OPEN8] = "Open8 8-bit RISC soft processor core";
	cpu_descriptions[EM_RL78] = "Renesas RL78 family";
	cpu_descriptions[EM_VIDEOCORE5] = "Broadcom VideoCore V processor";
	cpu_descriptions[EM_78KOR] = "Renesas 78KOR family";
	cpu_descriptions[EM_56800EX] = "Freescale 56800EX Digital Signal Controller (DSC)";
	cpu_descriptions[EM_BA1] = "Beyond BA1 CPU architecture";
	cpu_descriptions[EM_BA2] = "Beyond BA2 CPU architecture";
	cpu_descriptions[EM_XCORE] = "XMOS xCORE processor family";
	cpu_descriptions[EM_MCHP_PIC] = "Microchip 8-bit PIC(r) family";
	cpu_descriptions[EM_INTELGT] = "Intel Graphics Technology"; // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
	cpu_descriptions[EM_KM32] = "KM211 KM32 32-bit processor";
	cpu_descriptions[EM_KMX32] = "KM211 KMX32 32-bit processor";
	cpu_descriptions[EM_KMX16] = "KM211 KMX16 16-bit processor";
	cpu_descriptions[EM_KMX8] = "KM211 KMX8 8-bit processor";
	cpu_descriptions[EM_KVARC] = "KM211 KVARC processor";
	cpu_descriptions[EM_CDP] = "Paneve CDP architecture family";
	cpu_descriptions[EM_COGE] = "Cognitive Smart Memory Processor";
	cpu_descriptions[EM_COOL] = "Bluechip Systems CoolEngine";
	cpu_descriptions[EM_NORC] = "Nanoradio Optimized RISC";
	cpu_descriptions[EM_CSR_KALIMBA] = "CSR Kalimba architecture family";
	cpu_descriptions[EM_Z80] = "Zilog Z80";
	cpu_descriptions[EM_VISIUM] = "Controls and Data Services VISIUMcore processor";
	cpu_descriptions[EM_FT32] = "FTDI Chip FT32 high performance 32-bit RISC architecture";
	cpu_descriptions[EM_MOXIE] = "Moxie processor family";
	cpu_descriptions[EM_AMDGPU] = "AMD GPU architecture";
	cpu_descriptions[EM_RISCV] = "RISC-V";
	// https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
	cpu_descriptions[EM_LANAI] = "Lanai 32-bit processor";
	cpu_descriptions[EM_CEVA] = "CEVA Processor Architecture Family";
	cpu_descriptions[EM_CEVA_X2] = "CEVA X2 Processor Family";
	cpu_descriptions[EM_BPF] = "Linux BPF - in-kernel virtual machine";
	cpu_descriptions[EM_GRAPHCORE_IPU] = "Graphcore Intelligent Processing Unit";
	cpu_descriptions[EM_IMG1] = "Imagination Technologies";
	cpu_descriptions[EM_NFP] = "Netronome Flow Processor";
	cpu_descriptions[EM_VE] = "NEC Vector Engine";
	cpu_descriptions[EM_CSKY] = "C-SKY processor family";
	cpu_descriptions[EM_ARC_COMPACT3_64] = "Synopsys ARCv2.3 64-bit";
	cpu_descriptions[EM_MCS6502] = "MOS Technology MCS 6502 processor";
	cpu_descriptions[EM_ARC_COMPACT3] = "Synopsys ARCv2.3 32-bit";
	cpu_descriptions[EM_KVX] = "Kalray VLIW core of the MPPA processor family";
	cpu_descriptions[EM_65816] = "WDC 65816/65C816";
	cpu_descriptions[EM_LOONGARCH] = "LoongArch";
	cpu_descriptions[EM_KF32] = "ChipOn KungFu32";
	cpu_descriptions[EM_U16_U8CORE] = "LAPIS nX-U16/U8";
	cpu_descriptions[EM_TACHYUM] = "Tachyum";
	cpu_descriptions[EM_56800EF] = "NXP 56800EF Digital Signal Controller (DSC) ";
	cpu_descriptions[EM_HOBBIT] = "AT&T Hobbit";
	cpu_descriptions[EM_MOS] = "MOS 6502 (llvm-mos ELF specification)";
	header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(cpu_descriptions), offset_t(cpu));
	header_region.AddField("Object file version", Dumper::DecDisplay::Make(), offset_t(file_version));
	header_region.AddField("Entry", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(entry));
	header_region.AddField("Flags", Dumper::HexDisplay::Make(4), offset_t(flags));
	header_region.Display(dump);

	Dumper::Region program_header_region("Program header", program_header_offset, segments.size() * program_header_entry_size, 2 * wordbytes);
	program_header_region.AddField("Entry size", Dumper::HexDisplay::Make(4), offset_t(program_header_entry_size));
	program_header_region.AddField("Entry count", Dumper::DecDisplay::Make(), offset_t(segments.size()));
	program_header_region.Display(dump);

	Dumper::Region section_header_region("Section header", section_header_offset, sections.size() * section_header_entry_size, 2 * wordbytes);
	section_header_region.AddField("Entry size", Dumper::HexDisplay::Make(4), offset_t(section_header_entry_size));
	section_header_region.AddField("Entry count", Dumper::DecDisplay::Make(), offset_t(sections.size()));
	section_header_region.AddField("Section name string table", Dumper::DecDisplay::Make(), offset_t(section_name_string_table));
	if(section_name_string_table != 0)
		section_header_region.AddField("Section name string table name", Dumper::StringDisplay::Make(), sections[section_name_string_table].name);
	section_header_region.Display(dump);

	// TODO

	unsigned i = 0;
	for(auto& section : sections)
	{
		section.Dump(dump, wordbytes, i++);
	}

	// TODO: segments

	if(hobbit_beos_resource_offset != 0)
	{
		Dumper::Region resources_region("Resources", hobbit_beos_resource_offset, 4 + 20 * hobbit_beos_resources.size(), 8);
		resources_region.AddField("Entry count", Dumper::DecDisplay::Make(), offset_t(hobbit_beos_resources.size()));
		resources_region.Display(dump);

		i = 0;
		for(auto& resource : hobbit_beos_resources)
		{
			Dumper::Block resource_block("Resource", hobbit_beos_resource_offset + 4 + 20 * hobbit_beos_resources.size() + resource.offset, resource.image, 0, 8);
			resource_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
			resource_block.AddField("Type", Dumper::StringDisplay::Make(4, "'"), std::string(resource.type, 4));
			resource_block.AddField("Unknown entry 1", Dumper::HexDisplay::Make(), resource.unknown1);
			resource_block.AddField("Unknown entry 2", Dumper::HexDisplay::Make(), resource.unknown2);
			resource_block.Display(dump);
			i += 1;
		}
	}
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
		if((section.flags & SHF_MERGE))
		{
			Linker::Debug << "Debug: Mergaeble sections currently not supported" << std::endl;
			/* TODO - with OMF */
		}
		if((section.flags & SHF_GROUP))
		{
			Linker::Debug << "Debug: Groups currently not supported" << std::endl;
			/* TODO - when?? */
		}
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

		default:
			// unknown backend
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

