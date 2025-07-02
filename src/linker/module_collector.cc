
#include "module.h"
#include "module_collector.h"
#include "symbol_name.h"

using namespace Linker;

void ModuleCollector::SetupOptions(char special_char, std::shared_ptr<Linker::OutputFormat> output_format)
{
	special_prefix_char = special_char;
	this->output_format = output_format;
}

std::shared_ptr<Module> ModuleCollector::CreateModule(std::shared_ptr<const InputFormat> input_format, std::string file_name)
{
	std::shared_ptr<Module> module = std::make_shared<Module>(file_name);
	module->SetupOptions(special_prefix_char, output_format.lock(), input_format);
	return module;
}

void ModuleCollector::AddModule(std::shared_ptr<Module> module, bool is_library)
{
	Linker::Debug << "Register new module " << module->file_name << std::endl;
	/* attempts to resolve as many relocations as possible */
	/* this is needed because local symbols can get lost or duplicated, but segment references are still stored as references to symbol names */
	module->ResolveLocalRelocations();
	modules.push_back(module);
	for(auto& symbol_definition : module->global_symbols)
	{
		// TODO: refactor
		const std::string& symbol_name = symbol_definition.first;
		switch(symbol_definition.second.binding)
		{
		case SymbolDefinition::Global:
			{
				auto it = symbol_definitions.find(symbol_name);
				if(it != symbol_definitions.end())
				{
					if(it->second.binding == SymbolDefinition::Global)
					{
						Linker::Error << "Error: Symbol " << symbol_name << " defined in multiple modules: " << it->second.module.lock()->file_name << " and " << module->file_name << ", ignoring repetition" << std::endl;
						continue;
					}
				}
				symbol_definitions[symbol_name] = definition { SymbolDefinition::Global, module };
				Linker::Debug << "Register new symbol " << symbol_name << std::endl;
				if(required_symbols.find(symbol_name) != required_symbols.end())
				{
					/* include and link module */
					IncludeModule(module);
				}
			}
			break;
		case SymbolDefinition::Weak:
			if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
			{
				continue;
			}
			symbol_definitions[symbol_name] = definition { SymbolDefinition::Weak, module };
			Linker::Debug << "Register new symbol " << symbol_name << std::endl;
			if(required_symbols.find(symbol_name) != required_symbols.end())
			{
				/* include and link module */
				IncludeModule(module);
			}
			break;
		default:
			break;
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
			output_module.endiantype = module->endiantype; // TODO: verify all endian types are the same
		}
	}

	if(generate_got)
	{
		Linker::Debug << "Debug: Generating GOT section" << std::endl;
		std::shared_ptr<GlobalOffsetTable> got = std::make_shared<GlobalOffsetTable>(output_module.endiantype, ".got");
		output_module.AddSection(got);
		for(auto& module : modules)
		{
			if(!module->is_included)
				continue;

			for(auto& relocation : module->relocations)
			{
				if(relocation.kind != Linker::Relocation::GOTEntry)
					continue;

				Linker::Debug << "Debug: Module " << module->file_name << " has " << relocation << std::endl;
				got->AddEntry(GOTEntry(relocation.target));
			}
		}
	}

	// debug
	for(auto& symbol_definition : output_module.global_symbols)
	{
		Linker::Debug << "Debug: global symbol " << symbol_definition.first << " is defined as " << symbol_definition.second << std::endl;
	}
	for(auto& symbol_definition : output_module.local_symbols)
	{
		Linker::Debug << "Debug: local symbol " << symbol_definition.first << " is defined as " << symbol_definition.second << std::endl;
	}
}

void ModuleCollector::IncludeModule(std::shared_ptr<Module> module)
{
	if(module->is_included)
		return;

	Linker::Debug << "Debug: Include module " << module->file_name << std::endl;

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
	for(auto& relocation : module->relocations)
	{
		/* look for missing relocation targets */

		Linker::Debug << "Debug: Module " << module->file_name << " has " << relocation << std::endl;

		if(relocation.kind == Linker::Relocation::GOTEntry)
		{
			if(!generate_got)
				Linker::Debug << "Debug: GOT entry relocation" << std::endl;
			generate_got = true;
		}

		if(Linker::SymbolName * symbolp = std::get_if<Linker::SymbolName>(&relocation.target.target))
		{
			/* only check symbol names, not specific locations */

			if(*symbolp == Linker::SymbolName::GOT)
			{
				if(!generate_got)
					Linker::Debug << "Debug: target is GOT" << std::endl;
				generate_got = true;
			}

			std::string symbol_name;
			if(symbolp->GetLocalName(symbol_name))
			{
				/* symbol is an actual identifier, not a library load (used for Microsoft) */

				if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
				{
					IncludeModule(symbol_definitions[symbol_name].module.lock());
				}
				else
				{
					if(required_symbols.find(symbol_name) == required_symbols.end())
						Linker::Debug << "Debug: New required symbol " << symbol_name << std::endl;
					else
						Linker::Debug << "Debug: Required symbol " << symbol_name << std::endl;
					required_symbols.insert(symbol_name);
				}
			}
		}

		if(Linker::SymbolName * symbolp = std::get_if<Linker::SymbolName>(&relocation.reference.target))
		{
			/* only check symbol names, not specific locations */

			if(*symbolp == Linker::SymbolName::GOT)
			{
				if(!generate_got)
					Linker::Debug << "Debug: reference is GOT" << std::endl;
				generate_got = true;
			}

			std::string symbol_name;
			if(symbolp->GetLocalName(symbol_name))
			{
				/* symbol is an actual identifier, not a library load (used for Microsoft) */

				if(symbol_definitions.find(symbol_name) != symbol_definitions.end())
				{
					IncludeModule(symbol_definitions[symbol_name].module.lock());
				}
				else
				{
					if(required_symbols.find(symbol_name) == required_symbols.end())
						Linker::Debug << "Debug: New required symbol " << symbol_name << std::endl;
					else
						Linker::Debug << "Debug: Required symbol " << symbol_name << std::endl;
					required_symbols.insert(symbol_name);
				}
			}
		}
	}
}

