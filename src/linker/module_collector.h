#ifndef MODULE_COLLECTOR_H
#define MODULE_COLLECTOR_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "symbol_definition.h"

namespace Linker
{
	class Module;
	class InputFormat;
	class OutputFormat;

	/** @brief Helper class that collects object files and libraries, and includes library objects for required symbols */
	class ModuleCollector
	{
	public:
		/**
		 * @brief Initializes the reader for linking purposes
		 * @param special_char Most input formats do not provide support for the special requirements of the output format (such as segmentation for ELF). We work around this by introducing special name prefixes $$SEGOF$ where $ is the value of special_char.
		 * @param output_format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		void SetupOptions(char special_char, std::shared_ptr<OutputFormat> output_format);

		std::shared_ptr<Module> CreateModule(std::shared_ptr<const InputFormat> input_format, std::string file_name = "");
	private:
		/* GNU assembler can use '$', NASM must use '?' */
		char special_prefix_char = '$';
		std::weak_ptr<OutputFormat> output_format;

	public:
		struct definition
		{
			SymbolDefinition::binding_type binding;
			std::weak_ptr<Module> module;
		};
		std::vector<std::shared_ptr<Module>> modules;
		std::set<std::string> required_symbols;
		std::map<std::string, definition> symbol_definitions;

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

#endif /* MODULE_COLLECTOR_H */
