
#include "module_collector.h"

using namespace Linker;

void ModuleCollector::AddModule(std::shared_ptr<Module> module, bool is_library)
{
	/* attempts to resolve as many relocations as possible */
	/* this is needed because local symbols can get lost or duplicated, but segment references are still stored as references to symbol names */
	module->ResolveLocalRelocations();
	modules.push_back(module);
	for(auto& symbol_definition : module->global_symbols)
	{
		const std::string& symbol_name = symbol_definition.first;
		if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
		{
			if(weak_symbols.find(symbol_name) == weak_symbols.end())
			{
				// TODO: also display module file names
				Linker::Error << "Error: duplicate symbol " << symbol_name << ", ignoring duplicate" << std::endl;
				continue;
			}
			else
			{
				weak_symbols.erase(symbol_name);
			}
		}
		symbol_definitions[symbol_name] = module;
		if(required_symbols.find(symbol_name) != required_symbols.end())
		{
			/* include and link module */
			IncludeModule(module);
		}
	}
	for(auto& symbol_definition : module->weak_symbols)
	{
		const std::string& symbol_name = symbol_definition.first;
		if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
		{
			continue;
		}
		symbol_definitions[symbol_name] = module;
		weak_symbols.insert(symbol_name);
		if(required_symbols.find(symbol_name) != required_symbols.end())
		{
			/* include and link module */
			IncludeModule(module);
		}
	}
	if(!is_library)
	{
		IncludeModule(module);
	}
}

void ModuleCollector::AddLibraryModule(std::shared_ptr<Module> module)
{
	AddModule(module, true);
}

void ModuleCollector::CombineModulesInto(Module& output_module)
{
	for(auto& module : modules)
	{
		if(module->is_included)
		{
			output_module.Append(*module);
		}
	}
}

void ModuleCollector::IncludeModule(std::shared_ptr<Module> module)
{
	if(module->is_included)
		return;

	// must be set first to avoid a possible infinite recursion, if two modules reference each other
	module->is_included = true;

	for(auto& symbol_definition : module->global_symbols)
	{
		const std::string& symbol_name = symbol_definition.first;
		if(required_symbols.find(symbol_name) != required_symbols.end())
		{
			required_symbols.erase(symbol_name);
		}
	}
	for(auto& symbol_definition : module->weak_symbols)
	{
		const std::string& symbol_name = symbol_definition.first;
		if(required_symbols.find(symbol_name) != required_symbols.end())
		{
			required_symbols.erase(symbol_name);
		}
	}
	for(auto& relocation : module->relocations)
	{
		/* look for missing relocation targets */

		if(Linker::SymbolName * symbolp = std::get_if<Linker::SymbolName>(&relocation.target.target))
		{
			/* only check symbol names, not specific locations */

			std::string symbol_name;
			if(symbolp->GetLocalName(symbol_name))
			{
				/* symbol is an actual identifier, not a library load (used for Microsoft) */

				if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
				{
					IncludeModule(symbol_definitions[symbol_name].lock());
				}
				else
				{
					required_symbols.insert(symbol_name);
				}
			}
		}
	}
}

