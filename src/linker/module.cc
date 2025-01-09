
#include <sstream>
#include "module.h"

using namespace Linker;

void Module::SetupOptions(char special_char, std::shared_ptr<Linker::OutputFormat> output_format, std::shared_ptr<Linker::InputFormat> input_format)
{
	special_prefix_char = special_char;
	this->output_format = output_format;
	this->input_format = input_format;
}

std::string Module::export_prefix()
{
	std::ostringstream oss;
	oss << special_prefix_char << special_prefix_char << "EXPORT" << special_prefix_char;
	return oss.str();
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
	local_symbols[name] = location;
}

void Module::AddGlobalSymbol(std::string name, Location location)
{
	std::shared_ptr<Linker::OutputFormat> output_format = this->output_format.lock();
	std::shared_ptr<Linker::InputFormat> input_format = this->input_format.lock();

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

	_AddGlobalSymbol(name, location);
}

void Module::_AddGlobalSymbol(std::string name, Location location)
{
	symbols[name] = location;
}

void Module::AddCommonSymbol(std::string name, CommonSymbol symbol)
{
	unallocated_symbols[name] = symbol;
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
	location = it->second;
	return true;
}

bool Module::FindGlobalSymbol(std::string name, Location& location)
{
	auto it = symbols.find(name);
	if(it == symbols.end())
		return false;
	location = it->second;
	return true;
}

void Module::AddSection(std::shared_ptr<Section> section)
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

void Module::ResolveRelocations()
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
	for(auto& symbol : symbols)
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
	for(auto symbol : other.symbols)
	{
		if(symbols.find(symbol.first) != symbols.end())
		{
			Linker::Debug << "Symbol " << symbol.first << " defined in multiple modules, ignoring repetition" << std::endl;
			continue;
		}
		symbol.second.Displace(displacement);
		symbols.emplace(symbol);
	}
	for(auto unalloc : other.unallocated_symbols)
	{
		auto it = unallocated_symbols.find(unalloc.first);
		if(it == unallocated_symbols.end())
		{
			unallocated_symbols.emplace(unalloc);
		}
		else
		{
			if(it->second.size < unalloc.second.size)
				it->second.size = unalloc.second.size;
			if(it->second.align < unalloc.second.align)
				it->second.align = unalloc.second.align;
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

void Module::AllocateSymbols(std::shared_ptr<Section> section)
{
	for(auto it : unallocated_symbols)
	{
		if(symbols.find(it.first) == symbols.end())
		{
			section->RealignEnd(it.second.align);
			size_t offset = section->Size();
			section->Expand(offset + it.second.size);
			symbols[it.first] = Location(section, offset);
#if DISPLAY_LOGS
			Linker::Debug << "Allocating " << it.first << " in " << section->name << " to " << offset << ", size " << it.second.size << std::endl;
#endif
		}
	}
	unallocated_symbols.clear();
}

void Module::AllocateSymbols()
{
	AllocateSymbols(FetchSection(".comm", Section::Readable|Section::Writable|Section::ZeroFilled));
}

