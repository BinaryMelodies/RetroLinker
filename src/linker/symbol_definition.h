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
			/** @brief An undefined variable that needs to appear in another module for the linking process to succeed */
			Undefined,
			/** @brief A variable that is only visible in the current module */
			Local,
			/** @brief A variable that is visible in all modules and must appear once */
			Global,
			/** @brief A variable that is visible in all modules, however takes lower precedence than a Global variable */
			Weak,
			/** @brief A currently unallocated variable that should be allocated in the final stages of the linking process */
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

		/**
		 * @brief Recalculates the location after a section has moved
		 *
		 * Note that only local, global and weak symbol definitions have a location
		 *
		 * @param displacement A map from sections to locations, specifying the new starting place of the section.
		 * This can also indicate when a section has been appended to another one, and the location will be updated to reference the new section.
		 * @return True if location changed due to displacement.
		 */
		bool Displace(const Displacement& displacement);
	};

	std::ostream& operator <<(std::ostream& out, const SymbolDefinition& symbol);
}

#endif /*  SYMBOL_DEFINITION_H */
