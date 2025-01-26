#ifndef LINKER_H
#define LINKER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "module.h"

namespace Linker
{
	/** @brief Helper class that collects object files and libraries, and includes library objects for required symbols */
	class ModuleCollector
	{
	public:
		std::vector<std::shared_ptr<Module>> modules;
		std::set<std::string> required_symbols;
		std::map<std::string, std::weak_ptr<Module>> symbol_definitions;
		std::set<std::string> weak_symbols;

		void AddModule(std::shared_ptr<Module> module, bool is_library = false);
		void AddLibraryModule(std::shared_ptr<Module> module);
		void CombineModulesInto(Module& output_module);

	private:
		/**
		 * @brief Makes a module included in the linking process. The module must have already been added to the modules vector
		 */
		void IncludeModule(std::shared_ptr<Module> module);
	};
}

#endif /* LINKER_H */
