
#include "neexe.h"
#include "mzexe.h"

using namespace Microsoft;

void NEFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

bool NEFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool NEFormat::FormatIs16bit() const
{
	return true;
}

bool NEFormat::FormatSupportsLibraries() const
{
	return true;
}

unsigned NEFormat::FormatAdditionalSectionFlags(std::string section_name) const
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

bool NEFormat::IsLibrary() const
{
	return application_flags & LIBRARY;
}

NEFormat * NEFormat::SimulateLinker(compatibility_type compatibility)
{
	this->compatibility = compatibility;
	switch(compatibility)
	{
	case CompatibleNone:
		break;
	case CompatibleWatcom:
		linker_version = version{5, 1};
		break;
	case CompatibleMicrosoft:
		break;
	case CompatibleGNU:
		break;
	/* TODO: others */
	}
	return this;
}

NEFormat * NEFormat::CreateConsoleApplication(system_type system)
{
	switch(system)
	{
	case OS2:
	case Windows:
		return new NEFormat(system, MULTIPLEDATA, GUI_AWARE);
	case MSDOS4:
		return new NEFormat(system, MULTIPLEDATA | GLOBAL_INITIALIZATION, 0);
	default:
		assert(false);
	}
}

NEFormat * NEFormat::CreateGUIApplication(system_type system)
{
	switch(system)
	{
	case OS2:
		return new NEFormat(system, MULTIPLEDATA, GUI);
	case Windows:
		return new NEFormat(system, MULTIPLEDATA, GUI_AWARE);
	default:
		assert(false);
	}
}

NEFormat * NEFormat::CreateLibraryModule(system_type system)
{
	switch(system)
	{
	case OS2:
	case Windows:
		return new NEFormat(system, SINGLEDATA, GUI_AWARE | LIBRARY);
	case MSDOS4:
		return new NEFormat(system, SINGLEDATA | GLOBAL_INITIALIZATION, LIBRARY);
	default:
		assert(false);
	}
}

offset_t NEFormat::Entry::GetEntrySize() const
{
	/* Note: a bundle has at least 2 more bytes */
	switch(type)
	{
	case Unused:
		return 0;
	case Fixed:
		return 3;
	case Movable:
		return 6;
	default:
		assert(false);
	}
}

uint8_t NEFormat::Entry::GetIndicatorByte() const
{
	switch(type)
	{
	case Unused:
		return 0x00;
	case Fixed:
		return segment;
	case Movable:
		return 0xFF;
	default:
		assert(false);
	}
}

void NEFormat::Entry::WriteEntry(Linker::Writer& wr)
{
	switch(type)
	{
	case Unused:
		break;
	case Fixed:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		break;
	case Movable:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, INT_3Fh);
		wr.WriteWord(1, segment);
		wr.WriteWord(2, offset);
		break;
	}
}

NEFormat::Segment::Relocation::source_type NEFormat::Segment::Relocation::GetType(Linker::Relocation& rel)
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
			return Offset32;
		default:
			return (source_type)-1;
		}
	}
}

void NEFormat::Segment::AddRelocation(const Relocation& rel)
{
	relocations[rel.offset] = rel;
}

unsigned NEFormat::GetCodeSegmentFlags() const
{
	switch(system)
	{
	case OS2:
		return 3 << Segment::PrivilegeLevelShift;
	case Windows:
		if(IsLibrary())
			return Segment::Movable | Segment::Shareable | Segment::Preload | (3 << Segment::PrivilegeLevelShift);
		else
			return Segment::Preload | (3 << Segment::PrivilegeLevelShift);
	case MSDOS4:
		return Segment::Movable | Segment::Shareable | Segment::Preload | (0 << Segment::PrivilegeLevelShift);
	default:
		assert(false);
	}
}

unsigned NEFormat::GetDataSegmentFlags() const
{
	switch(system)
	{
	case MSDOS4:
		if(IsLibrary())
			return Segment::Movable | Segment::Data | Segment::Preload | (0 << Segment::PrivilegeLevelShift);
		else
			return Segment::Data | (0 << Segment::PrivilegeLevelShift);
	default:
		return GetCodeSegmentFlags() | Segment::Data;
	}
}

void NEFormat::AddSegment(const Segment& segment)
{
	segments.push_back(segment);
	segment_index[segments.back().image] = segments.size() - 1;
}

uint16_t NEFormat::FetchModule(std::string name)
{
	auto it = module_reference_offsets.find(name);
	if(it == module_reference_offsets.end())
	{
		uint16_t module = FetchImportedName(name);
		module_references.push_back(module);
		return module_reference_offsets[name] = module_references.size() - 1;
	}
	else
	{
		return it->second;
	}
}

