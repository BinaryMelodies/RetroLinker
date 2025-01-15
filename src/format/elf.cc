
#include "elf.h"

using namespace ELF;

void ELFFormat::SectionContents::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
}

void ELFFormat::SectionContents::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
}

offset_t ELFFormat::SymbolTable::ActualDataSize()
{
	return symbols.size() * entsize;
}

offset_t ELFFormat::SymbolTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::SymbolTable::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	for(auto& symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		symbol_entry.AddField("Name", Dumper::StringDisplay::Make(), symbol.name);
		symbol_entry.AddField("Name offset", Dumper::HexDisplay::Make(), offset_t(symbol.name_offset));
		if(symbol.shndx == SHN_COMMON)
			symbol_entry.AddField("Alignment", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(symbol.value));
		else
			symbol_entry.AddField("Value", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(symbol.value));
		symbol_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(symbol.size));
		symbol_entry.AddField("Section number", Dumper::DecDisplay::Make(), offset_t(symbol.shndx));
		symbol_entry.AddField("Section name", Dumper::StringDisplay::Make(),
			symbol.shndx == SHN_UNDEF ? "SHN_UNDEF" :
			symbol.shndx == SHN_ABS ? "SHN_ABS" :
			symbol.shndx == SHN_COMMON ? "SHN_COMMON" :
			symbol.shndx == SHN_XINDEX ? "SHN_XINDEX" :
			fmt.sections[symbol.shndx].name);

		std::map<offset_t, std::string> binding_descriptions;
		binding_descriptions[STB_LOCAL] = "STB_LOCAL";
		binding_descriptions[STB_GLOBAL] = "STB_GLOBAL";
		binding_descriptions[STB_WEAK] = "STB_WEAK";
		binding_descriptions[STB_ENTRY] = "STB_ENTRY - (IBM OS/2)";
		symbol_entry.AddField("Binding", Dumper::ChoiceDisplay::Make(binding_descriptions), offset_t(symbol.bind));

		std::map<offset_t, std::string> type_descriptions;
		type_descriptions[STT_NOTYPE] = "STT_NOTYPE";
		type_descriptions[STT_OBJECT] = "STT_OBJECT";
		type_descriptions[STT_FUNC] = "STT_FUNC";
		type_descriptions[STT_SECTION] = "STT_SECTION";
		type_descriptions[STT_FILE] = "STT_FILE";
		type_descriptions[STT_COMMON] = "STT_COMMON";
		type_descriptions[STT_TLS] = "STT_TLS";
		type_descriptions[STT_IMPORT] = "STT_IMPORT - (IBM OS/2)"; // TODO: st_value is offset of import table entry
		symbol_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(symbol.type));

		std::map<offset_t, std::string> visibility_descriptions;
		visibility_descriptions[STV_DEFAULT] = "STV_DEFAULT";
		visibility_descriptions[STV_INTERNAL] = "STV_INTERNAL";
		visibility_descriptions[STV_HIDDEN] = "STV_HIDDEN";
		visibility_descriptions[STV_PROTECTED] = "STV_PROTECTED";
		symbol_entry.AddField("Visibility", Dumper::ChoiceDisplay::Make(visibility_descriptions), offset_t(symbol.other));

		symbol_entry.Display(dump);

		i += 1;
	}
}

offset_t ELFFormat::StringTable::ActualDataSize()
{
	return size;
}

offset_t ELFFormat::StringTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::StringTable::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	offset_t offset = fmt.sections[index].file_offset;
	for(auto& s : strings)
	{
		Dumper::Entry string_entry("String", i + 1, offset, 2 * fmt.wordbytes);
		string_entry.AddField("Value name", Dumper::StringDisplay::Make(), s);
		string_entry.Display(dump);
		i += 1;
		offset += s.size() + 1;
	}
}

offset_t ELFFormat::Array::ActualDataSize()
{
	return array.size() * entsize;
}

offset_t ELFFormat::Array::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::Array::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	for(auto& value : array)
	{
		Dumper::Entry array_entry("Value", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		array_entry.AddField("Value", Dumper::HexDisplay::Make(2 * entsize), value);
		array_entry.Display(dump);
		i += 1;
	}
}

void ELFFormat::SectionGroup::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	// TODO: untested
	unsigned i = 0;
	for(auto& value : array)
	{
		Dumper::Entry array_entry("Value", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		array_entry.AddField("Section", Dumper::HexDisplay::Make(2 * entsize), value);
		array_entry.AddField("Section name", Dumper::StringDisplay::Make(),
			value == SHN_UNDEF ? "SHN_UNDEF" :
			value == SHN_ABS ? "SHN_ABS" :
			value == SHN_COMMON ? "SHN_COMMON" :
			value == SHN_XINDEX ? "SHN_XINDEX" :
			fmt.sections[value].name);
		array_entry.Display(dump);
		i += 1;
	}
}

