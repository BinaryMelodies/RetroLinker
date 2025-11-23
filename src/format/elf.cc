
#include <sstream>
#include "elf.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"
#include "../linker/section.h"
#include "../linker/writer.h"

using namespace ELF;

void ELFFormat::SectionContents::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
}

void ELFFormat::SectionContents::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
}

//// SymbolTable

offset_t ELFFormat::SymbolTable::ImageSize() const
{
	return symbols.size() * entsize;
}

offset_t ELFFormat::SymbolTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	for(auto symbol : symbols)
	{
		wr.Seek(file_offset);
		file_offset += entsize;
		wr.WriteWord(4, symbol.name_offset);
		if(wordbytes == 4)
		{
			wr.WriteWord(wordbytes, symbol.value);
			wr.WriteWord(wordbytes, symbol.size);
			wr.WriteWord(1, (symbol.bind << 4) | (symbol.type & 0xF));
			wr.WriteWord(1, symbol.other);
			wr.WriteWord(2, symbol.shndx);
		}
		else
		{
			wr.WriteWord(1, (symbol.bind << 4) | (symbol.type & 0xF));
			wr.WriteWord(1, symbol.other);
			wr.WriteWord(2, symbol.shndx);
			wr.WriteWord(wordbytes, symbol.value);
			wr.WriteWord(wordbytes, symbol.size);
		}
	}
	return ImageSize();
}

void ELFFormat::SymbolTable::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
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

		static const std::map<offset_t, std::string> binding_descriptions =
		{
			{ STB_LOCAL,  "STB_LOCAL" },
			{ STB_GLOBAL, "STB_GLOBAL" },
			{ STB_WEAK,   "STB_WEAK" },
			{ STB_ENTRY,  "STB_ENTRY - (IBM OS/2)" },
		};
		symbol_entry.AddField("Binding", Dumper::ChoiceDisplay::Make(binding_descriptions, Dumper::HexDisplay::Make(1)), offset_t(symbol.bind));

		static const std::map<offset_t, std::string> type_descriptions =
		{
			{ STT_NOTYPE,  "STT_NOTYPE" },
			{ STT_OBJECT,  "STT_OBJECT" },
			{ STT_FUNC,    "STT_FUNC" },
			{ STT_SECTION, "STT_SECTION" },
			{ STT_FILE,    "STT_FILE" },
			{ STT_COMMON,  "STT_COMMON" },
			{ STT_TLS,     "STT_TLS" },
			{ STT_IMPORT,  "STT_IMPORT - (IBM OS/2)" }, // TODO: st_value is offset of import table entry
		};
		symbol_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(1)), offset_t(symbol.type));

		static const std::map<offset_t, std::string> visibility_descriptions =
		{
			{ STV_DEFAULT,   "STV_DEFAULT" },
			{ STV_INTERNAL,  "STV_INTERNAL" },
			{ STV_HIDDEN,    "STV_HIDDEN" },
			{ STV_PROTECTED, "STV_PROTECTED" },
		};
		symbol_entry.AddField("Visibility", Dumper::ChoiceDisplay::Make(visibility_descriptions, Dumper::DecDisplay::Make()), offset_t(symbol.other));

		symbol_entry.Display(dump);

		i += 1;
	}
}

//// StringTable

offset_t ELFFormat::StringTable::ImageSize() const
{
	return size;
}

offset_t ELFFormat::StringTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	for(auto& s : strings)
	{
		wr.WriteData(s);
		wr.WriteWord(1, 0);
	}
	return ImageSize();
}

void ELFFormat::StringTable::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
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

//// Array

offset_t ELFFormat::Array::ImageSize() const
{
	return array.size() * entsize;
}

offset_t ELFFormat::Array::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	for(auto& entry : array)
	{
		wr.WriteWord(entsize, entry);
	}
	return ImageSize();
}

void ELFFormat::Array::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
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

//// SectionGroup

offset_t ELFFormat::SectionGroup::ImageSize() const
{
	return (1 + array.size()) * entsize;
}

offset_t ELFFormat::SectionGroup::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	wr.WriteWord(4, flags);
	file_offset += entsize;
	for(auto& entry : array)
	{
		wr.Seek(file_offset);
		wr.WriteWord(4, entry);
		file_offset += entsize;
	}
	return ImageSize();
}

void ELFFormat::SectionGroup::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
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

void ELFFormat::SectionGroup::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	region->AddField("Group flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("GRP_COMDAT"), true),
		flags);
}

//// IndexArray

void ELFFormat::IndexArray::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
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

//// Relocation

size_t ELFFormat::Relocation::GetSize(cpu_type cpu) const
{
	switch(cpu)
	{
	case EM_386:
		switch(type)
		{
		default:
			return 0;
		case R_386_8:
		case R_386_PC8:
			return 1;
		case R_386_16:
		case R_386_PC16:
		case R_386_SEG16:
		case R_386_SUB16:
		case R_386_SEGRELATIVE:
		case R_386_OZSEG16:
		case R_386_OZRELSEG16:
			return 2;
		case R_386_32:
		case R_386_PC32:
		case R_386_SUB32:
		case R_386_GOT32:
		case R_386_GOTPC:
		case R_386_GOTOFF:
		case R_386_PLT32:
			return 4;
		}
	case EM_68K:
		switch(type)
		{
		default:
			return 0;
		case R_68K_8:
		case R_68K_PC8:
		case R_68K_GOT8:
		case R_68K_GOT8O:
		case R_68K_PLT8:
		case R_68K_PLT8O:
			return 1;
		case R_68K_16:
		case R_68K_PC16:
		case R_68K_GOT16:
		case R_68K_GOT16O:
		case R_68K_PLT16:
		case R_68K_PLT16O:
			return 2;
		case R_68K_32:
		case R_68K_PC32:
		case R_68K_GOT32:
		case R_68K_GOT32O:
		case R_68K_PLT32:
		case R_68K_PLT32O:
			return 4;
		}
	case EM_ARM:
		switch(type)
		{
		default:
			return 0;
		case R_ARM_ABS8:
			return 1;
		case R_ARM_ABS16:
			return 2;
		case R_ARM_ABS32:
		case R_ARM_REL32:
		case R_ARM_CALL:
		case R_ARM_JUMP24:
		case R_ARM_PC24:
		case R_ARM_V4BX:
			return 4;
		}
	default:
		return 0;
	}
}

std::string ELFFormat::Relocation::GetName(cpu_type cpu) const
{
	// TODO: this could be handled by arrays
	switch(cpu)
	{
	case EM_386:
		switch(type)
		{
		default:
			return "";
		case R_386_8:
			return "R_386_8";
		case R_386_PC8:
			return "R_386_PC8";
		case R_386_16:
			return "R_386_16";
		case R_386_PC16:
			return "R_386_PC16";
		case R_386_SEG16:
			return "R_386_SEG16";
		case R_386_SUB16:
			return "R_386_SUB16";
		case R_386_SEGRELATIVE:
			return "R_386_SEGRELATIVE";
		case R_386_OZSEG16:
			return "R_386_OZSEG16";
		case R_386_OZRELSEG16:
			return "R_386_OZRELSEG16";
		case R_386_32:
			return "R_386_32";
		case R_386_PC32:
			return "R_386_PC32";
		case R_386_SUB32:
			return "R_386_SUB32";
		case R_386_GOT32:
			return "R_386_GOT32";
		case R_386_GOTPC:
			return "R_386_GOTPC";
		case R_386_GOTOFF:
			return "R_386_GOTOFF";
		case R_386_PLT32:
			return "R_386_PLT32";
		}
	case EM_68K:
		switch(type)
		{
		default:
			return "";
		case R_68K_8:
			return "R_68K_8";
		case R_68K_PC8:
			return "R_68K_PC8";
		case R_68K_16:
			return "R_68K_16";
		case R_68K_PC16:
			return "R_68K_PC16";
		case R_68K_32:
			return "R_68K_32";
		case R_68K_PC32:
			return "R_68K_PC32";
		case R_68K_GOT8:
			return "R_68K_GOT8";
		case R_68K_GOT8O:
			return "R_68K_GOT8O";
		case R_68K_PLT8:
			return "R_68K_PLT8";
		case R_68K_PLT8O:
			return "R_68K_PLT8O";
		case R_68K_GOT16:
			return "R_68K_GOT16";
		case R_68K_GOT16O:
			return "R_68K_GOT16O";
		case R_68K_PLT16:
			return "R_68K_PLT16";
		case R_68K_PLT16O:
			return "R_68K_PLT16O";
		case R_68K_GOT32:
			return "R_68K_GOT32";
		case R_68K_GOT32O:
			return "R_68K_GOT32O";
		case R_68K_PLT32:
			return "R_68K_PLT32";
		case R_68K_PLT32O:
			return "R_68K_PLT32O";
		}
	case EM_ARM:
		switch(type)
		{
		default:
			return "";
		case R_ARM_ABS8:
			return "R_ARM_ABS8";
		case R_ARM_ABS16:
			return "R_ARM_ABS16";
		case R_ARM_ABS32:
			return "R_ARM_ABS32";
		case R_ARM_REL32:
			return "R_ARM_REL32";
		case R_ARM_CALL:
			return "R_ARM_CALL";
		case R_ARM_JUMP24:
			return "R_ARM_JUMP24";
		case R_ARM_PC24:
			return "R_ARM_PC24";
		case R_ARM_V4BX:
			return "R_ARM_V4BX";
		}
	default:
		return "";
	}
}

//// Relocations

offset_t ELFFormat::Relocations::ImageSize() const
{
	return relocations.size() * entsize;
}

offset_t ELFFormat::Relocations::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	for(auto& rel : relocations)
	{
		wr.Seek(file_offset);
		wr.WriteWord(wordbytes, rel.offset);
		if(wordbytes == 4)
		{
			wr.WriteWord(wordbytes, (rel.symbol << 8) | (rel.type & 0xFF));
		}
		else
		{
			wr.WriteWord(wordbytes, (uint64_t(rel.symbol) << 32) | rel.type);
		}
		if(!rel.addend_from_section_data)
			wr.WriteWord(wordbytes, rel.addend);
		file_offset += entsize;
	}
	return ImageSize();
}

void ELFFormat::Relocations::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& rel : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		relocation_entry.AddField("Offset", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.offset));
		relocation_entry.AddField("Type", Dumper::DecDisplay::Make(), offset_t(rel.type));
		relocation_entry.AddOptionalField("Type name", Dumper::StringDisplay::Make(), rel.GetName(fmt.cpu));
		relocation_entry.AddField("Symbol index", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.symbol));
		Symbol& sym = fmt.sections[rel.sh_link].GetSymbolTable()->symbols[rel.symbol];
		if(sym.type == STT_SECTION)
		{
			relocation_entry.AddOptionalField("Section", Dumper::StringDisplay::Make(), fmt.sections[sym.shndx].name);
		}
		else
		{
			relocation_entry.AddOptionalField("Symbol", Dumper::StringDisplay::Make(), sym.name);
		}
		if(!rel.addend_from_section_data)
			relocation_entry.AddField("Addend", Dumper::HexDisplay::Make(2 * fmt.wordbytes), offset_t(rel.addend));
		// TODO: fill addend
		relocation_entry.Display(dump);
		i += 1;
	}
}

