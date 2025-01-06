
#include "leexe.h"
#include "mzexe.h"

using namespace Microsoft;

void LEFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

bool LEFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool LEFormat::FormatSupportsLibraries() const
{
	return true;
}

unsigned LEFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		return Linker::Section::Stack;
	}
	else if(section_name == ".heap" || section_name.rfind(".heap.", 0) == 0)
	{
		return Linker::Section::Heap;
	}
	else
	{
		return 0;
	}
}

bool LEFormat::IsOS2() const
{
	return extended_format;
}

bool LEFormat::IsLibrary() const
{
	return module_flags & Library;
}

bool LEFormat::IsDriver() const
{
	/* TODO: Windows only */
	return system == Windows;
}

std::shared_ptr<LEFormat> LEFormat::CreateConsoleApplication(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, GUIAware, true);
	case Windows386:
		return std::make_shared<LEFormat>(system, Library | NoExternalFixup, false); /* TODO: actually, a driver */
	case DOS4G: /* not an actual type */
		return std::make_shared<LEFormat>(OS2, GUIAware, false);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::CreateGUIApplication(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, GUI, true);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::CreateLibraryModule(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, Library | NoInternalFixup, true);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::SimulateLinker(compatibility_type compatibility)
{
	this->compatibility = compatibility;
	switch(compatibility)
	{
	case CompatibleNone:
		break;
	case CompatibleWatcom:
		break;
	case CompatibleMicrosoft:
		/* TODO */
		break;
	case CompatibleGNU:
		/* TODO */
		break;
	/* TODO: others */
	}
	return shared_from_this();
}

void LEFormat::AddRelocation(Object& object, unsigned type, unsigned flags, size_t offset, uint16_t module, uint32_t target, uint32_t addition)
{
	size_t page_index = object.page_table_index + (offset - object.image->base_address) / page_size;
	uint16_t page_offset = offset & (page_size - 1);
	Page::Relocation rel = Page::Relocation(type, flags, page_offset, module, target, addition);
#if 0
	Linker::Debug << "Debug: PAGES[" << page_index << "] <- " << page_offset << std::endl;
	for(auto& it : pages[page_index].relocations)
	{
		Linker::Debug << "Debug:\t" << it.first << "!" << std::endl;
	}
#endif
	pages[page_index].relocations[page_offset] = rel;
	if(page_offset + rel.GetSourceSize() > page_size)
	{
		/* crosses page boundaries */
		rel.DecrementSingleSourceOffset(page_size);
		pages[page_index + 1].relocations[page_offset - page_size] = rel;
	}
}

LEFormat::Page::Relocation::source_type LEFormat::Page::Relocation::GetType(Linker::Relocation& rel)
{
	if(rel.segment_of)
	{
		return Selector16;
	}
	else
	{
		switch(rel.size)
		{
		case 1:
			return Offset8;
		case 2:
			return Offset16;
		case 4:
			return rel.IsRelative() ? Relative32 : Offset32;
		default:
			return (source_type)-1;
		}
	}
}

bool LEFormat::Page::Relocation::IsExternal() const
{
	switch(flags & FlagTypeMask)
	{
	case Internal:
	case Entry: /* TODO: is this internal? */
		return false;
	default:
		return true;
	}
}

bool LEFormat::Page::Relocation::IsSelectorOrOffset() const
{
	switch(type & SourceTypeMask)
	{
	case Selector16:
	case Pointer32:
	case Pointer48:
		return true;
	default:
		return false;
	}
}

bool LEFormat::Page::Relocation::ComesBefore() const
{
	return IsExternal() || IsSelectorOrOffset();
}

size_t LEFormat::Page::Relocation::GetSourceSize() const
{
	switch(type & SourceTypeMask)
	{
	case Offset8:
		return 1;
	case Selector16:
	case Offset16:
		return 2;
	case Pointer32:
	case Offset32:
	case Relative32:
		return 4;
	case Pointer48:
		return 6;
	default:
		Linker::FatalError("Internal error: invalid relocation type");
	}
}

