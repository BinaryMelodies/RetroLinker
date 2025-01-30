
#include <sstream>
#include "module.h"
#include "format.h"

using namespace Linker;

void Module::AddSymbol(const SymbolDefinition& symbol)
{
	switch(symbol.binding)
	{
	case SymbolDefinition::Undefined:
		AddUndefinedSymbol(symbol);
		break;
	case SymbolDefinition::Local:
		AddLocalSymbol(symbol);
		break;
	case SymbolDefinition::Global:
		AddGlobalSymbol(symbol);
		break;
	case SymbolDefinition::Weak:
		AddWeakSymbol(symbol);
		break;
	case SymbolDefinition::Common:
		AddCommonSymbol(symbol);
		break;
	}
}

void Module::AddSymbolDefinition(const SymbolDefinition& mention)
{
	if(std::find(symbol_sequence.begin(), symbol_sequence.end(), mention) == symbol_sequence.end())
	{
		symbol_sequence.emplace_back(mention);
	}
}

void Module::AppendSymbolDefinition(const SymbolDefinition& mention)
{
	symbol_sequence.emplace_back(mention);
}

void Module::DeleteSymbolDefinition(const SymbolDefinition& mention)
{
	auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), mention);
	if(it != symbol_sequence.end())
	{
		symbol_sequence.erase(it);
	}
}

bool Module::HasSymbolDefinition(const SymbolDefinition& mention)
{
	auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), mention);
	return it != symbol_sequence.end();
}

void Module::NewSymbolDefinition(const SymbolDefinition& mention)
{
	// TODO: refactor
	switch(mention.binding)
	{
	case SymbolDefinition::Undefined:
		if(!HasSymbolDefinition(SymbolDefinition::CreateGlobal(mention.name, Location()))
		&& !HasSymbolDefinition(SymbolDefinition::CreateUndefined(mention.name))
		&& !HasSymbolDefinition(SymbolDefinition::CreateCommon(mention.name, "")))
		{
			AddSymbolDefinition(mention);
		}
		break;
	case SymbolDefinition::Local:
		AppendSymbolDefinition(mention);
		break;
	case SymbolDefinition::Global:
		AddSymbolDefinition(mention);
		DeleteSymbolDefinition(SymbolDefinition::CreateWeak(mention.name, Location()));
		DeleteSymbolDefinition(SymbolDefinition::CreateUndefined(mention.name));
		DeleteSymbolDefinition(SymbolDefinition::CreateCommon(mention.name, ""));
		break;
	case SymbolDefinition::Weak:
		if(!HasSymbolDefinition(SymbolDefinition::CreateGlobal(mention.name, Location())))
		{
			AddSymbolDefinition(mention);
			DeleteSymbolDefinition(SymbolDefinition::CreateUndefined(mention.name));
		}
		break;
	case SymbolDefinition::Common:
		if(!HasSymbolDefinition(SymbolDefinition::CreateGlobal(mention.name, Location())))
		{
			AddSymbolDefinition(mention);
			DeleteSymbolDefinition(SymbolDefinition::CreateUndefined(mention.name));
		}
		break;
	}
}

void Module::SetupOptions(char special_char, std::shared_ptr<Linker::OutputFormat> output_format, std::shared_ptr<const Linker::InputFormat> input_format)
{
	special_prefix_char = special_char;
	this->output_format = output_format;
	this->input_format = input_format;
}

std::string Module::segment_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEG" << special_prefix_char;
	return oss.str();
}

std::string Module::segment_of_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGOF" << special_prefix_char;
	return oss.str();
}

std::string Module::segment_at_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGAT" << special_prefix_char;
	return oss.str();
}

std::string Module::with_respect_to_segment_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "WRTSEG" << special_prefix_char;
	return oss.str();
}

std::string Module::segment_difference_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SEGDIF" << special_prefix_char;
	return oss.str();
}

std::string Module::import_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "IMPORT" << special_prefix_char;
	return oss.str();
}

std::string Module::segment_of_import_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "IMPSEG" << special_prefix_char;
	return oss.str();
}

std::string Module::export_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "EXPORT" << special_prefix_char;
	return oss.str();
}

std::string Module::fix_byte_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "FIX" << special_prefix_char;
	return oss.str();
}

std::string Module::resource_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "RSRC" << special_prefix_char;
	return oss.str();
}

bool Module::parse_imported_name(std::string reference_name, Linker::SymbolName& symbol)
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

bool Module::parse_exported_name(std::string reference_name, Linker::ExportedSymbol& symbol)
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