//// DynamicSection

offset_t ELFFormat::DynamicSection::ImageSize() const
{
	return dynamic.size() * entsize;
}

offset_t ELFFormat::DynamicSection::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	for(auto& dyn : dynamic)
	{
		wr.Seek(file_offset);
		wr.WriteWord(wordbytes, dyn.tag);
		wr.WriteWord(wordbytes, dyn.value);
		file_offset += entsize;
	}
	return ImageSize();
}

void ELFFormat::DynamicSection::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& dynobj : dynamic)
	{
		Dumper::Entry dynamic_entry("Object", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		static const std::map<offset_t, std::string> tag_descriptions =
		{
			{ DT_NULL, "DT_NULL" },
			{ DT_NEEDED, "DT_NEEDED" },
			{ DT_PLTRELSZ, "DT_PLTRELSZ" },
			{ DT_PLTGOT, "DT_PLTGOT" },
			{ DT_HASH, "DT_HASH" },
			{ DT_STRTAB, "DT_STRTAB" },
			{ DT_SYMTAB, "DT_SYMTAB" },
			{ DT_RELA, "DT_RELA" },
			{ DT_RELASZ, "DT_RELASZ" },
			{ DT_RELAENT, "DT_RELAENT" },
			{ DT_STRSZ, "DT_STRSZ" },
			{ DT_SYMENT, "DT_SYMENT" },
			{ DT_INIT, "DT_INIT" },
			{ DT_FINI, "DT_FINI" },
			{ DT_SONAME, "DT_SONAME" },
			{ DT_RPATH, "DT_RPATH" },
			{ DT_SYMBOLIC, "DT_SYMBOLIC" },
			{ DT_REL, "DT_REL" },
			{ DT_RELSZ, "DT_RELSZ" },
			{ DT_RELENT, "DT_RELENT" },
			{ DT_PLTREL, "DT_PLTREL" },
			{ DT_DEBUG, "DT_DEBUG" },
			{ DT_TEXTREL, "DT_TEXTREL" },
			{ DT_JMPREL, "DT_JMPREL" },
			{ DT_BIND_NOW, "DT_BIND_NOW" },
			{ DT_INIT_ARRAY, "DT_INIT_ARRAY" },
			{ DT_FINI_ARRAY, "DT_FINI_ARRAY" },
			{ DT_INIT_ARRAYSZ, "DT_INIT_ARRAYSZ" },
			{ DT_FINI_ARRAYSZ, "DT_FINI_ARRAYSZ" },
			{ DT_RUNPATH, "DT_RUNPATH" },
			{ DT_FLAGS, "DT_FLAGS" },
			{ DT_ENCODING, "DT_ENCODING" },
			{ DT_PREINIT_ARRAY, "DT_PREINIT_ARRAY" },
			{ DT_PREINIT_ARRAYSZ, "DT_PREINIT_ARRAYSZ" },
			{ DT_SYMTAB_SHNDX, "DT_SYMTAB_SHNDX" },
			{ DT_RELRSZ, "DT_RELRSZ" },
			{ DT_RELR, "DT_RELR" },
			{ DT_RELRENT, "DT_RELRENT" },
				/* IBM OS/2 specific */
			{ DT_EXPORT, "DT_EXPORT (IBM OS/2)" },
			{ DT_EXPORTSZ, "DT_EXPORTSZ (IBM OS/2)" },
			{ DT_EXPENT, "DT_EXPENT (IBM OS/2)" },
			{ DT_IMPORT, "DT_IMPORT (IBM OS/2)" },
			{ DT_IMPORTSZ, "DT_IMPORTSZ (IBM OS/2)" },
			{ DT_IMPENT, "DT_IMPENT (IBM OS/2)" },
			{ DT_IT, "DT_IT (IBM OS/2)" },
			{ DT_ITPRTY, "DT_ITPRTY (IBM OS/2)" },
			{ DT_INITTERM, "DT_INITTERM (IBM OS/2)" },
			{ DT_STACKSZ, "DT_STACKSZ (IBM OS/2)" },
				/* GNU binutils */
			{ DT_GNU_FLAGS1, "DT_GNU_FLAGS1" },
			{ DT_GNU_PRELINKED, "DT_GNU_PRELINKED" },
			{ DT_GNU_CONFLICTSZ, "DT_GNU_CONFLICTSZ" },
			{ DT_GNU_LIBLISTSZ, "DT_GNU_LIBLISTSZ" },
			{ DT_CHECKSUM, "DT_CHECKSUM" },
			{ DT_PLTPADSZ, "DT_PLTPADSZ" },
			{ DT_MOVEENT, "DT_MOVEENT" },
			{ DT_MOVESZ, "DT_MOVESZ" },
			{ DT_FEATURE, "DT_FEATURE" },
			{ DT_POSTFLAG_1, "DT_POSTFLAG_1" },
			{ DT_SYMINSZ, "DT_SYMINSZ" },
			{ DT_SYMINENT, "DT_SYMINENT" },
			{ DT_GNU_HASH, "DT_GNU_HASH" },
			{ DT_TLSDESC_PLT, "DT_TLSDESC_PLT" },
			{ DT_TLSDESC_GOT, "DT_TLSDESC_GOT" },
			{ DT_GNU_CONFLICT, "DT_GNU_CONFLICT" },
			{ DT_GNU_LIBLIST, "DT_GNU_LIBLIST" },
			{ DT_CONFIG, "DT_CONFIG" },
			{ DT_DEPAUDIT, "DT_DEPAUDIT" },
			{ DT_AUDIT, "DT_AUDIT" },
			{ DT_PLTPAD, "DT_PLTPAD" },
			{ DT_MOVETAB, "DT_MOVETAB" },
			{ DT_SYMINFO, "DT_SYMINFO" },
			{ DT_VERSYM, "DT_VERSYM" },
			{ DT_RELACOUNT, "DT_RELACOUNT" },
			{ DT_RELCOUNT, "DT_RELCOUNT" },
			{ DT_FLAGS_1, "DT_FLAGS_1" },
			{ DT_VERDEF, "DT_VERDEF" },
			{ DT_VERDEFNUM, "DT_VERDEFNUM" },
			{ DT_VERNEED, "DT_VERNEED" },
			{ DT_VERNEEDNUM, "DT_VERNEEDNUM" },
			{ DT_AUXILIARY, "DT_AUXILIARY" },
			{ DT_USED, "DT_USED" },
			{ DT_FILTER, "DT_FILTER" },
		};
		dynamic_entry.AddField("Tag", Dumper::ChoiceDisplay::Make(tag_descriptions, Dumper::HexDisplay::Make(2 * fmt.wordbytes)), dynobj.tag);
		if(dynobj.tag == DT_FLAGS)
		{
			dynamic_entry.AddField("Value",
				Dumper::BitFieldDisplay::Make(2 * fmt.wordbytes)
					->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("DF_ORIGIN"), true)
					->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("DF_SYMBOLIC"), true)
					->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("DF_TEXTREL"), true)
					->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("DF_BIND_NOW"), true)
					->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("DF_STATIC_TLS"), true),
			dynobj.value);
		}
		else
		{
			dynamic_entry.AddField("Value", Dumper::HexDisplay::Make(2 * fmt.wordbytes), dynobj.value);
			dynamic_entry.AddOptionalField("Value name", Dumper::StringDisplay::Make(), dynobj.name);
		}
		dynamic_entry.Display(dump);
		i += 1;
	}
}

//// HashTable

uint32_t ELFFormat::HashTable::Hash(const std::string& name)
{
	uint32_t hash = 0;
	for(uint8_t c : name)
	{
		hash = (hash << 4) + c;
		uint32_t top_bits = hash & 0xF0000000;
		if(top_bits != 0)
			hash ^= top_bits >> 24;
		hash &= 0x0FFFFFFF;
	}
	return hash;
}

offset_t ELFFormat::HashTable::ImageSize() const
{
	return (2 + buckets.size() + chains.size()) * 4;
}

offset_t ELFFormat::HashTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	wr.WriteWord(4, buckets.size());
	wr.WriteWord(4, chains.size());
	for(auto bucket : buckets)
	{
		wr.WriteWord(4, bucket);
	}
	for(auto chain : chains)
	{
		wr.WriteWord(4, chain);
	}
	return ImageSize();
}

void ELFFormat::HashTable::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	region->AddField("Buckets", Dumper::DecDisplay::Make(), offset_t(buckets.size()));
	region->AddField("Chains", Dumper::DecDisplay::Make(), offset_t(chains.size()));
}

void ELFFormat::HashTable::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto bucket : buckets)
	{
		Dumper::Entry bucket_entry("Bucket", i + 1, fmt.sections[index].file_offset + 8 + i * 4, 2 * fmt.wordbytes);
		bucket_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(bucket));
		Symbol& sym = fmt.sections[fmt.sections[index].link].GetSymbolTable()->symbols[bucket];
		bucket_entry.AddOptionalField("Symbol", Dumper::StringDisplay::Make(), sym.name);
		bucket_entry.Display(dump);
		i++;
	}

	i = 0;
	for(auto chain : chains)
	{
		Dumper::Entry chain_entry("Chain", i + 1, fmt.sections[index].file_offset + 8 + buckets.size() * 4 + i * 4, 2 * fmt.wordbytes);
		chain_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(chain));
		Symbol& sym = fmt.sections[fmt.sections[index].link].GetSymbolTable()->symbols[chain];
		chain_entry.AddOptionalField("Symbol", Dumper::StringDisplay::Make(), sym.name);
		chain_entry.Display(dump);
		i++;
	}
}

//// NotesSection

offset_t ELFFormat::NotesSection::ImageSize() const
{
	return size;
}

offset_t ELFFormat::NotesSection::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	for(auto& note : notes)
	{
		offset_t namesz = note.name.size() + 1;
		offset_t descsz = note.descriptor.size() + 1;
		wr.WriteWord(4, (namesz + 3) & ~3);
		wr.WriteWord(4, (descsz + 3) & ~3);
		wr.WriteWord(4, note.type);
		wr.WriteData(note.name);
		wr.WriteWord(1, 0);
		if((namesz & 3) != 0)
			wr.Skip((-namesz & 3));
		wr.WriteData(note.descriptor);
		wr.WriteWord(1, 0);
		if((descsz & 3) != 0)
			wr.Skip((-descsz & 3));
	}
	return ImageSize();
}

void ELFFormat::NotesSection::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
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

//// VersionRequirements

offset_t ELFFormat::VersionRequirements::ImageSize() const
{
	// TODO
	return 0;
}

offset_t ELFFormat::VersionRequirements::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO
	return 0;
}

void ELFFormat::VersionRequirements::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	// TODO
}


//// IBMSystemInfo