void ELFFormat::IndexArray::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	// TODO: untested
	unsigned i = 0;
	for(auto& value : array)
	{
		if(value == 0)
		{
			i += 1;
			continue;
		}
		Dumper::Entry array_entry("Value", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		array_entry.AddField("Section", Dumper::HexDisplay::Make(2 * entsize), value);
		array_entry.AddField("Section name", Dumper::StringDisplay::Make(),
			value == SHN_UNDEF ? "SHN_UNDEF" :
			value == SHN_ABS ? "SHN_ABS" :
			value == SHN_COMMON ? "SHN_COMMON" :
			value == SHN_XINDEX ? "SHN_XINDEX" :
			fmt.sections[value].name);
		array_entry.Display(dump);
		i += 1;
	}
}

offset_t ELFFormat::SectionGroup::ActualDataSize()
{
	return (1 + array.size()) * entsize;
}

offset_t ELFFormat::SectionGroup::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::SectionGroup::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	region->AddField("Group flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("GRP_COMDAT"), true),
		flags);
}

offset_t ELFFormat::Relocations::ActualDataSize()
{
	return relocations.size() * entsize;
}

offset_t ELFFormat::Relocations::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::Relocations::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	for(auto& rel : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		relocation_entry.AddField("Offset", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.offset));
		relocation_entry.AddField("Type", Dumper::DecDisplay::Make(), offset_t(rel.type)); // TODO: display name
		relocation_entry.AddField("Symbol index", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.symbol)); // TODO: display name
		if(!rel.addend_from_section_data)
			relocation_entry.AddField("Addend", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.addend));
		// TODO: display current value
		relocation_entry.Display(dump);
		i += 1;
	}
}

offset_t ELFFormat::DynamicSection::ActualDataSize()
{
	return dynamic.size() * entsize;
}

offset_t ELFFormat::DynamicSection::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::DynamicSection::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	for(auto& dynobj : dynamic)
	{
		Dumper::Entry dynamic_entry("Object", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		std::map<offset_t, std::string> tag_descriptions;
		tag_descriptions[DT_NULL] = "DT_NULL";
		tag_descriptions[DT_NEEDED] = "DT_NEEDED";
		tag_descriptions[DT_PLTRELSZ] = "DT_PLTRELSZ";
		tag_descriptions[DT_PLTGOT] = "DT_PLTGOT";
		tag_descriptions[DT_HASH] = "DT_HASH";
		tag_descriptions[DT_STRTAB] = "DT_STRTAB";
		tag_descriptions[DT_SYMTAB] = "DT_SYMTAB";
		tag_descriptions[DT_RELA] = "DT_RELA";
		tag_descriptions[DT_RELASZ] = "DT_RELASZ";
		tag_descriptions[DT_RELAENT] = "DT_RELAENT";
		tag_descriptions[DT_STRSZ] = "DT_STRSZ";
		tag_descriptions[DT_SYMENT] = "DT_SYMENT";
		tag_descriptions[DT_INIT] = "DT_INIT";
		tag_descriptions[DT_FINI] = "DT_FINI";
		tag_descriptions[DT_SONAME] = "DT_SONAME";
		tag_descriptions[DT_RPATH] = "DT_RPATH";
		tag_descriptions[DT_SYMBOLIC] = "DT_SYMBOLIC";
		tag_descriptions[DT_REL] = "DT_REL";
		tag_descriptions[DT_RELSZ] = "DT_RELSZ";
		tag_descriptions[DT_RELENT] = "DT_RELENT";
		tag_descriptions[DT_PLTREL] = "DT_PLTREL";
		tag_descriptions[DT_DEBUG] = "DT_DEBUG";
		tag_descriptions[DT_TEXTREL] = "DT_TEXTREL";
		tag_descriptions[DT_JMPREL] = "DT_JMPREL";
		tag_descriptions[DT_BIND_NOW] = "DT_BIND_NOW";
		tag_descriptions[DT_INIT_ARRAY] = "DT_INIT_ARRAY";
		tag_descriptions[DT_FINI_ARRAY] = "DT_FINI_ARRAY";
		tag_descriptions[DT_INIT_ARRAYSZ] = "DT_INIT_ARRAYSZ";
		tag_descriptions[DT_FINI_ARRAYSZ] = "DT_FINI_ARRAYSZ";
		tag_descriptions[DT_RUNPATH] = "DT_RUNPATH";
		tag_descriptions[DT_FLAGS] = "DT_FLAGS";
		tag_descriptions[DT_ENCODING] = "DT_ENCODING";
		tag_descriptions[DT_PREINIT_ARRAY] = "DT_PREINIT_ARRAY";
		tag_descriptions[DT_PREINIT_ARRAYSZ] = "DT_PREINIT_ARRAYSZ";
		tag_descriptions[DT_SYMTAB_SHNDX] = "DT_SYMTAB_SHNDX";
		tag_descriptions[DT_EXPORT] = "DT_EXPORT (IBM OS/2)";
		tag_descriptions[DT_EXPORTSZ] = "DT_EXPORTSZ (IBM OS/2)";
		tag_descriptions[DT_EXPENT] = "DT_EXPENT (IBM OS/2)";
		tag_descriptions[DT_IMPORT] = "DT_IMPORT (IBM OS/2)";
		tag_descriptions[DT_IMPORTSZ] = "DT_IMPORTSZ (IBM OS/2)";
		tag_descriptions[DT_IMPENT] = "DT_IMPENT (IBM OS/2)";
		tag_descriptions[DT_IT] = "DT_IT (IBM OS/2)";
		tag_descriptions[DT_ITPRTY] = "DT_ITPRTY (IBM OS/2)";
		tag_descriptions[DT_INITTERM] = "DT_INITTERM (IBM OS/2)";
		tag_descriptions[DT_STACKSZ] = "DT_STACKSZ (IBM OS/2)";
		dynamic_entry.AddField("Tag", Dumper::ChoiceDisplay::Make(tag_descriptions), dynobj.tag);
		//dynamic_entry.AddField("Tag", Dumper::HexDisplay::Make(2 * fmt.wordbytes), dynobj.tag);
		dynamic_entry.AddField("Value", Dumper::HexDisplay::Make(2 * fmt.wordbytes), dynobj.value);
		dynamic_entry.Display(dump);
		i += 1;
	}
}

