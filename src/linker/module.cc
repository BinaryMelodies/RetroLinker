
#include <algorithm>
#include <sstream>
#include "format.h"
#include "module.h"
#include "section.h"

using namespace Linker;

// when set to 1, undefined symbol definitions are stored in the global_symbols table
// this could help speed up removing undefined symbols, but then global symbol definitions need to be checked they are not Undefined
#define STORE_UNDEFINED_AS_GLOBAL 1

bool Module::AddSymbol(const SymbolDefinition& symbol)
{
	switch(symbol.binding)
	{
	case SymbolDefinition::Undefined:
		return AddUndefinedSymbol(symbol);
	case SymbolDefinition::Local:
		return AddLocalSymbol(symbol);
	case SymbolDefinition::Global:
		return AddGlobalSymbol(symbol);
	case SymbolDefinition::Weak:
		return AddWeakSymbol(symbol);
	case SymbolDefinition::Common:
		return AddCommonSymbol(symbol);
	case SymbolDefinition::LocalCommon:
		return AddLocalCommonSymbol(symbol);
	}
	assert(false);
}

void Module::AppendSymbolDefinition(const SymbolDefinition& symbol)
{
	if(!symbol.IsLocal())
	{
		// global symbols can be repeated
		auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), symbol);
		if(it != symbol_sequence.end() && it->binding == SymbolDefinition::Undefined)
		{
			Linker::Debug << "Debug: remove " << symbol.name << " from symbol_sequence" << std::endl;
			symbol_sequence.erase(it);
		}
	}
	Linker::Debug << "Debug: append " << symbol.name << " to symbol_sequence" << std::endl;
	symbol_sequence.emplace_back(symbol);
}

void Module::DeleteSymbolDefinition(const SymbolDefinition& symbol)
{
	auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), symbol);
	if(it != symbol_sequence.end())
	{
		Linker::Debug << "Debug: remove " << symbol.name << " from symbol_sequence" << std::endl;
		symbol_sequence.erase(it);
	}
}

SymbolDefinition * Module::FetchSymbolDefinition(const SymbolDefinition& symbol)
{
	auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), symbol);
	return it == symbol_sequence.end() ? nullptr : &*it;
}

bool Module::HasSymbolDefinition(const SymbolDefinition& symbol)
{
	auto it = std::find(symbol_sequence.begin(), symbol_sequence.end(), symbol);
	return it != symbol_sequence.end();
}

std::vector<Relocation>& Module::GetRelocations()
{
	return relocations;
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

std::string Module::section_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SECTION" << special_prefix_char;
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

std::string Module::segment_at_section_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "SECTSEG" << special_prefix_char;
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
		/* <library name> */
		/* <library name>$<ordinal> */
		/* <library name>$_<name> */
//		Linker::Debug << "Debug: Reference name " << reference_name << std::endl;
		size_t ix = reference_name.find(special_prefix_char);
		if(ix == std::string::npos)
		{
			symbol = Linker::SymbolName(reference_name, Linker::SymbolName::IsLibrary);
		}
		else
		{
			std::string library_name = reference_name.substr(0, ix);
			if(reference_name[ix + 1] == '_')
			{
				symbol = Linker::SymbolName(library_name, reference_name.substr(ix + 2));
			}
			else
			{
//				Linker::Debug << "Debug: Attempt parsing " << reference_name.substr(ix + 1) << std::endl;
				symbol = Linker::SymbolName(library_name, stoll(reference_name.substr(ix + 1), nullptr, 16));
			}
		}
		return true;
	}
	catch(std::invalid_argument& a)
	{
		return false;
	}
}

bool Module::parse_exported_name(std::string reference_name, Linker::ExportedSymbolName& symbol)
{
	try
	{
		/* <name> */
		/* <name>$<ordinal> */
		/* <ordinal>$_<name> */
		size_t ix = reference_name.find(special_prefix_char);
		if(ix == std::string::npos)
		{
			symbol = Linker::ExportedSymbolName(reference_name);
		}
		else if(ix < reference_name.size() - 1 && reference_name[ix + 1] == '_')
		{
			symbol = Linker::ExportedSymbolName(stoll(reference_name.substr(0, ix)), reference_name.substr(ix + 2));
		}
		else
		{
			symbol = Linker::ExportedSymbolName(reference_name.substr(0, ix), stoll(reference_name.substr(ix + 1)));
		}
		return true;
	}
	catch(std::invalid_argument& a)
	{
		return false;
	}
}

