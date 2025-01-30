#ifndef SYMBOL_DEFINITION_H
#define SYMBOL_DEFINITION_H

#include <iostream>
#include <optional>
#include <string>
#include "../common.h"
#include "location.h"

namespace Linker
{
	/**
	 * @brief Represents a symbol definition
	 */
	class SymbolDefinition
	{
	public:
		std::string name;
		enum binding_type
		{
			Undefined,
			Local,
			Global,
			Weak,
			/** @brief Represents a currently unallocated variable that should be allocated in the final stages of the linking process */
			Common,
		};
		binding_type binding = Undefined;
		Location location;
		offset_t size = 0, align = 1;
		std::string section_name, alternative_section_name;

	protected:
		SymbolDefinition(std::string name, binding_type binding, Location location, offset_t size, offset_t align, std::string section_name = "", std::string alternative_section_name = "")
			: name(name), binding(binding), location(location), size(size), align(align), section_name(section_name), alternative_section_name(alternative_section_name)
		{
		}

	public:
		SymbolDefinition()
		{
		}

		static SymbolDefinition CreateUndefined(std::string name, offset_t size = 0, offset_t align = 1);
		static SymbolDefinition CreateLocal(std::string name, Location location, offset_t size = 0, offset_t align = 1);
		static SymbolDefinition CreateGlobal(std::string name, Location location, offset_t size = 0, offset_t align = 1);
		static SymbolDefinition CreateWeak(std::string name, Location location, offset_t size = 0, offset_t align = 1);
		static SymbolDefinition CreateCommon(std::string name, std::string section, offset_t size = 0, offset_t align = 1);
		static SymbolDefinition CreateCommon(std::string name, std::string section, offset_t size, offset_t align, std::string alternative_section);
		static SymbolDefinition CreateCommon(std::string name, std::string section, std::string alternative_section);

		bool operator ==(const SymbolDefinition& other) const;
	};

	std::ostream& operator <<(std::ostream& out, const SymbolDefinition& symbol);
}

#endif /*  SYMBOL_DEFINITION_H */
