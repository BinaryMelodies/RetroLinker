
#include "symbol_definition.h"

using namespace Linker;

SymbolDefinition SymbolDefinition::CreateUndefined(std::string name, offset_t size, offset_t align)
{
	return SymbolDefinition(name, Undefined, Location(), size, align);
}

SymbolDefinition SymbolDefinition::CreateLocal(std::string name, Location location, offset_t size, offset_t align)
{
	return SymbolDefinition(name, Local, location, size, align);
}

SymbolDefinition SymbolDefinition::CreateGlobal(std::string name, Location location, offset_t size, offset_t align)
{
	return SymbolDefinition(name, Global, location, size, align);
}

SymbolDefinition SymbolDefinition::CreateWeak(std::string name, Location location, offset_t size, offset_t align)
{
	return SymbolDefinition(name, Weak, location, size, align);
}

SymbolDefinition SymbolDefinition::CreateCommon(std::string name, std::string section, offset_t size, offset_t align)
{
	return SymbolDefinition(name, Common, Location(), size, align, section);
}

SymbolDefinition SymbolDefinition::CreateCommon(std::string name, std::string section, offset_t size, offset_t align, std::string alternative_section)
{
	return SymbolDefinition(name, Common, Location(), size, align, section, alternative_section);
}

SymbolDefinition SymbolDefinition::CreateCommon(std::string name, std::string section, std::string alternative_section)
{
	return SymbolDefinition(name, Common, Location(), 0, 1, section, alternative_section);
}

bool SymbolDefinition::IsLocal() const
{
	return binding == Local || binding == LocalCommon;
}

bool SymbolDefinition::IsCommon() const
{
	return binding == Common || binding == LocalCommon;
}

bool SymbolDefinition::IsAllocated() const
{
	return binding == Local || binding == Global || binding == Weak;
}

bool SymbolDefinition::operator ==(const SymbolDefinition& other) const
{
	// definitions are identified by their name, whether they are local or global, and for local symbols by their location (TODO: this might not be necessary)
	// this ensures that global names appear only once but local names can appear repeatedly
	return name == other.name &&
		(binding == Local ? other.binding == Local && location == other.location
			: binding == LocalCommon ? other.binding == LocalCommon
			: !other.IsLocal());
}

bool SymbolDefinition::Displace(const Displacement& displacement)
{
	if(binding == Local || binding == Global || binding == Weak)
	{
		return location.Displace(displacement);
	}
	return false;
}

std::ostream& Linker::operator <<(std::ostream& out, const SymbolDefinition& symbol)
{
	switch(symbol.binding)
	{
	case SymbolDefinition::Undefined:
		out << "undefined ";
		break;
	case SymbolDefinition::Local:
		out << "local ";
		break;
	case SymbolDefinition::Global:
		out << "global ";
		break;
	case SymbolDefinition::Weak:
		out << "weak ";
		break;
	case SymbolDefinition::Common:
		out << "common ";
		break;
	case SymbolDefinition::LocalCommon:
		out << "local common ";
		break;
	}
	out << symbol.name;
	if(symbol.IsAllocated())
	{
		out << " at " << symbol.location;
	}
	if(symbol.size != 0)
	{
		out << " size " << symbol.size;
	}
	if(symbol.align != 0 && symbol.align != 1)
	{
		out << " align " << symbol.align;
	}
	if(symbol.IsCommon())
	{
		if(symbol.section_name != "")
		{
			out << " section " << symbol.section_name;
			if(symbol.alternative_section_name != "")
			{
				out << " or " << symbol.alternative_section_name;
			}
		}
	}
	return out;
}