/* do not call this */
void LEFormat::Page::Relocation::DecrementSingleSourceOffset(size_t amount)
{
	assert(sources.size() == 1);
	sources[0] -= amount;
}

bool LEFormat::Page::Relocation::IsSelector() const
{
	return (type & SourceTypeMask) == Selector16;
}

bool LEFormat::Page::Relocation::IsSourceList() const
{
	return type & SourceList;
}

bool LEFormat::Page::Relocation::IsAdditive() const
{
	return flags & Additive;
}

size_t LEFormat::Page::Relocation::GetTargetSize() const
{
	return flags & Target32 ? 4 : 2;
}

size_t LEFormat::Page::Relocation::GetAdditiveSize() const
{
	return flags & Additive32 ? 4 : 2;
}

size_t LEFormat::Page::Relocation::GetModuleSize() const
{
	return flags & Module16 ? 2 : 1;
}

size_t LEFormat::Page::Relocation::GetOrdinalSize() const
{
	return flags & Ordinal8 ? 1 : GetTargetSize();
}

void LEFormat::Page::Relocation::CalculateSizes(compatibility_type compatibility)
{
	if(sources.size() != 1)
		type = (source_type)(type | SourceList);
	if(module > 0xFF)
		flags = (flag_type)(flags | Module16);
	switch(flags & FlagTypeMask)
	{
	case Internal:
		if(!IsSelector() && target > 0xFFFF)
			flags = (flag_type)(flags | Target32);
		break;
	case ImportOrdinal:
		if(target <= 0xFF && compatibility != CompatibleWatcom)
			flags = (flag_type)(flags | Ordinal8);
		else if(target > 0xFFFF)
			flags = (flag_type)(flags | Target32);
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = (flag_type)(flags | Additive32);
		break;
	case ImportName:
		if(target > 0xFFFF)
			flags = (flag_type)(flags | Target32);
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = (flag_type)(flags | Additive32);
		break;
	case Entry:
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = (flag_type)(flags | Additive32);
		break;
	}
}

size_t LEFormat::Page::Relocation::GetSize() const
{
	//CalculateSizes();
	size_t size = 2 + 2 * sources.size();
	if(IsSourceList())
	{
		/* it is
			count ... list of sources
		instead of
			source
		*/
		size += 1;
	}
	switch(flags & FlagTypeMask)
	{
	case Internal:
		size += GetModuleSize();
		if(!IsSelector())
			size += GetTargetSize();
		break;
	case ImportOrdinal:
		size += GetModuleSize() + GetTargetSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	case ImportName:
		size += GetModuleSize() + GetOrdinalSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	case Entry:
		size += GetModuleSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	}
	return size;
}

void LEFormat::Page::Relocation::WriteFile(Linker::Writer& wr, compatibility_type compatibility)
{
	CalculateSizes(compatibility);
	wr.WriteWord(1, type);
	wr.WriteWord(1, flags);
	if(IsSourceList())
	{
		wr.WriteWord(1, sources.size());
	}
	else
	{
		assert(sources.size() == 1);
		wr.WriteWord(2, sources[0]);
	}
	switch(flags & FlagTypeMask)
	{
	case Internal:
		wr.WriteWord(GetModuleSize(), module);
		if(!IsSelector())
			wr.WriteWord(GetTargetSize(), target);
		break;
	case ImportOrdinal:
		wr.WriteWord(GetModuleSize(), module);
		wr.WriteWord(GetTargetSize(), target);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	case ImportName:
		wr.WriteWord(GetModuleSize(), module);
		wr.WriteWord(GetOrdinalSize(), target);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	case Entry:
		wr.WriteWord(GetModuleSize(), module);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	}
	if(IsSourceList())
	{
		for(uint16_t source : sources)
			wr.WriteWord(2, source);
	}
}

LEFormat::Page LEFormat::Page::LEPage(uint16_t fixup_table_index, uint8_t type)
{
	return Page(fixup_table_index, type);
}