bool Module::AddLocalSymbol(std::string name, Location location)
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
		return false;
	}

	return AddLocalSymbol(SymbolDefinition::CreateLocal(name, location));
}

bool Module::AddLocalSymbol(const SymbolDefinition& symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddLocalSymbol(" << symbol.name << ")" << std::endl;
	if(local_symbols.find(symbol.name) != local_symbols.end())
	{
		Linker::Debug << "Debug: duplicated local symbol " << symbol.name << " in module " << file_name << ", ignoring" << std::endl;
		return false;
	}
	local_symbols[symbol.name] = symbol;
	AppendSymbolDefinition(symbol);
	return true;
}

bool Module::AddGlobalSymbol(std::string name, Location location)
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
		Linker::ExportedSymbolName name("");
		if(parse_exported_name(reference_name, name))
		{
			AddExportedSymbol(name, location);
			return false;
		}
		else
		{
			Linker::Error << "Error: Unable to parse export name " << name << ", proceeding" << std::endl;
		}
	}

	return AddGlobalSymbol(SymbolDefinition::CreateGlobal(name, location));
}

bool Module::AddGlobalSymbol(const SymbolDefinition& symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddGlobalSymbol(" << symbol.name << ")" << std::endl;
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		Linker::Debug << "Debug: AddGlobalSymbol received duplicate" << std::endl;
		switch(it->second.binding)
		{
#if STORE_UNDEFINED_AS_GLOBAL
		case SymbolDefinition::Undefined:
			// remove undefined
			break;
#endif
		case SymbolDefinition::Global:
			Linker::Error << "Error: symbol " << symbol.name << " defined multiple times, ignoring" << std::endl;
			return false;
		case SymbolDefinition::Weak:
			// overriding weak definition
			break;
		case SymbolDefinition::Common:
			// overriding common definition
			// TODO: check segments
			break;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return false;
		}
		if(!HasSymbolDefinition(symbol))
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in global_symbols but not in symbol_sequence" << std::endl;
		}
		//Linker::Debug << "Debug: attempting to delete " << symbol.name << " from symbol_sequence" << std::endl;
		DeleteSymbolDefinition(symbol);
	}
	global_symbols[symbol.name] = symbol;

#if STORE_UNDEFINED_AS_GLOBAL
	if(HasSymbolDefinition(symbol))
	{
		Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
	}
#else
	if(auto def = FetchSymbolDefinition(symbol))
	{
		if(def->binding == SymbolDefinition::Undefined)
		{
			DeleteSymbolDefinition(symbol);
		}
		else
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
		}
	}
#endif
	AppendSymbolDefinition(symbol);

	return true;
}

bool Module::AddWeakSymbol(std::string name, Location location)
{
	return AddWeakSymbol(SymbolDefinition::CreateWeak(name, location));
}

bool Module::AddWeakSymbol(const SymbolDefinition& symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddWeakSymbol(" << symbol.name << ")" << std::endl;
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		switch(it->second.binding)
		{
#if STORE_UNDEFINED_AS_GLOBAL
		case SymbolDefinition::Undefined:
			// remove undefined
			break;
#endif
		case SymbolDefinition::Global:
			// ignore weak definition
			return false;
		case SymbolDefinition::Weak:
			// ignore weak definition
			return false;
		case SymbolDefinition::Common:
			// overriding common definition
			break;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return false;
		}
		if(!HasSymbolDefinition(symbol))
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in global_symbols but not in symbol_sequence" << std::endl;
		}
		DeleteSymbolDefinition(symbol);
	}
	global_symbols[symbol.name] = symbol;

#if STORE_UNDEFINED_AS_GLOBAL
	if(HasSymbolDefinition(symbol))
	{
		Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
	}
#else
	if(auto def = FetchSymbolDefinition(symbol))
	{
		if(def->binding == SymbolDefinition::Undefined)
		{
			DeleteSymbolDefinition(symbol);
		}
		else
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
		}
	}