offset_t ELFFormat::NotesSection::ActualDataSize()
{
	return size;
}

offset_t ELFFormat::NotesSection::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::NotesSection::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	unsigned i = 0;
	offset_t offset = 0;
	for(auto& note : notes)
	{
		// TODO: probably wrong
		Dumper::Entry note_entry("Note", i + 1, fmt.sections[index].file_offset + offset, 2 * fmt.wordbytes);
		note_entry.AddField("Name", Dumper::StringDisplay::Make(), note.name);
		note_entry.AddField("Description", Dumper::StringDisplay::Make(), note.descriptor);
		note_entry.AddField("Type", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(note.type));
		note_entry.Display(dump);
		i += 1;
		offset += 3 * 4 + note.name.size() + 1;
		offset = ::AlignTo(offset, 4);
		offset += note.descriptor.size() + 1;
		offset = ::AlignTo(offset, 4);
	}
}

bool ELFFormat::SystemInfo::IsOS2Specific() const
{
	return os_type == EOS_OS2 && os_size >= 16;
}

offset_t ELFFormat::SystemInfo::ActualDataSize()
{
	return 8 + os_size;
}

offset_t ELFFormat::SystemInfo::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset)
{
	// TODO
	return 0;
}

void ELFFormat::SystemInfo::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	std::map<offset_t, std::string> operating_system_descriptions;
	operating_system_descriptions[SystemInfo::EOS_NONE] = "EOS_NONE - Unknown";
	operating_system_descriptions[SystemInfo::EOS_PN] = "EOS_PN - IBM Microkernel personality neutral";
	operating_system_descriptions[SystemInfo::EOS_SVR4] = "EOS_SVR4 - UNIX System V Release 4 operating system environment";
	operating_system_descriptions[SystemInfo::EOS_AIX] = "EOS_AIX - IBM AIX operating system environment";
	operating_system_descriptions[SystemInfo::EOS_OS2] = "EOS_OS2 - IBM OS/2 operating system, 32 bit environment";
	region->AddField("OS type", Dumper::ChoiceDisplay::Make(operating_system_descriptions), offset_t(os_type));

	region->AddField("System specific information", Dumper::HexDisplay::Make(8), offset_t(os_size));
	if(IsOS2Specific())
	{
		std::map<offset_t, std::string> session_type_descriptions;
		session_type_descriptions[SystemInfo::os2_specific::OS2_SES_NONE] = "OS2_SES_NONE - None";
		session_type_descriptions[SystemInfo::os2_specific::OS2_SES_FS] = "OS2_SES_FS - Full Screen session";
		session_type_descriptions[SystemInfo::os2_specific::OS2_SES_PM] = "OS2_SES_PM - Presentation Manager session";
		session_type_descriptions[SystemInfo::os2_specific::OS2_SES_VIO] = "OS2_SES_VIO - Windowed (character-mode) session";
		region->AddField("Session type", Dumper::ChoiceDisplay::Make(session_type_descriptions), offset_t(os2.sessiontype));

		region->AddField("Session flags", Dumper::HexDisplay::Make(8), offset_t(os2.sessionflags));
	}
	else
	{
		// TODO
	}
}

std::shared_ptr<Linker::Section> ELFFormat::Section::GetSection()
{
	return std::dynamic_pointer_cast<Linker::Section>(contents);
}

const std::shared_ptr<Linker::Section> ELFFormat::Section::GetSection() const
{
	return std::dynamic_pointer_cast<Linker::Section>(contents);
}

std::shared_ptr<ELFFormat::SymbolTable> ELFFormat::Section::GetSymbolTable()
{
	return std::dynamic_pointer_cast<SymbolTable>(contents);
}

const std::shared_ptr<ELFFormat::SymbolTable> ELFFormat::Section::GetSymbolTable() const
{
	return std::dynamic_pointer_cast<SymbolTable>(contents);
}

std::shared_ptr<ELFFormat::StringTable> ELFFormat::Section::GetStringTable()
{
	return std::dynamic_pointer_cast<StringTable>(contents);
}

std::shared_ptr<ELFFormat::Array> ELFFormat::Section::GetArray()
{
	return std::dynamic_pointer_cast<Array>(contents);
}