uint16_t NEFormat::FetchImportedName(std::string name)
{
	auto it = imported_name_offsets.find(name);
	if(it == imported_name_offsets.end())
	{
		uint16_t offset = imported_names_length;
		Linker::Debug << "Debug: New imported name: " << name << " = " << offset << std::endl;
		imported_names.push_back(name);
		imported_name_offsets[name] = offset;
		imported_names_length += 1 + name.size();
		return offset;
	}
	else
	{
		return it->second;
	}
}

std::string NEFormat::MakeProcedureName(std::string name)
{
	if(option_capitalize_names)
	{
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
	}
	return name;
}

uint16_t NEFormat::MakeEntry(Linker::Position value)
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

uint16_t NEFormat::MakeEntry(uint16_t index, Linker::Position value)
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
	uint16_t segment = segment_index[value.segment];
	/* TODO: other flags? */
	if((segments[segment].flags & Segment::Movable))
	{
		entries[index] = Entry(Entry::Movable, segment + 1, Entry::Exported | Entry::SharedData, value.address);
	}
	else
	{
		entries[index] = Entry(Entry::Fixed, segment + 1, Entry::Exported, value.address);
	}
	return index;
}

uint8_t NEFormat::CountBundles(size_t entry_index)
{
	size_t entry_count;
	for(entry_count = 1; entry_count < 0xFF && entry_index + entry_count < entries.size(); entry_count++)
	{
		if(entries[entry_index].type != entries[entry_index + entry_count].type)
		{
			break;
		}
		if(entries[entry_index].type == Entry::Fixed && entries[entry_index].segment != entries[entry_index + entry_count].segment)
		{
			break;
		}
	}
	return entry_count;
}

void NEFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub_file = FetchOption(options, "stub", "");

	switch(system & ~PharLap)
	{
	case Windows:
		option_capitalize_names = true;
		break;
	case OS2:
	default: /* TODO: other systems? */
		option_capitalize_names = false;
		break;
	}
	/* TODO */
}

void NEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	/* TODO: stack, heap */

	if(segment->name == ".heap")
	{
		//heap = segment;
	}
	else if(segment->name == ".stack")
	{
		stack = segment;
	}
	else
	{
Linker::Debug << "Debug: New segment " << segment->name << std::endl;
		AddSegment(Segment(segment, segment->sections[0]->IsExecable() ? GetCodeSegmentFlags() : GetDataSegmentFlags()));

		if(segment->name == ".data")
			automatic_data = segments.size();
	}
}

std::unique_ptr<Script::List> NEFormat::GetScript(Linker::Module& module)
{
	static const char SmallScript[] = R"(
".code"
{
	all exec;
};

".data"
{
	at 0;
	all not zero;
	all not ".stack";
};

".stack"
{
	all;
};
)";

	static const char SmallScript_msdos4[] = R"(
".code"
{
	all exec;
};

".data"
{
	at 0;
	all not zero;
	all not ".stack";
	all;
};
)";

	/* TODO: stack, heap */
	static const char LargeScript[] = R"(
".code"
{
	all ".code" or ".text";
};

".data"
{
	at 0;
	all ".data" or ".rodata";
	all ".bss" or ".comm";
	all ".stack";
};

for not ".heap"
{
	at 0;
	all any and not zero;
	all any;
};
)";

	if(linker_script != "")
	{
		return LinkerManager::GetScript(module);
	}
	else
	{
		switch(memory_model)
		{
		case MODEL_SMALL:
			if(system == MSDOS4)
			{
				return Script::parse_string(SmallScript_msdos4);
			}
			else
			{
				return Script::parse_string(SmallScript);
			}
		case MODEL_LARGE:
			/* TODO: this is MSDOS4 only */
			return Script::parse_string(LargeScript);
		}
	}
	assert(false);
}

void NEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void NEFormat::ProcessModule(Linker::Module& module)
{
	sector_shift = 1; /* TODO: parametrize */
	for(auto& section : module.Sections())
	{
		section->RealignEnd(1 << sector_shift); /* TODO: this is probably what Watcom does */
	}

	Link(module);

	resident_names.push_back(Name{module_name, 0});
	FetchImportedName("");
	nonresident_names.push_back(Name{program_name, 0});

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string name;
		if(symbol.LoadName(name))
		{
			FetchImportedName(MakeProcedureName(name));
		}
	}

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string library;
		if(symbol.LoadLibraryName(library))
		{
			FetchModule(library);
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
				nonresident_names.push_back(Name{MakeProcedureName(internal_name), ordinal});
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
			resident_names.push_back(Name{MakeProcedureName(exported_name), ordinal});
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
					Linker::Error << rel << std::endl;
					continue;
				}
				Linker::Position position = rel.source.GetPosition();
				Segment& source_segment = segments[segment_index[position.segment]];
				uint8_t target_segment = segment_index[resolution.target];
				Segment& target_segment_object = segments[target_segment];
				target_segment += 1;
				int type = Segment::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				else if((target_segment_object.flags & Segment::Movable))
				{
					/* references to movable segments require a new entry point */
					assert(resolution.value == 0); /* TODO: target != 0 ? */
					if(target_segment_object.movable_entry_index == 0)
					{
						entries.push_back(Entry(Entry::Movable, target_segment, 0, 0));
						target_segment_object.movable_entry_index = entries.size();
					}
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::Internal,
							position.address, 0xFF, target_segment_object.movable_entry_index)
					);
					rel.WriteWord(0xFFFFFFFF);
				}
				else
				{
					//assert(resolution.value == 0); /* TODO: target != 0 ? */
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::Internal,
							position.address, target_segment, resolution.value)
					);
					rel.WriteWord(0xFFFFFFFF);
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
				Segment& source_segment = segments[segment_index[position.segment]];

				std::string library, name;
				uint16_t ordinal;
				int type = Segment::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				if(symbol->GetImportedName(library, name))
				{
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::ImportName,
							position.address, FetchModule(library) + 1, FetchImportedName(MakeProcedureName(name)))
					);
					rel.WriteWord(0xFFFFFFFF);
					/* TODO: if not additive? */
					continue;
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::ImportOrdinal,
							position.address, FetchModule(library) + 1, ordinal)
					);
					rel.WriteWord(0xFFFFFFFF);
					/* TODO: if not additive? */
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
		sp = position.address;
		ss = segment_index[position.segment] + 1;
	}
	else if(system != MSDOS4)
	{
		/* top of automatic data segment */
		sp = 0;
		ss = automatic_data;
	}
	else
	{
		/* Multitasking MS-DOS 4 */
		if(automatic_data == 0)
		{
			sp = 0;
			ss = 0;
		}
		else
		{
			sp = segments[automatic_data - 1].image->TotalSize();
			ss = automatic_data;
//			Linker::Debug << "Debug: End of memory: " << sp << std::endl;
//			Linker::Debug << "Debug: Total size: " << image.TotalSize() << std::endl;
//			Linker::Debug << "Debug: Stack base: " << ss << std::endl;
//			if(!(stack->GetFlags() & Linker::Section::Stack))
//			{
//				Linker::Warning << "Warning: no stack top specified, using end of .bss segment" << std::endl;
//			}
		}
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		ip = position.address;
		cs = segment_index[position.segment] + 1;
	}
	else
	{
		ip = 0;
		cs = 1;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void NEFormat::CalculateValues()
{
	offset_t current_offset;

	if((program_flags & (SINGLEDATA|MULTIPLEDATA)) == NODATA)
		automatic_data = 0;

	/* TODO: should be a parameter */
	switch(system)
	{
	case MSDOS4:
		stack_size = 0;
		heap_size = 0;
		windows_version.major = 0;
		windows_version.minor = 0;
		break;
	case OS2:
		stack_size = 0x1000;
		heap_size = 0;
		windows_version.major = 0;
		windows_version.minor = 0;
		break;
	case Windows:
		stack_size = 0x2000;
		heap_size = 0x400;
		windows_version.major = 3;
		windows_version.minor = 0;
		break;
	default:
		assert(false);
	}

	if(ss != automatic_data)
		stack_size = 0; /* no initial stack possible */

	if(IsLibrary())
		stack_size = ss = sp = 0;

	file_offset = GetStubImageSize();

	segment_table_offset = file_offset + 0x40;

	resource_table_offset = segment_table_offset + 8 * segments.size();

	resident_name_table_offset = resource_table_offset + 0; /* TODO: resource table */
	current_offset = resident_name_table_offset + 1;
	for(Name& name : resident_names)
	{
		current_offset += 3 + name.name.size();
	}

	module_reference_table_offset = current_offset;

	current_offset = imported_names_table_offset = module_reference_table_offset + 2 * module_references.size();
	for(std::string& name : imported_names)
	{
		current_offset += 1 + name.size();
	}

	entry_table_offset = current_offset;
	entry_table_length = 2;
	movable_entry_count = 0;

	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		entry_table_length += 2 + entry_count * entries[entry_index].GetEntrySize();
		if(entries[entry_index].type == Entry::Movable)
			movable_entry_count += entry_count;
		entry_index += entry_count;
	}

	nonresident_name_table_offset = entry_table_offset + entry_table_length;
	nonresident_name_table_length = 1;
	for(Name& name : nonresident_names)
	{
		nonresident_name_table_length += 3 + name.name.size();
	}

	current_offset = nonresident_name_table_offset + nonresident_name_table_length;

	for(Segment& segment : segments)
	{
		segment.data_offset = ::AlignTo(current_offset, 1 << sector_shift);
		current_offset = segment.data_offset + segment.image->data_size;
		if(segment.relocations.size() != 0)
		{
			segment.flags = (Segment::flag_type)(segment.flags | Segment::Relocations);
			current_offset += 2 + 8 * segment.relocations.size();
		}
	}

	/* TODO: these are not going to be implemented */
	fast_load_area_offset = 0 >> sector_shift;
	fast_load_area_length = 0 >> sector_shift;
}

void NEFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	WriteStubImage(wr);
	/* new header */
	wr.Seek(file_offset);
	wr.WriteData(2, "NE");
	wr.WriteWord(1, linker_version.major);
	wr.WriteWord(1, linker_version.minor);
	wr.WriteWord(2, entry_table_offset - file_offset);
	wr.WriteWord(2, entry_table_length);
	wr.WriteWord(4, 0); /* CRC */
	wr.WriteWord(1, program_flags);
	wr.WriteWord(1, application_flags);
	wr.WriteWord(2, automatic_data);
	wr.WriteWord(2, heap_size);
	wr.WriteWord(2, stack_size);
	wr.WriteWord(2, ip);
	wr.WriteWord(2, cs);
	wr.WriteWord(2, sp);
	wr.WriteWord(2, ss);
	wr.WriteWord(2, segments.size());
	wr.WriteWord(2, module_references.size());
	wr.WriteWord(2, nonresident_name_table_length);
	wr.WriteWord(2, segment_table_offset - file_offset);
	wr.WriteWord(2, resource_table_offset - file_offset);
	wr.WriteWord(2, resident_name_table_offset - file_offset);
	wr.WriteWord(2, module_reference_table_offset - file_offset);
	wr.WriteWord(2, imported_names_table_offset - file_offset);
	wr.WriteWord(4, nonresident_name_table_offset);
	wr.WriteWord(2, movable_entry_count);
	wr.WriteWord(2, sector_shift);
	wr.WriteWord(2, resources.size());
	wr.WriteWord(1, system);
	wr.WriteWord(1, additional_flags);
	wr.WriteWord(2, fast_load_area_offset);
	wr.WriteWord(2, fast_load_area_length);
	wr.WriteWord(2, code_swap_area_length);
	/* following Watcom */
	wr.WriteWord(1, windows_version.minor);
	wr.WriteWord(1, windows_version.major);

	/* Segment table */
	for(Segment& segment : segments)
	{
		wr.WriteWord(2, segment.data_offset >> sector_shift);
		wr.WriteWord(2, segment.image->data_size);
		wr.WriteWord(2, segment.flags);
		wr.WriteWord(2, segment.image->TotalSize());
	}

	/* Resource table */
	/* TODO */

	/* Resident name table */
	for(Name& name : resident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name.c_str());
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	/* Module reference table */
	for(uint16_t module : module_references)
	{
		wr.WriteWord(2, module);
	}

	/* Imported name table */
	for(std::string& name : imported_names)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name.c_str());
	}

	/* Entry table */
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		wr.WriteWord(1, entry_count);
		wr.WriteWord(1, entries[entry_index].GetIndicatorByte());
		for(size_t entry_offset = 0; entry_offset < entry_count; entry_offset ++)
		{
			entries[entry_index + entry_offset].WriteEntry(wr);
		}
		entry_index += entry_count;
	}
	wr.WriteWord(2, 0);

	/* Nonresident name table */
	for(Name& name : nonresident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name.c_str());
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	for(Segment& segment : segments)
	{
		wr.Seek(segment.data_offset);
		segment.image->WriteFile(wr);
		if(segment.relocations.size() != 0)
		{
			wr.WriteWord(2, segment.relocations.size());
			for(auto& it : segment.relocations)
			{
				wr.WriteWord(1, it.second.type);
				wr.WriteWord(1, it.second.flags);
				wr.WriteWord(2, it.second.offset);
				wr.WriteWord(2, it.second.module);
				wr.WriteWord(2, it.second.target);
			}
		}
	}
}


void NEFormat::SetModel(std::string model)
{
	if(model == "" || model == "small")
	{
		memory_model = MODEL_SMALL;
	}
	else if(model == "large")
	{
		memory_model = MODEL_LARGE;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_SMALL;
	}
}

void NEFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	program_name = filename;
	size_t ix = filename.rfind('.');
	if(ix != std::string::npos)
		module_name = filename.substr(0, ix);
	else
		module_name = filename;
	if(module.cpu != Linker::Module::I86)
	{
		Linker::Error << "Fatal error: Format only supports Intel 8086 binaries" << std::endl;
		exit(1);
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string NEFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