#endif
	AppendSymbolDefinition(symbol);

	return true;
}

bool Module::AddCommonSymbol(SymbolDefinition symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddCommonSymbol(" << symbol.name << ")" << std::endl;
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		switch(it->second.binding)
		{
#if STORE_UNDEFINED_AS_GLOBAL
		case SymbolDefinition::Undefined:
			// remove undefined
			break;
#endif
		case SymbolDefinition::Global:
			// ignore common definition
			// TODO: check segments and alignment
			return false;
		case SymbolDefinition::Weak:
			// ignore weak definition
			break;
		case SymbolDefinition::Common:
			if(it->second.size < symbol.size)
				it->second.size = symbol.size;
			if(it->second.align < symbol.align)
				it->second.align = symbol.align;
			if(auto def = FetchSymbolDefinition(symbol))
			{
				*def = it->second;
			}
			else
			{
				Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in global_symbols but not in symbol_sequence" << std::endl;
			}
			return false;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return false;
		}
		if(!HasSymbolDefinition(symbol))
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in global_symbols but not in symbol_sequence" << std::endl;
		}
		DeleteSymbolDefinition(symbol);
	}
	global_symbols[symbol.name] = symbol;

#if STORE_UNDEFINED_AS_GLOBAL
	if(HasSymbolDefinition(symbol))
	{
		Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
	}
#else
	if(auto def = FetchSymbolDefinition(symbol))
	{
		if(def->binding == SymbolDefinition::Undefined)
		{
			DeleteSymbolDefinition(symbol);
		}
		else
		{
			Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in symbol_sequence but not in global_symbols" << std::endl;
		}
	}
#endif
	AppendSymbolDefinition(symbol);

	return true;
}

bool Module::AddLocalCommonSymbol(SymbolDefinition symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddLocalCommonSymbol(" << symbol.name << ")" << std::endl;
	auto it = local_symbols.find(symbol.name);
	if(it != local_symbols.end())
	{
		switch(it->second.binding)
		{
		case SymbolDefinition::Local:
			// ignore common definition
			return false;
		case SymbolDefinition::LocalCommon:
			// TODO: is this the expected behavior?
			if(it->second.size < symbol.size)
				it->second.size = symbol.size;
			if(it->second.align < symbol.align)
				it->second.align = symbol.align;
			if(auto def = FetchSymbolDefinition(symbol))
			{
				*def = it->second;
			}
			else
			{
				Linker::Error << "Internal error: symbol table mismatch for " << symbol.name << ", in local_symbols but not in symbol_sequence" << std::endl;
			}
			return false;
		default:
			Linker::Error << "Internal error: invalid symbol type" << std::endl;
			return false;
		}
	}
	local_symbols[symbol.name] = symbol;

	AppendSymbolDefinition(symbol);

	return true;
}

void Module::AddImportedSymbol(SymbolName name)
{
	if(std::find(imported_symbols.begin(), imported_symbols.end(), name) == imported_symbols.end())
	{
		imported_symbols.push_back(name);
	}
}

void Module::AddExportedSymbol(ExportedSymbolName name, Location symbol)
{
	exported_symbols[name] = symbol;
}

bool Module::AddUndefinedSymbol(std::string symbol_name)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	if(output_format != nullptr && output_format->FormatSupportsSegmentation()
	&& input_format != nullptr && !input_format->FormatProvidesSegmentation())
	{
		if(symbol_name.rfind(section_prefix(), 0) == 0
		|| symbol_name.rfind(segment_prefix(), 0) == 0
		|| symbol_name.rfind(segment_of_prefix(), 0) == 0
		|| symbol_name.rfind(segment_at_prefix(), 0) == 0
		|| symbol_name.rfind(segment_at_section_prefix(), 0) == 0
		|| symbol_name.rfind(with_respect_to_segment_prefix(), 0) == 0
		|| symbol_name.rfind(segment_difference_prefix(), 0) == 0)
		{
			/* these are used for relocations, we do not need to include them in the module symbol table */
			return false;
		}
	}

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
			return false;
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
			return false;
		}
	}

	return AddUndefinedSymbol(SymbolDefinition::CreateUndefined(symbol_name));
}