std::shared_ptr<ELFFormat::Relocations> ELFFormat::Section::GetRelocations()
{
	return std::dynamic_pointer_cast<ELFFormat::Relocations>(contents);
}

const std::shared_ptr<ELFFormat::Relocations> ELFFormat::Section::GetRelocations() const
{
	return std::dynamic_pointer_cast<ELFFormat::Relocations>(contents);
}

std::shared_ptr<ELFFormat::DynamicSection> ELFFormat::Section::GetDynamicSection()
{
	return std::dynamic_pointer_cast<ELFFormat::DynamicSection>(contents);
}

std::shared_ptr<ELFFormat::NotesSection> ELFFormat::Section::GetNotesSection()
{
	return std::dynamic_pointer_cast<ELFFormat::NotesSection>(contents);
}

std::shared_ptr<ELFFormat::SystemInfo> ELFFormat::Section::GetSystemInfo()
{
	return std::dynamic_pointer_cast<ELFFormat::SystemInfo>(contents);
}

ELFFormat::Section::stored_format ELFFormat::Section::GetStoredFormatKind() const
{
	switch(type)
	{
	default:
	case Section::SHT_PROGBITS:
	case Section::SHT_NOBITS:
		return SectionLike;
	case Section::SHT_SYMTAB:
	case Section::SHT_DYNSYM:
		return SymbolTableLike;
	case Section::SHT_STRTAB:
		return StringTableLike;
	case Section::SHT_RELA:
	case Section::SHT_REL:
		return RelocationLike;
	case Section::SHT_DYNAMIC:
		return DynamicLike;
	case Section::SHT_NOTE:
		return NoteLike;
	case Section::SHT_INIT_ARRAY:
	case Section::SHT_FINI_ARRAY:
	case Section::SHT_PREINIT_ARRAY:
	case Section::SHT_HASH: // TODO: improve
		return ArrayLike;
	case Section::SHT_GROUP:
	case Section::SHT_SYMTAB_SHNDX:
		return SectionArrayLike;
	case Section::SHT_OS:
		return IBMSystemInfo;
	}
}

bool ELFFormat::Section::GetFileSize() const
{
	return type == SHT_NOBITS ? 0 : size;
}

offset_t ELFFormat::Segment::Part::GetOffset(ELFFormat& fmt)
{
	switch(type)
	{
	case Section:
		return fmt.sections[index].file_offset + offset;
	case Block:
		return fmt.blocks[index].offset + offset;
	default:
		assert(false);
	}
}

offset_t ELFFormat::Segment::Part::GetActualSize(ELFFormat& fmt)
{
	switch(type)
	{
	case Section:
		return fmt.sections[index].GetFileSize();
	case Block:
		return fmt.blocks[index].size;
	default:
		assert(false);
	}
}