void Module::AddLocalSymbol(std::string name, Location location)
{
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	/* The w65-wdc assembler is bugged and sometimes erases bytes, this is an ugly work around to fix it */
	if(input_format != nullptr && input_format->FormatRequiresDataStreamFix() && name.rfind(fix_byte_prefix(), 0) == 0)
	{
		/* $$FIX$<byte>$<rest> */
		size_t dollar_offset = name.find("$", fix_byte_prefix().size()) - fix_byte_prefix().size();
		std::string byte_text = name.substr(fix_byte_prefix().size(), dollar_offset);
		Linker::Debug << "Debug: Interpreting " << byte_text << " as part of " << name << std::endl;
		unsigned byte_value = stoul(byte_text, nullptr, 16);
		location.section->WriteWord(1, location.offset, byte_value, ::LittleEndian);
		return;
	}

	AddLocalSymbol(SymbolDefinition::CreateLocal(name, location));
}

void Module::AddLocalSymbol(const SymbolDefinition& symbol)
{
	if(local_symbols.find(symbol.name) != local_symbols.end())
	{
		Linker::Debug << "Debug: duplicated local symbol " << symbol.name << " in module " << file_name << ", ignoring" << std::endl;
		return;
	}
	local_symbols[symbol.name] = symbol;
	NewSymbolDefinition(symbol);
}

void Module::AddGlobalSymbol(std::string name, Location location)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	if(output_format != nullptr && output_format->FormatSupportsLibraries()
	&& input_format != nullptr && !input_format->FormatProvidesLibraries()
	&& name.rfind(export_prefix(), 0) == 0)
	{
		/* $$EXPORT$<name> */
		/* $$EXPORT$<name>$<ordinal> */
		std::string reference_name = name.substr(export_prefix().size());
		Linker::ExportedSymbol name("");
		if(parse_exported_name(reference_name, name))
		{
			AddExportedSymbol(name, location);
			return;
		}
		else
		{
			Linker::Error << "Error: Unable to parse export name " << name << ", proceeding" << std::endl;
		}
	}

	AddGlobalSymbol(SymbolDefinition::CreateGlobal(name, location));
}

void Module::AddGlobalSymbol(const SymbolDefinition& symbol)
{
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		switch(it->second.binding)
		{
		case SymbolDefinition::Global:
			Linker::Error << "Error: symbol " << symbol.name << " defined multiple times, ignoring" << std::endl;
			return;
		case SymbolDefinition::Weak:
			// overriding weak definition
			break;
		case SymbolDefinition::Common:
			// overriding common definition
			// TODO: check segments
			break;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return;
		}
	}
	global_symbols[symbol.name] = symbol;

	// TODO
	NewSymbolDefinition(symbol);
}

void Module::AddWeakSymbol(std::string name, Location location)
{
	AddWeakSymbol(SymbolDefinition::CreateWeak(name, location));
}

void Module::AddWeakSymbol(const SymbolDefinition& symbol)
{
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		switch(it->second.binding)
		{
		case SymbolDefinition::Global:
			// ignore weak definition
			return;
		case SymbolDefinition::Weak:
			// ignore weak definition
			return;
		case SymbolDefinition::Common:
			// overriding common definition
			// TODO: is this the expected behavior?
			// TODO: check segments
			break;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return;
		}
	}
	global_symbols[symbol.name] = symbol;

	NewSymbolDefinition(symbol);
}

void Module::AddCommonSymbol(SymbolDefinition symbol)
{
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		switch(it->second.binding)
		{
		case SymbolDefinition::Global:
			// ignore common definition
			// TODO: check segments
			break;
		case SymbolDefinition::Weak:
			// ignore weak definition
			// TODO: is this the expected behavior?
			return;
		case SymbolDefinition::Common:
			// TODO: is this the expected behavior?
			if(it->second.size < symbol.size)
				it->second.size = symbol.size;
			if(it->second.align < symbol.align)
				it->second.align = symbol.align;
			// TODO: also replace in symbol sequence
			return;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return;
		}
	}
	global_symbols[symbol.name] = symbol;

	NewSymbolDefinition(symbol);
}

void Module::AddImportedSymbol(SymbolName name)
{
	if(std::find(imported_symbols.begin(), imported_symbols.end(), name) == imported_symbols.end())
	{
		imported_symbols.push_back(name);
	}
}

void Module::AddExportedSymbol(ExportedSymbol name, Location symbol)
{
	exported_symbols[name] = symbol;
}