bool Module::AddUndefinedSymbol(const SymbolDefinition& symbol)
{
	Linker::Debug << "Debug: `" << file_name << "'.AddUndefinedSymbol(" << symbol.name << ")" << std::endl;
#if STORE_UNDEFINED_AS_GLOBAL
	auto it = global_symbols.find(symbol.name);
	if(it != global_symbols.end())
	{
		return false;
	}
	global_symbols[symbol.name] = symbol;
#endif

	AppendSymbolDefinition(symbol);

	return true;
}

void Module::AddRelocation(Relocation relocation)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<const Linker::InputFormat> input_format = this->input_format.lock();

	/* Attempt to parse extended relocation syntax */
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
				if(unparsed_name.rfind(section_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SECT$<section name> */
					/* Note: can only refer to a currently present section */
					std::string section_name = unparsed_name.substr(section_prefix().size());
					std::shared_ptr<Section> section = FindSection(section_name);
					if(section == nullptr)
					{
						Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
					}
					else
					{
#if 0
						relocation =
							output_format->FormatIsProtectedMode()
							? Linker::Relocation::Absolute(relocation.size, relocation.source, Linker::Target(Linker::Location(section)), relocation.addend)
							: Linker::Relocation::Offset(relocation.size, relocation.source, Linker::Target(Linker::Location(section)), relocation.addend);
#endif
						bool is_offset = relocation.IsOffset();
						relocation.target = Linker::Target(Linker::Location(section));
						if(is_offset)
							relocation.reference = Linker::Target(Linker::Location(section)).GetSegment();
					}
				}
				else if(unparsed_name.rfind(segment_prefix(), 0) == 0 && relocation.size == 2)
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
#if 0
						relocation =
							output_format->FormatIsProtectedMode()
							? Linker::Relocation::Selector(relocation.source, Linker::Target(Linker::Location(section)).GetSegment(), relocation.addend)
							: Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::Location(section)).GetSegment(), relocation.addend);
#endif
						relocation.kind = output_format->FormatIsProtectedMode() ? Linker::Relocation::SelectorIndex : Linker::Relocation::ParagraphAddress;
						relocation.target = Linker::Target(Linker::Location(section)).GetSegment();
						// in case this was originally an Offset() relocation
						relocation.reference = Target();
					}
				}
				else if(unparsed_name.rfind(segment_of_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SEGOF$<symbol name> */
					std::string symbol_name = unparsed_name.substr(segment_of_prefix().size());
#if 0
					relocation =
						output_format->FormatIsProtectedMode()
						? Linker::Relocation::Selector(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)).GetSegment(), relocation.addend)
						: Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)).GetSegment(), relocation.addend);
#endif
					relocation.kind = output_format->FormatIsProtectedMode() ? Linker::Relocation::SelectorIndex : Linker::Relocation::ParagraphAddress;
					relocation.target = Linker::Target(Linker::SymbolName(symbol_name)).GetSegment();
					// in case this was originally an Offset() relocation
					relocation.reference = Target();
				}
				else if(unparsed_name.rfind(segment_at_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SEGAT$<symbol name> */
					std::string symbol_name = unparsed_name.substr(segment_of_prefix().size());
#if 0
					relocation =
						output_format->FormatIsProtectedMode()
						? Linker::Relocation::Selector(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)), relocation.addend)
						: Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::SymbolName(symbol_name)), relocation.addend);
