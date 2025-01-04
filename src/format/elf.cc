
#include "elf.h"

using namespace ELF;

void ELFFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

void ELFFormat::SetupOptions(char special_char, Linker::OutputFormat * format)
{
	special_prefix_char = special_char;
	output_format = format;
	option_segmentation = format->FormatSupportsSegmentation();
	option_16bit = format->FormatIs16bit();
	option_linear = format->FormatIsLinear();
//	option_stack_section = format->FormatSupportsStackSection();
	option_resources = format->FormatSupportsResources();
	option_libraries = format->FormatSupportsLibraries();
}

void ELFFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	this->module = &module;
	ReadFile(rd); /* TODO: refactor */
}

std::string ELFFormat::segment_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEG" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::segment_of_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGOF" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::segment_at_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGAT" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::with_respect_to_segment_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "WRTSEG" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::segment_difference_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGDIF" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::import_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "IMPORT" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::segment_of_import_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "IMPSEG" << special_prefix_char;
	return oss.str();
}

std::string ELFFormat::export_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "EXPORT" << special_prefix_char;
	return oss.str();
}

/* sections */
std::string ELFFormat::resource_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "RSRC" << special_prefix_char;
	return oss.str();
}

bool ELFFormat::parse_imported_name(std::string reference_name, Linker::SymbolName& symbol)
{
	try
	{
		/* <library name>$<ordinal> */
		/* <library name>$_<name> */
//		Linker::Debug << "Debug: Reference name " << reference_name << std::endl;
		size_t ix = reference_name.find(special_prefix_char);
		std::string library_name = reference_name.substr(0, ix);
		if(reference_name[ix + 1] == '_')
		{
			symbol = Linker::SymbolName(library_name, reference_name.substr(ix + 2));
		}
		else
		{
//			Linker::Debug << "Debug: Attempt parsing " << reference_name.substr(ix + 1) << std::endl;
			symbol = Linker::SymbolName(library_name, stoll(reference_name.substr(ix + 1), nullptr, 16));
		}
		return true;
	}
	catch(std::invalid_argument& a)
	{
		return false;
	}
}

bool ELFFormat::parse_exported_name(std::string reference_name, Linker::ExportedSymbol& symbol)
{
	try
	{
		/* <name> */
		/* <name>$<ordinal> */
		/* <ordinal>$_<name> */
		size_t ix = reference_name.find(special_prefix_char);
		if(ix == std::string::npos)
		{
			symbol = Linker::ExportedSymbol(reference_name);
		}
		else if(ix < reference_name.size() - 1 && reference_name[ix + 1] == '_')
		{
			symbol = Linker::ExportedSymbol(stoll(reference_name.substr(0, ix)), reference_name.substr(ix + 2));
		}
		else
		{
			symbol = Linker::ExportedSymbol(reference_name.substr(0, ix), stoll(reference_name.substr(ix + 1)));
		}
		return true;
	}
	catch(std::invalid_argument& a)
	{
		return false;
	}
}

