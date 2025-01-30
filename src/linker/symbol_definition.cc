
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

bool SymbolDefinition::operator ==(const SymbolDefinition& other) const
{
	// definitions are identified by their name and for local symbols by their location
	return name == other.name && (binding == Local ? other.binding == Local && location == other.location : other.binding != Local);
}