void Module::AddUndefinedSymbol(std::string symbol_name)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	if(output_format != nullptr && output_format->FormatSupportsLibraries()
	&& input_format != nullptr && !input_format->FormatProvidesLibraries())
	{
		if(symbol_name.rfind(import_prefix(), 0) == 0)
		{
			/* $$IMPORT$<library name>$<ordinal> */
			/* $$IMPORT$<library name>$_<name> */
			std::string reference_name = symbol_name.substr(import_prefix().size());
			Linker::SymbolName name("");
			if(parse_imported_name(reference_name, name))
			{
				AddImportedSymbol(name);
			}
			else
			{
				Linker::Error << "Error: Unable to parse import name " << symbol_name << ", proceeding" << std::endl;
			}
		}
		else if(symbol_name.rfind(segment_of_import_prefix(), 0) == 0)
		{
			/* $$IMPSEG$<library name>$<ordinal> */
			/* $$IMPSEG$<library name>$_<name> */
			std::string reference_name = symbol_name.substr(segment_of_import_prefix().size());
			Linker::SymbolName name("");
			if(parse_imported_name(reference_name, name))
			{
				AddImportedSymbol(name);
			}
			else
			{
				Linker::Error << "Error: Unable to parse import name " << symbol_name << ", proceeding" << std::endl;
			}
		}
	}

	AddUndefinedSymbol(SymbolDefinition::CreateUndefined(symbol_name));
}

void Module::AddUndefinedSymbol(const SymbolDefinition& symbol)
{
	// undefined symbol ignored

	NewSymbolDefinition(symbol);
}

void Module::AddRelocation(Relocation relocation)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	if(SymbolName * symbolp = std::get_if<SymbolName>(&relocation.target.target))
	{
		/* undefined symbol */
		std::string unparsed_name;
		assert(symbolp->LoadName(unparsed_name));

		if(output_format != nullptr && input_format != nullptr
		&& (cpu == I86 || cpu == I386))
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
			if(output_format->FormatSupportsSegmentation() && !input_format->FormatProvidesSegmentation())
			{
				if(unparsed_name.rfind(segment_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SEG$<section name> */
					/* Note: can only refer to a currently present section */
					std::string section_name = unparsed_name.substr(segment_prefix().size());
					std::shared_ptr<Section> section = FindSection(section_name);
					if(section == nullptr)
					{
						Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
					}
					else
					{
						relocation = Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::Location(section)).GetSegment(), relocation.addend);
					}
				}
				else if(unparsed_name.rfind(segment_of_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SEGOF$<symbol name> */
					std::string symbol_name = unparsed_name.substr(segment_of_prefix().size());
					relocation = Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)).GetSegment(), relocation.addend);
				}
				else if(unparsed_name.rfind(segment_at_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SEGAT$<symbol name> */
					std::string symbol_name = unparsed_name.substr(segment_of_prefix().size());
					relocation = Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)), relocation.addend);
				}
				else if(unparsed_name.rfind(with_respect_to_segment_prefix(), 0) == 0)
				{
					/* $$WRTSEG$<symbol name>$<section name> */
					size_t sep = unparsed_name.rfind(special_prefix_char);
					std::string symbol_name = unparsed_name.substr(with_respect_to_segment_prefix().size(),
						sep - with_respect_to_segment_prefix().size());
					std::string section_name = unparsed_name.substr(sep + 1);
					//Linker::Debug << "Debug: " << symbol_name << " wrt " << section_name << std::endl;
					std::shared_ptr<Linker::Section> section = FindSection(section_name);
					if(section == nullptr)
					{
						Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
					}
					else
					{
						relocation = Linker::Relocation::OffsetFrom(relocation.size, relocation.source,
							Linker::Target(Linker::SymbolName(symbol_name)), Linker::Target(Linker::Location(section)).GetSegment(), relocation.addend, ::LittleEndian);
					}
				}
				else if(unparsed_name.rfind(segment_difference_prefix(), 0) == 0)
				{
					/* $$SEGDIF$<section name>$<section name> */
					size_t sep = unparsed_name.rfind(special_prefix_char);
					std::string section1_name = unparsed_name.substr(segment_difference_prefix().size(),
						sep - with_respect_to_segment_prefix().size());
					std::shared_ptr<Linker::Section> section1 = FindSection(section1_name);
					std::string section2_name = unparsed_name.substr(sep + 1);
					std::shared_ptr<Linker::Section> section2 = FindSection(section2_name);
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
						relocation = Linker::Relocation::ParagraphDifference(relocation.source,
							Linker::Target(Linker::Location(section1)).GetSegment(), Linker::Target(Linker::Location(section2)).GetSegment(), relocation.addend);
					}
				}
			}
			if(output_format->FormatSupportsLibraries() && !input_format->FormatProvidesLibraries())
			{
				if(unparsed_name.rfind(import_prefix(), 0) == 0)
				{
					/* $$IMPORT$<library name>$<ordinal> */
					/* $$IMPORT$<library name>$_<name> */
					std::string reference_name = unparsed_name.substr(import_prefix().size());
					Linker::SymbolName symbol("");
					if(parse_imported_name(reference_name, symbol))
					{
						relocation =
							relocation.IsRelative()
							? Linker::Relocation::Relative(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend)
							: output_format->FormatIsLinear()
								? Linker::Relocation::Absolute(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend)
								: Linker::Relocation::Offset(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend);
					}
					else
					{
						Linker::Error << "Error: Unable to parse import name " << unparsed_name << ", proceeding" << std::endl;
					}
				}
				else if(unparsed_name.rfind(segment_of_import_prefix(), 0) == 0)
				{
					/* $$IMPSEG$<library name>$<ordinal> */
					/* $$IMPSEG$<library name>$_<name> */
					std::string reference_name = unparsed_name.substr(segment_of_import_prefix().size());
					Linker::SymbolName symbol("");
					if(parse_imported_name(reference_name, symbol))
					{
						relocation = Linker::Relocation::Paragraph(relocation.source, Linker::Target(symbol), relocation.addend);
					}
					else
					{
						Linker::Error << "Error: Unable to parse import name " << unparsed_name << ", proceeding" << std::endl;
					}
				}
			}
		}
	}

	relocations.push_back(relocation);
}