bool ELFFormat::IBMSystemInfo::IsOS2Specific() const
{
	return os_type == EOS_OS2 && os_size >= 16;
}

offset_t ELFFormat::IBMSystemInfo::ImageSize() const
{
	return 8 + os_size;
}

offset_t ELFFormat::IBMSystemInfo::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO: untested
	assert(offset == 0 && count == ImageSize());
	wr.WriteWord(4, os_type);
	wr.WriteWord(4, os_size);
	if(IsOS2Specific())
	{
		wr.WriteWord(1, os2.sessiontype);
		wr.WriteWord(1, os2.sessionflags);
		wr.Skip(14 - 1);
		wr.WriteWord(1, 0);
	}
	else
	{
		wr.WriteData(os_specific);
	}
	return ImageSize();
}

void ELFFormat::IBMSystemInfo::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	static const std::map<offset_t, std::string> operating_system_descriptions =
	{
		{ IBMSystemInfo::EOS_NONE, "EOS_NONE - Unknown" },
		{ IBMSystemInfo::EOS_PN,   "EOS_PN - IBM Microkernel personality neutral" },
		{ IBMSystemInfo::EOS_SVR4, "EOS_SVR4 - UNIX System V Release 4 operating system environment" },
		{ IBMSystemInfo::EOS_AIX,  "EOS_AIX - IBM AIX operating system environment" },
		{ IBMSystemInfo::EOS_OS2,  "EOS_OS2 - IBM OS/2 operating system, 32 bit environment" },
	};
	region->AddField("OS type", Dumper::ChoiceDisplay::Make(operating_system_descriptions, Dumper::HexDisplay::Make(2)), offset_t(os_type));

	region->AddField("System specific information", Dumper::HexDisplay::Make(8), offset_t(os_size));
	if(IsOS2Specific())
	{
		static const std::map<offset_t, std::string> session_type_descriptions =
		{
			{ IBMSystemInfo::os2_specific::OS2_SES_NONE, "OS2_SES_NONE - None" },
			{ IBMSystemInfo::os2_specific::OS2_SES_FS,   "OS2_SES_FS - Full Screen session" },
			{ IBMSystemInfo::os2_specific::OS2_SES_PM,   "OS2_SES_PM - Presentation Manager session" },
			{ IBMSystemInfo::os2_specific::OS2_SES_VIO,  "OS2_SES_VIO - Windowed (character-mode) session" },
		};
		region->AddField("Session type", Dumper::ChoiceDisplay::Make(session_type_descriptions, Dumper::HexDisplay::Make(2)), offset_t(os2.sessiontype));

		region->AddField("Session flags", Dumper::HexDisplay::Make(8), offset_t(os2.sessionflags));
	}
	else
	{
		// TODO
	}
}

//// IBMImportTable

offset_t ELFFormat::IBMImportTable::ImageSize() const
{
	return imports.size() * entsize;
}

offset_t ELFFormat::IBMImportTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	for(auto& import : imports)
	{
		wr.Seek(file_offset);
		wr.WriteWord(4, import.ordinal);
		wr.WriteWord(4, import.name_offset);
		wr.WriteWord(4, (import.type << 24) | (import.dll & 0x00FFFFFF));
		file_offset += entsize;
	}
	return ImageSize();
}

void ELFFormat::IBMImportTable::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& import : imports)
	{
		Dumper::Entry import_entry("Import", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		if(import.ordinal != uint32_t(-1))
			import_entry.AddField("Ordinal", Dumper::DecDisplay::Make(), offset_t(import.ordinal));
		import_entry.AddOptionalField("Name offset", Dumper::HexDisplay::Make(8), offset_t(import.name_offset));
		import_entry.AddOptionalField("Name", Dumper::StringDisplay::Make(), import.name);

		static const std::map<offset_t, std::string> type_descriptions =
		{
			{ IBMImportEntry::IMP_IGNORED, "IMP_IGNORED - Ignored" },
			{ IBMImportEntry::IMP_STR_IDX, "IMP_STR_IDX - String table index" },
			{ IBMImportEntry::IMP_DT_IDX,  "IMP_DT_IDX - Dynamic segment DT_NEEDED index" },
		};
		import_entry.AddField("Load module type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(8)), offset_t(import.type));

		import_entry.AddField("DLL value", Dumper::HexDisplay::Make(8), offset_t(import.dll));
		import_entry.AddOptionalField("DLL name", Dumper::StringDisplay::Make(), import.dll_name);

		import_entry.Display(dump);
		i++;
	}
}

//// IBMExportTable

offset_t ELFFormat::IBMExportTable::ImageSize() const
{
	return exports.size() * entsize;
}

offset_t ELFFormat::IBMExportTable::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	assert(offset == 0 && count == ImageSize());
	offset_t file_offset = wr.Tell();
	for(auto& _export : exports)
	{
		wr.Seek(file_offset);
		wr.WriteWord(4, _export.ordinal);
		wr.WriteWord(4, _export.symbol_index);
		wr.WriteWord(4, _export.name_offset);
		file_offset += entsize;
	}
	return ImageSize();
}

void ELFFormat::IBMExportTable::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& _export : exports)
	{
		Dumper::Entry export_entry("Export", i + 1, fmt.sections[index].file_offset + i * entsize, 2 * fmt.wordbytes);
		if(_export.ordinal != uint32_t(-1))
			export_entry.AddField("Ordinal", Dumper::DecDisplay::Make(), offset_t(_export.ordinal));
		export_entry.AddField("Symbol index", Dumper::DecDisplay::Make(), offset_t(_export.symbol_index));
		Symbol& sym = fmt.sections[fmt.sections[index].link].GetSymbolTable()->symbols[_export.symbol_index];
		export_entry.AddOptionalField("Symbol", Dumper::StringDisplay::Make(), sym.name);
		export_entry.AddOptionalField("Name offset", Dumper::HexDisplay::Make(8), offset_t(_export.name_offset));
		export_entry.AddOptionalField("Name", Dumper::StringDisplay::Make(), _export.name);

		export_entry.Display(dump);
		i++;
	}
}

//// IBMResourceCollection

offset_t ELFFormat::IBMResourceCollection::ImageSize() const
{
	// TODO
	return 0;
}

offset_t ELFFormat::IBMResourceCollection::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	// TODO
	return 0;
}

void ELFFormat::IBMResourceCollection::AddDumperFields(std::unique_ptr<Dumper::Region>& region, Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	region->AddField("Version", Dumper::DecDisplay::Make(), offset_t(version));
	region->AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(flags));
	region->AddField("Name", Dumper::StringDisplay::Make(), name);
	region->AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(name_offset));
	region->AddField("Item array offset", Dumper::HexDisplay::Make(8), offset_t(item_array_offset));
	region->AddField("Item array entry size", Dumper::HexDisplay::Make(8), offset_t(item_array_entry_size));
	region->AddField("Header size", Dumper::HexDisplay::Make(8), offset_t(header_size));
	region->AddField("String table offset", Dumper::HexDisplay::Make(8), offset_t(string_table_offset));
	region->AddField("Locale offset", Dumper::HexDisplay::Make(8), offset_t(locale_offset));
	if(locale_offset != 0)
	{
		char _country[2] = { char(country[0]), char(country[1]) }; // TODO: wide characters
		char _language[2] = { char(language[0]), char(language[1]) }; // TODO: wide characters
		region->AddField("Country", Dumper::StringDisplay::Make(), std::string(_country, 2));
		region->AddField("Language", Dumper::StringDisplay::Make(), std::string(_language, 2));
	}
}

void ELFFormat::IBMResourceCollection::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	unsigned i = 0;
	for(auto& resource : resources)
	{
		Dumper::Block resource_block("Resource", offset + resource.data_offset, resource.data->AsImage(), 0, 2 * fmt.wordbytes);
		resource_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		resource_block.AddField("Type", Dumper::HexDisplay::Make(8), offset_t(resource.type));
		resource_block.AddField("Ordinal", Dumper::HexDisplay::Make(8), offset_t(resource.ordinal));
		resource_block.AddField("Name", Dumper::StringDisplay::Make(), resource.name);
		resource_block.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(resource.name_offset));
		resource_block.AddField("Data offset", Dumper::HexDisplay::Make(8), offset_t(resource.data_offset));
		resource_block.Display(dump);

		i++;
	}
}

//// Section

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

#if 0
std::shared_ptr<ELFFormat::StringTable> ELFFormat::Section::GetStringTable()
{
	return std::dynamic_pointer_cast<StringTable>(contents);
}
#endif

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

#if 0
std::shared_ptr<ELFFormat::NotesSection> ELFFormat::Section::GetNotesSection()
{
	return std::dynamic_pointer_cast<ELFFormat::NotesSection>(contents);
}

std::shared_ptr<ELFFormat::IBMSystemInfo> ELFFormat::Section::GetIBMSystemInfo()
{
	return std::dynamic_pointer_cast<ELFFormat::IBMSystemInfo>(contents);
}
#endif

std::shared_ptr<ELFFormat::IBMImportTable> ELFFormat::Section::GetIBMImportTable()
{
	return std::dynamic_pointer_cast<ELFFormat::IBMImportTable>(contents);
}

std::shared_ptr<ELFFormat::IBMExportTable> ELFFormat::Section::GetIBMExportTable()
{
	return std::dynamic_pointer_cast<ELFFormat::IBMExportTable>(contents);
}

bool ELFFormat::Section::GetFileSize() const
{
	return type == SHT_NOBITS ? 0 : size;
}

std::shared_ptr<Linker::Section> ELFFormat::Section::ReadProgBits(Linker::Reader& rd, offset_t file_offset, const std::string& name, offset_t size)
{
	std::shared_ptr<Linker::Section> section = std::make_shared<Linker::Section>(name);
	rd.Seek(file_offset);
	section->Expand(size);
	section->ReadFile(rd);
	return section;
}

std::shared_ptr<Linker::Section> ELFFormat::Section::ReadNoBits(const std::string& name, offset_t size)
{
	std::shared_ptr<Linker::Section> section = std::make_shared<Linker::Section>(name);
	section->SetZeroFilled(true);
	section->Expand(size);
	return section;
}

std::shared_ptr<ELFFormat::SymbolTable> ELFFormat::Section::ReadSymbolTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize, uint32_t section_link, size_t wordbytes)
{
	std::shared_ptr<SymbolTable> symbol_table = std::make_shared<SymbolTable>(wordbytes, entsize);
	for(size_t j = 0; j < section_size; j += entsize)
	{
		rd.Seek(file_offset + j);
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
		symbol.sh_link = section_link;
		symbol_table->symbols.push_back(symbol);
	}
	return symbol_table;
}

std::shared_ptr<ELFFormat::StringTable> ELFFormat::Section::ReadStringTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size)
{
	std::shared_ptr<StringTable> string_table = std::make_shared<StringTable>(section_size);
	rd.Seek(file_offset);
	while(rd.Tell() < file_offset + section_size)
	{
		std::string s = rd.ReadASCIIZ();
		string_table->strings.push_back(s);
	}
	return string_table;
}

