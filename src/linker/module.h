#ifndef MODULE_H
#define MODULE_H

#include <map>
#include <memory>
#include <vector>
#include <string>
#include "../common.h"
#include "location.h"
#include "relocation.h"
#include "symbol_definition.h"
#include "symbol_name.h"

namespace Linker
{
	class InputFormat;
	class OutputFormat;
	class Section;

	/**
	 * @brief Encodes an object module file as a collection of sections, symbols and relocations
	 */
	class Module
	{
	public:
		/** @brief Stores source/destination file name */
		std::string file_name;

		Module(std::string file_name = "")
			: file_name(file_name)
		{
		}

		/**
		 * @brief Supported CPU types
		 */
		enum cpu_type
		{
			NONE,
			I80, /* also Z80 */
			I86,
			I386,
			X86_64,
			M6800,
			M6809,
			M68K,
			MOS6502,
			W65K,
			Z8K,
			PPC,
			PPC64,
			ARM,
			ARM64,
			PDP11,
			MIPS,
			SPARC,
			SH,
		};

		/**
		 * @brief Encodes the CPU for the target
		 */
		cpu_type cpu = NONE;
		/** @brief Sorted collection of all symbols appearing in the module, duplicates removed */
		std::vector<SymbolDefinition> symbol_sequence;
	private:
		std::vector<std::shared_ptr<Section>> sections;
		std::map<std::string, std::shared_ptr<Section>> section_names;
		/** Mapping global symbols to their definitions */
		std::map<std::string, SymbolDefinition> global_symbols; // includes Weak and Common symbols
		/** Mapping local symbols to their definitions */
		std::map<std::string, SymbolDefinition> local_symbols;
		std::vector<SymbolName> imported_symbols;
		std::map<ExportedSymbolName, Location> exported_symbols;

		friend class ModuleCollector;

	private:
		bool AddSymbol(const SymbolDefinition& symbol);
		void AppendSymbolDefinition(const SymbolDefinition& symbol);
		void DeleteSymbolDefinition(const SymbolDefinition& symbol);
		SymbolDefinition * FetchSymbolDefinition(const SymbolDefinition& symbol);
	public:
		bool HasSymbolDefinition(const SymbolDefinition& symbol);

		/**
		 * @brief List of relocations within the module
		 */
		std::vector<Relocation> relocations;

		/** @brief Set to true if module is included in the linking process, relevant for libraries */
		bool is_included = false;

		/**
		 * @brief Initializes the reader for linking purposes
		 * @param special_char Most input formats do not provide support for the special requirements of the output format (such as segmentation for ELF). We work around this by introducing special name prefixes $$SEGOF$ where $ is the value of special_char.
		 * @param output_format The output format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 * @param input_format The input format that will be used. This is required to know which extra special features need to be implemented (such as segmentation).
		 */
		void SetupOptions(char special_char, std::shared_ptr<OutputFormat> output_format, std::shared_ptr<const InputFormat> input_format);

	private:
		/* GNU assembler can use '$', NASM must use '?' */
		char special_prefix_char = '$';
		std::weak_ptr<OutputFormat> output_format;
		std::weak_ptr<const InputFormat> input_format;

		/* symbols */
		std::string segment_prefix();
		std::string segment_of_prefix();
		std::string segment_at_prefix();
		std::string with_respect_to_segment_prefix();
		std::string segment_difference_prefix();
		std::string import_prefix();
		std::string segment_of_import_prefix();
		std::string export_prefix();
		std::string fix_byte_prefix();

		/* sections */
		std::string resource_prefix();

		bool parse_imported_name(std::string reference_name, SymbolName& symbol);
		bool parse_exported_name(std::string reference_name, ExportedSymbolName& symbol);

	public:
		/**
		 * @brief Adds an internal symbol
		 */
		bool AddLocalSymbol(std::string name, Location location);
	private:
		bool AddLocalSymbol(const SymbolDefinition& symbol);

	public:
		/**
		 * @brief Adds and processes exported symbol for extended syntax
		 */
		bool AddGlobalSymbol(std::string name, Location location);
	private:
		bool AddGlobalSymbol(const SymbolDefinition& symbol);

	public:
		/**
		 * @brief Adds a weakly bound symbol
		 */
		bool AddWeakSymbol(std::string name, Location location);
	private:
		bool AddWeakSymbol(const SymbolDefinition& symbol);

	public:
		/**
		 * @brief Adds a common symbol
		 */
		bool AddCommonSymbol(SymbolDefinition symbol);

		/**
		 * @brief Adds a local common symbol
		 */
		bool AddLocalCommonSymbol(SymbolDefinition symbol);

		/**
		 * @brief Adds an imported symbol (Microsoft format: library name + symbol name + ordinal/hint)
		 */
		void AddImportedSymbol(SymbolName name);

		/**
		 * @brief Adds an exported symbol (Microsoft format: library name + symbol name + ordinal/hint)
		 */
		void AddExportedSymbol(ExportedSymbolName name, Location symbol);

		/**
		 * @brief Processes undefined symbol for extended syntax
		 */
		bool AddUndefinedSymbol(std::string symbol_name);
	private:
		bool AddUndefinedSymbol(const SymbolDefinition& symbol);

	public:
		/**
		 * @brief Adds and processes relocation for extended syntax
		 */
		void AddRelocation(Relocation relocation);

		/**
		 * @brief Retrieves list of all imported symbols
		 */
		const std::vector<SymbolName>& GetImportedSymbols() const;

		/**
		 * @brief Retrieves map of all exported symbols and their locations
		 */
		const std::map<ExportedSymbolName, Location>& GetExportedSymbols() const;

		/**
		 * @brief Searches for a local symbol
		 */
		bool FindLocalSymbol(std::string name, Location& location);

		/**
		 * @brief Searches for a global or weak symbol
		 */
		bool FindGlobalSymbol(std::string name, Location& location);

		/**
		 * @brief Adds a new section
		 */
		void AddSection(std::shared_ptr<Section> section);

	private:
		void _AddSection(std::shared_ptr<Section> section);

	public:
		/**
		 * @brief Retrieves list of all sections
		 */
		const std::vector<std::shared_ptr<Section>>& Sections() const;

		/**
		 * @brief Deletes a specific sections
		 */
		void DeleteSection(size_t index);

		/**
		 * @brief Removes all sections from internal list, without deleting them
		 */
		void RemoveSections();

		/**
		 * @brief Searches for a section with a specific name
		 */
		std::shared_ptr<Section> FindSection(std::string name);

		/**
		 * @brief Searches or creates a section with a specific name, with the assigned flags
		 */
		std::shared_ptr<Section> FetchSection(std::string name, unsigned default_flags);

		/**
		 * @brief Attempts to resolve the local targets of all relocations
		 */
		void ResolveLocalRelocations();

		/**
		 * @brief Appends two sections by shifting all symbols locations and relocation targets in the second one
		 */
		void Append(std::shared_ptr<Section> dst, std::shared_ptr<Section> src);

		/**
		 * @brief Appends another module object, merging identically named sections
		 */
		void Append(Module& other);

		/**
		 * @brief All common symbols are converted to global symbols and assigned addresses within a their section (usually ".comm")
		 */
		void AllocateSymbols(std::string default_section_name = ".comm");
	};
}

#endif /* MODULE_H */