const std::vector<SymbolName>& Module::GetImportedSymbols() const
{
	return imported_symbols;
}

const std::map<ExportedSymbol, Location>& Module::GetExportedSymbols() const
{
	return exported_symbols;
}

bool Module::FindLocalSymbol(std::string name, Location& location)
{
	auto it = local_symbols.find(name);
	if(it == local_symbols.end())
		return false;
	location = it->second.location;
	return true;
}

bool Module::FindGlobalSymbol(std::string name, Location& location)
{
	auto it = global_symbols.find(name);
	if(it == global_symbols.end())
		return false;
	location = it->second.location;
	return true;
}

void Module::AddSection(std::shared_ptr<Section> section)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	if(output_format != nullptr)
		section->SetFlag(output_format->FormatAdditionalSectionFlags(section->name));

	if(output_format != nullptr && output_format->FormatSupportsResources()
	&& input_format != nullptr && !input_format->FormatProvidesResources()
	&& section->name.rfind(resource_prefix() + "_",  0) == 0)
	{
		/* $$RSRC$_<type>$<id> */
		section->SetFlag(Linker::Section::Resource);
		size_t sep = section->name.rfind(special_prefix_char);
		std::string resource_type = section->name.substr(resource_prefix().size() + 1, sep - resource_prefix().size() - 1);
		try
		{
			uint16_t resource_id = stoll(section->name.substr(sep + 1), nullptr, 16);
			section->resource_type = resource_type;
			section->resource_id = resource_id;
			Linker::Debug << "Debug: Resource type " << resource_type << ", id " << resource_id << std::endl;
		}
		catch(std::invalid_argument& a)
		{
			Linker::Error << "Error: Unable to parse resource section name " << section->name << ", proceeding" << std::endl;
		}
	}

	_AddSection(section);
}

void Module::_AddSection(std::shared_ptr<Section> section)
{
	sections.push_back(section);
	if(section->name != "")
	{
		if(section_names.find(section->name) == section_names.end())
		{
			section_names[section->name] = section;
		}
		else
		{
			Linker::Warning << "Warning: section of name `" << section->name << "' already exists" << std::endl;
		}
	}
}

const std::vector<std::shared_ptr<Section>>& Module::Sections() const
{
	return sections;
}

void Module::DeleteSection(size_t index)
{
	/* Note: this function should not be called without calling RemoveSections before any further operations, as it leaves the module in a vulnerable state */
	sections[index] = nullptr;
}

void Module::RemoveSections()
{
	sections.clear();
	section_names.clear();
}

std::shared_ptr<Section> Module::FindSection(std::string name)
{
	auto it = section_names.find(name);
	return it == section_names.end() ? nullptr : it->second;
}

std::shared_ptr<Section> Module::FetchSection(std::string name, unsigned default_flags)
{
	std::shared_ptr<Section> section = FindSection(name);
	if(section == nullptr)
	{
		section = std::make_shared<Section>(name, default_flags);
		AddSection(section);
	}
	return section;
}

void Module::ResolveLocalRelocations()
{
	for(auto& rel : relocations)
	{
#if 0
		Relocation tmp = rel;
		if(rel.target.ResolveLocals(*this) | rel.reference.ResolveLocals(*this))
		{
			Linker::Debug << "Resolved " << tmp << std::endl << "\tto " << rel << std::endl;
		}
#else
		rel.target.ResolveLocals(*this);
		rel.reference.ResolveLocals(*this);
#endif
	}
}