std::shared_ptr<ELFFormat::Relocations> ELFFormat::Section::ReadRelocations(Linker::Reader& rd, Section::section_type type, offset_t file_offset, offset_t section_size, offset_t entsize, uint32_t section_link, uint32_t section_info, size_t wordbytes)
{
	std::shared_ptr<Relocations> relocations = std::make_shared<Relocations>(wordbytes, entsize);
	for(size_t j = 0; j < section_size; j += entsize)
	{
		rd.Seek(file_offset + j);
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
		rel.addend_from_section_data = type == Section::SHT_REL;
		if(rel.addend_from_section_data)
			rel.addend = 0;
		else
			rel.addend = rd.ReadUnsigned(wordbytes);
//		Debug::Debug << "Debug: Type " << sections[i].type << " addend " << rel.addend << std::endl;
		rel.sh_link = section_link;
		rel.sh_info = section_info;
		relocations->relocations.push_back(rel);
	}
	return relocations;
}

std::shared_ptr<ELFFormat::Array> ELFFormat::Section::ReadArray(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize)
{
	std::shared_ptr<Array> array = std::make_shared<Array>(entsize);
	rd.Seek(file_offset);
	for(size_t j = 0; j < section_size / entsize; j++)
	{
		array->array.push_back(rd.ReadUnsigned(entsize));
	}
	return array;
}

std::shared_ptr<ELFFormat::SectionGroup> ELFFormat::Section::ReadSectionGroup(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize)
{
	// TODO: untested
	std::shared_ptr<SectionGroup> section_group = std::make_shared<SectionGroup>(entsize);
	rd.Seek(file_offset);
	section_group->flags = rd.ReadUnsigned(4);
	for(size_t j = 0; j < section_size / entsize; j++)
	{
		rd.Seek(file_offset + j * entsize);
		section_group->array.push_back(rd.ReadUnsigned(4));
	}
	return section_group;
}

std::shared_ptr<ELFFormat::IndexArray> ELFFormat::Section::ReadIndexArray(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize)
{
	// TODO: untested
	std::shared_ptr<IndexArray> array = std::make_shared<IndexArray>(entsize);
	for(size_t j = 0; j < section_size / entsize; j++)
	{
		rd.Seek(file_offset + j * entsize);
		array->array.push_back(rd.ReadUnsigned(4));
	}
	return array;
}

std::shared_ptr<ELFFormat::HashTable> ELFFormat::Section::ReadHashTable(Linker::Reader& rd, offset_t file_offset)
{
	std::shared_ptr<HashTable> hash_table = std::make_shared<HashTable>();
	rd.Seek(file_offset);
	uint32_t nbucket = rd.ReadUnsigned(4);
	uint32_t nchain = rd.ReadUnsigned(4);
	for(size_t j = 0; j < nbucket; j++)
	{
		hash_table->buckets.push_back(rd.ReadUnsigned(4));
	}
	for(size_t j = 0; j < nchain; j++)
	{
		hash_table->chains.push_back(rd.ReadUnsigned(4));
	}
	return hash_table;
}

std::shared_ptr<ELFFormat::DynamicSection> ELFFormat::Section::ReadDynamic(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize, size_t wordbytes)
{
	std::shared_ptr<DynamicSection> dynamic_section = std::make_shared<DynamicSection>(wordbytes, entsize);
	for(size_t j = 0; j < section_size; j += entsize)
	{
		rd.Seek(file_offset + j);
		DynamicObject dyn;
		dyn.tag = rd.ReadSigned(wordbytes);
		dyn.value = rd.ReadSigned(wordbytes);
		dynamic_section->dynamic.push_back(dyn);
	}
	return dynamic_section;
}

std::shared_ptr<ELFFormat::NotesSection> ELFFormat::Section::ReadNote(Linker::Reader& rd, offset_t file_offset, offset_t section_size)
{
	std::shared_ptr<NotesSection> notes = std::make_shared<NotesSection>(section_size);
	rd.Seek(file_offset);
	while(rd.Tell() < file_offset + section_size)
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
		notes->notes.push_back(note);
	}
	return notes;
}

std::shared_ptr<ELFFormat::VersionRequirements> ELFFormat::Section::ReadVersionRequirements(Linker::Reader& rd, offset_t file_offset, offset_t section_link, offset_t section_info)
{
	std::shared_ptr<VersionRequirements> verneed = std::make_shared<VersionRequirements>();
	rd.Seek(file_offset);
	for(offset_t j = 0; j < section_info; j++)
	{
		VersionRequirement vern;
		vern.version = rd.ReadUnsigned(2);
		uint16_t vn_cnt = rd.ReadUnsigned(2);
		vern.file_name_offset = rd.ReadUnsigned(4);
		vern.offset_auxiliary_array = rd.ReadUnsigned(4);
		vern.offset_next_entry = rd.ReadUnsigned(4);
		Linker::Debug << "Debug: next entry " << vern.offset_next_entry << std::endl;
		Linker::Debug << "Debug: reading " << vn_cnt << " auxiliary" << std::endl;
		rd.Seek(file_offset + vern.offset_auxiliary_array);
		for(int i = 0; i < vn_cnt; i++)
		{
			VersionRequirement::Auxiliary vernaux;
			vernaux.hash = rd.ReadUnsigned(4);
			vernaux.flags = rd.ReadUnsigned(2);
			vernaux.other = rd.ReadUnsigned(2);
			vernaux.name_offset = rd.ReadUnsigned(4);
			vernaux.offset_next_entry = rd.ReadUnsigned(4);
			vern.auxiliary_array.push_back(vernaux);
			if(i != vn_cnt - 1)
				rd.Seek(file_offset + vernaux.offset_next_entry);
		}
		verneed->requirements.push_back(vern);
		if(j != section_info - 1)
			rd.Seek(file_offset + vern.offset_next_entry);
	}
	return verneed;
}

std::shared_ptr<ELFFormat::IBMSystemInfo> ELFFormat::Section::ReadIBMSystemInfo(Linker::Reader& rd, offset_t file_offset)
{
	std::shared_ptr<IBMSystemInfo> system_info = std::make_shared<IBMSystemInfo>();
	rd.Seek(file_offset);
	system_info->os_type = IBMSystemInfo::system_type(rd.ReadUnsigned(4));
	system_info->os_size = rd.ReadUnsigned(4);
	if(system_info->IsOS2Specific())
	{
		system_info->os2.sessiontype = IBMSystemInfo::os2_specific::os2_session(rd.ReadUnsigned(1));
		system_info->os2.sessionflags = rd.ReadUnsigned(1);
	}
	else
	{
		system_info->os_specific.resize(system_info->os_size, '\0');
		rd.ReadData(system_info->os_specific);
	}
	return system_info;
}

std::shared_ptr<ELFFormat::IBMImportTable> ELFFormat::Section::ReadIBMImportTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize)
{
	std::shared_ptr<IBMImportTable> table = std::make_shared<IBMImportTable>(entsize);
	for(size_t j = 0; j < section_size; j += entsize)
	{
		rd.Seek(file_offset + j);
		IBMImportEntry import;
		import.ordinal = rd.ReadUnsigned(4);
		import.name_offset = rd.ReadUnsigned(4);
		import.dll = rd.ReadUnsigned(4);
		import.type = IBMImportEntry::import_type(import.dll >> 24);
		import.dll &= 0x00FFFFFF;
		table->imports.push_back(import);
	}
	return table;
}

std::shared_ptr<ELFFormat::IBMExportTable> ELFFormat::Section::ReadIBMExportTable(Linker::Reader& rd, offset_t file_offset, offset_t section_size, offset_t entsize)
{
	std::shared_ptr<IBMExportTable> table = std::make_shared<IBMExportTable>(entsize);
	for(size_t j = 0; j < section_size; j += entsize)
	{
		rd.Seek(file_offset + j);
		IBMExportEntry _export;
		_export.ordinal = rd.ReadUnsigned(4);
		_export.symbol_index = rd.ReadUnsigned(4);
		_export.name_offset = rd.ReadUnsigned(4);
		table->exports.push_back(_export);
	}
	return table;
}

std::shared_ptr<ELFFormat::IBMResourceCollection> ELFFormat::Section::ReadIBMResourceCollection(Linker::Reader& rd, offset_t file_offset)
{
	std::shared_ptr<IBMResourceCollection> collection = std::make_shared<IBMResourceCollection>();
	rd.Seek(file_offset);
	collection->offset = file_offset;
	collection->version = rd.ReadUnsigned(2);
	collection->flags = rd.ReadUnsigned(2);
	collection->name_offset = rd.ReadUnsigned(4);
	collection->item_array_offset = rd.ReadUnsigned(4); // rioff
	collection->item_array_entry_size = rd.ReadUnsigned(4); // rientsize
	uint32_t item_array_count = rd.ReadUnsigned(4); // rinum
	collection->header_size = rd.ReadUnsigned(4); // rhsize
	collection->string_table_offset = rd.ReadUnsigned(4); // strtab
	collection->locale_offset = rd.ReadUnsigned(4); // locale

	if(collection->locale_offset != 0)
	{
		rd.Seek(file_offset + collection->locale_offset);
		collection->country[0] = rd.ReadUnsigned(2);
		collection->country[1] = rd.ReadUnsigned(2);
		collection->language[0] = rd.ReadUnsigned(2);
		collection->language[1] = rd.ReadUnsigned(2);
	}

	rd.Seek(file_offset + collection->string_table_offset + collection->name_offset);
	collection->name = rd.ReadASCIIZ();

	for(size_t i = 0; i < item_array_count; i++)
	{
		rd.Seek(file_offset + collection->item_array_offset + collection->item_array_entry_size * i);
		IBMResource resource;
		resource.type = rd.ReadUnsigned(4);
		resource.ordinal = rd.ReadUnsigned(4);
		resource.name_offset = rd.ReadUnsigned(4);
		resource.data_offset = rd.ReadUnsigned(4);
		resource.data_size = rd.ReadUnsigned(4);
		collection->resources.push_back(resource);
	}

	for(auto& resource : collection->resources)
	{
		if(resource.name_offset != 0)
		{
			rd.Seek(file_offset + collection->string_table_offset + resource.name_offset);
			resource.name = rd.ReadASCIIZ();
		}
		rd.Seek(file_offset + resource.data_offset);
		resource.data = Linker::Buffer::ReadFromFile(rd, resource.data_size);
	}

	return collection;
}

