#ifndef MODULE_COLLECTOR_H
#define MODULE_COLLECTOR_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "symbol_definition.h"
#include "table_section.h"
#include "target.h"

namespace Linker
{
	class Module;
	class InputFormat;
	class OutputFormat;

	/**
	 * @brief Helper class that collects object files and libraries, and includes library objects for required symbols
	 *
	 * Object and library modules can both be added to the collector, which determines which ones need to be part of the final binary object.
	 */
	class ModuleCollector
	{
	public:
		/**
		 * @brief Initializes the reader for linking purposes
		 * @param special_char Most input formats do not provide support for the special requirements of the output format (such as segmentation for ELF). We work around this by introducing special name prefixes $$SEGOF$ where $ is the value of special_char.
		 * @param output_format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		void SetupOptions(char special_char, std::shared_ptr<OutputFormat> output_format);

		/** @brief Set to true when Global Offset Table generation is requested */
		bool generate_got = false;
		std::shared_ptr<Module> CreateModule(std::shared_ptr<const InputFormat> input_format, std::string file_name = "");
	private:
		/* GNU assembler can use '$', NASM must use '?' */
		char special_prefix_char = '$';
		std::weak_ptr<OutputFormat> output_format;

	public:
		/** @brief Description of a symbol definition in some module */
		struct definition
		{
			/** @brief The binding type of the symbol where it is defined */
			SymbolDefinition::binding_type binding;
			/** @brief The module where a symbol is defined */
			std::weak_ptr<Module> module;
		};
		/** @brief A list of all modules to be considered, not all of which will be included in the final generated binary */
		std::vector<std::shared_ptr<Module>> modules;
		/**
		 * @brief Symbols that are referenced but have not yet been defined among the included modules
		 *
		 * When a symbol that appears in this collection is defined in a module, that modules gets included automatically.
		 */
		std::set<std::string> required_symbols;
		/**
		 * @brief Collection of symbols defined thus far
		 *
		 * When a module gets added to the collector, its definition is stored in this collection.
		 */
		std::map<std::string, definition> symbol_definitions;

		/**
		 * @brief Adds a module to the modules vector, also registers all the non-local symbols, and determines if any additional modules need to be included
		 *
		 * Non-library modules are automatically included in the final binary, library modules are only included on demand.
		 */
		void AddModule(std::shared_ptr<Module> module, bool is_library = false);
		/**
		 * @brief Convenience method to add a library
		 */
		void AddLibraryModule(std::shared_ptr<Module> module);
		/**
		 * @brief Appends all the modules that have been determined to be included, into a resulting object
		 */
		void CombineModulesInto(Module& output_module);

	private:
		/**
		 * @brief Makes a module included in the linking process. The module must have already been added to the modules vector
		 */
		void IncludeModule(std::shared_ptr<Module> module);
	};

	// TODO: uint64_t
	class GOTEntry : public Word<uint32_t>
	{
	public:
		std::optional<Target> target;

		GOTEntry()
			: target()
		{
		}

		GOTEntry(Target target)
			: target(target)
		{
		}

		bool operator ==(const GOTEntry& other) const
		{
			return target == other.target;
		}
	};

	class GlobalOffsetTable : public TableSection<GOTEntry>
	{
	public:
		GlobalOffsetTable(::EndianType endian_type, std::string name, int flags = Readable)
			: TableSection<GOTEntry>(endian_type, name, flags)
		{
		}

		GlobalOffsetTable(std::string name, int flags = Readable)
			: TableSection<GOTEntry>(name, flags)
		{
		}

		void AddEntry(GOTEntry entry)
		{
			if(std::find(entries.begin(), entries.end(), entry) == entries.end())
			{
				entries.push_back(entry);
			}
		}
	};
}

#endif /* MODULE_COLLECTOR_H */