void Module::Append(std::shared_ptr<Section> dst, std::shared_ptr<Section> src)
{
	Displacement displacement;
	displacement[src] = Location(dst, dst->Append(*src));
	for(auto& symbol : global_symbols)
	{
		symbol.second.Displace(displacement);
	}
	for(auto& symbol : local_symbols)
	{
		symbol.second.Displace(displacement);
	}
	for(Relocation& rel : relocations)
	{
		rel.Displace(displacement);
	}
}

void Module::Append(Module& other)
{
	Displacement displacement;
	if(cpu == NONE)
	{
		cpu = other.cpu;
	}
	else
	{
		assert(cpu == other.cpu);
	}
	for(auto& other_section : other.sections)
	{
		std::shared_ptr<Section> section = FindSection(other_section->name);
		if(section == nullptr)
		{
			AddSection(other_section);
		}
		else
		{
			displacement[other_section] = Location(section, section->Append(*other_section));
		}
	}
	// TODO: use the sequence to add symbols
	for(auto mention : other.symbol_sequence)
	{
		NewSymbolDefinition(mention);
	}
	for(auto symbol : other.global_symbols)
	{
		auto it = global_symbols.find(symbol.first);
		if(it != global_symbols.end())
		{
			// TODO: this duplicates the behavior of AddSymbol, should be removed
			switch(symbol.second.binding)
			{
			case SymbolDefinition::Global:
				switch(it->second.binding)
				{
				case SymbolDefinition::Global:
					Linker::Debug << "Duplicate symbol " << symbol.first << ", ignoring duplicate" << std::endl;
					continue;
				case SymbolDefinition::Weak:
					// overwrite
					break;
				case SymbolDefinition::Common:
					// TODO
					break;
				}
				break;
			case SymbolDefinition::Weak:
				switch(it->second.binding)
				{
				case SymbolDefinition::Global:
				case SymbolDefinition::Weak:
					Linker::Debug << "Weak symbol " << symbol.first << " defined in multiple modules, ignoring duplicate" << std::endl;
					continue;
				case SymbolDefinition::Common:
					// TODO
					break;
				}
				break;
			case SymbolDefinition::Common:
				break;
			}
		}
		symbol.second.Displace(displacement);
		AddSymbol(symbol.second);
	}
	// local symbols are not copied over in the dictionary
#if 0
	for(auto& symbols_pair : other.local_symbols)
	{
		if(local_symbols.find(symbols_pair.first) == local_symbols.end())
		{
			local_symbols[symbols_pair.first] = std::vector<Location>();
		}
		std::vector<Location>& symbol_vector = local_symbols[symbols_pair.first];
		for(auto& symbol : symbols_pair.second)
		{
			symbol.Displace(displacement);
			symbol_vector.push_back(symbol);
		}
	}
#endif
	for(auto& import : other.imported_symbols)
	{
		auto it = std::find(imported_symbols.begin(), imported_symbols.end(), import);
		if(it == imported_symbols.end())
		{
			imported_symbols.push_back(import);
		}
	}
	for(auto& _export : other.exported_symbols)
	{
		if(exported_symbols.find(_export.first) != exported_symbols.end())
		{
			Linker::Debug << "Symbol " << _export.first << " defined in multiple modules, ignoring repetition" << std::endl;
			continue;
		}
		_export.second.Displace(displacement);
		exported_symbols.emplace(_export);
	}
	for(Relocation rel : other.relocations)
	{
		rel.Displace(displacement);
		relocations.push_back(rel);
	}
}

void Module::AllocateSymbols(std::string default_section_name)
{
	FetchSection(default_section_name, Section::Readable|Section::Writable|Section::ZeroFilled);
	for(auto it : global_symbols)
	{
		if(it.second.binding == SymbolDefinition::Common)
		{
			std::shared_ptr<Section> section = FetchSection(it.second.section_name != "" ? it.second.section_name : default_section_name, Section::Readable|Section::Writable|Section::ZeroFilled);
			section->RealignEnd(it.second.align);
			size_t offset = section->Size();
			section->Expand(offset + it.second.size);
			global_symbols[it.first] = SymbolDefinition::CreateGlobal(it.second.name, Location(section, offset));
#if DISPLAY_LOGS
			Linker::Debug << "Allocating " << it.first << " in " << section->name << " to " << offset << ", size " << it.second.size << std::endl;
#endif
		}
	}
}