void ELFFormat::Section::Dump(Dumper::Dumper& dump, const ELFFormat& fmt, unsigned index) const
{
	std::unique_ptr<Dumper::Region> region;
	if(auto section = std::dynamic_pointer_cast<Linker::Buffer>(contents))
	{
		region = std::make_unique<Dumper::Block>("Section", file_offset, section, address, 2 * fmt.wordbytes);
		for(auto& section : fmt.sections)
		{
			auto relocations = section.GetRelocations();
			if(relocations != nullptr && section.info == index)
			{
				for(auto rel : relocations->relocations)
				{
					size_t size = rel.GetSize(fmt.cpu);
					if(size != 0)
						dynamic_cast<Dumper::Block *>(region.get())->AddSignal(rel.offset, size);
				}
			}
		}
	}
	else
	{
		region = std::make_unique<Dumper::Region>("Section", file_offset, contents != nullptr ? contents->ImageSize() : 0, 2 * fmt.wordbytes);
	}

	region->InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index));
	region->InsertField(1, "Name", Dumper::StringDisplay::Make(), name);
	region->InsertField(2, "Name offset", Dumper::HexDisplay::Make(), offset_t(name_offset));

	static std::map<offset_t, std::string> type_descriptions =
	{
		{ SHT_NULL,                   "SHT_NULL - Inactive" },
		{ SHT_PROGBITS,               "SHT_PROGBITS - Program data" },
		{ SHT_SYMTAB,                 "SHT_SYMTAB - Symbol table" },
		{ SHT_STRTAB,                 "SHT_STRTAB - String table" },
		{ SHT_RELA,                   "SHT_RELA - Relocations with explicit addends" },
		{ SHT_HASH,                   "SHT_HASH - Hash table" },
		{ SHT_DYNAMIC,                "SHT_DYNAMIC - Dynamic linking information" },
		{ SHT_NOTE,                   "SHT_NOTE - File information" },
		{ SHT_NOBITS,                 "SHT_NOBITS - Zero filled section" },
		{ SHT_REL,                    "SHT_REL - Relocations without explicit addends" },
		{ SHT_SHLIB,                  "SHT_SHLIB - reserved" },
		{ SHT_DYNSYM,                 "SHT_DYNSYM - Dynamic linking symbol table" },
		//{ SHT_INIT_ARRAY,             "SHT_INIT_ARRAY - Array of initialization functions" }, // overloaded by SHT_OLD_EXPORTS
		//{ SHT_FINI_ARRAY,             "SHT_FINI_ARRAY - Array of termination functions" }, // overloaded by SHT_OLD_RES
		{ SHT_PREINIT_ARRAY,          "SHT_PREINIT_ARRAY - Array of pre-initialization functions" },
		{ SHT_GROUP,                  "SHT_GROUP - Section group" },
		{ SHT_SYMTAB_SHNDX,           "SHT_SYMTAB_SHNDX - Symbol table section indexes" },
			/* IBM OS/2 specific */
		{ SHT_OS,                     "SHT_OS - (IBM OS/2) Target operating system identification" },
		{ SHT_IMPORTS,                "SHT_IMPORTS - (IBM OS/2) External symbol references" },
		{ SHT_EXPORTS,                "SHT_EXPORTS - (IBM OS/2) Exported symbols" },
		{ SHT_RES,                    "SHT_RES - (IBM OS/2) Resource data" },
			/* GNU extensions */
		{ SHT_GNU_INCREMENTAL_INPUTS, "SHT_GNU_INCREMENTAL_INPUTS" },
		{ SHT_GNU_ATTRIBUTES,         "SHT_GNU_ATTRIBUTES" },
		{ SHT_GNU_HASH,               "SHT_GNU_HASH" },
		{ SHT_GNU_LIBLIST,            "SHT_GNU_LIBLIST" },
		{ SHT_SUNW_verdef,            "SHT_SUNW_verdef" },
		{ SHT_SUNW_verneed,           "SHT_SUNW_verneed" },
		{ SHT_SUNW_versym,            "SHT_SUNW_versym" },
	};

	if(!fmt.IsOldOS2Format())
	{
		type_descriptions.erase(SHT_OLD_OS);
		type_descriptions.erase(SHT_OLD_IMPORTS);

		type_descriptions[SHT_INIT_ARRAY]  = "SHT_INIT_ARRAY - Array of initialization functions";
		type_descriptions[SHT_FINI_ARRAY]  = "SHT_FINI_ARRAY - Array of termination functions";
	}
	else
	{
		type_descriptions[SHT_OLD_OS]      = "SHT_OS - (IBM OS/2) Target operating system identification";
		type_descriptions[SHT_OLD_IMPORTS] = "SHT_IMPORTS - (IBM OS/2) External symbol references";
		type_descriptions[SHT_OLD_EXPORTS] = "SHT_EXPORTS - (IBM OS/2) Exported symbols";
		type_descriptions[SHT_OLD_RES]     = "SHT_RES - (IBM OS/2) Resource data";
	}

	region->AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(8)), offset_t(type));

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
	case SHT_IMPORTS:
		name_link = "Link to string table";
		break;
	case SHT_HASH:
	case SHT_REL:
	case SHT_RELA:
	case SHT_EXPORTS:
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
	case SHT_EXPORTS:
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

//// Segment

offset_t ELFFormat::Segment::Part::GetOffset(const ELFFormat& fmt) const
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

offset_t ELFFormat::Segment::Part::GetActualSize(const ELFFormat& fmt) const
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

//// ELFFormat

void ELFFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	rd.Skip(4); // skip signature

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

	rd.Seek(file_offset + 16);

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
		rd.Seek(file_offset + section_header_offset + i * section_header_entry_size);
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
		rd.Seek(file_offset + program_header_offset + i * program_header_entry_size);
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
		rd.Seek(file_offset + sections[section_name_string_table].file_offset + sections[i].name_offset);
		sections[i].name = rd.ReadASCIIZ();
#if DISPLAY_LOGS
		Linker::Debug << "Debug: Section #" << i << ": `" << sections[i].name << "'" << ", type: " << sections[i].type << ", flags: " << sections[i].flags << std::endl;