void ELFFormat::Section::Dump(Dumper::Dumper& dump, ELFFormat& fmt, unsigned index)
{
	std::unique_ptr<Dumper::Region> region;
	if(auto section = std::dynamic_pointer_cast<Linker::Buffer>(contents))
	{
		region = std::make_unique<Dumper::Block>("Section", file_offset, section, address, 2 * fmt.wordbytes);
		// TODO: provide relocations for the section data
	}
	else
	{
		region = std::make_unique<Dumper::Region>("Section", file_offset, contents != nullptr ? contents->ActualDataSize() : 0, 2 * fmt.wordbytes);
	}

	region->InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index));
	region->InsertField(1, "Name", Dumper::StringDisplay::Make(), name);
	region->InsertField(2, "Name offset", Dumper::HexDisplay::Make(), offset_t(name_offset));

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
	type_descriptions[SHT_FINI_ARRAY] = "SHT_FINI_ARRAY - Array of termination functions";
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
	region->AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(type));

	region->AddField("Flags",
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
	region->AddField("Size", Dumper::HexDisplay::Make(), offset_t(size));

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
	region->AddOptionalField(name_link, Dumper::DecDisplay::Make(), offset_t(link));

	std::string name_info;
	switch(type)
	{
	case SHT_GROUP:
		name_info = "Info, symbol index";
		break;
	case SHT_SYMTAB:
	case SHT_DYNSYM:
		name_info = "Info, symbol index after last local symbol";
		break;
	case SHT_REL:
	case SHT_RELA:
		name_info = "Info, section index";
		break;
	default:
		if((flags & SHF_INFO_LINK))
			name_info = "Info, section index";
		else
			name_info = "Info";
		break;
	}
	region->AddOptionalField(name_info, Dumper::DecDisplay::Make(), offset_t(info));

	region->AddField("Alignment", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(align));

	std::string name_entsize;
	if((flags & SHF_STRINGS))
		name_entsize = "Character size";
	else
		name_entsize = "Entry size";
	region->AddOptionalField(name_entsize, Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(entsize));

	if(auto section_contents = std::dynamic_pointer_cast<SectionContents>(contents))
	{
		section_contents->AddDumperFields(region, dump, fmt, index);
	}

	region->Display(dump);

	if(auto section_contents = std::dynamic_pointer_cast<SectionContents>(contents))
	{
		section_contents->Dump(dump, fmt, index);
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
		if(shnum == SHN_XINDEX && i == 0)
			shnum = section.info;
	}

	if(section_name_string_table == SHN_XINDEX)
	{
		section_name_string_table = sections[0].link;
	}

	for(size_t i = 0; i < phnum; i++)
	{
		rd.Seek(program_header_offset + i * program_header_entry_size);
		Segment segment;
		segment.type = Segment::segment_type(rd.ReadUnsigned(4));
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
		switch(sections[i].GetStoredFormatKind())
		{
		case Section::Empty:
		case Section::SectionLike:
			sections[i].contents = std::make_shared<Linker::Section>(sections[i].name);
			if(sections[i].type == Section::SHT_NOBITS)
			{
				sections[i].GetSection()->SetZeroFilled(true);
			}
			sections[i].GetSection()->Expand(sections[i].size);
			if(sections[i].type != Section::SHT_NOBITS)
			{
				rd.Seek(sections[i].file_offset);
				sections[i].GetSection()->ReadFile(rd);
			}
			break;
		case Section::SymbolTableLike:
			sections[i].contents = std::make_shared<SymbolTable>(sections[i].entsize);
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
				sections[i].GetSymbolTable()->symbols.push_back(symbol);
			}
			break;
		case Section::StringTableLike:
			sections[i].contents = std::make_shared<StringTable>(sections[i].size);
			rd.Seek(sections[i].file_offset);
			while(rd.Tell() < sections[i].file_offset + sections[i].size)
			{
				std::string s = rd.ReadASCIIZ();
				sections[i].GetStringTable()->strings.push_back(s);
			}
			break;
		case Section::RelocationLike:
			sections[i].contents = std::make_shared<Relocations>(sections[i].entsize);
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
//				Debug::Debug << "Debug: Type " << sections[i].type << " addend " << rel.addend << std::endl;
				rel.sh_link = sections[i].link;
				rel.sh_info = sections[i].info;
				sections[i].GetRelocations()->relocations.push_back(rel);
			}
			break;
		case Section::ArrayLike:
		case Section::SectionArrayLike: // TODO: untested
			rd.Seek(sections[i].file_offset);
			if(sections[i].type == Section::SHT_GROUP)
			{
				sections[i].contents = std::make_shared<SectionGroup>(sections[i].entsize);
				std::dynamic_pointer_cast<SectionGroup>(sections[i].contents)->flags = rd.ReadUnsigned(sections[i].entsize);
			}
			else if(sections[i].type == Section::SHT_SYMTAB_SHNDX)
			{
				sections[i].contents = std::make_shared<IndexArray>(sections[i].entsize);
			}
			else
			{
				sections[i].contents = std::make_shared<Array>(sections[i].entsize);
			}
			for(size_t j = 0; j < sections[i].size / 4; j++)
			{
				sections[i].GetArray()->array.push_back(rd.ReadUnsigned(sections[i].entsize));
			}
			break;
		case Section::DynamicLike:
			sections[i].contents = std::make_shared<DynamicSection>(sections[i].entsize);
			for(size_t j = 0; j < sections[i].size; j += sections[i].entsize)
			{
				rd.Seek(sections[i].file_offset + j);
				DynamicObject dyn;
				dyn.tag = rd.ReadSigned(wordbytes);
				dyn.value = rd.ReadSigned(wordbytes);
				sections[i].GetDynamicSection()->dynamic.push_back(dyn);
			}
			break;
		case Section::NoteLike:
			sections[i].contents = std::make_shared<NotesSection>(sections[i].size);
			rd.Seek(sections[i].file_offset);
			while(rd.Tell() < sections[i].file_offset + sections[i].size)
			{
				Note note;
				offset_t namesz = rd.ReadUnsigned(4);
				offset_t descsz = rd.ReadUnsigned(4);
				note.type = rd.ReadUnsigned(4);
				note.name = rd.ReadASCIIZ(namesz);
				if((namesz & 3) != 0)
					rd.Skip((-namesz & 3));
				note.descriptor = rd.ReadASCIIZ(descsz);
				if((descsz & 3) != 0)
					rd.Skip((-descsz & 3));
				sections[i].GetNotesSection()->notes.push_back(note);
			}
			break;
		case Section::IBMSystemInfo:
			sections[i].contents = std::make_shared<SystemInfo>();
			rd.Seek(sections[i].file_offset);
			sections[i].GetSystemInfo()->os_type = SystemInfo::system_type(rd.ReadUnsigned(4));
			sections[i].GetSystemInfo()->os_size = rd.ReadUnsigned(4);
			if(sections[i].GetSystemInfo()->IsOS2Specific())
			{
				sections[i].GetSystemInfo()->os2.sessiontype = SystemInfo::os2_specific::os2_session(rd.ReadUnsigned(1));
				sections[i].GetSystemInfo()->os2.sessionflags = rd.ReadUnsigned(1);
			}
			else
			{
				sections[i].GetSystemInfo()->os_specific.resize(sections[i].GetSystemInfo()->os_size, '\0');
				rd.ReadData(sections[i].GetSystemInfo()->os_specific);
			}
			break;
		}
		if(sections[i].GetSection() != nullptr)
		{
			sections[i].GetSection()->SetReadable(true);
			if((sections[i].flags & SHF_WRITE))
			{
				sections[i].GetSection()->SetWritable(true);
			}
			if((sections[i].flags & SHF_EXECINSTR))
			{
				sections[i].GetSection()->SetExecable(true);
			}
			if((sections[i].flags & SHF_MERGE))
			{
				sections[i].GetSection()->SetMergeable(true);
			}
//			if((sections[i].flags & SHF_GROUP))
//			{
//				Linker::Debug << "Debug: Groups currently not supported" << std::endl;
//				/* TODO - when?? */
//			}
#if 0
			if(option_stack_section && sections[i].name == ".stack")
			{
				sections[i].GetSection()->SetFlag(Linker::Section::Stack);
			}
			if(option_heap_section && sections[i].name == ".heap")
			{
				sections[i].GetSection()->SetFlag(Linker::Section::Heap);
			}
#endif
		}
	}