void ELFFormat::ReadFile(Linker::Reader& in)
{
	int c;
	in.Seek(4);
	switch(c = in.ReadUnsigned(1))
	{
	case ELFCLASS32:
		wordbytes = 4;
		break;
	case ELFCLASS64:
		wordbytes = 8;
		break;
	default:
		Linker::Error << "Fatal Error: Illegal ELF class " << c << std::endl;
		exit(1);
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
		Linker::Error << "Fatal Error: Illegal ELF byte order " << c << std::endl;
		exit(1);
	}
	in.Seek(16 + 2);
	uint16_t tmp;
	switch(tmp = in.ReadUnsigned(2))
	{
	case EM_386:
		cpu = I386;
		module->cpu = option_16bit ? Linker::Module::I86 : Linker::Module::I386;
		break;
	case EM_68K:
		cpu = M68K;
		module->cpu = Linker::Module::M68K;
		break;
	case EM_ARM:
		cpu = ARM;
		module->cpu = Linker::Module::ARM;
		break;
	default:
		Linker::Error << "Fatal Error: Unknown CPU type in ELF file " << tmp << std::endl;
		exit(1);
	}
	in.Skip(4 + 2 * wordbytes);
	uint64_t shoff = in.ReadUnsigned(wordbytes);
	in.Skip(10);
	uint16_t shentsize = in.ReadUnsigned(2);
	uint16_t shnum = in.ReadUnsigned(2);
	uint16_t shstrndx = in.ReadUnsigned(2);
	for(size_t i = 0; i < shnum; i++)
	{
		in.Seek(shoff + i * shentsize);
		Section section;
		section.name_offset = in.ReadUnsigned(4);
		section.type = in.ReadUnsigned(4);
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
	for(size_t i = 0; i < shnum; i++)
	{
		in.Seek(sections[shstrndx].file_offset + sections[i].name_offset);
		sections[i].name = in.ReadASCIIZ();
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Section #" << i << ": `" << sections[i].name << "'" << ", type: " << sections[i].type << ", flags: " << sections[i].flags << std::endl;
#endif
		sections[i].section = nullptr;
		switch(sections[i].type)
		{
		case SHT_PROGBITS:
			if((sections[i].flags & 0x0002))
			{
				/* ALLOC */
				sections[i].section = std::make_shared<Linker::Section>(sections[i].name);
				sections[i].section->Expand(sections[i].size);
				in.Seek(sections[i].file_offset);
				sections[i].section->ReadFile(in);
			}
			break;
		case SHT_SYMTAB:
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
		//case SHT_STRTAB:
		//	break;
		case SHT_RELA:
		case SHT_REL:
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
				rel.addend_from_section_data = sections[i].type == SHT_REL;
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
		case SHT_NOBITS:
			sections[i].section = std::make_shared<Linker::Section>(sections[i].name);
			sections[i].section->SetZeroFilled(true);
			sections[i].section->Expand(sections[i].size);
			break;
		case SHT_GROUP:
			/* TODO */
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
			sections[i].section->SetFlag(output_format->FormatAdditionalSectionFlags(sections[i].name));
			if(option_resources && sections[i].name.rfind(resource_prefix() + "_",  0) == 0)
			{
				/* $$RSRC$_<type>$<id> */
				sections[i].section->SetFlag(Linker::Section::Resource);
				size_t sep = sections[i].name.rfind(special_prefix_char);
				std::string resource_type = sections[i].name.substr(resource_prefix().size() + 1, sep - resource_prefix().size() - 1);
				try
				{
					uint16_t resource_id = stoll(sections[i].name.substr(sep + 1), nullptr, 16);
					sections[i].section->resource_type = resource_type;
					sections[i].section->resource_id = resource_id;
					Linker::Debug << "Debug: Resource type " << resource_type << ", id " << resource_id << std::endl;
				}
				catch(std::invalid_argument& a)
				{
					Linker::Error << "Error: Unable to parse resource section name " << sections[i].name << ", proceeding" << std::endl;
				}
			}
			module->AddSection(sections[i].section);
		}
	}
	size_t i = 0;
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
				if(symbol.name != "" && symbol.defined)
				{
					switch(symbol.bind)
					{
					case STB_LOCAL:
						module->AddLocalSymbol(symbol.name, symbol.location);
						break;
					default:
						if(option_libraries && symbol.name.rfind(export_prefix(), 0) == 0)
						{
							/* $$EXPORT$<name> */
							/* $$EXPORT$<name>$<ordinal> */
							std::string reference_name = symbol.name.substr(export_prefix().size());
							Linker::ExportedSymbol name("");
							if(parse_exported_name(reference_name, name))
							{
								module->AddExportedSymbol(name, symbol.location);
								break;
							}
							else
							{
								Linker::Error << "Error: Unable to parse export name " << symbol.name << ", proceeding" << std::endl;
							}
						}
						module->AddGlobalSymbol(symbol.name, symbol.location);
						break;
					}
				}
				else if(symbol.unallocated)
				{
					module->AddCommonSymbol(symbol.name, symbol.specification);
				}
				else if(option_libraries && symbol.name.rfind(import_prefix(), 0) == 0)
				{
					/* $$IMPORT$<library name>$<ordinal> */
					/* $$IMPORT$<library name>$_<name> */
					std::string reference_name = symbol.name.substr(import_prefix().size());
					Linker::SymbolName name("");
					if(parse_imported_name(reference_name, name))
					{
						module->AddImportedSymbol(name);
					}
					else
					{
						Linker::Error << "Error: Unable to parse import name " << symbol.name << ", proceeding" << std::endl;
					}
				}
				else if(option_libraries && symbol.name.rfind(segment_of_import_prefix(), 0) == 0)
				{
					/* $$IMPSEG$<library name>$<ordinal> */
					/* $$IMPSEG$<library name>$_<name> */
					std::string reference_name = symbol.name.substr(segment_of_import_prefix().size());
					Linker::SymbolName name("");
					if(parse_imported_name(reference_name, name))
					{
						module->AddImportedSymbol(name);
					}
					else
					{
						Linker::Error << "Error: Unable to parse import name " << symbol.name << ", proceeding" << std::endl;
					}
				}
				i++;
			}
		}
	}
	for(Relocation rel : relocations)
	{
		Linker::Location rel_source = Linker::Location(sections[rel.sh_info].section, rel.offset);
		Symbol& sym_target = sections[rel.sh_link].symbols[rel.symbol];
		Linker::Target rel_target = sym_target.defined ? Linker::Target(sym_target.location) : Linker::Target(Linker::SymbolName(sym_target.name));
		Linker::Relocation obj_rel = Linker::Relocation::Empty();
		size_t rel_size;
		switch(cpu)
		{
		case I386:
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
				if(!sym_target.defined)
				{
					/* Note: a much more general syntax could be developed:
						segment :=
							SEGMENT(section name)
							SEGMENT OF(symbol)
						frame :=
							symbol
							segment
						reference :=
							frame
							SEGMENT AT(symbol)
							symbol WRT frame
							SEGMENT frame WRT frame
					However, this would probably result in an overwhelming syntax
					*/
					if(option_segmentation && sym_target.name.rfind(segment_prefix(), 0) == 0 && rel_size == 2)
					{
						/* $$SEG$<section name> */
						/* Note: can only refer to a currently present section */
						std::string section_name = sym_target.name.substr(segment_prefix().size());
						std::shared_ptr<Linker::Section> section = module->FindSection(section_name);
						if(section == nullptr)
						{
							Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
						}
						else
						{
							obj_rel = Linker::Relocation::Paragraph(rel_source, Linker::Target(Linker::Location(section)).GetSegment(), rel.addend);
						}
					}
					else if(option_segmentation && sym_target.name.rfind(segment_of_prefix(), 0) == 0 && rel_size == 2)
					{
						/* $$SEGOF$<symbol name> */
						std::string symbol_name = sym_target.name.substr(segment_of_prefix().size());
						obj_rel = Linker::Relocation::Paragraph(rel_source, Linker::Target(Linker::SymbolName(symbol_name)).GetSegment(), rel.addend);
					}
					else if(option_segmentation && sym_target.name.rfind(segment_at_prefix(), 0) == 0 && rel_size == 2)
					{
						/* $$SEGAT$<symbol name> */
						std::string symbol_name = sym_target.name.substr(segment_of_prefix().size());
						obj_rel = Linker::Relocation::Paragraph(rel_source, Linker::Target(Linker::SymbolName(symbol_name)), rel.addend);
					}
					else if(option_segmentation && sym_target.name.rfind(with_respect_to_segment_prefix(), 0) == 0)
					{
						/* $$WRTSEG$<symbol name>$<section name> */
						size_t sep = sym_target.name.rfind(special_prefix_char);
						std::string symbol_name = sym_target.name.substr(with_respect_to_segment_prefix().size(),
							sep - with_respect_to_segment_prefix().size());
						std::string section_name = sym_target.name.substr(sep + 1);
						//Linker::Debug << "Debug: " << symbol_name << " wrt " << section_name << std::endl;
						std::shared_ptr<Linker::Section> section = module->FindSection(section_name);
						if(section == nullptr)
						{
							Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
						}
						else
						{
							obj_rel = Linker::Relocation::OffsetFrom(rel_size, rel_source,
								Linker::Target(Linker::SymbolName(symbol_name)), Linker::Target(Linker::Location(section)).GetSegment(), rel.addend, ::LittleEndian);
						}
					}
					else if(option_segmentation && sym_target.name.rfind(segment_difference_prefix(), 0) == 0)
					{
						/* $$SEGDIF$<section name>$<section name> */
						size_t sep = sym_target.name.rfind(special_prefix_char);
						std::string section1_name = sym_target.name.substr(segment_difference_prefix().size(),
							sep - with_respect_to_segment_prefix().size());
						std::shared_ptr<Linker::Section> section1 = module->FindSection(section1_name);
						std::string section2_name = sym_target.name.substr(sep + 1);
						std::shared_ptr<Linker::Section> section2 = module->FindSection(section2_name);
						if(section1 == nullptr)
						{
							Linker::Error << "Error: invalid section in extended relocation `" << section1_name << "'" << std::endl;
						}
						else if(section2 == nullptr)
						{
							Linker::Error << "Error: invalid section in extended relocation `" << section2_name << "'" << std::endl;
						}
						else
						{
							obj_rel = Linker::Relocation::ParagraphDifference(rel_source,
								Linker::Target(Linker::Location(section1)).GetSegment(), Linker::Target(Linker::Location(section2)).GetSegment(), rel.addend);
						}
					}
					else if(option_libraries && sym_target.name.rfind(import_prefix(), 0) == 0)
					{
						/* $$IMPORT$<library name>$<ordinal> */
						/* $$IMPORT$<library name>$_<name> */
						std::string reference_name = sym_target.name.substr(import_prefix().size());
#if 0
						try
						{
//									Linker::Debug << "Debug: Reference name " << reference_name << std::endl;
							size_t ix = reference_name.find(special_prefix_char);
							std::string library_name = reference_name.substr(0, ix);
							Linker::SymbolName symbol("");
							if(reference_name[ix + 1] == '_')
							{
								symbol = Linker::SymbolName(library_name, reference_name.substr(ix + 2));
							}
							else
							{
//										Linker::Debug << "Debug: Attempt parsing " << reference_name.substr(ix + 1) << std::endl;
								symbol = Linker::SymbolName(library_name, stoll(reference_name.substr(ix + 1), nullptr, 16));
							}
							obj_rel = Linker::Relocation::Offset(rel_size, rel_source, Linker::Target(symbol), rel.addend);
						}
						catch(std::invalid_argument a)
#endif
						Linker::SymbolName symbol("");
						if(parse_imported_name(reference_name, symbol))
						{
							obj_rel = option_linear
								? Linker::Relocation::Absolute(rel_size, rel_source, Linker::Target(symbol), rel.addend)
								: Linker::Relocation::Offset(rel_size, rel_source, Linker::Target(symbol), rel.addend);
						}
						else
						{
							Linker::Error << "Error: Unable to parse import name " << sym_target.name << ", proceeding" << std::endl;
						}
					}
					else if(option_libraries && sym_target.name.rfind(segment_of_import_prefix(), 0) == 0)
					{
						/* $$IMPSEG$<library name>$<ordinal> */
						/* $$IMPSEG$<library name>$_<name> */
						std::string reference_name = sym_target.name.substr(segment_of_import_prefix().size());
#if 0
						try
						{
							size_t ix = reference_name.find(special_prefix_char);
							std::string library_name = reference_name.substr(0, ix);
							Linker::SymbolName symbol("");
							if(reference_name[ix + 1] == '_')
							{
								symbol = Linker::SymbolName(library_name, reference_name.substr(ix + 2));
							}
							else
							{
								symbol = Linker::SymbolName(library_name, stoll(reference_name.substr(ix + 1), nullptr, 16));
							}
							obj_rel = Linker::Relocation::Paragraph(rel_source, Linker::Target(symbol), rel.addend);
						}
						catch(std::invalid_argument a)
#endif
						Linker::SymbolName symbol("");
						if(parse_imported_name(reference_name, symbol))
						{
							obj_rel = Linker::Relocation::Paragraph(rel_source, Linker::Target(symbol), rel.addend);
						}
						else
						{
							Linker::Error << "Error: Unable to parse import name " << sym_target.name << ", proceeding" << std::endl;
						}
					}
				}
				break;
			case R_386_PC8:
			case R_386_PC16:
			case R_386_PC32:
				obj_rel = Linker::Relocation::Relative(rel_size, rel_source, rel_target, rel.addend, ::LittleEndian);
				if(!sym_target.defined)
				{
					if(option_libraries && sym_target.name.rfind(import_prefix(), 0) == 0)
					{
						/* $$IMPORT$<library name>$<ordinal> */
						/* $$IMPORT$<library name>$_<name> */
						std::string reference_name = sym_target.name.substr(import_prefix().size());
						Linker::SymbolName symbol("");
						if(parse_imported_name(reference_name, symbol))
						{
							obj_rel = Linker::Relocation::Relative(rel_size, rel_source, Linker::Target(symbol), rel.addend);
						}
						else
						{
							Linker::Error << "Error: Unable to parse import name " << sym_target.name << ", proceeding" << std::endl;
						}
					}
				}
				break;
			}
			break;

		case M68K:
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

		case ARM:
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
		module->relocations.push_back(obj_rel);
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Relocation at #" << rel.sh_info << ":" << std::hex << rel.offset << std::dec << " to symbol " << rel.symbol << ", type " << rel.type << std::endl;
#endif
	}
}