#endif
		switch(sections[i].type)
		{
		default:
		case Section::SHT_PROGBITS:
			sections[i].contents = Section::ReadProgBits(rd, sections[i].file_offset, sections[i].name, sections[i].size);
			break;
		case Section::SHT_NOBITS:
			sections[i].contents = Section::ReadNoBits(sections[i].name, sections[i].size);
			break;
		case Section::SHT_SYMTAB:
		case Section::SHT_DYNSYM:
			sections[i].contents = Section::ReadSymbolTable(rd, sections[i].file_offset, sections[i].size, sections[i].entsize, sections[i].link, wordbytes);
			break;
		case Section::SHT_STRTAB:
			sections[i].contents = Section::ReadStringTable(rd, sections[i].file_offset, sections[i].size);
			break;
		case Section::SHT_RELA:
		case Section::SHT_REL:
			sections[i].contents = Section::ReadRelocations(rd, sections[i].type, sections[i].file_offset, sections[i].size, sections[i].entsize, sections[i].link, sections[i].info, wordbytes);
			break;
		case Section::SHT_INIT_ARRAY:
		//case Section::SHT_OLD_EXPORTS:
			if(IsOldOS2Format())
				goto parse_exports_section;
			else
				goto parse_array_section;

		case Section::SHT_FINI_ARRAY:
		//case Section::SHT_OLD_RES:
			if(IsOldOS2Format())
				goto parse_res_section;
			else
				goto parse_array_section;

		case Section::SHT_PREINIT_ARRAY:

		parse_array_section:
			Linker::Debug << "Debug: type " << sections[i].type << " of entry size " << sections[i].entsize << std::endl;
			sections[i].contents = Section::ReadArray(rd, sections[i].file_offset, sections[i].size,
				sections[i].entsize != 0 ? sections[i].entsize : wordbytes);
			break;
		case Section::SHT_HASH:
			sections[i].contents = Section::ReadHashTable(rd, sections[i].file_offset);
			break;
		case Section::SHT_GROUP:
			sections[i].contents = Section::ReadSectionGroup(rd, sections[i].file_offset, sections[i].size, sections[i].entsize ? 4 : sections[i].entsize);
			break;
		case Section::SHT_SYMTAB_SHNDX:
			sections[i].contents = Section::ReadIndexArray(rd, sections[i].file_offset, sections[i].size, sections[i].entsize == 0 ? 4 : sections[i].entsize);
			break;
		case Section::SHT_DYNAMIC:
			sections[i].contents = Section::ReadDynamic(rd, sections[i].file_offset, sections[i].size, sections[i].entsize, wordbytes);
			break;
		case Section::SHT_NOTE:
			sections[i].contents = Section::ReadNote(rd, sections[i].file_offset, sections[i].size);
			break;
		case Section::SHT_SUNW_versym:
			sections[i].contents = Section::ReadArray(rd, sections[i].file_offset, sections[i].size,
				sections[i].entsize != 0 ? sections[i].entsize : 2);
			// TODO: link seems to point to dynsym
			break;
#if 0
		// TODO: does not work yet
		case Section::SHT_SUNW_verneed:
			sections[i].contents = Section::ReadVersionRequirements(rd, sections[i].file_offset, sections[i].link, sections[i].info);
			break;
#endif
		case Section::SHT_OS:
		case Section::SHT_OLD_OS:
			sections[i].contents = Section::ReadIBMSystemInfo(rd, sections[i].file_offset);
			break;
		case Section::SHT_IMPORTS:
		case Section::SHT_OLD_IMPORTS:
			sections[i].contents = Section::ReadIBMImportTable(rd, sections[i].file_offset, sections[i].size, sections[i].entsize == 0 ? 32 : sections[i].entsize);
			break;
		case Section::SHT_EXPORTS:
		//case Section::SHT_OLD_EXPORTS:
		parse_exports_section:
			sections[i].contents = Section::ReadIBMExportTable(rd, sections[i].file_offset, sections[i].size, sections[i].entsize == 0 ? 32 : sections[i].entsize);
			break;
		case Section::SHT_RES:
		//case Section::SHT_OLD_RES: // TODO: seems like it uses a different format
		parse_res_section:
			sections[i].contents = Section::ReadIBMResourceCollection(rd, sections[i].file_offset);
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
				sections[i].GetSection()->SetExecutable(true);
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
				rd.Seek(file_offset + sections[symbol.sh_link].file_offset + symbol.name_offset);
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
					symbol.specification = Linker::SymbolDefinition::CreateCommon(symbol.name, "", symbol.size, symbol.value);
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
		case Section::SHT_DYNAMIC:
			for(auto& dynamic_object : section.GetDynamicSection()->dynamic)
			{
				switch(dynamic_object.tag)
				{
				case DT_NEEDED:
				case DT_SONAME:
				case DT_RPATH:
//				case DT_RUNPATH:
					rd.Seek(file_offset + sections[section.link].file_offset + dynamic_object.value);
					dynamic_object.name = rd.ReadASCIIZ();
					break;
				}
			}
			break;
		case Section::SHT_IMPORTS:
		case Section::SHT_OLD_IMPORTS:
			for(IBMImportEntry& import : section.GetIBMImportTable()->imports)
			{
				if(import.name_offset != 0)
				{
					rd.Seek(file_offset + sections[section.link].file_offset + import.name_offset);
					import.name = rd.ReadASCIIZ();
				}
				switch(import.type)
				{
				default:
					break;
				case IBMImportEntry::IMP_STR_IDX:
					rd.Seek(file_offset + sections[section.link].file_offset + import.dll);
					import.dll_name = rd.ReadASCIIZ();
					break;
				case IBMImportEntry::IMP_DT_IDX:
					for(auto& dynamic_section : sections)
					{
						if(dynamic_section.type == Section::SHT_DYNAMIC)
						{
							unsigned dt_needed_index = 1;
							for(auto& dynobj : dynamic_section.GetDynamicSection()->dynamic)
							{
								if(dynobj.tag == DT_NEEDED)
								{
									if(dt_needed_index == import.dll)
									{
										rd.Seek(file_offset + sections[dynamic_section.link].file_offset + dynobj.value);
										import.dll_name = rd.ReadASCIIZ();
										break;
									}
									else
									{
										dt_needed_index ++;
									}
								}
							}
							break;
						}
					}
					break;
				}
			}
			break;
		case Section::SHT_OLD_EXPORTS:
			if(!IsOldOS2Format())
			{
				break;
			}

		case Section::SHT_EXPORTS:
			for(IBMExportEntry& _export : section.GetIBMExportTable()->exports)
			{
				if(_export.name_offset != 0)
				{
					rd.Seek(file_offset + sections[section.info].file_offset + _export.name_offset);
					_export.name = rd.ReadASCIIZ();
				}
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
				rd.Seek(file_offset + covered);
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

offset_t ELFFormat::WriteFile(Linker::Writer& wr) const
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

	for(auto& section : sections)
	{
		if(section.contents != nullptr)
		{
			section.contents->WriteFile(wr);
		}
	}

	for(auto& block : blocks)
	{
		if(block.image != nullptr)
		{
			block.image->WriteFile(wr);
		}
	}

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

	return offset_t(-1);
}

void ELFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("ELF format");

//	Dumper::Region file_region("File", 0, 0, 2 * wordbytes); /* TODO: file size */
//	/* TODO */
//	file_region.Display(dump);

	Dumper::Region identification_region("ELF Identification", 0, 16, 2);
	static const std::map<offset_t, std::string> class_descriptions =
	{
		{ ELFCLASS32, "32-bit" },
		{ ELFCLASS64, "64-bit" },
	};
	identification_region.AddField("File class", Dumper::ChoiceDisplay::Make(class_descriptions, Dumper::HexDisplay::Make(2)), offset_t(file_class));
	static const std::map<offset_t, std::string> encoding_descriptions =
	{
		{ ELFDATA2LSB, "2's complement little endian" },
		{ ELFDATA2MSB, "2's complement big endian" },
	};
	identification_region.AddField("Data encoding", Dumper::ChoiceDisplay::Make(encoding_descriptions, Dumper::HexDisplay::Make(2)), offset_t(data_encoding));
	identification_region.AddField("ELF header version", Dumper::DecDisplay::Make(), offset_t(header_version));
	static const std::map<offset_t, std::string> osabi_descriptions =
	{
		{ 0,  "None" },
		{ 1,  "Hewlett-Packard HP-UX" },
		{ 2,  "NetBSD" },
		{ 3,  "GNU (Linux)" },
		{ 6,  "Sun Solaris" },
		{ 7,  "AIX" },
		{ 8,  "IRIX" },
		{ 9,  "FreeBSD" },
		{ 10, "Compaq TRU64 UNIX" },
		{ 11, "Novell Modesto" },
		{ 12, "Open BSD" },
		{ 13, "Open VMS" },
		{ 14, "Hewlett-Packard Non-Stop Kernel" },
		{ 15, "Amiga Research OS" },
		{ 16, "The FenixOS highly scalable multi-core OS" },
		{ 17, "Nuxi CloudABI" },
		{ 18, "Stratus Technologies OpenVOS" },
	};
	identification_region.AddField("OS/ABI extensions", Dumper::ChoiceDisplay::Make(osabi_descriptions, Dumper::HexDisplay::Make(2)), offset_t(osabi));
	identification_region.AddField("ABI version", Dumper::DecDisplay::Make(), offset_t(abi_version));
	identification_region.Display(dump);

	Dumper::Region header_region("ELF Header", 0, elf_header_size, 2 * wordbytes);
	static const std::map<offset_t, std::string> file_type_descriptions =
	{
		{ ET_NONE, "No file type" },
		{ ET_REL,  "Relocatable file" },
		{ ET_EXEC, "Executable file" },
		{ ET_DYN,  "Shared object file" },
		{ ET_CORE, "Core file" },
	};
	header_region.AddField("Object file type", Dumper::ChoiceDisplay::Make(file_type_descriptions, Dumper::HexDisplay::Make(8)), offset_t(object_file_type));

	static std::map<offset_t, std::string> cpu_descriptions =
	{
		// Most of these descriptions are from https://www.sco.com/developers/gabi/latest/ch4.eheader.html
		// Some adjustments using https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
		{ EM_NONE, "No machine" },
		{ EM_M32, "AT&T WE 32100" },
		{ EM_SPARC, "SUN SPARC" },
		{ EM_386, "Intel 80386 (also Intel 8086/80286)" },
		{ EM_68K, "Motorola 68000 (m68k family)" },
		{ EM_88K, "Motorola 88000 (m88k family)" },
		{ EM_IAMCU, "Intel MCU" },
		{ EM_860, "Intel 80860" },
		{ EM_MIPS, "MIPS I Architecture (MIPS R3000, big-endian only)" },
		{ EM_S370, "IBM System/370 Processor" },
		{ EM_MIPS_RS3_LE, "MIPS RS3000 Little-endian (deprecated)" },
		{ EM_OLD_SPARCV9, "Old version of Sparc v9 (deprecated)" }, // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
		{ EM_PARISC, "Hewlett-Packard PA-RISC" },
		//{ EM_VPP500, "Fujitsu VPP500" }, // clashes with OS/2 PowerPC
		{ EM_SPARC32PLUS, "Enhanced instruction set SPARC (\"v8plus\")" },
		{ EM_960, "Intel 80960" },
		{ EM_PPC, "PowerPC" },
		{ EM_PPC64, "64-bit PowerPC" },
		{ EM_S390, "IBM System/390 Processor" },
		{ EM_SPU, "Sony/Toshiba/IBM SPU/SPC" },
		{ EM_V800, "NEC V800 series" },
		{ EM_FR20, "Fujitsu FR20" },
		{ EM_RH32, "TRW RH-32" },
		{ EM_MCORE, "Motorola RCE (M*Core) or Fujitsu MMA" },
		{ EM_ARM, "ARM 32-bit architecture (AARCH32)" },
		{ EM_ALPHA, "Digital Alpha" },
		{ EM_SH, "Renesas (Hitachi) SuperH/SH" },
		{ EM_SPARCV9, "SPARC Version 9 (64-bit)" },
		{ EM_TRICORE, "Siemens TriCore embedded processor" },
		{ EM_ARC, "Argonaut RISC Core, Argonaut Technologies Inc." },
		{ EM_H8_300, "Renesas (Hitachi) H8/300" },
		{ EM_H8_300H, "Renesas (Hitachi) H8/300H" },
		{ EM_H8S, "Renesas (Hitachi) H8S" },
		{ EM_H8_500, "Renesas (Hitachi) H8/500" },
		{ EM_IA_64, "Intel IA-64 processor architecture (Itanium)" },
		{ EM_MIPS_X, "Stanford MIPS-X" },
		{ EM_COLDFIRE, "Motorola ColdFire" },
		{ EM_68HC12, "Motorola M68HC12" },
		{ EM_MMA, "Fujitsu MMA Multimedia Accelerator" },
		{ EM_PCP, "Siemens PCP" },
		{ EM_NCPU, "Sony nCPU embedded RISC processor" },
		{ EM_NDR1, "Denso NDR1 microprocessor" },
		{ EM_STARCORE, "Motorola Star*Core processor" },
		{ EM_ME16, "Toyota ME16 processor" },
		{ EM_ST100, "STMicroelectronics ST100 processor" },
		{ EM_TINYJ, "Advanced Logic Corp. TinyJ embedded processor family" },
		{ EM_X86_64, "AMD x86-64 architecture" },
		{ EM_PDSP, "Sony DSP Processor" },
		{ EM_PDP10, "Digital Equipment Corp. PDP-10" },
		{ EM_PDP11, "Digital Equipment Corp. PDP-11" },
		{ EM_FX66, "Siemens FX66 microcontroller" },
		{ EM_ST9PLUS, "STMicroelectronics ST9+ 8/16 bit microcontroller" },
		{ EM_ST7, "STMicroelectronics ST7 8-bit microcontroller" },
		{ EM_68HC16, "Motorola MC68HC16 Microcontroller" },
		{ EM_68HC11, "Motorola MC68HC11 Microcontroller" },
		{ EM_68HC08, "Motorola MC68HC08 Microcontroller" },
		{ EM_68HC05, "Motorola MC68HC05 Microcontroller" },
		{ EM_SVX, "Silicon Graphics SVx" },
		{ EM_ST19, "STMicroelectronics ST19 8-bit microcontroller" },
		{ EM_VAX, "Digital VAX" },
		{ EM_CRIS, "Axis Communication 32-bit embedded processor" },
		{ EM_JAVELIN, "Infineon Technologies 32-bit embedded processor" },
		{ EM_FIREPATH, "Element 14 64-bit DSP Processor" },
		{ EM_ZSP, "LSI Logic 16-bit DSP Processor" },
		{ EM_MMIX, "Donald Knuth's educational 64-bit processor" },
		{ EM_HUANY, "Harvard University machine-independent object file" },
		{ EM_PRISM, "SiTera Prism" },
		{ EM_AVR, "Atmel AVR 8-bit microcontroller" },
		{ EM_FR30, "Fujitsu FR30" },
		{ EM_D10V, "Mitsubishi D10V" },
		{ EM_D30V, "Mitsubishi D30V" },
		{ EM_V850, "Renesas (NEC) v850" },
		{ EM_M32R, "Renesas (Mitsubishi) M32R" },
		{ EM_MN10300, "Matsushita MN10300" },
		{ EM_MN10200, "Matsushita MN10200" },
		{ EM_PJ, "picoJava" },
		{ EM_OPENRISC, "OpenRISC 1000 32-bit embedded processor" },
		{ EM_ARC_COMPACT, "ARC International ARCompact processor" },
		{ EM_XTENSA, "Tensilica Xtensa Architecture" },
		{ EM_VIDEOCORE, "Alphamosaic VideoCore processor/old Sunplus S+core7 backend magic number" }, // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
		{ EM_TMM_GPP, "Thompson Multimedia General Purpose Processor" },
		{ EM_NS32K, "National Semiconductor 32000 series" },
		{ EM_TPC, "Tenor Network TPC processor" },
		{ EM_SNP1K, "Trebia SNP 1000 processor/old picoJava" },
		{ EM_ST200, "STMicroelectronics (www.st.com) ST200 microcontroller" },
		{ EM_IP2K, "Ubicom IP2022/IP2xxx microcontroller family" },
		{ EM_MAX, "MAX Processor" },
		{ EM_CR, "National Semiconductor CompactRISC microprocessor" },
		{ EM_F2MC16, "Fujitsu F2MC16" },
		{ EM_MSP430, "Texas Instruments embedded microcontroller msp430" },
		{ EM_BLACKFIN, "Analog Devices Blackfin (DSP) processor" },
		{ EM_SE_C33, "S1C33 Family of Seiko Epson processor" },
		{ EM_SEP, "Sharp embedded microprocessor" },
		{ EM_ARCA, "Arca RISC Microprocessor" },
		{ EM_UNICORE, "Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University" },
		{ EM_EXCESS, "eXcess: 16/32/64-bit configurable embedded CPU" },
		{ EM_DXP, "Icera Semiconductor Inc. Deep Execution Processor" },
		{ EM_ALTERA_NIOS32, "Altera Nios II soft-core processor" },
		{ EM_CRX, "National Semiconductor CompactRISC CRX microprocessor" },
		{ EM_XGATE, "Motorola XGATE embedded processor/old National Semiconductor CompactRISC" },
		{ EM_C166, "Infineon C16x/XC16x processor" },
		{ EM_M16C, "Renesas M16C series microprocessors" },
		{ EM_DSPIC30F, "Microchip Technology dsPIC30F Digital Signal Controller" },
		{ EM_CE, "Freescale Communication Engine RISC core" },
		{ EM_M32C, "Renesas M32C series microprocessors" },
		{ EM_TSK3000, "Altium TSK3000 core" },
		{ EM_RS08, "Freescale RS08 embedded processor" },
		{ EM_SHARC, "Analog Devices SHARC family of 32-bit DSP processors" },
		{ EM_ECOG2, "Cyan Technology eCOG2 microprocessor" },
		{ EM_SCORE7, "Sunplus Score/Sunplus S+core7 RISC processor" },
		{ EM_DSP24, "New Japan Radio (NJR) 24-bit DSP Processor" },
		{ EM_VIDEOCORE3, "Broadcom Videocore III processor" },
		{ EM_LATTICEMICO32, "RISC processor for Lattice FPGA architecture" },
		{ EM_SE_CE17, "Seiko Epson C17 family" },
		{ EM_TI_C6000, "The Texas Instruments TMS320C6000 DSP family" },
		{ EM_TI_C2000, "The Texas Instruments TMS320C2000 DSP family" },
		{ EM_TI_C5500, "The Texas Instruments TMS320C55x DSP family" },
		{ EM_TI_ARP32, "Texas Instruments Application Specific RISC Processor, 32-bit fetch" },
		{ EM_TI_PRU, "Texas Instruments Programmable Realtime Unit" },
		{ EM_MMDSP_PLUS, "STMicroelectronics 64bit VLIW Data Signal Processor" },
		{ EM_CYPRESS_M8C, "Cypress M8C microprocessor" },
		{ EM_R32C, "Renesas R32C series microprocessors" },
		{ EM_TRIMEDIA, "NXP Semiconductors TriMedia architecture family" },
		{ EM_QDSP6, "QUALCOMM DSP6 Processor" },
		{ EM_8051, "Intel 8051 and variants" },
		{ EM_STXP7X, "STMicroelectronics STxP7x family of configurable and extensible RISC processors" },
		{ EM_NDS32, "Andes Technology compact code size embedded RISC processor family" },
		{ EM_ECOG1, "Cyan Technology eCOG1X family" },
		{ EM_MAXQ30, "Dallas Semiconductor MAXQ30 Core Micro-controllers" },
		{ EM_XIMO16, "New Japan Radio (NJR) 16-bit DSP Processor" },
		{ EM_MANIK, "M2000 Reconfigurable RISC Microprocessor" },
		{ EM_CRAYNV2, "Cray Inc. NV2 vector architecture" },
		{ EM_RX, "Renesas RX family" },
		{ EM_METAG, "Imagination Technologies META processor architecture" },
		{ EM_MCST_ELBRUS, "MCST Elbrus general purpose hardware architecture" },
		{ EM_ECOG16, "Cyan Technology eCOG16 family" },
		{ EM_CR16, "National Semiconductor CompactRISC CR16 16-bit microprocessor" },
		{ EM_ETPU, "Freescale Extended Time Processing Unit" },
		{ EM_SLE9X, "Infineon Technologies SLE9X core" },
		{ EM_L10M, "Intel L10M" },
		{ EM_K10M, "Intel K10M" },
		{ EM_AARCH64, "ARM 64-bit architecture (AARCH64)" },
		{ EM_AVR32, "Atmel Corporation 32-bit microprocessor family" },
		{ EM_STM8, "STMicroelectronics STM8 8-bit microcontroller" },
		{ EM_TILE64, "Tilera TILE64 multicore architecture family" },
		{ EM_TILEPRO, "Tilera TILEPRO multicore architecture family" },
		{ EM_MICROBLAZE, "Xilinx MicroBlaze 32-bit RISC soft processor core" },
		{ EM_CUDA, "NVIDIA CUDA architecture" },
		{ EM_TILEGX, "Tilera TILE-Gx multicore architecture family" },
		{ EM_CLOUDSHIELD, "CloudShield architecture family" },
		{ EM_COREA_1ST, "KIPO-KAIST Core-A 1st generation processor family" },
		{ EM_COREA_2ND, "KIPO-KAIST Core-A 2nd generation processor family" },
		{ EM_ARC_COMPACT2, "Synopsys ARCompact V2" },
		{ EM_OPEN8, "Open8 8-bit RISC soft processor core" },
		{ EM_RL78, "Renesas RL78 family" },
		{ EM_VIDEOCORE5, "Broadcom VideoCore V processor" },
		{ EM_78KOR, "Renesas 78KOR family" },
		{ EM_56800EX, "Freescale 56800EX Digital Signal Controller (DSC)" },
		{ EM_BA1, "Beyond BA1 CPU architecture" },
		{ EM_BA2, "Beyond BA2 CPU architecture" },
		{ EM_XCORE, "XMOS xCORE processor family" },
		{ EM_MCHP_PIC, "Microchip 8-bit PIC(r) family" },
		{ EM_INTELGT, "Intel Graphics Technology" }, // https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
		{ EM_KM32, "KM211 KM32 32-bit processor" },
		{ EM_KMX32, "KM211 KMX32 32-bit processor" },
		{ EM_KMX16, "KM211 KMX16 16-bit processor" },
		{ EM_KMX8, "KM211 KMX8 8-bit processor" },
		{ EM_KVARC, "KM211 KVARC processor" },
		{ EM_CDP, "Paneve CDP architecture family" },
		{ EM_COGE, "Cognitive Smart Memory Processor" },
		{ EM_COOL, "Bluechip Systems CoolEngine" },
		{ EM_NORC, "Nanoradio Optimized RISC" },
		{ EM_CSR_KALIMBA, "CSR Kalimba architecture family" },
		{ EM_Z80, "Zilog Z80" },
		{ EM_VISIUM, "Controls and Data Services VISIUMcore processor" },
		{ EM_FT32, "FTDI Chip FT32 high performance 32-bit RISC architecture" },
		{ EM_MOXIE, "Moxie processor family" },
		{ EM_AMDGPU, "AMD GPU architecture" },
		{ EM_RISCV, "RISC-V" },
		// https://github.com/bminor/binutils-gdb/blob/master/include/elf/common.h
		{ EM_LANAI, "Lanai 32-bit processor" },
		{ EM_CEVA, "CEVA Processor Architecture Family" },
		{ EM_CEVA_X2, "CEVA X2 Processor Family" },
		{ EM_BPF, "Linux BPF - in-kernel virtual machine" },
		{ EM_GRAPHCORE_IPU, "Graphcore Intelligent Processing Unit" },
		{ EM_IMG1, "Imagination Technologies" },
		{ EM_NFP, "Netronome Flow Processor" },
		{ EM_VE, "NEC Vector Engine" },
		{ EM_CSKY, "C-SKY processor family" },
		{ EM_ARC_COMPACT3_64, "Synopsys ARCv2.3 64-bit" },
		{ EM_MCS6502, "MOS Technology MCS 6502 processor" },
		{ EM_ARC_COMPACT3, "Synopsys ARCv2.3 32-bit" },
		{ EM_KVX, "Kalray VLIW core of the MPPA processor family" },
		{ EM_65816, "WDC 65816/65C816" },
		{ EM_LOONGARCH, "LoongArch" },
		{ EM_KF32, "ChipOn KungFu32" },
		{ EM_U16_U8CORE, "LAPIS nX-U16/U8" },
		{ EM_TACHYUM, "Tachyum" },
		{ EM_56800EF, "NXP 56800EF Digital Signal Controller (DSC) " },
		// others
		{ EM_HOBBIT, "AT&T Hobbit" },
		{ EM_MOS, "MOS 6502 (llvm-mos ELF specification)" },
	};

	if(!IsOldOS2Format())
	{
		cpu_descriptions[EM_VPP500] = "Fujitsu VPP500";
	}
	else
	{
		cpu_descriptions[EM_VPP500] = "PowerPC (beta OS/2)";
	}
	header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(cpu_descriptions, Dumper::DecDisplay::Make()), offset_t(cpu));
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

		static std::map<offset_t, std::string> type_descriptions =
		{
			{ Segment::PT_NULL, "PT_NULL - ignored" },
			{ Segment::PT_LOAD, "PT_LOAD - loadable segment" },
			{ Segment::PT_DYNAMIC, "PT_DYNAMIC - dynamic linking information" },
			{ Segment::PT_INTERP, "PT_INTERP - interpreter name" },
			{ Segment::PT_NOTE, "PT_NOTE - auxiliary information" },
			{ Segment::PT_SHLIB, "PT_SHLIB - reserved" },
			{ Segment::PT_PHDR, "PT_PHDR - program header table" },
			//{ Segment::PT_TLS, "PT_TLS - thread-local storage" }, // clashes with PT_OLD_OS
			{ Segment::PT_OS, "PT_OS - (IBM OS/2) target operating system identification" },
			{ Segment::PT_RES, "PT_RES - (IBM OS/2) read-only resource data" },
			{ Segment::PT_SUNW_EH_FRAME, "PT_SUNW_EH_FRAME - frame unwind information" },
			{ Segment::PT_GNU_STACK, "PT_GNU_STACK - stack flags" },
			{ Segment::PT_GNU_RELRO, "PT_GNU_RELRO - read-only after relocation" },
			{ Segment::PT_GNU_PROPERTY, "PT_GNU_PROPERTY - GNU property" },
			{ Segment::PT_GNU_SFRAME, "PT_GNU_SFRAME - stack trace information" },
			{ Segment::PT_OPENBSD_MUTABLE, "PT_OPENBSD_MUTABLE - mutable .bss" },
			{ Segment::PT_OPENBSD_RANDOMIZE, "PT_OPENBSD_RANDOMIZE - fill with random data" },
			{ Segment::PT_OPENBSD_WXNEEDED, "PT_OPENBSD_WXNEEDED - program does W^X violation" },
			{ Segment::PT_OPENBSD_NOBTCFI, "PT_OPENBSD_NOBTCFI - no branch target CFI" },
			{ Segment::PT_OPENBSD_BOOTDATA, "PT_OPENBSD_BOOTDATA - section for boot arguments" },
		};
		if(!IsOldOS2Format())
		{
			type_descriptions.erase(Segment::PT_OLD_RES);

			type_descriptions[Segment::PT_TLS] = "PT_TLS - thread-local storage";
		}
		else
		{
			type_descriptions[Segment::PT_OLD_OS] = "PT_OS - (IBM OS/2) target operating system identification";
			type_descriptions[Segment::PT_OLD_RES] = "PT_RES - (IBM OS/2) read-only resource data";
		}
		segment_region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(8)), offset_t(segment.type));

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
				Dumper::Block block("Block", blocks[part.index].offset, blocks[part.index].image->AsImage(),
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

bool ELFFormat::IsOldOS2Format() const
{
	if(cpu != EM_VPP500)
	{
		return false;
	}
	if(segments.size() > 0)
	{
		bool found_old_os = false;
		for(auto& segment : segments)
		{
			if(segment.type == Segment::PT_OLD_RES)
			{
				return true;
			}
			if(segment.type == Segment::PT_OLD_OS)
			{
				found_old_os = false;
			}
		}
		if(!found_old_os)
		{
			/* Such an executable must contain a PT_OLD_OS segment, however that is also used by PT_TLS */
			return false;
		}
	}
	for(auto& section : sections)
	{
		if(section.type == Section::SHT_OLD_OS || section.type == Section::SHT_OLD_IMPORTS)
		{
			return true;
		}
	}
	return false;
}

void ELFFormat::SetupOptions(std::shared_ptr<Linker::OutputFormat> format)
{
	option_16bit = format->FormatIs16bit();
	option_linear = format->FormatIsLinear();
	option_pmode = format->FormatIsProtectedMode();
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
	module.endiantype = endiantype;

	for(const Section& section : sections)
	{
		if((section.flags & SHF_MERGE))
		{
			Linker::Debug << "Debug: Mergeable sections currently not supported" << std::endl;
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
					case STB_GLOBAL:
						module.AddGlobalSymbol(symbol.name, symbol.location);
						break;
					case STB_WEAK:
						module.AddWeakSymbol(symbol.name, symbol.location);
						break;
					default:
						Linker::Debug << "Debug: Unknown symbol binding type " << int(symbol.bind) << ", ignoring" << std::endl;
						break;
					}
				}
				else if(symbol.unallocated)
				{
					module.AddCommonSymbol(symbol.specification);
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
				const Section& src_section = sections[rel.sh_info];
				if(src_section.type != Section::SHT_PROGBITS && src_section.type != Section::SHT_NOBITS)
					continue;

				Linker::Location rel_source = Linker::Location(src_section.GetSection(), rel.offset);
				const Symbol& sym_target = sections[rel.sh_link].GetSymbolTable()->symbols[rel.symbol];
				Linker::SymbolName sym_name = Linker::SymbolName(sym_target.name);
				Linker::Target rel_target = sym_target.defined ? Linker::Target(sym_target.location) : Linker::Target(sym_name);
				Linker::Relocation obj_rel = Linker::Relocation::Empty();
				size_t rel_size;
				switch(cpu)
				{
				case EM_386:
					rel_size = rel.GetSize(cpu);
					if(rel_size == 0)
						continue;

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
					case R_386_SUB16:
					case R_386_SUB32:
						// TODO: untested
						obj_rel =
							Linker::Relocation::OffsetFrom(rel_size, rel_source, Linker::Target(Linker::Location()),
								rel_target, rel.addend, ::LittleEndian);
						break;
					case R_386_SEG16:
						// TODO: untested
						obj_rel =
							option_pmode
							? Linker::Relocation::Selector(rel_source, rel_target, rel.addend)
							: Linker::Relocation::Paragraph(rel_source, rel_target, rel.addend);
						break;
					case R_386_SEGRELATIVE:
						// TODO
						Linker::Debug << "Internal error: SEGRELATIVE not supported" << std::endl;
						continue;
					case R_386_OZSEG16:
						obj_rel =
							option_pmode
							? Linker::Relocation::Selector(rel_source, rel_target.GetSegment(), rel.addend)
							: Linker::Relocation::Paragraph(rel_source, rel_target.GetSegment(), rel.addend);
						break;
					case R_386_OZRELSEG16:
						// TODO
						Linker::Debug << "Internal error: OZRELSEG16 not supported" << std::endl;
						continue;
					case R_386_GOT32:
						obj_rel = Linker::Relocation::GOTEntryAbsolute(rel_size, rel_source, sym_name, rel.addend, ::LittleEndian);
						// TODO: for PIC, use GOTEntryOffset instead
						break;
					case R_386_GOTPC:
						obj_rel = Linker::Relocation::Relative(rel_size, rel_source, Linker::SymbolName::GOT, rel.addend, ::LittleEndian);
						break;
					case R_386_GOTOFF:
						obj_rel = Linker::Relocation::OffsetFrom(rel_size, rel_source, rel_target, Linker::Target(Linker::SymbolName::GOT), rel.addend, ::LittleEndian);
						break;
					case R_386_PLT32:
						// TODO
						Linker::Debug << "Internal error: PLT32 not supported" << std::endl;
						continue;
					}
					break;

				case EM_68K:
					rel_size = rel.GetSize(cpu);
					if(rel_size == 0)
						continue;

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
					case R_68K_GOT8:
					case R_68K_GOT16:
					case R_68K_GOT32:
						obj_rel = Linker::Relocation::GOTEntryAbsolute(rel_size, rel_source, sym_name, rel.addend, ::BigEndian);
						break;
					case R_68K_GOT8O:
					case R_68K_GOT16O:
					case R_68K_GOT32O:
						obj_rel = Linker::Relocation::GOTEntryRelative(rel_size, rel_source, sym_name, rel.addend, ::BigEndian);
						break;
					case R_68K_PLT8:
						// TODO
						Linker::Debug << "Internal error: PLT8 not supported" << std::endl;
						continue;
					case R_68K_PLT16:
						// TODO
						Linker::Debug << "Internal error: PLT16 not supported" << std::endl;
						continue;
					case R_68K_PLT32:
						// TODO
						Linker::Debug << "Internal error: PLT32 not supported" << std::endl;
						continue;
					case R_68K_PLT8O:
						// TODO
						Linker::Debug << "Internal error: PLT8O not supported" << std::endl;
						continue;
					case R_68K_PLT16O:
						// TODO
						Linker::Debug << "Internal error: PLT16O not supported" << std::endl;
						continue;
					case R_68K_PLT32O:
						// TODO
						Linker::Debug << "Internal error: PLT32O not supported" << std::endl;
						continue;
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

FatELFFormat::Record FatELFFormat::Record::Read(Linker::Reader& rd)
{
	Record record;
	record.cpu = ELFFormat::cpu_type(rd.ReadUnsigned(2));
	record.osabi = rd.ReadUnsigned(1);
	record.abi_version = rd.ReadUnsigned(1);
	record.file_class = rd.ReadUnsigned(1);
	record.data_encoding = rd.ReadUnsigned(1);
	rd.Skip(2);
	record.offset = rd.ReadUnsigned(8);
	record.size = rd.ReadUnsigned(8);
	return record;
}

void FatELFFormat::Record::Write(Linker::Writer& wr) const
{
	wr.WriteWord(2, cpu);
	wr.WriteWord(1, osabi);
	wr.WriteWord(1, abi_version);
	wr.WriteWord(1, file_class);
	wr.WriteWord(1, data_encoding);
	wr.Skip(2);
	wr.WriteWord(8, offset);
	wr.WriteWord(8, size);
}

offset_t FatELFFormat::ImageSize() const
{
	// the size of the header
	offset_t furthest = 8 + records.size() * 24;
	for(auto& record : records)
	{
		offset_t record_end = record.offset + record.size;
		if(record_end > furthest)
			furthest = record_end;
	}
	return furthest;
}

void FatELFFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Skip(4); // magic code
	version = rd.ReadUnsigned(2);
	if(version != ELFFormat::EV_CURRENT)
	{
		std::ostringstream oss;
		oss << "Fatal error: unknown FatELF version " << version;
		Linker::FatalError(oss.str());
	}
	uint8_t record_count = rd.ReadUnsigned(1);
	rd.Skip(1);
	for(int i = 0; i < record_count; i++)
	{
		records.emplace_back(Record::Read(rd));
	}
	for(auto& record : records)
	{
		std::shared_ptr<ELFFormat> elf = std::make_shared<ELFFormat>();
		record.image = elf;
		rd.Seek(record.offset);
		elf->ReadFile(rd);
	}
}

offset_t FatELFFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData("\xFA\x70\x0E\x1F");
	wr.WriteWord(2, version);
	if(records.size() > 255)
	{
		std::ostringstream oss;
		oss << "Fatal error: number of binaries exceeds 255, " << records.size() << " found";
		Linker::FatalError(oss.str());
	}
	wr.WriteWord(1, records.size());
	wr.Skip(1);
	for(auto& record : records)
	{
		record.Write(wr);
	}
	for(auto& record : records)
	{
		wr.Seek(record.offset);
		record.image->WriteFile(wr);
	}
	return ImageSize();
}

void FatELFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("FatELF format");

	// TODO
	for(auto& record : records)
	{
		if(ELFFormat * elf = dynamic_cast<ELFFormat *>(record.image.get()))
		{
			elf->Dump(dump);
		}
		else
		{
			// TODO
			//record.image->Dump(dump);
		}
	}
}