LEFormat::Page LEFormat::Page::LXPage(uint32_t offset, uint16_t size, uint8_t flags)
{
	return Page(offset, size, flags);
}

bool LEFormat::Entry::SameBundle(const Entry& other) const
{
	if(type != other.type)
		return false;
	switch(type)
	{
	case Unused:
	case Forwarder:
		return true;
	case Entry16:
	case CallGate286:
	case Entry32:
		return object == other.object;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

offset_t LEFormat::Entry::GetEntryHeadSize() const
{
	switch(type)
	{
	case Unused:
		return 2;
	case Entry16:
	case CallGate286:
	case Entry32:
		return 4;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

offset_t LEFormat::Entry::GetEntryBodySize() const
{
	switch(type)
	{
	case Unused:
		return 0;
	case Entry16:
		return 3;
	case CallGate286:
	case Entry32:
		return 5;
	case Forwarder:
		return 7;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

void LEFormat::Entry::WriteEntryHead(Linker::Writer& wr)
{
	wr.WriteWord(1, type);
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
	case CallGate286:
	case Entry32:
		wr.WriteWord(2, object);
		break;
	case Forwarder:
		wr.WriteWord(2, 0); /* reserved */
		break;
	}
}

void LEFormat::Entry::WriteEntryBody(Linker::Writer& wr)
{
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		break;
	case CallGate286:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		wr.WriteWord(2, 0); /* reserved - call gate */
		break;
	case Entry32:
		wr.WriteWord(1, flags);
		wr.WriteWord(4, offset);
		break;
	case Forwarder:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, object); /* module */
		wr.WriteWord(4, offset); /* ordinal or name */
		break;
	}
}

unsigned LEFormat::GetDefaultObjectFlags() const
{
	if(extended_format)
		return 0;
	else
		return Object::PreloadPages;
}

void LEFormat::AddObject(const Object& object)
{
	objects.push_back(object);
	object_index[objects.back().image] = objects.size() - 1;
}

uint16_t LEFormat::FetchImportedModuleName(std::string name)
{
	auto it = std::find(imported_modules.begin(), imported_modules.end(), name);
	if(it == imported_modules.end())
	{
		uint16_t offset = imported_modules.size() + 1;
		Linker::Debug << "Debug: New imported module name: " << name << " = " << offset << std::endl;
		imported_modules.push_back(name);
		return offset;
	}
	else
	{
		return it - imported_modules.begin() + 1;
	}
}

uint16_t LEFormat::FetchImportedProcedureName(std::string name)
{
	auto it = imported_procedure_name_offsets.find(name);
	if(it == imported_procedure_name_offsets.end())
	{
		uint16_t offset = imported_procedure_names_length;
		Linker::Debug << "Debug: New imported procedure name: " << name << " = " << offset << std::endl;
		imported_procedures.push_back(name);
		imported_procedure_name_offsets[name] = offset;
		imported_procedure_names_length += 1 + name.size();
		return offset;
	}
	else
	{
//Linker::Debug << "Debug: Procedure " << name << " is " << it->second << std::endl;
		return it->second;
	}
}

uint16_t LEFormat::MakeEntry(Linker::Position value)
{
	uint16_t index;
	for(index = 0; index < entries.size(); index ++)
	{
		if(entries[index].type == Entry::Unused)
		{
			break;
		}
	}
	return MakeEntry(index, value);
}

uint16_t LEFormat::MakeEntry(uint16_t index, Linker::Position value)
{
	if(index >= entries.size())
	{
		entries.resize(index + 1);
	}
	if(entries[index].type != Entry::Unused)
	{
		Linker::Error << "Error: entry #" << (index + 1) << " is redefined, ignoring" << std::endl;
		return index;
	}
	uint16_t object = object_index[value.segment];
	/* TODO: other flags? */
	entries[index] = Entry(Entry::Entry32, object + 1, Entry::Exported, value.address - value.segment->base_address); /* TODO: 16-bit */
	return index;
}

uint8_t LEFormat::CountBundles(size_t entry_index)
{
	size_t entry_count;
	for(entry_count = 1; entry_count < 0xFF && entry_index + entry_count < entries.size(); entry_count++)
	{
		if(!entries[entry_index].SameBundle(entries[entry_index + entry_count]))
		{
			break;
		}
	}
	return entry_count;
}

void LEFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub_file = FetchOption(options, "stub", "");
	/* TODO */
}

void LEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	/* TODO: stack, heap */

	if(segment->name == ".heap")
	{
		heap = segment;
	}
	else if(segment->name == ".stack")
	{
		stack = segment;
	}
	else
	{
		std::shared_ptr<Linker::Section> section = segment->sections[0];
		unsigned flags = GetDefaultObjectFlags() | Object::BigSegment;
		if(section->IsReadable())
			flags |= Object::Readable;
		if(section->IsWritable())
			flags |= Object::Writable;
		if(section->IsExecable())
			flags |= Object::Execable;
		AddObject(Object(segment, flags));

		if(segment->name == ".data")
			automatic_data = objects.size();
	}
}

std::unique_ptr<Script::List> LEFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?base_address?;
	all ".code" or ".text" or ".rodata" or ".eh_frame";
};