#if DISPLAY_LOGS
	size_t i = 0;
#endif
	for(Section& section : sections)
	{
		switch(section.type)
		{
		case Section::SHT_SYMTAB_SHNDX:
			// TODO: untested
			for(size_t index = 0; index < section.GetArray()->array.size(); index++)
			{
				Symbol& symbol = sections[section.link].GetSymbolTable()->symbols[index];
				if(symbol.shndx == SHN_XINDEX)
				{
#if DISPLAY_LOGS
					Linker::Debug << "Debug: Symbol #" << i << ": SHN_XINDEX" << std::endl;
#endif
					symbol.shndx = section.GetArray()->array[index];
					symbol.location = Linker::Location(sections[symbol.shndx].GetSection(), symbol.value);
					symbol.defined = true;
				}
			}
			break;
		case Section::SHT_SYMTAB:
		case Section::SHT_DYNSYM:
			for(Symbol& symbol : section.GetSymbolTable()->symbols)
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
#if 0
				case SHN_XINDEX:
					Linker::Warning << "Warning: Extended section numbers not supported" << std::endl;
					break;
#endif
				default:
					symbol.location = Linker::Location(sections[symbol.shndx].GetSection(), symbol.value);
					symbol.defined = true;
				}
#if DISPLAY_LOGS
				i++;
#endif
			}
			break;
		default:
			break;
		}
	}

	/* in all_parts, offset refers to the offset within the file instead within the section/block */
	std::vector<Segment::Part> all_parts;

	/* header */
	blocks.push_back(Block(0, elf_header_size));
	all_parts.push_back(Segment::Part(Segment::Part::Block, 0, 0, elf_header_size));
	/* section header */
	blocks.push_back(Block(section_header_offset, shnum * section_header_entry_size));
	if(shnum != 0)
		all_parts.push_back(Segment::Part(Segment::Part::Block, 1, section_header_offset, shnum * section_header_entry_size));
	/* program header */
	blocks.push_back(Block(program_header_offset, phnum * program_header_entry_size));
	if(phnum != 0)
		all_parts.push_back(Segment::Part(Segment::Part::Block, 2, program_header_offset, phnum * program_header_entry_size));

	unsigned i = 0;
	for(auto& section : sections)
	{
		if(section.type != Section::SHT_NULL)
			all_parts.push_back(Segment::Part(Segment::Part::Section, i, section.file_offset,
				section.GetFileSize()));
		i++;
	}

	for(size_t i = 0; i < phnum; i++)
	{
		offset_t offset = segments[i].offset;
		offset_t end = segments[i].offset + segments[i].filesz;
		offset_t covered = offset;
		while(offset < end)
		{
			offset_t next_offset = end;
			for(auto& part : all_parts)
			{
				if(offset < part.offset && part.offset < next_offset)
				{
					next_offset = part.offset;
				}

				if(part.offset == offset)
				{
					offset_t size = std::min(end - offset, part.size);
					segments[i].parts.push_back(Segment::Part(part.type, part.index, 0, size));
					if(covered < offset + size)
						covered = offset + size;
				}
				else if(offset == segments[i].offset && part.offset < offset && offset < part.offset + part.size)
				{
					offset_t cutoff = offset - part.offset;
					offset_t size = std::min(end - offset, part.size - cutoff);
					segments[i].parts.push_back(Segment::Part(part.type, part.index, cutoff, size));
					if(covered < offset + size)
						covered = offset + size;
				}
			}
			if(covered < next_offset)
			{
				Block block;
				block.offset = covered;
				rd.Seek(covered);
				block.size = next_offset - covered;
				block.image = Linker::Buffer::ReadFromFile(rd, block.size);
				blocks.push_back(block);
				segments[i].parts.push_back(Segment::Part(Segment::Part::Block, blocks.size() - 1, 0, block.size));
				all_parts.push_back(Segment::Part(Segment::Part::Block, blocks.size() - 1, covered, block.size));
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
	wr.WriteWord(2, section_name_string_table >= SHN_LORESERVE ? SHN_XINDEX : 0); // shstrndx

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

	if(hobbit_beos_resource_offset != 0)
	{
		/* BeOS Hobbit section */

		// TODO: untested
		wr.Seek(hobbit_beos_resource_offset);
		wr.WriteWord(4, hobbit_beos_resources.size());
		for(auto& resource : hobbit_beos_resources)
		{
			wr.WriteData(4, resource.type);
			wr.WriteWord(4, resource.unknown1);
			wr.WriteWord(4, resource.offset);
			wr.WriteWord(4, resource.size);
			wr.WriteWord(4, resource.unknown2);
		}
		for(auto& resource : hobbit_beos_resources)
		{
			wr.Seek(hobbit_beos_resource_offset + 4 + 20 * hobbit_beos_resources.size() + resource.offset);
			resource.image->WriteFile(wr);
		}

		wr.SeekEnd();
		offset_t end = wr.Tell();
		wr.Seek(end - 8);
		wr.WriteData(4, "RSRC");
		wr.WriteWord(4, hobbit_beos_resource_offset);
	}
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

	unsigned i = 0;
	for(auto& section : sections)
	{
		section.Dump(dump, *this, i++);
	}

	i = 0;
	for(auto& segment : segments)
	{
		Dumper::Region segment_region("Segment", segment.offset, segment.filesz, 2 * wordbytes);
		segment_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i));

		std::map<offset_t, std::string> type_descriptions;
		type_descriptions[Segment::PT_NULL] = "PT_NULL - ignored";
		type_descriptions[Segment::PT_LOAD] = "PT_LOAD - loadable segment";
		type_descriptions[Segment::PT_DYNAMIC] = "PT_DYNAMIC - dynamic linking information";
		type_descriptions[Segment::PT_INTERP] = "PT_INTERP - interpreter name";
		type_descriptions[Segment::PT_NOTE] = "PT_NOTE - auxiliary information";
		type_descriptions[Segment::PT_SHLIB] = "PT_SHLIB - reserved";
		type_descriptions[Segment::PT_PHDR] = "PT_PHDR - program header table";
		type_descriptions[Segment::PT_TLS] = "PT_TLS - thread-local storage";
		type_descriptions[Segment::PT_OS] = "PT_OS - (IBM OS/2) target operating system identification";
		type_descriptions[Segment::PT_RES] = "PT_RES - (IBM OS/2) read-only resource data";
		type_descriptions[Segment::PT_SUNW_EH_FRAME] = "PT_SUNW_EH_FRAME - frame unwind information";
		type_descriptions[Segment::PT_GNU_STACK] = "PT_GNU_STACK - stack flags";
		type_descriptions[Segment::PT_GNU_RELRO] = "PT_GNU_RELRO - read-only after relocation";
		type_descriptions[Segment::PT_GNU_PROPERTY] = "PT_GNU_PROPERTY - GNU property";
		type_descriptions[Segment::PT_GNU_SFRAME] = "PT_GNU_SFRAME - stack trace information";
		//type_descriptions[Segment::PT_GNU_MBIND_LO] = "",
		//type_descriptions[Segment::PT_GNU_MBIND_HI] = "",
		type_descriptions[Segment::PT_OPENBSD_MUTABLE] = "PT_OPENBSD_MUTABLE - mutable .bss";
		type_descriptions[Segment::PT_OPENBSD_RANDOMIZE] = "PT_OPENBSD_RANDOMIZE - fill with random data";
		type_descriptions[Segment::PT_OPENBSD_WXNEEDED] = "PT_OPENBSD_WXNEEDED - program does W^X violation";
		type_descriptions[Segment::PT_OPENBSD_NOBTCFI] = "PT_OPENBSD_NOBTCFI - no branch target CFI";
		type_descriptions[Segment::PT_OPENBSD_BOOTDATA] = "PT_OPENBSD_BOOTDATA - section for boot arguments";
		segment_region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(segment.type));

		segment_region.AddField("Flags",
			Dumper::BitFieldDisplay::Make()
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("PF_X - executable"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("PF_W - writable"), true)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("PF_R - readable"), true)
				->AddBitField(24, 1, Dumper::ChoiceDisplay::Make("PF_S - (IBM OS/2) shared segment"), true)
				->AddBitField(25, 1, Dumper::ChoiceDisplay::Make("PF_K - (IBM OS/2) mapped to kernel address space"), true)
				->AddBitField(26, 1, Dumper::ChoiceDisplay::Make("PF_M - (IBM OS/2) memory resident"), true),
			offset_t(flags));

		segment_region.AddField("Virtual address", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(segment.vaddr));
		if(segment.vaddr != segment.paddr)
			segment_region.AddField("Physical address", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(segment.paddr));
		if(segment.filesz != segment.memsz)
			segment_region.AddField("Memory length", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(segment.memsz));
		if(segment.align > 1)
			segment_region.AddField("Alignment", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(segment.align));
		segment_region.Display(dump);

		unsigned j = 0;
		for(auto& part : segment.parts)
		{
			Dumper::Entry part_entry("Part", j + 1, part.GetOffset(*this), 8);
			std::string part_name = "invalid";
			switch(part.type)
			{
			case Segment::Part::Section:
				if(part.offset == 0 && part.size == part.GetActualSize(*this))
					part_name = "section";
				else
					part_name = "partial section";
				break;
			case Segment::Part::Block:
				switch(part.index)
				{
				case 0:
					part_name = "file header";
					break;
				case 1:
					part_name = "section header";
					break;
				case 2:
					part_name = "program header";
					break;
				default:
					part_name = "block";
					break;
				}
				break;
			}
			part_entry.AddField("Type", Dumper::StringDisplay::Make(), part_name);
			if(part.type == Segment::Part::Section)
			{
				part_entry.AddField("Section", Dumper::DecDisplay::Make(), offset_t(part.index));
				part_entry.AddField("Section name", Dumper::StringDisplay::Make(), sections[part.index].name);
				part_entry.AddOptionalField("Offset within section", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(part.offset));
			}
			part_entry.AddField("Length", Dumper::HexDisplay::Make(2 * wordbytes), offset_t(part.size));
			part_entry.Display(dump);
			if(part.type == Segment::Part::Block && part.offset == 0 && part.size == part.GetActualSize(*this) && blocks[part.index].image != nullptr)
			{
				Dumper::Block block("Block", blocks[part.index].offset, blocks[part.index].image,
					segment.vaddr + (blocks[part.index].offset - segment.offset),
					2 * wordbytes);
				block.Display(dump);
			}
			j++;
		}

		i++;
	}

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
		if((section.flags & SHF_ALLOC) != 0 && section.GetSection() != nullptr)
		{
			if(section.type == Section::SHT_PROGBITS || section.type == Section::SHT_NOBITS)
				module.AddSection(section.GetSection());
		}
	}

	for(const Section& section : sections)
	{
		if(auto symbol_table = section.GetSymbolTable())
		{
			for(const Symbol& symbol : symbol_table->symbols)
			{
				if(symbol.shndx == SHN_XINDEX)
				{
					Linker::Error << "Error: Extended section numbers not updated" << std::endl;
					continue;
				}

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

	for(const Section& section : sections)
	{
		if(auto relocations = section.GetRelocations())
		{
			for(Relocation rel : relocations->relocations)
			{
				Linker::Location rel_source = Linker::Location(sections[rel.sh_info].GetSection(), rel.offset);
				const Symbol& sym_target = sections[rel.sh_link].GetSymbolTable()->symbols[rel.symbol];
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
	}
}

void ELFFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	ReadFile(rd);
	GenerateModule(module);
}

void ELFFormat::CalculateValues()
{
	// TODO: untested
	// TODO: unfinished
	size_t minimum_elf_header_size = 0;
	size_t minimum_program_header_entry_size = 0;
	size_t minimum_section_header_entry_size = 0;
	switch(wordbytes)
	{
	default:
		if(file_class != ELFCLASSNONE)
			break; // we will assume the user knows what they are doing
		Linker::Error << "Error: unspecified or invalid word size, assuming 32-bit" << std::endl;
		wordbytes = 4;
	case 4:
		file_class = ELFCLASS32;
		minimum_elf_header_size = 52;
		minimum_program_header_entry_size = 32;
		minimum_section_header_entry_size = 40;
		break;
	case 8:
		file_class = ELFCLASS64;
		minimum_elf_header_size = 64;
		minimum_program_header_entry_size = 56;
		minimum_section_header_entry_size = 64;
		break;
	}

	switch(endiantype)
	{
	default:
		if(file_class != ELFDATANONE)
			break; // we will assume the user knows what they are doing
		Linker::Error << "Error: unspecified or invalid byte order, assuming little endian" << std::endl;
		endiantype = ::LittleEndian;
	case ::LittleEndian:
		data_encoding = ELFDATA2LSB;
		break;
	case ::BigEndian:
		data_encoding = ELFDATA2MSB;
		break;
	}

	if(header_version == EV_NONE)
	{
		header_version = EV_CURRENT;
	}

	if(file_version == EV_NONE)
	{
		file_version = EV_CURRENT;
	}

	if(elf_header_size < minimum_elf_header_size)
	{
		elf_header_size = minimum_elf_header_size;
	}

	if(sections.size() != 0 && section_header_entry_size < minimum_section_header_entry_size)
	{
		section_header_entry_size = minimum_section_header_entry_size;
	}

	if(segments.size() != 0 && program_header_entry_size < minimum_program_header_entry_size)
	{
		program_header_entry_size = minimum_program_header_entry_size;
	}

	if(program_header_offset < elf_header_size)
	{
		program_header_offset = elf_header_size;
	}

	if(sections.size() >= SHN_LORESERVE)
		sections[0].info = sections.size();

	if(section_name_string_table >= SHN_LORESERVE)
		sections[0].link = section_name_string_table;

	for(Section& section : sections)
	{
		switch(section.type)
		{
		case Section::SHT_SYMTAB_SHNDX:
			// TODO: untested
			// TODO: create the section if required
			for(size_t index = 0; index < section.GetArray()->array.size(); index++)
			{
				Symbol& symbol = sections[section.link].GetSymbolTable()->symbols[index];
				if(symbol.shndx >= SHN_LORESERVE)
				{
					section.GetArray()->array[index] = symbol.shndx;
				}
			}
			break;
		default:
			break;
		}
	}
}