#endif
					relocation.kind = output_format->FormatIsProtectedMode() ? Linker::Relocation::SelectorIndex : Linker::Relocation::ParagraphAddress;
					relocation.target = Linker::Target(Linker::SymbolName(symbol_name));
					// in case this was originally an Offset() relocation
					relocation.reference = Target();
				}
				else if(unparsed_name.rfind(segment_at_section_prefix(), 0) == 0 && relocation.size == 2)
				{
					/* $$SECTSEG$<symbol name> */
					std::string section_name = unparsed_name.substr(segment_at_section_prefix().size());
					std::shared_ptr<Section> section = FindSection(section_name);
					if(section == nullptr)
					{
						Linker::Error << "Error: invalid section in extended relocation `" << section_name << "'" << std::endl;
					}
					else
					{
#if 0
						relocation =
							output_format->FormatIsProtectedMode()
							? Linker::Relocation::Selector(relocation.source, Linker::Target(Linker::Location(section)), relocation.addend)
							: Linker::Relocation::Paragraph(relocation.source, Linker::Target(Linker::Location(section)), relocation.addend);
#endif
						relocation.kind = output_format->FormatIsProtectedMode() ? Linker::Relocation::SelectorIndex : Linker::Relocation::ParagraphAddress;
						relocation.target = Linker::Target(Linker::Location(section));
						// in case this was originally an Offset() relocation
						relocation.reference = Target();
					}
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
#if 0
						relocation = Linker::Relocation::OffsetFrom(relocation.size, relocation.source,
							Linker::Target(Linker::SymbolName(symbol_name)), Linker::Target(Linker::Location(section)).GetSegment(), relocation.addend, ::LittleEndian);
#endif
						relocation.target = Linker::Target(Linker::SymbolName(symbol_name));
						relocation.reference = Linker::Target(Linker::Location(section)).GetSegment();
					}
				}
				else if(unparsed_name.rfind(segment_difference_prefix(), 0) == 0 && relocation.size == 2)
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
#if 0
						relocation = Linker::Relocation::ParagraphDifference(relocation.source,
							Linker::Target(Linker::Location(section1)).GetSegment(), Linker::Target(Linker::Location(section2)).GetSegment(), relocation.addend);
#endif
						relocation.kind = Linker::Relocation::ParagraphAddress;
						relocation.target = Linker::Target(Linker::Location(section1)).GetSegment();
						relocation.reference = Linker::Target(Linker::Location(section2)).GetSegment();
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
#if 0
						relocation =
							relocation.IsRelative()
							? Linker::Relocation::Relative(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend)
							: output_format->FormatIsLinear()
								? Linker::Relocation::Absolute(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend)
								: Linker::Relocation::Offset(relocation.size, relocation.source, Linker::Target(symbol), relocation.addend);
#endif
						bool is_offset = relocation.IsOffset();
						relocation.target = Linker::Target(symbol);
						if(is_offset)
							relocation.reference = Linker::Target(symbol).GetSegment();
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
#if 0
						relocation =
							output_format->FormatIsProtectedMode()
							? Linker::Relocation::Selector(relocation.source, Linker::Target(symbol), relocation.addend)
							: Linker::Relocation::Paragraph(relocation.source, Linker::Target(symbol), relocation.addend);
#endif
						relocation.kind = output_format->FormatIsProtectedMode() ? Linker::Relocation::SelectorIndex : Linker::Relocation::ParagraphAddress;
						relocation.target = Linker::Target(symbol);
						// in case this was originally an Offset() relocation
						relocation.reference = Target();
					}
					else
					{
						Linker::Error << "Error: Unable to parse import name " << unparsed_name << ", proceeding" << std::endl;
					}
				}
			}
		}
	}

	/* Check if two relocations can be combined */
	auto index_it = relocation_indexes.find(relocation.source);
	if(index_it != relocation_indexes.end())
	{
		if(relocations[index_it->second].Combine(relocation))
		{
			// the two relocations were successfully combined
			Linker::Debug << "Debug: Relocations combined" << std::endl;
			return;
		}
		else
		{
			Linker::Warning << "Warning: Two relocations for the same source location could not be combined" << std::endl;
		}
	}

	relocation_indexes[relocation.source] = relocations.size();
	relocations.push_back(relocation);
}

const std::vector<SymbolName>& Module::GetImportedSymbols() const
{
	return imported_symbols;
}

const std::map<ExportedSymbolName, Location>& Module::GetExportedSymbols() const
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
#if STORE_UNDEFINED_AS_GLOBAL
	if(it->second.binding == SymbolDefinition::Undefined)
		return false;
#endif
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
	for(auto symbol : other.symbol_sequence)
	{
		symbol.Displace(displacement);
		if(symbol.IsLocal())
		{
			// local symbols are only added to the symbol sequence, which permits duplication
			AppendSymbolDefinition(symbol);
		}
		else
		{
			AddSymbol(symbol);
		}
	}
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