".data"
{
	at align(here, ?align?);
	all ".data";
	all ".bss" or ".comm";
};

for ".heap"
{
	at align(here, ?align?);
	all any;
};

for not ".stack"
{
	at align(here, ?align?);
	all any;
};
)";

	if(linker_script != "")
	{
		return LinkerManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void LEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void LEFormat::ProcessModule(Linker::Module& module)
{
	if(system == Windows386)
	{
		linker_parameters["align"] = Linker::Location(0x1000);
		linker_parameters["base_address"] = Linker::Location();
	}
	else
	{
		linker_parameters["align"] = Linker::Location(0x10000);
		linker_parameters["base_address"] = Linker::Location(0x10000);
	}

	Link(module);

	resident_names.push_back(Name{module_name, 0});
	FetchImportedProcedureName("");

	offset_t page_offset = 0;
	pages.push_back(Page());
	for(Object& object : objects)
	{
		object.page_table_index = pages.size();
//		Linker::Debug << "Debug: " << object.image->data_size << " / " << page_size << std::endl;
		object.page_entry_count = ::AlignTo(object.image->data_size, page_size) / page_size;

		for(size_t i = 0; i < object.page_entry_count; i++)
		{
			if(extended_format)
			{
				uint32_t current_page_size;
				if(i == object.page_entry_count - 1)
				{
					current_page_size = object.image->data_size & (page_size - 1);
					if(current_page_size == 0)
						current_page_size = page_size;
				}
				else
				{
					current_page_size = page_size;
				}
				pages.push_back(Page::LXPage(page_offset, current_page_size, Page::Preload));
				page_offset += current_page_size;
			}
			else
			{
				uint32_t fixup_table_index = pages.size();
				pages.push_back(Page::LEPage(fixup_table_index, 0)); //Page::Relocations));
				/* TODO: change fixup_table_index to 0 unless a relocation is present */
				/* Idea: set fixup_table_index after going through all relocations */
			}
		}
	}

	pages.push_back(Page());

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string library;
		if(symbol.LoadLibraryName(library))
		{
			FetchImportedModuleName(library);
		}
	}

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string name;
		if(symbol.LoadName(name))
		{
			FetchImportedProcedureName(name);
		}
	}

	/* first make entries for those symbols exported by ordinals, or those that have hints */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(it.first.LoadOrdinalOrHint(ordinal))
		{
			MakeEntry(ordinal - 1, it.second.GetPosition());
			if(it.first.IsExportedByOrdinal())
			{
				std::string internal_name = "";
				it.first.LoadName(internal_name);
				nonresident_names.push_back(Name{internal_name, ordinal});
			}
		}
	}

	/* then make entries for those symbols exported by name */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(!it.first.LoadOrdinalOrHint(ordinal))
		{
			ordinal = MakeEntry(it.second.GetPosition()) + 1;
		}
		if(!it.first.IsExportedByOrdinal())
		{
			std::string exported_name = "";
			it.first.LoadName(exported_name);
			resident_names.push_back(Name{exported_name, ordinal});
		}
	}

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(rel.Resolve(module, resolution))
		{
			if(resolution.target != nullptr)
			{
				if(resolution.reference != nullptr)
				{
					Linker::Error << "Error: intersegment relocations not supported, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				}
				Linker::Position position = rel.source.GetPosition();
				Object& source_object = objects[object_index[position.segment]];
				uint8_t target_object = object_index[resolution.target] + 1;
				int type = Page::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				if(type == Page::Relocation::Selector16)
				{
					/* Segment relocation to internal reference 0:0 */
					AddRelocation(source_object, type, Page::Relocation::Internal, position.address, target_object);
					rel.WriteWord(0);
					continue;
				}
				else if(!(IsOS2() && !IsLibrary()))
				{
					AddRelocation(source_object, type, Page::Relocation::Internal,
							position.address, target_object, resolution.value - resolution.target->base_address);
					if(IsOS2())
						rel.WriteWord(resolution.value);
					else
						rel.WriteWord(resolution.value - resolution.target->base_address);
				}
				else
				{
					/* OS/2 application */
					rel.WriteWord(resolution.value);
				}
				/* TODO: if not additive? */
			}
			else
			{
				rel.WriteWord(resolution.value);
			}
		}
		else
		{
			if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
			{
				Linker::Position position = rel.source.GetPosition();
				Object& source_object = objects[object_index[position.segment]];

				std::string library, name;
				uint16_t ordinal;
				int type = Page::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				else if(type == Page::Relocation::Selector16)
				{
					/* Segment relocation to internal reference 0:0 */
					AddRelocation(source_object, type, Page::Relocation::Internal, position.address, 1);
					rel.WriteWord(0);
					continue;
				}
				if(symbol->GetImportedName(library, name))
				{
					AddRelocation(source_object, type, Page::Relocation::ImportName,
						position.address, FetchImportedModuleName(library), FetchImportedProcedureName(name));
					rel.WriteWord(rel.IsRelative() ? 0 : resolution.value);
					continue;
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					AddRelocation(source_object, type, Page::Relocation::ImportOrdinal,
						position.address, FetchImportedModuleName(library), ordinal);
					rel.WriteWord(rel.IsRelative() ? 0 : resolution.value);
					continue;
				}
			}
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
	}

	/* TODO: allocate stack instead */
	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		esp_object = object_index[position.segment] + 1;
		esp_value = position.address - position.segment->base_address;
	}
	else if(automatic_data != 0 && module.FindSection(".stack") != nullptr && module.FindSection(".stack")->Size() != 0)
	{
		esp_object = automatic_data;
		esp_value = objects[automatic_data - 1].image->TotalSize();
	}
	else if(automatic_data != 0 && system == OS2 && !IsLibrary())
	{
		if(system == OS2 && !IsLibrary())
			objects[automatic_data - 1].image->zero_fill += 0x1000; /* TODO: make into a parameter */
		esp_object = automatic_data;
		esp_value = objects[automatic_data - 1].image->TotalSize();
	}
	else
	{
		/* top of automatic data segment */
		esp_object = automatic_data;
		esp_value = 0;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		eip_object = object_index[position.segment] + 1;
		eip_value = position.address - position.segment->base_address;
	}
	else
	{
		eip_object = 1;
		eip_value = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void LEFormat::CalculateValues()
{
	/* TODO: where does the stack value come from? */
	switch(system)
	{
	case OS2:
		stack_size = 0x1000;
		heap_size = 0;
		break;
	case Windows386:
		stack_size = 0;
		heap_size = 0;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: " << system;
			Linker::FatalError(message.str());
		}
	}

	if(IsLibrary() || IsDriver())
		stack_size = esp_object = esp_value = 0;

	/* TODO */

	if(extended_format)
	{
		page_offset_shift = 0;
	}
	else
	{
		last_page_size = objects.back().image->data_size & (page_size - 1);
#if 0
		for(Object& object : objects)
		{
			Linker::Debug << "Debug: " << object.image->name << " -> " << object.image->data_size << std::endl;
		}
#endif
	}

	file_offset = GetStubImageSize();

	object_table_offset = file_offset + 0xC4;

	object_page_table_offset = object_table_offset + 0x18 * objects.size();
	object_iterated_pages_offset = 0;

	resource_table_offset = object_page_table_offset + (extended_format ? 8 : 4) * (pages.size() - 2);
	resource_table_entry_count = 0; /* TODO: resources */
	uint32_t resource_table_size = 0; /* TODO */
	resident_name_table_offset = resource_table_offset + resource_table_size;
	uint32_t resident_name_table_length = 1;
	for(Name& name : resident_names)
	{
		resident_name_table_length += 3 + name.name.size();
	}
	entry_table_offset = resident_name_table_offset + resident_name_table_length;
	uint32_t entry_table_length = 1; /* terminating zero */
//offset_t _ = entry_table_length;
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		entry_table_length += entries[entry_index].GetEntryHeadSize() + entry_count * entries[entry_index].GetEntryBodySize();
//Linker::Debug << "Debug: Count " << entry_table_length - _ << std::endl; _ = entry_table_length;
		entry_index += entry_count;
	}

	fixup_page_table_offset = entry_table_offset + entry_table_length;
	fixup_record_table_offset = fixup_page_table_offset + 4 * (pages.size() - 1); /* including terminator entry (first entry fake because pages are 1 based) */

	offset_t fixup_offset = 0;
	for(Page& page : pages)
	{
		if(&page == &pages.front())
			continue;
		page.fixup_offset = fixup_offset;
		if(&page == &pages.back())
			continue;
//Linker::Debug << "Debug: Expect page " << page.relocations.size() << std::endl;
		for(auto it : page.relocations)
		{
			it.second.CalculateSizes(compatibility);
			fixup_offset += it.second.GetSize();
//Linker::Debug << "Debug: Expect " << it.second.GetSize() << std::endl;
		}
	}

	imported_module_table_offset = fixup_record_table_offset + fixup_offset;

	uint32_t imported_module_table_length = 0;
	for(std::string& name : imported_modules)
	{
		imported_module_table_length += 1 + name.size();
	}

	imported_procedure_table_offset = imported_module_table_offset + imported_module_table_length;

	uint32_t imported_procedure_table_length = 0;
	for(std::string& name : imported_procedures)
	{
		imported_procedure_table_length += 1 + name.size();
	}

	uint32_t imported_procedure_table_end = imported_procedure_table_offset + imported_procedure_table_length;

	loader_section_size = fixup_page_table_offset - object_table_offset;
	fixup_section_size = imported_procedure_table_end - fixup_page_table_offset;

	data_pages_offset = imported_procedure_table_end;
	if(compatibility == CompatibleWatcom && (page_offset_shift < 2 || !extended_format))
	{
		data_pages_offset = ::AlignTo(data_pages_offset, 4); /* TODO: Watcom? */
	}
	else if(extended_format)
	{
		data_pages_offset = ::AlignTo(data_pages_offset, 1 << page_offset_shift);
	}

	uint32_t pages_offset = data_pages_offset;
	for(Object& object : objects)
	{
		if(!extended_format)
		{
			pages_offset = ::AlignTo(pages_offset - data_pages_offset, page_size) + data_pages_offset;
		}
		object.data_pages_offset = pages_offset;
		pages_offset += object.image->data_size;
	}

	if(nonresident_names.size() == 0)
	{
		nonresident_name_table_offset = 0;
		nonresident_name_table_size = 0;
	}
	else
	{
		nonresident_name_table_offset = pages_offset;
		nonresident_name_table_size = 1;
		for(Name& name : nonresident_names)
		{
			nonresident_name_table_size += 3 + name.name.size();
		}
	}
}

void LEFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = endiantype;
	WriteStubImage(wr);

	/* new header */
	wr.Seek(file_offset);
	wr.WriteData(2, extended_format ? "LX" : "LE");
	switch(endiantype)
	{
	case ::LittleEndian:
		wr.WriteData(2, "\0\0");
		break;
	case ::BigEndian:
		wr.WriteData(2, "\1\1");
		break;
	case ::PDP11Endian:
		wr.WriteData(2, "\0\1");
		break;
	case ::AntiPDP11Endian:
		wr.WriteData(2, "\1\0");
		break;
	default:
		Linker::FatalError("Internal error: invalid endianness");
	}
	wr.WriteWord(4, 0); /* format level */
	wr.WriteWord(2, cpu);
	wr.WriteWord(2, system);
	wr.WriteWord(4, 0); /* module version */
	wr.WriteWord(4, module_flags);
	wr.WriteWord(4, pages.size() - 2); /* page 0 is fake, final page is only used in the page table */
	wr.WriteWord(4, eip_object);
	wr.WriteWord(4, eip_value);
	wr.WriteWord(4, esp_object);
	wr.WriteWord(4, esp_value);
	wr.WriteWord(4, page_size); /* page size */
	wr.WriteWord(4, page_offset_shift); /* or size of last page */
	wr.WriteWord(4, fixup_section_size);
	wr.WriteWord(4, 0); /* fixup section checksum */
	wr.WriteWord(4, loader_section_size);
	wr.WriteWord(4, 0); /* loader section checksum */
	wr.WriteWord(4, object_table_offset - file_offset);
	wr.WriteWord(4, objects.size());
	wr.WriteWord(4, object_page_table_offset - file_offset);
	wr.WriteWord(4, object_iterated_pages_offset != 0 ? object_iterated_pages_offset - file_offset : 0);
	wr.WriteWord(4, resource_table_offset - file_offset);
	wr.WriteWord(4, resource_table_entry_count);
	wr.WriteWord(4, resident_name_table_offset - file_offset);
	wr.WriteWord(4, entry_table_offset - file_offset);
	wr.WriteWord(4, 0); /* module directives offset */
	wr.WriteWord(4, 0); /* module directives entries */
	wr.WriteWord(4, fixup_page_table_offset - file_offset);
	wr.WriteWord(4, fixup_record_table_offset - file_offset);
	wr.WriteWord(4, imported_module_table_offset - file_offset);
	wr.WriteWord(4, imported_modules.size());
	wr.WriteWord(4, imported_procedure_table_offset - file_offset);
	wr.WriteWord(4, 0); /* per-page checksum offset */
	wr.WriteWord(4, data_pages_offset);
	wr.WriteWord(4, 0); /* preload page count */
	wr.WriteWord(4, nonresident_name_table_offset);
	wr.WriteWord(4, nonresident_name_table_size);
	wr.WriteWord(4, 0); /* non-resident name table checksum */
	wr.WriteWord(4, automatic_data);
	wr.WriteWord(4, 0); /* debug info offset */
	wr.WriteWord(4, 0); /* debug info size */
	wr.WriteWord(4, 0); /* instance preload page count */
	wr.WriteWord(4, 0); /* instance demand page count */
	wr.WriteWord(4, heap_size);
	wr.WriteWord(4, stack_size);

	if(system == Windows386 && compatibility == CompatibleWatcom)
	{
		wr.Seek(file_offset + 0xC0);
		wr.WriteWord(4, 0x40000); /* TODO: Watcom? */
	}
	wr.Seek(file_offset + 0xC4);

	/*** Loader Section ***/
	/* Object Table */
	assert(wr.Tell() == object_table_offset);
	for(Object& object : objects)
	{
		wr.WriteWord(4, object.image->TotalSize());
		wr.WriteWord(4, object.image->base_address);
		wr.WriteWord(4, object.flags);
		wr.WriteWord(4, object.page_table_index);
		wr.WriteWord(4, object.page_entry_count);
		wr.WriteWord(4, 0);
	}

	/* Object Page Table */
	assert(wr.Tell() == object_page_table_offset);
	if(extended_format)
	{
		for(Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(4, page.lx.offset >> page_offset_shift);
			wr.WriteWord(2, page.lx.size);
			wr.WriteWord(2, page.lx.flags);
		}
	}
	else
	{
		for(Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(3, page.le.fixup_table_index, ::BigEndian);
			wr.WriteWord(1, page.le.type);
		}
	}

	/* Resource Table */
	assert(wr.Tell() == resource_table_offset);
	/* TODO */

	/* Resource Name Table */
	/* TODO */

	/* Resident Name Table */
	assert(wr.Tell() == resident_name_table_offset);
	for(Name& name : resident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name);
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	/* Entry Table */
	assert(wr.Tell() == entry_table_offset);