void FatELFFormat::CalculateValues()
{
	version = ELFFormat::EV_CURRENT;

	offset_t current_offset = 8 + records.size() * 24;
	for(auto& record : records)
	{
		if(ELFFormat * elf = dynamic_cast<ELFFormat *>(record.image.get()))
		{
			record.cpu = elf->cpu;
			record.osabi = elf->osabi;
			record.abi_version = elf->abi_version;
			record.file_class = elf->file_class;
			record.data_encoding = elf->data_encoding;
		}
		else
		{
			auto image = record.image->AsImage();
			record.osabi = image->GetByte(ELFFormat::EI_OSABI);
			record.abi_version = image->GetByte(ELFFormat::EI_ABIVERSION);
			record.file_class = image->GetByte(ELFFormat::EI_CLASS);
			record.data_encoding = image->GetByte(ELFFormat::EI_DATA);
			if(record.data_encoding == ELFFormat::ELFDATA2MSB)
				record.cpu = ELFFormat::cpu_type((image->GetByte(18) << 8) + image->GetByte(19));
			else
				record.cpu = ELFFormat::cpu_type(image->GetByte(18) + (image->GetByte(19) << 8));
		}
		offset_t page_size = 4096; // TODO: depends on architecture
		record.offset = ::AlignTo(current_offset, page_size);
		record.size = record.image->ImageSize(); // TODO
		current_offset = record.offset + record.size;
	}
}