//offset_t _ = wr.Tell();
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		wr.WriteWord(1, entry_count);
		entries[entry_index].WriteEntryHead(wr);
		for(size_t entry_offset = 0; entry_offset < entry_count; entry_offset ++)
		{
			entries[entry_index + entry_offset].WriteEntryBody(wr);
		}
		entry_index += entry_count;
//Linker::Debug << "Debug: Write " << wr.Tell() - _ << std::endl; _ = Tell();
	}
	wr.WriteWord(1, 0);

	/* Module Format Directives Table */
	/* Resident Directives Table */
	/* Per-page Checksum (LX) */

	/*** Fixup Section ***/
	/* Fixup Page Table */
	assert(wr.Tell() == fixup_page_table_offset);
	for(Page& page : pages)
	{
		if(&page == &pages.front())
			continue;
		wr.WriteWord(4, page.fixup_offset);
	}

	/* Fixup Record Table */
	assert(wr.Tell() == fixup_record_table_offset);
//size_t _ = wr.Tell();
	for(Page& page : pages)
	{
		if(&page == &pages.front() || &page == &pages.back())
			continue;
//Linker::Debug << "Debug: Receive page (before) " << page.relocations.size() << std::endl;
		for(auto& it : page.relocations)
		{
			if(it.second.ComesBefore())
				it.second.WriteFile(wr, compatibility);
//Linker::Debug << "Debug: Receive " << wr.Tell() - _ << std::endl; _ = Tell();
		}
//Linker::Debug << "Debug: Receive page (after) " << page.relocations.size() << std::endl;
		for(auto& it : page.relocations)
		{
			if(!it.second.ComesBefore())
				it.second.WriteFile(wr, compatibility);
//Linker::Debug << "Debug: Receive " << wr.Tell() - _ << std::endl; _ = Tell();
		}
	}

	/* Import Module Name Table */
	assert(wr.Tell() == imported_module_table_offset);
	for(std::string& name : imported_modules)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name);
	}

	/* Import Procedure Name Table */
	assert(wr.Tell() == imported_procedure_table_offset);
	for(std::string& name : imported_procedures)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name);
	}

	/* Per-page Checksum (LE) */

	/*** Segment Data ***/
	for(Object& object : objects)
	{
		wr.Seek(object.data_pages_offset);
		object.image->WriteFile(wr);
#if 0
		/* TODO: is this still needed? */
		if(!extended_format && &object != &objects.back())
		{
			/* TODO: fill in */
		}
#endif
	}

	/*** Non-Resident ***/
	/* Non-Resident Name Table */
	for(Name& name : nonresident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name);
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);
}

void LEFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	program_name = filename;
	size_t ix = filename.rfind('.');
	if(ix != std::string::npos)
		module_name = filename.substr(0, ix);
	else
		module_name = filename;
	if(module.cpu != Linker::Module::I386)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 80386 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string LEFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

